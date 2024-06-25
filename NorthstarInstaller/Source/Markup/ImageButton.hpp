#pragma once
#include "KlemmUI/Markup/Markup.h"
#include "KlemmUI/UI/UIBackground.h"
#include "KlemmUI/UI/UIBox.h"
#include "KlemmUI/UI/UIButton.h"
#include "KlemmUI/UI/UIText.h"
class ImageButton : public KlemmUI::UIBox
{
	std::string Image = std::string("Back.png");
	std::string Text = std::string("");

public:
	KlemmUI::UIBackground* unnamed_0;
	KlemmUI::UIButton* button;
	KlemmUI::UIText* text;
	ImageButton() : UIBox(true)
	{
	{
	button = new KlemmUI::UIButton(true, 0, 1, nullptr);
	button->BoxBorder = KlemmUI::UIBox::BorderType::Rounded;
	button->BorderRadius = float(5);
	button->BorderSizeMode = KlemmUI::UIBox::SizeMode::PixelRelative;
	button->SetVerticalAlign(KlemmUI::UIBox::Align::Centered);
	button->SetPadding((float)float(10));
	button->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	this->AddChild(button);
	{
	auto* e_2 = new KlemmUI::UIBackground(true, 0, 1);
	e_2->SetColor(Vector3f(float(0)));
	e_2->SetUseTexture(true, Image);
	e_2->SetMinSize(Vector2f(float(20)));
	e_2->SetMaxSize(Vector2f(float(20)));
	e_2->SetSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_2->SetPadding((float)float(5));
	e_2->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	button->AddChild(e_2);
	unnamed_0 = e_2;
	text = new KlemmUI::UIText(1, 1, "", nullptr);
	text->SetText(Text);
	text->SetColor(Vector3f(float(0)));
	text->SetTextSize(float(13));
	text->SetTextSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	text->SetFont(KlemmUI::MarkupLanguageManager::GetActive()->GetFont(""));
	text->SetPadding((float)float(5));
	text->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	button->AddChild(text);
	}
	}

	}

	void SetImage(std::string NewValue)
	{
		Image = NewValue;
		unnamed_0->SetUseTexture(true, Image);
	}
	void SetText(std::string NewValue)
	{
		Text = NewValue;
		text->SetText(Text);
	}
};
