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
/*
 * OK so this is the VSTGUI IRunLoop which provides VSTGUI a chance to register global timer
 * and fd handlers. But CLAP has per-plugin timer and fd handlers. So we have to mesh those.
 * Here's how we do it
 *
 * The VSTGUI is to add and remvoe timer handlers and event handlers (FD handlers). As well
 * as registering those with a plugin, we keep a map of their registration charactierstics.
 *
 * Then as plugins enter- and exit- the gui fray, they register her. If a plugin unregisters
 * and it is the plugin we used to run the timer and fd, we unregister its timers and fd and
 * register them with a different plugin. If no such plugin is available we wait for one to be,
 * and when it pops up, we register with it. This is the concept of 'primary plugin'
 *
 * So this code looks like (basically) three sets of handlers
 *
 * 1. A map of what VSTGUi has registered with me
 * 2. A handler for plugin changes
 * 3. And combining those, registering and registering the event handlers with the plugin in
 * question
 *
 */
struct ClapRunLoop : public VSTGUI::X11::IRunLoop, public VSTGUI::AtomicReferenceCounted
{
    std::unordered_set<ClapSawDemo *> plugins;
    ClapSawDemo *primaryPlugin{nullptr};
    ClapRunLoop(ClapSawDemo *p)
    {
        _DBGCOUT << "Creating ClapRunLoop" << std::endl;
        addPlugin(p);
    }
    ~ClapRunLoop()
    {
        _DBGCOUT << _D(registeredTimers.size()) << _D(registeredEventHandlers.size()) << std::endl;
    }

    void initializationOver() { _DBGMARK; }

    void setAsPrimaryPlugin(ClapSawDemo *p)
    {
        primaryPlugin = p;
        _DBGCOUT << "Setting new primary plugin " << _D(primaryPlugin) << std::endl;
        for (auto &[fd, handler] : registeredEventHandlers)
        {
            _DBGCOUT << "    Re-registering FD   : " << _D(fd) << std::endl;
            primaryPlugin->registerPosixFd(fd);
        }

        assert(clapTimerIdToHandlerMap.size() == 0);
        for (auto &[interval, handler] : registeredTimers)
        {
            clap_id id;
            primaryPlugin->registerTimer(interval, &id);
            _DBGCOUT << "    Re-registering Timer : " << _D(handler) << _D(interval) << " at "
                     << _D(id) << std::endl;
            clapTimerIdToHandlerMap[id] = handler;
        }
    }

    void addPlugin(ClapSawDemo *p)
    {
        _DBGCOUT << _D(p) << std::endl;
        if (primaryPlugin == nullptr)
        {
            setAsPrimaryPlugin(p);
        }
        plugins.insert(p);
    }
    void removePlugin(ClapSawDemo *p)
    {
        _DBGCOUT << _D(p) << std::endl;
        if (p == primaryPlugin)
        {
            _DBGCOUT << "Primary Plugin being removed. Switching." << std::endl;
            for (auto &[fd, handler] : registeredEventHandlers)
            {
                _DBGCOUT << "    Event Unregister: " << _D(fd) << std::endl;
                primaryPlugin->unregisterPosixFD(fd);
            }

            // We clear the timer handlers but we leave them in registeredTimers, which means
            // here we actually dont iterate over the registeredTimers
            assert(registeredTimers.size() == clapTimerIdToHandlerMap.size());
            for (const auto &[id, han] : clapTimerIdToHandlerMap)
            {
                _DBGCOUT << "    Timer Unregister: " << _D(id) << std::endl;
                primaryPlugin->unregisterTimer(id);
            }
            clapTimerIdToHandlerMap.clear();

            plugins.erase(p);
            if (plugins.size() >= 1)
            {
                _DBGCOUT << "    Moving handlers to new plugin" << std::endl;
                setAsPrimaryPlugin(*(plugins.begin()));
            }
            else
            {
                primaryPlugin = nullptr;
            }
        }
        else
        {
            plugins.erase(p);
        }
    }

