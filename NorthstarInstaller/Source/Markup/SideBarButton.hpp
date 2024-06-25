#pragma once
#include "KlemmUI/Markup/Markup.h"
#include "KlemmUI/UI/UIBackground.h"
#include "KlemmUI/UI/UIBox.h"
#include "KlemmUI/UI/UIButton.h"
class SideBarButton : public KlemmUI::UIBox
{

public:
	KlemmUI::UIBackground* buttonIcon;
	KlemmUI::UIButton* button;
	SideBarButton() : UIBox(true)
	{
	{
	button = new KlemmUI::UIButton(true, 0, 1, nullptr);
	button->SetColor(Vector3f(float(1), float(1), float(1)));
	button->SetHoveredColor(Vector3f(float(1), float(1), float(1)) * 0.75f);
	button->SetPressedColor(Vector3f(float(1), float(1), float(1)) * 0.5f);
	button->BoxBorder = KlemmUI::UIBox::BorderType::Rounded;
	button->BorderRadius = float(5);
	button->BorderSizeMode = KlemmUI::UIBox::SizeMode::PixelRelative;
	button->SetUpPadding((float)float(5));
	button->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	button->SetDownPadding((float)float(0));
	button->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	button->SetLeftPadding((float)float(5));
	button->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	button->SetRightPadding((float)float(5));
	button->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	this->AddChild(button);
	{
	buttonIcon = new KlemmUI::UIBackground(true, 0, 1);
	buttonIcon->SetColor(Vector3f(float(0), float(0), float(0)));
	buttonIcon->SetMinSize(Vector2f(float(60)));
	buttonIcon->SetMaxSize(Vector2f(float(60)));
	buttonIcon->SetSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	button->AddChild(buttonIcon);
	}
	}

	}

};
