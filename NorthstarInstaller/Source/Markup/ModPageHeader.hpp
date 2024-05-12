#pragma once
#include "ImageButton.hpp"
#include "KlemmUI/Markup/Markup.h"
#include "KlemmUI/UI/UIBackground.h"
#include "KlemmUI/UI/UIBox.h"
#include "KlemmUI/UI/UIText.h"
class ModPageHeader : public KlemmUI::UIBox
{
	std::string Description;
	std::string Name;

public:
	ImageButton* backButton;
	KlemmUI::UIBackground* modImage;
	KlemmUI::UIBox* actionsBox;
	KlemmUI::UIText* unnamed_0;
	KlemmUI::UIText* unnamed_1;
	ModPageHeader() : UIBox(true)
	{
	this->SetTryFill((bool)true);
	this->SetHorizontal(false);
	{
	backButton = new ImageButton();
	backButton->SetImage("Back.png");
	backButton->SetText("Back");
	this->AddChild(backButton);
	auto* e_2 = new KlemmUI::UIBackground(true, 0, 1);
	e_2->SetColor(Vector3f(1));
	e_2->SetMinSize(Vector2f(2));
	e_2->SetSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_2->SetPadding((float)5);
	e_2->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_2->SetTryFill((bool)true);
	this->AddChild(e_2);
	auto* e_3 = new KlemmUI::UIBox(true);
	e_3->SetLeftPadding((float)0.1);
	e_3->SetHorizontal(true);
	this->AddChild(e_3);
	{
	modImage = new KlemmUI::UIBackground(true, 0, 1);
	modImage->SetMinSize(Vector2f(150));
	modImage->SetMaxSize(Vector2f(150));
	modImage->SetSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	modImage->SetPadding((float)5);
	modImage->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_3->AddChild(modImage);
	auto* e_5 = new KlemmUI::UIBox(true);
	e_5->SetHorizontal(false);
	e_3->AddChild(e_5);
	{
	auto* e_6 = new KlemmUI::UIText(1, 1, "", nullptr);
	e_6->SetText(Name);
	e_6->SetTextSize(20);
	e_6->SetTextSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_6->SetFont(KlemmUI::MarkupLanguageManager::GetActive()->GetFont(""));
	e_6->SetPadding((float)5);
	e_6->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_5->AddChild(e_6);
	unnamed_0 = e_6;
	auto* e_7 = new KlemmUI::UIText(1, 1, "", nullptr);
	e_7->SetText(Description);
	e_7->SetTextSize(14);
	e_7->SetTextSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_7->SetFont(KlemmUI::MarkupLanguageManager::GetActive()->GetFont(""));
	e_7->SetPadding((float)5);
	e_7->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_5->AddChild(e_7);
	unnamed_1 = e_7;
	actionsBox = new KlemmUI::UIBox(true);
	actionsBox->SetPadding((float)5);
	actionsBox->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_5->AddChild(actionsBox);
	}
	}
	auto* e_9 = new KlemmUI::UIBackground(true, 0, 1);
	e_9->SetColor(Vector3f(1));
	e_9->SetMinSize(Vector2f(2));
	e_9->SetSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_9->SetPadding((float)5);
	e_9->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_9->SetTryFill((bool)true);
	this->AddChild(e_9);
	}

	}

	void SetDescription(std::string NewValue)
	{
		Description = NewValue;
		unnamed_1->SetText(Description);
	}
	void SetName(std::string NewValue)
	{
		Name = NewValue;
		unnamed_0->SetText(Name);
	}
};
