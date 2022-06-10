//
// Created by Paul Walker on 6/10/22.
//

#ifndef STUPISAW_STUPIEDITOR_H
#define STUPISAW_STUPIEDITOR_H
#include <vstgui/vstgui.h>

namespace BaconPaul
{
struct StupiEditor : public VSTGUI::VSTGUIEditorInterface
{
    StupiEditor();
    ~StupiEditor();

    VSTGUI::CFrame *getFrame() const { return frame; }

  private:
    VSTGUI::CFrame *frame{nullptr};
};
} // namespace BaconPaul
#endif // STUPISAW_STUPIEDITOR_H
