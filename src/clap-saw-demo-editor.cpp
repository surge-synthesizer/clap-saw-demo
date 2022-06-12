//
// Created by Paul Walker on 6/10/22.
//

#include "clap-saw-demo-editor.h"
#include "clap-saw-demo.h"
#include <vstgui/lib/vstguiinit.h>

#if IS_LINUX
#include "vstgui/lib/platform/platform_x11.h"
#include "vstgui/lib/platform/linux/x11platform.h"
#include <map>
#endif

namespace sst::clap_saw_demo
{
#if IS_LINUX
struct ClapRunLoop : public VSTGUI::X11::IRunLoop, public VSTGUI::AtomicReferenceCounted
{
    ClapSawDemo *plugin{nullptr};
    ClapRunLoop(ClapSawDemo *p) : plugin(p) {
        _DBGCOUT << "Creating ClapRunLoop" << std::endl;
    }

    std::multimap<int, VSTGUI::X11::IEventHandler *> eventHandlers;
    bool registerEventHandler(int fd, VSTGUI::X11::IEventHandler *handler) override
    {
        _DBGCOUT << _D(fd) << _D(handler) << std::endl;
        auto res = plugin->registerPosixFd(fd);
        eventHandlers.insert({fd,handler});
        return res;
    }
    bool unregisterEventHandler(VSTGUI::X11::IEventHandler *handler) override {
        _DBGCOUT << _D(handler) << std::endl;
        auto res = false;
        for (const auto &[k,v] : eventHandlers)
            if (v == handler)
                res = plugin->unregisterPosixFD(k);
        return res;
    }
    void fireFd(int fd)
    {
        for (const auto &[k,v] : eventHandlers)
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
    bool unregisterTimer(VSTGUI::X11::ITimerHandler *handler) override {
        _DBGCOUT << "unregsiterTimer" << _D(handler) << std::endl;
        for (const auto &[k,v] : timerHandlers)
            if (v == handler)
                return plugin->unregisterTimer(k);
        return false;
    }
    void fireTimer(clap_id id)
    {
        for (const auto &[k,v] : timerHandlers)
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
#endif

bool ClapSawDemo::guiIsApiSupported(const char *api, bool isFloating) noexcept
{
    if (isFloating)
        return false;
#if IS_MAC
    if (strcmp(api, CLAP_WINDOW_API_COCOA) == 0)
        return true;
#endif

#if IS_WINDOWS
    if (strcmp(api, CLAP_WINDOW_API_WIN32) == 0)
        return true;
#endif

#if IS_LINUX
    if (strcmp(api, CLAP_WINDOW_API_X11) == 0)
        return true;
#endif

    return false;
}
bool ClapSawDemo::guiCreate(const char *api, bool isFloating) noexcept
{
    static bool everInit{false};
    if (!everInit)
    {
#if IS_MAC
        VSTGUI::init(CFBundleGetMainBundle());
#endif
#if IS_WINDOWS
        CoInitialize (nullptr);
        VSTGUI::init (GetModuleHandle (nullptr));
#endif
#if IS_LINUX
        VSTGUI::init(nullptr);
        VSTGUI::X11::RunLoop::init(VSTGUI::owned(new ClapRunLoop(this)));
#endif
        everInit = true;
    }
    editor = new ClapSawDemoEditor(toUiQ, fromUiQ, dataCopyForUI);

    for (const auto &[k,v] : paramToValue)
    {
        auto r = ToUI{.type = ToUI::PARAM_VALUE, .id = k, .value = *v};
        toUiQ.try_enqueue(r);
    }
    return editor != nullptr;
}
void ClapSawDemo::guiDestroy() noexcept
{
    if (editor)
        delete editor;
    editor = nullptr;
}

bool ClapSawDemo::guiSetParent(const clap_window *window) noexcept
{
#if IS_MAC
    editor->getFrame()->open(window->cocoa);
#endif
#if IS_LINUX
    editor->getFrame()->open((void*)(window->x11));
#endif
#if IS_WINDOWS
    assert(false);
#endif
    editor->setupUI(window);
    return true;
}

bool ClapSawDemo::guiSetSize(uint32_t width, uint32_t height) noexcept
{
    assert(editor);
    _DBGCOUT << _D(width) << _D(height) << std::endl;
    editor->getFrame()->setSize(width, height);
    editor->resize();
    editor->getFrame()->invalid();
    return true;
}


bool ClapSawDemo::guiAdjustSize(uint32_t *width, uint32_t *height) noexcept
{
    assert(editor);
    _DBGCOUT << _D(width) << _D(height) << std::endl;

    return true;
}


ClapSawDemoEditor::ClapSawDemoEditor(ClapSawDemo::SynthToUI_Queue_t &i,
                                     ClapSawDemo::UIToSynth_Queue_t &o,
                                     const ClapSawDemo::DataCopyForUI &d)
    : inbound(i), outbound(o), synthData(d)
{
    frame = new VSTGUI::CFrame(VSTGUI::CRect(0, 0, ClapSawDemo::GUI_DEFAULT_W, ClapSawDemo::GUI_DEFAULT_H), this);
    frame->setBackgroundColor(VSTGUI::CColor(0x30, 0x30, 0x80));
    frame->remember();
}

void ClapSawDemoEditor::setupUI(const clap_window_t *w)
{
    _DBGMARK;

    auto l = new VSTGUI::CTextLabel(VSTGUI::CRect(0, 0, getFrame()->getWidth(), 40), "Clap Saw Synth Demo");
    l->setTransparency(false);
    l->setBackColor(VSTGUI::CColor(0x50, 0x20, 0x20));
    l->setFont(VSTGUI::kNormalFontVeryBig);
    l->setHoriAlign(VSTGUI::CHoriTxtAlign::kCenterText);
    l->setFrameWidth(1);
    l->setFrameColor(VSTGUI::CColor(0xFF, 0xFF, 0xFF));
    topLabel = l;
    frame->addView(topLabel);

    l = new VSTGUI::CTextLabel(VSTGUI::CRect(VSTGUI::CPoint(0,40), VSTGUI::CPoint(getFrame()->getWidth(), 20)), "status");
    l->setTransparency(false);
    l->setBackColor(VSTGUI::CColor(0x50, 0x20, 0x20));
    l->setFont(VSTGUI::kNormalFontSmaller);
    l->setHoriAlign(VSTGUI::CHoriTxtAlign::kCenterText);
    l->setFrameWidth(1);
    l->setFrameColor(VSTGUI::CColor(0xFF, 0xFF, 0xFF));
    statusLabel = l;
    frame->addView(statusLabel);

    l = new VSTGUI::CTextLabel(VSTGUI::CRect(VSTGUI::CPoint(0,getFrame()->getHeight()-20), VSTGUI::CPoint(getFrame()->getWidth(), 20)), "https://some-url-soon");
    l->setTransparency(false);
    l->setBackColor(VSTGUI::CColor(0x50, 0x20, 0x20));
    l->setFont(VSTGUI::kNormalFontSmaller);
    l->setHoriAlign(VSTGUI::CHoriTxtAlign::kCenterText);
    l->setFrameWidth(1);
    l->setFrameColor(VSTGUI::CColor(0xFF, 0xFF, 0xFF));
    bottomLabel = l;
    frame->addView(bottomLabel);

    auto mkSlider = [this](int x, int y, int tag)
    {
        auto q = new VSTGUI::CSlider(VSTGUI::CRect(VSTGUI::CPoint(x, y),
                                                   VSTGUI::CPoint(25, 150)),
                                     this, tag, 0, 150, nullptr,
                                     nullptr);
        q->setMin(0);
        q->setMax(1);
        q->setDrawStyle(VSTGUI::CSlider::kDrawFrame | VSTGUI::CSlider::kDrawValue |
                        VSTGUI::CSlider::kDrawBack);
        q->setStyle(VSTGUI::CSlider::kVertical | VSTGUI::CSlider::kBottom);
        frame->addView(q);
        return q;
    };

    oscUnison = mkSlider(10,65,tags::unict);
    paramIdToCControl[ClapSawDemo::pmUnisonCount] = oscUnison;

    oscSpread = mkSlider(40,65,tags::unisp);
    paramIdToCControl[ClapSawDemo::pmUnisonSpread] = oscSpread;

    ampAttack = mkSlider(70, 65, tags::env_a);
    paramIdToCControl[ClapSawDemo::pmAmpAttack] = ampAttack;

    ampRelease = mkSlider(100, 65, tags::env_r);
    paramIdToCControl[ClapSawDemo::pmAmpRelease] = ampRelease;

    idleTimer = new VSTGUI::CVSTGUITimer([this](VSTGUI::CVSTGUITimer *) { this->idle(); }, 33);
    idleTimer->remember();
}

ClapSawDemoEditor::~ClapSawDemoEditor()
{
    _DBGMARK;
    frame->forget();
    idleTimer->stop();
    idleTimer->forget();
}

void ClapSawDemoEditor::resize()
{
    auto w = getFrame()->getWidth();
    auto h = getFrame()->getHeight();
    topLabel->setViewSize(VSTGUI::CRect(0,0,w,40));
    topLabel->invalid();

    statusLabel->setViewSize(VSTGUI::CRect(VSTGUI::CPoint(0,40), VSTGUI::CPoint(w,20)));
    bottomLabel->setViewSize(VSTGUI::CRect(VSTGUI::CPoint(0,h-20), VSTGUI::CPoint(w,20)));
}

uint32_t ClapSawDemoEditor::paramIdFromTag(int32_t tag)
{
    switch (tag)
    {
    case tags::env_r:
        return ClapSawDemo::pmAmpRelease;
    case tags::unict:
        return ClapSawDemo::pmUnisonCount;
    case tags::unisp:
        return ClapSawDemo::pmUnisonSpread;
    case tags::env_a:
        return ClapSawDemo::pmAmpAttack;
    }
    assert(false);
    return 0;
}

void ClapSawDemoEditor::valueChanged(VSTGUI::CControl *c)
{
    auto t = (tags)c->getTag();
    auto q = ClapSawDemo::FromUI();
    q.id = paramIdFromTag(t);
    q.type = ClapSawDemo::FromUI::MType::ADJUST_VALUE;
    auto send = true;
    switch (t)
    {
        // 0..1
    case tags::env_a:
    case tags::env_r:
    {
        q.value = c->getValue();
        break;
    }
    case tags::unisp:
    {
        q.value = c->getValue() * 100.0;
        break;
    }
    case tags::unict:
    {
        q.value = c->getValue() * SawDemoVoice::max_uni;
    }
    }
    if (send)
        outbound.try_enqueue(q);
}

void ClapSawDemoEditor::idle()
{
    ClapSawDemo::ToUI r;
    while (inbound.try_dequeue(r))
    {
        if (r.type == ClapSawDemo::ToUI::MType::PARAM_VALUE)
        {
            auto q = paramIdToCControl.find(r.id);

            if (q != paramIdToCControl.end())
            {
                auto cc = q->second;
                auto val = r.value;

                switch(r.id)
                {
                case ClapSawDemo::pmUnisonSpread:
                    val = val / 100.0;
                    break;
                case ClapSawDemo::pmUnisonCount:
                    val = val / SawDemoVoice::max_uni;
                    break;

                default:
                    break;
                }
                cc->setValue(val);
                cc->invalid();
            }
        }
    }

    if (synthData.updateCount != lastDataUpdate)
    {
        lastDataUpdate = synthData.updateCount;

        auto sl = std::string( "status : poly=" ) + std::to_string(synthData.polyphony);
        statusLabel->setText(sl.c_str());
        statusLabel->invalid();
    }
}
void ClapSawDemoEditor::beginEdit(int32_t index)
{
    auto q = ClapSawDemo::FromUI();
    q.id = paramIdFromTag(index);
    q.type = ClapSawDemo::FromUI::MType::BEGIN_EDIT;
    outbound.try_enqueue(q);
}
void ClapSawDemoEditor::endEdit(int32_t index)
{
    auto q = ClapSawDemo::FromUI();
    q.id = paramIdFromTag(index);
    q.type = ClapSawDemo::FromUI::MType::END_EDIT;
    outbound.try_enqueue(q);
}

} // namespace sst::clap_saw_demo