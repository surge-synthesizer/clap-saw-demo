//
// Created by Paul Walker on 6/10/22.
//

#ifndef CLAP_SAW_DEMO_EDITOR_H
#define CLAP_SAW_DEMO_EDITOR_H
#include <vstgui/vstgui.h>
#include "clap-saw-demo.h"

namespace sst::clap_saw_demo
{
struct ClapSawDemoEditor : public VSTGUI::VSTGUIEditorInterface, public VSTGUI::IControlListener
{
    ClapSawDemo::SynthToUI_Queue_t &inbound;
    ClapSawDemo::UIToSynth_Queue_t &outbound;

    ClapSawDemoEditor(ClapSawDemo::SynthToUI_Queue_t &, ClapSawDemo::UIToSynth_Queue_t &);
    ~ClapSawDemoEditor();

    void valueChanged(VSTGUI::CControl *) override;
    void beginEdit(int32_t index) override;
    void endEdit(int32_t index) override;
    enum tags : int32_t
    {
        env_a = 100
    };

    void setupUI(const clap_window_t *);
    uint32_t paramIdFromTag(int32_t tag);

    void idle();

    VSTGUI::CVSTGUITimer *idleTimer;

  private:
    // These are all weak references owned by the frame
    VSTGUI::CControl *ampAttack{nullptr}, *ampRelease{nullptr};
    VSTGUI::CControl *oscUnison{nullptr}, *oscSpread{nullptr};
    VSTGUI::CControl *filtCutoff{nullptr}, *filtRes{nullptr};
    VSTGUI::CControl *filtDecay{nullptr}, *filtModDepth{nullptr};
};
} // namespace sst::clap_saw_demo
#endif
