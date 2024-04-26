#include "UITab.h"
#include <KlemmUI/UI/UIBackground.h>
#include "../UI/TitleBar.h"
#include "../UI/Sidebar.h"

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

void UITab::UpdateAll()
{
	for (auto i : Sidebar::Tabs)
	{
		i->Background->SetPosition(Vector2f(
			-1 + Sidebar::GetSize(),
			-1));

		Vector2f BackgroundSize = Vector2f(2) - Vector2f(
			Sidebar::GetSize(),
			TitleBar::GetSize());

		i->Background->SetMinSize(BackgroundSize);
		i->Background->SetMaxSize(BackgroundSize);
		i->Tick();
	}
}

void UITab::OnTranslationChanged()
{
}
