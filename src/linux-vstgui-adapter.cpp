//
// Created by paul on 6/13/22.
//

#include <vstgui/lib/vstguiinit.h>

#if IS_LINUX
#include "vstgui/lib/platform/platform_x11.h"
#include "vstgui/lib/platform/linux/x11platform.h"
#include <map>
#endif
#include "clap-saw-demo.h"
#include "linux-vstgui-adapter.h"

namespace sst::clap_saw_demo
{

#if IS_LINUX
struct ClapRunLoop : public VSTGUI::X11::IRunLoop, public VSTGUI::AtomicReferenceCounted
{
    ClapSawDemo *plugin{nullptr};
    ClapRunLoop(ClapSawDemo *p) : plugin(p) { _DBGCOUT << "Creating ClapRunLoop" << std::endl; }

    std::multimap<int, VSTGUI::X11::IEventHandler *> eventHandlers;
    bool registerEventHandler(int fd, VSTGUI::X11::IEventHandler *handler) override
    {
        _DBGCOUT << _D(fd) << _D(handler) << std::endl;
        auto res = plugin->registerPosixFd(fd);
        eventHandlers.insert({fd, handler});
        return res;
    }
    bool unregisterEventHandler(VSTGUI::X11::IEventHandler *handler) override
    {
        _DBGCOUT << _D(handler) << std::endl;
        auto it = eventHandlers.begin();
        while (it != eventHandlers.end())
        {
            const auto &[k, v] = *it;
            if (v == handler)
            {
                _DBGCOUT << "Found an event handler to erase" << std::endl;
                eventHandlers.erase(it);
                return plugin->unregisterPosixFD(k);
            }
            it++;
        }

        return false;
    }
    void fireFd(int fd)
    {
        for (const auto &[k, v] : eventHandlers)
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
        auto res = plugin->registerTimer(interval, &id);
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
                return plugin->unregisterTimer(k);
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
    clp->fireTimer(timerId);
}
void ClapSawDemo::onPosixFd(int fd, int flags) noexcept
{
    auto rlp = VSTGUI::X11::RunLoop::get().get();
    auto clp = reinterpret_cast<ClapRunLoop *>(rlp);
    clp->fireFd(fd);
}

void addLinuxVSTGUIPlugin(ClapSawDemo *that)
{
    _DBGCOUT << "addlingLinxuVSTGUIPlugin " << that << std::endl;
    VSTGUI::init(nullptr);
    VSTGUI::X11::RunLoop::init(VSTGUI::owned(new ClapRunLoop(that)));
}
void removeLinuxVSTGUIPlugin(ClapSawDemo *that)
{
    _DBGCOUT << "addlingLinxuVSTGUIPlugin " << that << std::endl;
}
#endif

} // namespace sst::clap_saw_demo