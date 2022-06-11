//
// Created by Paul Walker on 6/10/22.
//

#ifndef STUPISAW_STUPIEDITOR_H
#define STUPISAW_STUPIEDITOR_H
#include <vstgui/vstgui.h>
#include "stupisaw.h"

namespace BaconPaul
{
struct StupiEditor : public VSTGUI::VSTGUIEditorInterface, public VSTGUI::IControlListener
{
    StupiSaw::SynthToUI_Queue_t &inbound;
    StupiSaw::UIToSynth_Queue_t &outbound;

    StupiEditor(StupiSaw::SynthToUI_Queue_t &, StupiSaw::UIToSynth_Queue_t &);
    ~StupiEditor();

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
} // namespace BaconPaul
#endif // STUPISAW_STUPIEDITOR_H
