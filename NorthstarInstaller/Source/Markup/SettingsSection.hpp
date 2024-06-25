#pragma once
#include "KlemmUI/Markup/Markup.h"
#include "KlemmUI/UI/UIBackground.h"
#include "KlemmUI/UI/UIBox.h"
#include "KlemmUI/UI/UIText.h"
class SettingsSection : public KlemmUI::UIBox
{
	std::string Icon;
	std::string Title;

public:
	KlemmUI::UIBackground* unnamed_0;
	KlemmUI::UIBox* contentBox;
	KlemmUI::UIText* unnamed_1;
	SettingsSection() : UIBox(true)
	{
	this->SetTryFill((bool)true);
	this->SetHorizontal(false);
	{
	auto* e_1 = new KlemmUI::UIBox(true);
	e_1->SetVerticalAlign(KlemmUI::UIBox::Align::Centered);
	e_1->SetPadding((float)float(5));
	e_1->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	this->AddChild(e_1);
	{
	auto* e_2 = new KlemmUI::UIBackground(true, 0, 1);
	e_2->SetUseTexture(true, Icon);
	e_2->SetMinSize(Vector2f(float(35)));
	e_2->SetMaxSize(Vector2f(float(35)));
	e_2->SetSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_2->SetPadding((float)float(5));
	e_2->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_1->AddChild(e_2);
	unnamed_0 = e_2;
	auto* e_3 = new KlemmUI::UIText(1, 1, "", nullptr);
	e_3->SetText(Title);
	e_3->SetTextSize(float(20));
	e_3->SetTextSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_3->SetFont(KlemmUI::MarkupLanguageManager::GetActive()->GetFont(""));
	e_3->SetPadding((float)float(5));
	e_3->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_1->AddChild(e_3);
	unnamed_1 = e_3;
	}
	auto* e_4 = new KlemmUI::UIBackground(true, 0, 1);
	e_4->SetMinSize(Vector2f(float(1)));
	e_4->SetSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_4->SetPadding((float)float(5));
	e_4->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_4->SetTryFill((bool)true);
	this->AddChild(e_4);
	contentBox = new KlemmUI::UIBox(true);
	contentBox->SetLeftPadding((float)float(50));
	contentBox->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	contentBox->SetHorizontal(false);
	this->AddChild(contentBox);
	}

	}

	void SetIcon(std::string NewValue)
	{
		Icon = NewValue;
		unnamed_0->SetUseTexture(true, Icon);
	}
	void SetTitle(std::string NewValue)
	{
		Title = NewValue;
		unnamed_1->SetText(Title);
	}
};
