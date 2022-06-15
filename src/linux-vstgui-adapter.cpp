//
// Created by paul on 6/13/22.
//

#include <vstgui/lib/vstguiinit.h>

#if IS_LINUX
#include "vstgui/lib/platform/platform_x11.h"
#include "vstgui/lib/platform/linux/x11platform.h"
#include <map>
#include <unordered_set>
#endif
#include "clap-saw-demo.h"
#include "linux-vstgui-adapter.h"

namespace sst::clap_saw_demo
{

#if IS_LINUX
struct ClapRunLoop : public VSTGUI::X11::IRunLoop, public VSTGUI::AtomicReferenceCounted
{
    std::unordered_set<ClapSawDemo *> plugins;
    ClapSawDemo *primaryPlugin{nullptr};
    ClapRunLoop(ClapSawDemo *p)  {
        addPlugin(p);
        _DBGCOUT << "Creating ClapRunLoop" << std::endl;
    }

    void initializationOver()
    {
        _DBGMARK;
    }

    void addPlugin(ClapSawDemo *p)
    {
        _DBGMARK;
        if (primaryPlugin == nullptr)
        {
            primaryPlugin = p;
            if (firstFdSeen)
            {
                _DBGCOUT << "Re-registering with primary plugin anew" << std::endl;
                primaryPlugin->registerPosixFd(firstFd);
            }

            for (auto &[handler, interval] : unparentedTimers)
            {
                registerTimer(interval, handler);
            }
            unparentedTimers.clear();
        }
        plugins.insert(p);
    }
    void removePlugin(ClapSawDemo *p)
    {
        _DBGMARK;
        if (p == primaryPlugin)
        {
            _DBGCOUT << "Primary plugin closed; undoing FD" << std::endl;
            if (firstFdSeen)
                p->unregisterPosixFD(firstFd);

            auto timerCopy = timerToInterval;
            for (auto &[handler, interval] : timerCopy)
            {
                unregisterTimer(handler);
            }

            plugins.erase(p);
            if (plugins.size() >= 1)
            {
                _DBGCOUT << "Adopted by someone else" << std::endl;
                primaryPlugin = (*plugins.begin());
                if (firstFdSeen)
                    primaryPlugin->registerPosixFd(firstFd);

                for (auto &[handler, interval] : timerCopy)
                    registerTimer(interval, handler);
            }
            else
            {
                primaryPlugin = nullptr;
                for (auto hi : timerCopy)
                    unparentedTimers.emplace_back(hi);
            }
        }
        else
        {
            plugins.erase(p);
        }
    }

    bool firstFdSeen{false};
    int firstFd{0};
    VSTGUI::X11::IEventHandler *firstFdHandler{nullptr};

    std::map<VSTGUI::X11::IEventHandler *, int> eventHandlers;
    bool registerEventHandler(int fd, VSTGUI::X11::IEventHandler *handler) override
    {
        _DBGCOUT << _D(fd) << _D(handler) << std::endl;
        if (!firstFdSeen)
        {
            _DBGCOUT << "This is the special VSTGUI First FD" << std::endl;
            firstFdSeen = true;
            firstFd = fd;
            firstFdHandler = handler;
        }
        auto res = primaryPlugin->registerPosixFd(fd);
        eventHandlers.insert({handler, fd});
        return res;
    }
    bool unregisterEventHandler(VSTGUI::X11::IEventHandler *handler) override
    {
        _DBGCOUT << _D(handler) << std::endl;
        if (handler == firstFdHandler)
        {
            _DBGCOUT << "And its the firstFD handler" << std::endl;
        }
        auto it = eventHandlers.begin();
        while (it != eventHandlers.end())
        {
            const auto &[v, fd] = *it;
            if (v == handler)
            {
                _DBGCOUT << "Found an event handler to erase " << _D(fd) << std::endl;
                eventHandlers.erase(it);
                if (primaryPlugin)
                    return primaryPlugin->unregisterPosixFD(fd);
                else
                    return true;
            }
            it++;
        }

        return false;
    }
    void fireFd(int fd)
    {
        for (const auto &[v, k] : eventHandlers)
        {
            if (k == fd)
                v->onEvent();
        }
    }

