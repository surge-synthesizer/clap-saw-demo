//
// Created by Paul Walker on 6/10/22.
//

#ifndef CLAP_SAW_DEMO_EDITOR_H
#define CLAP_SAW_DEMO_EDITOR_H
#include <vstgui/vstgui.h>
#include "clap-saw-demo.h"

namespace sst::clap_saw_demo
{
struct ClapSawDemoBackground;

/*
 * ClapSawDemoEditor is a VSTGUI Editor class and IControllerListener which does nothing
 * special or unexpected; it creates and holds a CFrame, handles edit events, and so on.
 * The majority of the explanatory comments are in clap-saw-demo-editor.cpp
 */
struct ClapSawDemoEditor : public VSTGUI::VSTGUIEditorInterface, public VSTGUI::IControlListener
{
    ClapSawDemo::SynthToUI_Queue_t &inbound;
    ClapSawDemo::UIToSynth_Queue_t &outbound;
    const ClapSawDemo::DataCopyForUI &synthData;
    std::function<void()> paramRequestFlush;

    ClapSawDemoEditor(ClapSawDemo::SynthToUI_Queue_t &, ClapSawDemo::UIToSynth_Queue_t &,
                      const ClapSawDemo::DataCopyForUI &, std::function<void()>);
    ~ClapSawDemoEditor();

    void haltIdleTimer();

    void valueChanged(VSTGUI::CControl *) override;
    void beginEdit(int32_t index) override;
    void endEdit(int32_t index) override;
    enum tags : int32_t
    {
        env_a = 100,
        env_r,
        unict,
        unisp,
        oscdetune,

        vca,
        cutoff,
        resonance

    };

    void setupUI();
    uint32_t paramIdFromTag(int32_t tag);

    void idle();
    void resize();

    VSTGUI::CVSTGUITimer *idleTimer{nullptr};

  private:
    uint32_t lastDataUpdate{0};
    ClapSawDemoBackground *backgroundRender{nullptr};
    // These are all weak references owned by the frame
    VSTGUI::CTextLabel *topLabel{nullptr}, *bottomLabel{nullptr}, *statusLabel{nullptr};

    VSTGUI::CTextLabel *ampLabel{nullptr};
    VSTGUI::CCheckBox *ampToggle{nullptr};
    VSTGUI::CSlider *ampAttack{nullptr}, *ampRelease{nullptr};
    VSTGUI::CSlider *oscUnison{nullptr}, *oscSpread{nullptr}, *oscDetune{nullptr};
    VSTGUI::CSlider *preFilterVCA{nullptr}, *filtCutoff{nullptr}, *filtRes{nullptr};

    std::unordered_map<int, VSTGUI::CControl *> paramIdToCControl;
};
} // namespace sst::clap_saw_demo
#endif
