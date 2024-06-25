#pragma once
#include "KlemmUI/Markup/Markup.h"
#include "KlemmUI/UI/UIBackground.h"
#include "KlemmUI/UI/UIBox.h"
#include "KlemmUI/UI/UIButton.h"
class TitleBarButton : public KlemmUI::UIBox
{
	KlemmUI::AnyContainer ButtonColor;

public:
	KlemmUI::UIBackground* icon;
	KlemmUI::UIButton* button;
	TitleBarButton() : UIBox(true)
	{
	{
	button = new KlemmUI::UIButton(true, 0, 1, nullptr);
	button->SetColor(Vector3f(float(0.1)));
	button->SetHoveredColor(Vector3f(float(0.1)) * 0.75f);
	button->SetPressedColor(Vector3f(float(0.1)) * 0.5f);
	this->AddChild(button);
	{
	icon = new KlemmUI::UIBackground(true, 0, 1);
	icon->SetMinSize(Vector2f(float(20)));
	icon->SetMaxSize(Vector2f(float(20)));
	icon->SetSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	icon->SetPadding((float)float(10));
	icon->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	button->AddChild(icon);
	}
	}

	}

	void SetButtonColor(KlemmUI::AnyContainer NewValue)
	{
		ButtonColor = NewValue;
	}
};