    /*
     * This is the VSTGUI FD API. We cache the eventHandler and fd pair as registered.
     * The easiest structure for that really is a list of pairs. The lookups are so varied
     * a linear search is fine.
     */
    std::vector<std::pair<int, VSTGUI::X11::IEventHandler *>> registeredEventHandlers;
    bool registerEventHandler(int fd, VSTGUI::X11::IEventHandler *handler) override
    {
        _DBGCOUT << _D(fd) << _D(handler) << std::endl;
        registeredEventHandlers.emplace_back(fd, handler);

        if (primaryPlugin)
        {
            auto res = primaryPlugin->registerPosixFd(fd);

            return res;
        }
        return true;
    }
    bool unregisterEventHandler(VSTGUI::X11::IEventHandler *handler) override
    {
        _DBGCOUT << _D(handler) << std::endl;

        auto it = registeredEventHandlers.begin();
        while (it != registeredEventHandlers.end())
        {
            const auto &[fd, v] = *it;
            if (v == handler)
            {
                _DBGCOUT << "Found an event handler to erase " << _D(fd) << std::endl;
                registeredEventHandlers.erase(it);
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
        for (const auto &[k, v] : registeredEventHandlers)
        {
            if (k == fd)
                v->onEvent();
        }
    }

    /*
     * Timers are slightly different. We need to keep their clap_id since that is
     * how we get called back. So we have both the registered timers and the clapTimerIdToHandlerMap
     */
    std::map<clap_id, VSTGUI::X11::ITimerHandler *> clapTimerIdToHandlerMap;
    std::vector<std::pair<uint64_t, VSTGUI::X11::ITimerHandler *>> registeredTimers;
    bool registerTimer(uint64_t interval, VSTGUI::X11::ITimerHandler *handler) override
    {
        _DBGCOUT << "regsiterTimer" << _D(handler) << std::endl;

        registeredTimers.emplace_back(interval, handler);

        if (primaryPlugin)
        {
            clap_id id;
            auto res = primaryPlugin->registerTimer(interval, &id);
            clapTimerIdToHandlerMap[id] = handler;
            _DBGCOUT << _D(interval) << _D(handler) << " as " << id << std::endl;
            return res;
        }
        else
        {
            _DBGCOUT << "Timer registered without primary plugin" << std::endl;
        }
        return true;
    }
    bool unregisterTimer(VSTGUI::X11::ITimerHandler *handler) override
    {
        _DBGCOUT << "unregsiterTimer" << _D(handler) << std::endl;

        auto rit = registeredTimers.begin();
        while (rit != registeredTimers.end())
        {
            if (rit->second == handler)
            {
                rit = registeredTimers.erase(rit);
                break;
            }
            rit++;
        }

        auto it = clapTimerIdToHandlerMap.begin();
        while (it != clapTimerIdToHandlerMap.end())
        {
            const auto &[k, v] = *it;
            if (v == handler)
            {
                _DBGCOUT << "Found a timer handler to erase " << k << std::endl;
                clapTimerIdToHandlerMap.erase(it);
                auto res = primaryPlugin->unregisterTimer(k);
                return res;
            }
            it++;
        }

        // Sigh. On stop we unregister twice.
        _DBGCOUT << "Found no timer for " << _D(handler) << ". "
                 << _D(clapTimerIdToHandlerMap.size()) << _D(registeredTimers.size()) << std::endl;
        return true;
    }
    void fireTimer(clap_id id)
    {
        for (const auto &[k, v] : clapTimerIdToHandlerMap)
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
void ClapSawDemo::onPosixFd(int fd, clap_posix_fd_flags_t flags) noexcept
{
    auto rlp = VSTGUI::X11::RunLoop::get().get();
    auto clp = reinterpret_cast<ClapRunLoop *>(rlp);
    if (clp)
        clp->fireFd(fd);
}

static bool everInit{false};

void addLinuxVSTGUIPlugin(ClapSawDemo *that)
{
    if (!everInit)
    {
        _DBGCOUT << "Creating VSTGUI RunLoop" << std::endl;
        VSTGUI::init(nullptr);
        VSTGUI::X11::RunLoop::init(VSTGUI::owned(new ClapRunLoop(that)));

        // What is this?
        VSTGUI::X11::RunLoop::init(nullptr);
        everInit = true;

        auto rl = VSTGUI::X11::RunLoop::get().get();
        auto crl = dynamic_cast<ClapRunLoop *>(rl);

        crl->initializationOver();
    }
    else
    {
        auto rl = VSTGUI::X11::RunLoop::get().get();
        if (!rl)
        {
            VSTGUI::X11::RunLoop::init(VSTGUI::owned(new ClapRunLoop(that)));
            rl = VSTGUI::X11::RunLoop::get().get();
        }

        auto crl = dynamic_cast<ClapRunLoop *>(rl);
        if (crl)
            crl->addPlugin(that);
    }
}
void removeLinuxVSTGUIPlugin(ClapSawDemo *that)
{
    auto rl = VSTGUI::X11::RunLoop::get().get();
    auto crl = dynamic_cast<ClapRunLoop *>(rl);

    if (crl)
    {
        crl->removePlugin(that);
    }
}

void exitLinuxVSTGUI() { VSTGUI::X11::RunLoop::exit(); }
#endif

} // namespace sst::clap_saw_demo