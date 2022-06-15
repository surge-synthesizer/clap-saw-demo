//
// Created by Paul Walker on 6/10/22.
//

#include "clap-saw-demo-editor.h"
#include "clap-saw-demo.h"
#include <vstgui/lib/vstguiinit.h>
#include "vstgui/lib/finally.h"

#if IS_LINUX
#include "vstgui/lib/platform/platform_x11.h"
#include "vstgui/lib/platform/linux/x11platform.h"
#include <map>
#include "linux-vstgui-adapter.h"
#endif

#if IS_WIN
#include <Windows.h>
#endif

namespace sst::clap_saw_demo
{
/*
 * Part one of this implementation is the plugin adapters which allow a GUI to attach.
 * These are the ClapSawDemo methods. First up: Which windowing API do we support.
 * Pretty obviously, mac supports cocoa, windows supports win32, and linux supports
 * X11.
 */
bool ClapSawDemo::guiIsApiSupported(const char *api, bool isFloating) noexcept
{
    if (isFloating)
        return false;
#if IS_MAC
    if (strcmp(api, CLAP_WINDOW_API_COCOA) == 0)
        return true;
#endif

#if IS_WIN
    if (strcmp(api, CLAP_WINDOW_API_WIN32) == 0)
        return true;
#endif

#if IS_LINUX
    if (strcmp(api, CLAP_WINDOW_API_X11) == 0)
        return true;
#endif

    return false;
}

/*
 * GUICreate gets called when the host requests the plugin create its editor with
 * a given API. We ignore the API and isFloating here, because we handled them
 * above and assuem our host follows the protocol that it only calls us with
 * values which are supported.
 *
 * The important thing from a VSTGUI perspective here is that we have to initialize
 * the VSTGUI static data structures. On Mac and Windows, this is an easy call and
 * we can use the VSTGUI::finally mechanism to clean up. On Linux there is a more
 * complicated global event loop to merge which, thanks to the way VSTGUI structures
 * their event loops, is a touch more awkward. As such the linux code is all in a different
 * cpp file for individual documentation (Please see the README for any linux disclaimers
 * and most recent status).
 */
bool ClapSawDemo::guiCreate(const char *api, bool isFloating) noexcept
{
    _DBGMARK;
    static bool everInit{false};
    if (!everInit)
    {
#if IS_MAC
        VSTGUI::init(CFBundleGetMainBundle());
#endif
#if IS_WIN
        VSTGUI::init(GetModuleHandle(nullptr));
#endif


        // This prooves unreliable
        static auto cleanup = VSTGUI::finally(
            []()
            {
                _DBGCOUT << "Exiting VSTGUI" << std::endl;
                VSTGUI::exit();
                _DBGCOUT << "VSTGUI Exit done" << std::endl;
            });


        everInit = true;
    }

#if IS_LINUX
    addLinuxVSTGUIPlugin(this);
#endif
    editor = new ClapSawDemoEditor(toUiQ, fromUiQ, dataCopyForUI);

    return editor != nullptr;
}

/*
 * guiDestroy destorys the editor object and returns it to the
 * nullptr sentinel, to stop ::process sending events to the ui.
 */
void ClapSawDemo::guiDestroy() noexcept
{
    _DBGMARK;

    // We need to split this because of linux
    editor->haltIdleTimer();

#if IS_LINUX
    removeLinuxVSTGUIPlugin(this);
#endif

    if (editor)
        delete editor;
    editor = nullptr;
}

/*
 * guiSetParent is the core API for a clap HOST which has a window to
 * reparent the editor to that host managed window. It sends a
 * `const clap_window *window` data structure which contains a union of
 * platform specific window pointers.
 *
 * VSTGUI handles reparenting through `VSTGUI::CFrame::open` which consumes
 * a pointer to a native window. This makes adapting easy. Our editor object
 * owns a `CFrame` as its base window, and setParent opens it with the new
 * parent platform specific item handed to it.
 */
bool ClapSawDemo::guiSetParent(const clap_window *window) noexcept
{
#if IS_MAC
    editor->getFrame()->open(window->cocoa);
#endif
#if IS_LINUX
    editor->getFrame()->open((void *)(window->x11));
#endif
#if IS_WIN
    editor->getFrame()->open(window->win32);
#endif

    // Once we are reparented, we can set up our UI
    editor->setupUI();

    // and ask the engine to refresh
    refreshUIValues = true;

    // And we are done!
    return true;
}

/*
 * Sizing is described in the gui extension, but this implementation
 * means that if the host drags to resize, we accept its size and resize our frame
 */
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

/*
 * Part TWO of this example is actually writing a VSTGUI UI
 *
 * Surge used to be all VSTGUI but we ported to JUCE. Doing that meant my
 * VSTGUI is a bit rusty. Also I'm not a designer. But really the only thing
 * here which is even mildly unexpected is how events go from the editor back to the engine
 * and from the engine to the UI using the thread safe queues, which our editor is
 * constructed with references to.
 */

/*
 * This is a throwaway component which just makes the background.
 */
struct ClapSawDemoBackground : public VSTGUI::CView
{
    explicit ClapSawDemoBackground(const VSTGUI::CRect &s) : VSTGUI::CView(s) {}

