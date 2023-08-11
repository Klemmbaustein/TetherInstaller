#include "UITab.h"
#include <KlemmUI/UI/UIBackground.h>

UITab::UITab()
{
	Background = new UIBox(false, Vector2f(-1, -1));
	Background->BoxAlign = UIBox::Align::Reverse;
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
