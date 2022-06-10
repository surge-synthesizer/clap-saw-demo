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
    enum tags : int32_t
    {
        env_a = 100
    };

    VSTGUI::CFrame *getFrame() const override { return frame; }


    void setupUI();

    void idle();

    VSTGUI::CVSTGUITimer *idleTimer;

  private:

    VSTGUI::CFrame *frame{nullptr};

    // These are all weak references owned by the frame
    VSTGUI::CControl *ampAttack{nullptr};
};
} // namespace BaconPaul
#endif // STUPISAW_STUPIEDITOR_H
