//
// Created by Paul Walker on 6/10/22.
//

#include "stupieditor.h"
#include "stupisaw.h"
#include <vstgui/lib/vstguiinit.h>

namespace BaconPaul
{
bool StupiSaw::guiCreate(const char *api, bool isFloating) noexcept
{
    _DBGMARK;
    static bool everInit{false};
    if (!everInit)
    {
        VSTGUI::init(CFBundleGetMainBundle());
        everInit = true;
    }
    editor = new StupiEditor();
    return editor != nullptr;
}
void StupiSaw::guiDestroy() noexcept
{
    _DBGMARK;
    if (editor)
        delete editor;
    editor = nullptr;
}
bool StupiSaw::guiShow() noexcept
{
    _DBGMARK;
    return Plugin::guiShow();
}
bool StupiSaw::guiHide() noexcept
{
    _DBGMARK;
    return Plugin::guiHide();
}
bool StupiSaw::guiSetParent(const clap_window *window) noexcept
{
    _DBGMARK;
    editor->getFrame()->open((void *)window->cocoa);
    return true;
}
bool StupiSaw::guiSetSize(uint32_t width, uint32_t height) noexcept
{
    _DBGMARK;
    editor->getFrame()->setSize(width, height);
    return true;
}

StupiEditor::StupiEditor()
{
    _DBGMARK;
    frame = new VSTGUI::CFrame(VSTGUI::CRect(0, 0, StupiSaw::guiw, StupiSaw::guih), this);
    frame->remember();
    auto lab = new VSTGUI::CTextLabel(VSTGUI::CRect(10, 10, 200, 200), "Hello");
    frame->addView(lab);
}

StupiEditor::~StupiEditor() { frame->forget(); }
} // namespace BaconPaul