    void draw(VSTGUI::CDrawContext *dc) override;
    int polyCount{0};
};

ClapSawDemoEditor::ClapSawDemoEditor(ClapSawDemo::SynthToUI_Queue_t &i,
                                     ClapSawDemo::UIToSynth_Queue_t &o,
                                     const ClapSawDemo::DataCopyForUI &d)
    : inbound(i), outbound(o), synthData(d)
{
    frame = new VSTGUI::CFrame(
        VSTGUI::CRect(0, 0, ClapSawDemo::GUI_DEFAULT_W, ClapSawDemo::GUI_DEFAULT_H), this);
    frame->setBackgroundColor(VSTGUI::CColor(0x30, 0x30, 0x80));
    frame->remember();
}

// Create and add our UI objects with a callback tag. Completely standard VSTGUI
void ClapSawDemoEditor::setupUI()
{
    _DBGMARK;

    backgroundRender = new ClapSawDemoBackground(
        VSTGUI::CRect(0, 0, getFrame()->getWidth(), getFrame()->getHeight()));
    frame->addView(backgroundRender);

    auto l = new VSTGUI::CTextLabel(VSTGUI::CRect(0, 0, getFrame()->getWidth(), 40),
                                    "Clap Saw Synth Demo");
    l->setTransparency(true);
    l->setFont(VSTGUI::kNormalFontVeryBig);
    l->setHoriAlign(VSTGUI::CHoriTxtAlign::kCenterText);
    topLabel = l;
    frame->addView(topLabel);

    l = new VSTGUI::CTextLabel(
        VSTGUI::CRect(VSTGUI::CPoint(0, 40), VSTGUI::CPoint(getFrame()->getWidth(), 20)), "poly=0");
    l->setTransparency(true);
    l->setFont(VSTGUI::kNormalFont);
    l->setHoriAlign(VSTGUI::CHoriTxtAlign::kCenterText);
    statusLabel = l;
    frame->addView(statusLabel);

    l = new VSTGUI::CTextLabel(VSTGUI::CRect(VSTGUI::CPoint(0, getFrame()->getHeight() - 40),
                                             VSTGUI::CPoint(getFrame()->getWidth(), 20)),
                               "https://github.com/surge-synthesizer/clap-saw-demo");
    l->setTransparency(true);
    l->setFont(VSTGUI::kNormalFontSmaller);
    l->setHoriAlign(VSTGUI::CHoriTxtAlign::kCenterText);
    bottomLabel = l;
    frame->addView(bottomLabel);

    auto sl = std::string("MIT License; CLAP v.") + std::to_string(CLAP_VERSION_MAJOR) + "." +
              std::to_string(CLAP_VERSION_MINOR) + "." + std::to_string(CLAP_VERSION_REVISION);
    l = new VSTGUI::CTextLabel(VSTGUI::CRect(VSTGUI::CPoint(0, getFrame()->getHeight() - 20),
                                             VSTGUI::CPoint(getFrame()->getWidth(), 20)),
                               sl.c_str());
    l->setTransparency(true);
    l->setFont(VSTGUI::kNormalFontSmaller);
    l->setHoriAlign(VSTGUI::CHoriTxtAlign::kCenterText);
    bottomLabel = l;
    frame->addView(bottomLabel);

    auto mkSliderWithLabel = [this](int x, int y, int tag, const std::string &label)
    {
        auto q = new VSTGUI::CSlider(VSTGUI::CRect(VSTGUI::CPoint(x, y), VSTGUI::CPoint(25, 150)),
                                     this, tag, 0, 150, nullptr, nullptr);
        q->setMin(0);
        q->setMax(1);
        q->setDrawStyle(VSTGUI::CSlider::kDrawFrame | VSTGUI::CSlider::kDrawValue |
                        VSTGUI::CSlider::kDrawBack);
        q->setStyle(VSTGUI::CSlider::kVertical | VSTGUI::CSlider::kBottom);
        frame->addView(q);

        auto l = new VSTGUI::CTextLabel(
            VSTGUI::CRect(VSTGUI::CPoint(x - 10, y + 155), VSTGUI::CPoint(45, 15)));
        l->setText(label.c_str());

        frame->addView(l);
        return q;
    };

    auto oscRow = 70, aegRow = 70 + 150 + 80, endRow = 160;
    oscUnison = mkSliderWithLabel(10, oscRow, tags::unict, "Uni Ct");
    paramIdToCControl[ClapSawDemo::pmUnisonCount] = oscUnison;

    oscSpread = mkSliderWithLabel(70, oscRow, tags::unisp, "Spread");
    paramIdToCControl[ClapSawDemo::pmUnisonSpread] = oscSpread;

    oscDetune = mkSliderWithLabel(130, oscRow, tags::oscdetune, "Detune");
    oscDetune->setMin(-1);
    oscDetune->setDrawStyle(oscDetune->getDrawStyle() | VSTGUI::CSlider::kDrawValueFromCenter |
                            VSTGUI::CSlider::kDrawInverted);
    paramIdToCControl[ClapSawDemo::pmOscDetune] = oscDetune;

    ampAttack = mkSliderWithLabel(10, aegRow, tags::env_a, "Attack");
    paramIdToCControl[ClapSawDemo::pmAmpAttack] = ampAttack;

    ampRelease = mkSliderWithLabel(70, aegRow, tags::env_r, "Release");
    paramIdToCControl[ClapSawDemo::pmAmpRelease] = ampRelease;

    preFilterVCA = mkSliderWithLabel(210, endRow, tags::vca, "VCA");
    paramIdToCControl[ClapSawDemo::pmPreFilterVCA] = preFilterVCA;

    filtCutoff = mkSliderWithLabel(290, endRow, tags::cutoff, "Cutoff");
    paramIdToCControl[ClapSawDemo::pmCutoff] = filtCutoff;

    filtRes = mkSliderWithLabel(350, endRow, tags::resonance, "Res");
    paramIdToCControl[ClapSawDemo::pmResonance] = filtRes;

    idleTimer = new VSTGUI::CVSTGUITimer([this](VSTGUI::CVSTGUITimer *) { this->idle(); }, 33);
    idleTimer->remember();
}

ClapSawDemoEditor::~ClapSawDemoEditor()
{
    _DBGMARK;
    frame->forget();
    frame = nullptr;
}

void ClapSawDemoEditor::haltIdleTimer()
{
    _DBGCOUT << "Stopping idle timer" << std::endl;
    idleTimer->stop();
    idleTimer->forget();
    idleTimer = nullptr;
}
// We add this resize method and call it from setSize to scale the background and recenter labels
void ClapSawDemoEditor::resize()
{
    auto w = getFrame()->getWidth();
    auto h = getFrame()->getHeight();
    backgroundRender->setViewSize(VSTGUI::CRect(0, 0, w, h));
    backgroundRender->invalid();

    topLabel->setViewSize(VSTGUI::CRect(0, 0, w, 40));
    topLabel->invalid();

    statusLabel->setViewSize(VSTGUI::CRect(VSTGUI::CPoint(0, 40), VSTGUI::CPoint(w, 20)));
    bottomLabel->setViewSize(VSTGUI::CRect(VSTGUI::CPoint(0, h - 20), VSTGUI::CPoint(w, 20)));
}

/*
 * A tiny little utility mapping between our VSTGUI control tags and
 * our synth parameters. These *could* be the same but having them different
 * makes sure I don't assume they are the same.
 */
uint32_t ClapSawDemoEditor::paramIdFromTag(int32_t tag)
{
    switch ((ClapSawDemoEditor::tags)tag)
    {
    case tags::env_r:
        return ClapSawDemo::pmAmpRelease;
    case tags::unict:
        return ClapSawDemo::pmUnisonCount;
    case tags::unisp:
        return ClapSawDemo::pmUnisonSpread;
    case tags::env_a:
        return ClapSawDemo::pmAmpAttack;
    case tags::vca:
        return ClapSawDemo::pmPreFilterVCA;
    case tags::cutoff:
        return ClapSawDemo::pmCutoff;
    case tags::resonance:
        return ClapSawDemo::pmResonance;
    case tags::oscdetune:
        return ClapSawDemo::pmOscDetune;
    }
    assert(false);
    return 0;
}

/*
 * The primary thing valueChanged needs to do is
 *
 * 1; Scale our VSTGUI 0..1 or -1..1 values to the right scale and
 * 2: Send an outbound queue event to the lock free ui -> engine queue with
 *    the info.
 */
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
    case tags::resonance:
    case tags::vca:
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
    case tags::oscdetune:
    {
        q.value = c->getValue() * 200.0;
        break;
    }
    case tags::unict:
    {
        q.value = c->getValue() * SawDemoVoice::max_uni;
        break;
    }
    case tags::cutoff:
    {
        q.value = c->getValue() * 126 + 1;
    }
    }
    if (send)
        outbound.try_enqueue(q);
}

