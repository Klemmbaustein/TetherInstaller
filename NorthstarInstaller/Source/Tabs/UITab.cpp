#include "UITab.h"
#include <KlemmUI/UI/UIBackground.h>

using namespace KlemmUI;

void UITab::OnClicked()
{
}

UITab::UITab()
{
	Background = new UIBox(false, Vector2f(-1, -1));
	Background->SetVerticalAlign(UIBox::Align::Reverse);
	Background->SetMinSize(Vector2f(2, 1.8));
	Background->SetMaxSize(Vector2f(2, 1.8));
}

void UITab::Tick()
{
}

UITab::~UITab()
{
	delete Background;
}

void UITab::OnTranslationChanged()
{
}
