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
    if (editor)
        delete editor;
    editor = nullptr;
}

bool StupiSaw::guiSetParent(const clap_window *window) noexcept
{
    editor->getFrame()->open(window->cocoa);
    editor->setupUI(window);
    return true;
}

StupiEditor::StupiEditor(StupiSaw::SynthToUI_Queue_t &i, StupiSaw::UIToSynth_Queue_t &o)
    : inbound(i), outbound(o)
{
    frame = new VSTGUI::CFrame(VSTGUI::CRect(0, 0, StupiSaw::guiw, StupiSaw::guih), this);
    frame->setBackgroundColor(VSTGUI::CColor(0x30, 0x30, 0x80));
    frame->remember();
}

void StupiEditor::setupUI(const clap_window_t *w)
{
    _DBGMARK;

    auto l = new VSTGUI::CTextLabel(VSTGUI::CRect(10, 5, 200, 40), "StupiSaw");
    l->setFont(VSTGUI::kNormalFontVeryBig);
    frame->addView(l);

    auto q = new VSTGUI::CSlider(VSTGUI::CRect(10, 30, 55, 100), this, tags::env_a, 0, 100, nullptr,
                                 nullptr);
    q->setMin(0);
    q->setMax(1);
    q->setDrawStyle(VSTGUI::CSlider::kDrawFrame | VSTGUI::CSlider::kDrawValue |
                    VSTGUI::CSlider::kDrawBack);
    q->setStyle(VSTGUI::CSlider::kVertical | VSTGUI::CSlider::kBottom);
    ampAttack = q;
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

uint32_t StupiEditor::paramIdFromTag(int32_t tag)
{
    switch (tag)
    {
    case tags::env_a:
        return StupiSaw::pmAmpAttack;
    }
    assert(false);
    return 0;
}

void StupiEditor::valueChanged(VSTGUI::CControl *c)
{
    auto t = (tags)c->getTag();
    switch (t)
    {
    case tags::env_a:
    {
        auto q = StupiSaw::FromUI();
        q.value = c->getValue();
        q.id = paramIdFromTag(t);
        q.type = StupiSaw::FromUI::MType::ADJUST_VALUE;
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
void StupiEditor::beginEdit(int32_t index)
{
    auto q = StupiSaw::FromUI();
    q.id = paramIdFromTag(index);
    q.type = StupiSaw::FromUI::MType::BEGIN_EDIT;
    outbound.try_enqueue(q);
}
void StupiEditor::endEdit(int32_t index)
{
    auto q = StupiSaw::FromUI();
    q.id = paramIdFromTag(index);
    q.type = StupiSaw::FromUI::MType::END_EDIT;
    outbound.try_enqueue(q);
}

} // namespace BaconPaul