/*
 * Similarly, beginEdit / endEdit need to map the gui tag to a param id and then
 * enqueue an outbound event.
 */
void ClapSawDemoEditor::beginEdit(int32_t tag)
{
    auto q = ClapSawDemo::FromUI();
    q.id = paramIdFromTag(tag);
    q.type = ClapSawDemo::FromUI::MType::BEGIN_EDIT;
    outbound.try_enqueue(q);
}
void ClapSawDemoEditor::endEdit(int32_t tag)
{
    auto q = ClapSawDemo::FromUI();
    q.id = paramIdFromTag(tag);
    q.type = ClapSawDemo::FromUI::MType::END_EDIT;
    outbound.try_enqueue(q);
}

/*
 * The ::idle method polls the inbound queue and value-based data structure,
 * responds by rescaling values and setting them on UI elements, and then invalidates
 * the appropriate UI control.
 */
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

                switch (r.id)
                {
                case ClapSawDemo::pmUnisonSpread:
                    val = val / 100.0;
                    break;
                case ClapSawDemo::pmOscDetune:
                    val = val / 200.0;
                    break;
                case ClapSawDemo::pmUnisonCount:
                    val = val / SawDemoVoice::max_uni;
                    break;
                case ClapSawDemo::pmCutoff:
                    val = (val - 1) / 126.0;
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

        auto sl = std::string("poly=") + std::to_string(synthData.polyphony);
        statusLabel->setText(sl.c_str());
        statusLabel->invalid();
        backgroundRender->polyCount = synthData.polyphony;
        backgroundRender->invalid();
    }
}

