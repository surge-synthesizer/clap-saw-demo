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
        if (primaryPlugin == nullptr)
        {
            primaryPlugin = p;
        }
        plugins.insert(p);
    }
    void removePlugin(ClapSawDemo *p)
    {
        if (p == primaryPlugin)
        {
            if (plugins.size() == 1)
            {
                _DBGCOUT << "TRUE EXIT CONDITION" << std::endl;
            }
            else
            {
                _DBGCOUT << "HANDOFF" << std::endl;
            }
        }
        plugins.erase(p);
    }

    std::map<VSTGUI::X11::IEventHandler *, int> eventHandlers;
    bool registerEventHandler(int fd, VSTGUI::X11::IEventHandler *handler) override
    {
        _DBGCOUT << _D(fd) << _D(handler) << std::endl;
        auto res = primaryPlugin->registerPosixFd(fd);
        eventHandlers.insert({handler, fd});
        return res;
    }
    bool unregisterEventHandler(VSTGUI::X11::IEventHandler *handler) override
    {
        _DBGCOUT << _D(handler) << std::endl;
        auto it = eventHandlers.begin();
        while (it != eventHandlers.end())
        {
            const auto &[v, fd] = *it;
            if (v == handler)
            {
                _DBGCOUT << "Found an event handler to erase " << _D(fd) << std::endl;
                eventHandlers.erase(it);
                return primaryPlugin->unregisterPosixFD(fd);
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
    bool registerTimer(uint64_t interval, VSTGUI::X11::ITimerHandler *handler) override
    {
        _DBGCOUT << _D(interval) << _D(handler) << std::endl;
        clap_id id;
        auto res = primaryPlugin->registerTimer(interval, &id);
        timerHandlers[id] = handler;
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
                _DBGCOUT << "Found a timer handler to erase" << std::endl;
                timerHandlers.erase(it);
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
        if (crl->plugins.size() == 1)
        {
            _DBGCOUT << "Last person out the door" << std::endl;
        }
    }

}
#endif

} // namespace sst::clap_saw_demo