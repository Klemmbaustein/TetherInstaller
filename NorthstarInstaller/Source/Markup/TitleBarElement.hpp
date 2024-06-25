#pragma once
#include "KlemmUI/Markup/Markup.h"
#include "KlemmUI/UI/UIBackground.h"
#include "KlemmUI/UI/UIBox.h"
#include "KlemmUI/UI/UIText.h"
class TitleBarElement : public KlemmUI::UIBox
{
	std::string Title;

public:
	KlemmUI::UIBox* buttonBox;
	KlemmUI::UIText* titleText;
	TitleBarElement() : UIBox(true)
	{
	{
	auto* e_1 = new KlemmUI::UIBackground(true, 0, 1);
	e_1->SetColor(Vector3f(float(0.1)));
	e_1->SetMinSize(Vector2f(float(1.5), float(0)));
	e_1->SetVerticalAlign(KlemmUI::UIBox::Align::Default);
	e_1->SetTryFill((bool)true);
	this->AddChild(e_1);
	{
	titleText = new KlemmUI::UIText(1, 1, "", nullptr);
	titleText->SetText(Title);
	titleText->SetTextSize(float(15));
	titleText->SetTextSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	titleText->SetFont(KlemmUI::MarkupLanguageManager::GetActive()->GetFont(""));
	titleText->SetUpPadding((float)float(8));
	titleText->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	titleText->SetDownPadding((float)float(5));
	titleText->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	titleText->SetLeftPadding((float)float(80));
	titleText->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	titleText->SetRightPadding((float)float(5));
	titleText->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_1->AddChild(titleText);
	}
	auto* e_3 = new KlemmUI::UIBackground(true, 0, 1);
	e_3->SetColor(Vector3f(float(0.1)));
	e_3->SetMinSize(Vector2f(float(0.5), float(0)));
	e_3->SetHorizontalAlign(KlemmUI::UIBox::Align::Reverse);
	this->AddChild(e_3);
	{
	buttonBox = new KlemmUI::UIBox(true);
	buttonBox->SetHorizontalAlign(KlemmUI::UIBox::Align::Reverse);
	e_3->AddChild(buttonBox);
	}
	}

	}

	void SetTitle(std::string NewValue)
	{
		Title = NewValue;
		titleText->SetText(Title);
	}
};