    std::map<clap_id, VSTGUI::X11::ITimerHandler *> timerHandlers;
    std::unordered_map<VSTGUI::X11::ITimerHandler *, uint64_t> timerToInterval;
    std::list<std::pair<VSTGUI::X11::ITimerHandler *, uint64_t>> unparentedTimers;
    bool registerTimer(uint64_t interval, VSTGUI::X11::ITimerHandler *handler) override
    {
        _DBGCOUT << _D(interval) << _D(handler) << std::endl;

        clap_id id;
        auto res = primaryPlugin->registerTimer(interval, &id);
        timerToInterval[handler] = interval;
        timerHandlers[id] = handler;
        _DBGCOUT << "Timer registered at " << id << std::endl;
        return res;
    }
    bool unregisterTimer(VSTGUI::X11::ITimerHandler *handler) override
    {
        _DBGCOUT << "unregsiterTimer" << _D(handler) << std::endl;
        auto it = timerHandlers.begin();
        while (it != timerHandlers.end())
        {
            const auto &[k, v] = *it;
            if (v == handler)
            {
                _DBGCOUT << "Found a timer handler to erase " << k << std::endl;
                timerHandlers.erase(it);
                timerToInterval.erase(v);
                return primaryPlugin->unregisterTimer(k);
            }
            it++;
        }
        return false;
    }
    void fireTimer(clap_id id)
    {
        for (const auto &[k, v] : timerHandlers)
        {
            if (k == id)
                v->onTimer();
        }
    }
};

void ClapSawDemo::onTimer(clap_id timerId) noexcept
{
    auto rlp = VSTGUI::X11::RunLoop::get().get();
    auto clp = reinterpret_cast<ClapRunLoop *>(rlp);
    if (clp)
        clp->fireTimer(timerId);
}
void ClapSawDemo::onPosixFd(int fd, int flags) noexcept
{
    auto rlp = VSTGUI::X11::RunLoop::get().get();
    auto clp = reinterpret_cast<ClapRunLoop *>(rlp);
    if (clp)
        clp->fireFd(fd);
}

static bool everInit{false};

void addLinuxVSTGUIPlugin(ClapSawDemo *that)
{
    _DBGCOUT << "addlingLinxuVSTGUIPlugin " << that << std::endl;

    if (!everInit)
    {
        VSTGUI::init(nullptr);
        VSTGUI::X11::RunLoop::init(VSTGUI::owned(new ClapRunLoop(that)));
        everInit = true;

        auto rl = VSTGUI::X11::RunLoop::get().get();
        auto crl = dynamic_cast<ClapRunLoop *>(rl);

        crl->initializationOver();
        _DBGCOUT << _D(rl) << _D(crl) << std::endl;
    }
    else
    {
        auto rl = VSTGUI::X11::RunLoop::get().get();
        auto crl = dynamic_cast<ClapRunLoop *>(rl);
        _DBGCOUT << _D(rl) << _D(crl) << std::endl;
        assert(crl);
        if (crl)
            crl->addPlugin(that);
    }
}
void removeLinuxVSTGUIPlugin(ClapSawDemo *that)
{
    _DBGCOUT << "remotelingLinxuVSTGUIPlugin " << that << std::endl;
    auto rl = VSTGUI::X11::RunLoop::get().get();
    auto crl = dynamic_cast<ClapRunLoop *>(rl);

    if (crl)
    {
        crl->removePlugin(that);
    }

}

void exitLinuxVSTGUI()
{
    VSTGUI::X11::RunLoop::exit();
}
#endif

} // namespace sst::clap_saw_demo