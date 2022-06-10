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
    editor = new StupiEditor(toUiQ, fromUiQ);
    return editor != nullptr;
}
void StupiSaw::guiDestroy() noexcept
{
    _DBGMARK;
    if (editor)
        delete editor;
    editor = nullptr;
}

bool StupiSaw::guiSetParent(const clap_window *window) noexcept
{
    _DBGMARK;
    editor->getFrame()->open((void *)window->cocoa);
    editor->setupUI();
    return true;
}

StupiEditor::StupiEditor(StupiSaw::SynthToUI_Queue_t &i, StupiSaw::UIToSynth_Queue_t &o)
    : inbound(i), outbound(o)
{
    _DBGMARK;
    frame = new VSTGUI::CFrame(VSTGUI::CRect(0, 0, StupiSaw::guiw, StupiSaw::guih), this);
    frame->remember();
}

void StupiEditor::setupUI()
{
    _DBGMARK;

    ampAttack = new VSTGUI::CTextButton(VSTGUI::CRect(10, 10, 50, 50), this, 100, "Yo");
    _DBGCOUT << ampAttack->getTag() << std::endl;
    frame->addView(ampAttack);

    idleTimer = new VSTGUI::CVSTGUITimer([this](VSTGUI::CVSTGUITimer *) { this->idle(); }, 33);
    idleTimer->remember();

}

StupiEditor::~StupiEditor()
{
    _DBGMARK;
    frame->forget();
    idleTimer->stop();
    idleTimer->forget();
}

void StupiEditor::valueChanged(VSTGUI::CControl *c)
{
    auto t = (tags)c->getTag();
    std::cout << "ValueChanged " << c->getTag() << " " << c->getValue() << std::endl;
    switch (t)
    {
    case tags::env_a:
    {
        auto q = StupiSaw::FromUI();
        q.value = c->getValue();
        q.id = StupiSaw::pmAmpAttack;
        outbound.try_enqueue(q);
        break;
    }
    }
}

void StupiEditor::idle()
{
    StupiSaw::ToUI r;
    while (inbound.try_dequeue(r))
    {
        if (r.type == StupiSaw::ToUI::MType::PARAM_VALUE)
        {
            if (r.id == StupiSaw::pmAmpAttack)
            {
                ampAttack->setValue(r.value);
                ampAttack->invalid();
            }
        }
        _DBGCOUT << "inbound " << r.id << " " << r.value << std::endl;
    }
}

} // namespace BaconPaul