//
// Created by Paul Walker on 6/10/22.
//

#include "clap-saw-demo-editor.h"
#include "clap-saw-demo.h"
#include <vstgui/lib/vstguiinit.h>

namespace sst::clap_saw_demo
{
bool ClapSawDemo::guiCreate(const char *api, bool isFloating) noexcept
{
    static bool everInit{false};
    if (!everInit)
    {
        VSTGUI::init(CFBundleGetMainBundle());
        everInit = true;
    }
    editor = new ClapSawDemoEditor(toUiQ, fromUiQ);
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
    editor->getFrame()->open(window->cocoa);
    editor->setupUI(window);
    return true;
}

ClapSawDemoEditor::ClapSawDemoEditor(ClapSawDemo::SynthToUI_Queue_t &i,
                                     ClapSawDemo::UIToSynth_Queue_t &o)
    : inbound(i), outbound(o)
{
    frame = new VSTGUI::CFrame(VSTGUI::CRect(0, 0, ClapSawDemo::guiw, ClapSawDemo::guih), this);
    frame->setBackgroundColor(VSTGUI::CColor(0x30, 0x30, 0x80));
    frame->remember();
}

void ClapSawDemoEditor::setupUI(const clap_window_t *w)
{
    _DBGMARK;

    auto l = new VSTGUI::CTextLabel(VSTGUI::CRect(10, 5, 200, 40), "ClapSawDemo");
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

ClapSawDemoEditor::~ClapSawDemoEditor()
{
    _DBGMARK;
    frame->forget();
    idleTimer->stop();
    idleTimer->forget();
}

uint32_t ClapSawDemoEditor::paramIdFromTag(int32_t tag)
{
    switch (tag)
    {
    case tags::env_a:
        return ClapSawDemo::pmAmpAttack;
    }
    assert(false);
    return 0;
}

void ClapSawDemoEditor::valueChanged(VSTGUI::CControl *c)
{
    auto t = (tags)c->getTag();
    switch (t)
    {
    case tags::env_a:
    {
        auto q = ClapSawDemo::FromUI();
        q.value = c->getValue();
        q.id = paramIdFromTag(t);
        q.type = ClapSawDemo::FromUI::MType::ADJUST_VALUE;
        outbound.try_enqueue(q);
        break;
    }
    }
}

void ClapSawDemoEditor::idle()
{
    ClapSawDemo::ToUI r;
    while (inbound.try_dequeue(r))
    {
        if (r.type == ClapSawDemo::ToUI::MType::PARAM_VALUE)
        {
            if (r.id == ClapSawDemo::pmAmpAttack)
            {
                ampAttack->setValue(r.value);
                ampAttack->invalid();
            }
        }
        _DBGCOUT << "inbound " << r.id << " " << r.value << std::endl;
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