// Small irrelevant detail of how we draw the background
void ClapSawDemoBackground::draw(VSTGUI::CDrawContext *dc)
{
    auto r = VSTGUI::CRect(0, 0, getWidth(), getHeight());
    dc->setFillColor(VSTGUI::CColor(0x20, 0x20, 0x50));
    dc->drawRect(r, VSTGUI::kDrawFilled);

    auto t = VSTGUI::CRect(0, 0, getWidth(), 60);
    dc->setFillColor(VSTGUI::CColor(0x40, 0x40, 0x90));
    dc->drawRect(t, VSTGUI::kDrawFilled);

    auto b = VSTGUI::CRect(VSTGUI::CPoint(0, getHeight() - 40), VSTGUI::CPoint(getWidth(), 40));
    dc->setFillColor(VSTGUI::CColor(0x40, 0x40, 0x90));
    dc->drawRect(b, VSTGUI::kDrawFilled);

    if (polyCount == 0)
    {
        dc->setFrameColor(VSTGUI::CColor(0x80, 0x80, 0xA0));
        dc->setLineWidth(1);
    }
    else
    {
        auto add = std::clamp(polyCount * 5, 0, 0x40);
        dc->setFrameColor(VSTGUI::CColor(0xAF + add, 0xAF + add, 0xAF + add));
        dc->setLineWidth(2 + polyCount / 5.0);
    }

    dc->drawLine(VSTGUI::CPoint(160, 90), VSTGUI::CPoint(222, 90));
    dc->drawLine(VSTGUI::CPoint(222, 90), VSTGUI::CPoint(222, 150));
    dc->drawLine(VSTGUI::CPoint(100, 400), VSTGUI::CPoint(222, 400));
    dc->drawLine(VSTGUI::CPoint(222, 400), VSTGUI::CPoint(222, 340));
    dc->drawLine(VSTGUI::CPoint(240, 235), VSTGUI::CPoint(285, 235));
}

} // namespace sst::clap_saw_demo