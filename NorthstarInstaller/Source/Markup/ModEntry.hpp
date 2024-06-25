#pragma once
#include "KlemmUI/Markup/Markup.h"
#include "KlemmUI/UI/UIBackground.h"
#include "KlemmUI/UI/UIBox.h"
#include "KlemmUI/UI/UIButton.h"
#include "KlemmUI/UI/UIText.h"
class ModEntry : public KlemmUI::UIBox
{
	Vector3f Color = Vector3f(float(1), float(1), float(1));
	std::string Name;

public:
	KlemmUI::UIBackground* entryImage;
	KlemmUI::UIButton* entryButton;
	KlemmUI::UIText* entryText;
	ModEntry() : UIBox(true)
	{
	{
	entryButton = new KlemmUI::UIButton(true, 0, 1, nullptr);
	entryButton->SetColor(Vector3f(Color));
	entryButton->SetHoveredColor(Vector3f(Color) * 0.75f);
	entryButton->SetPressedColor(Vector3f(Color) * 0.5f);
	entryButton->BoxBorder = KlemmUI::UIBox::BorderType::Rounded;
	entryButton->BorderRadius = float(5);
	entryButton->BorderSizeMode = KlemmUI::UIBox::SizeMode::PixelRelative;
	entryButton->SetMinSize(Vector2f(float(140), float(190)));
	entryButton->SetMaxSize(Vector2f(float(140), float(190)));
	entryButton->SetSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	entryButton->SetPadding((float)float(5));
	entryButton->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	entryButton->SetHorizontal(false);
	this->AddChild(entryButton);
	{
	auto* e_2 = new KlemmUI::UIBackground(true, 0, 1);
	e_2->SetColor(Vector3f(float(0.1)));
	entryButton->AddChild(e_2);
	{
	entryImage = new KlemmUI::UIBackground(true, 0, 1);
	entryImage->SetMinSize(Vector2f(float(140)));
	entryImage->SetMaxSize(Vector2f(float(140)));
	entryImage->SetSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	entryImage->SetVerticalAlign(KlemmUI::UIBox::Align::Centered);
	entryImage->SetHorizontalAlign(KlemmUI::UIBox::Align::Centered);
	e_2->AddChild(entryImage);
	}
	entryText = new KlemmUI::UIText(1, 1, "", nullptr);
	entryText->SetText(Name);
	entryText->SetColor(Vector3f(float(0)));
	entryText->SetTextSize(float(10));
	entryText->SetTextSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	entryText->SetFont(KlemmUI::MarkupLanguageManager::GetActive()->GetFont(""));
	entryText->WrapDistance = float(260);
	entryText->Wrap = true;
	entryText->WrapSizeMode = KlemmUI::UIBox::SizeMode::PixelRelative;
	entryText->SetPadding((float)float(2));
	entryText->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	entryButton->AddChild(entryText);
	}
	}

	}

	void SetColor(Vector3f NewValue)
	{
		Color = NewValue;
		entryButton->SetColor(Vector3f(Color));
		entryButton->SetHoveredColor(Vector3f(Color) * 0.75f);
		entryButton->SetPressedColor(Vector3f(Color) * 0.5f);
	}
	void SetName(std::string NewValue)
	{
		Name = NewValue;
		entryText->SetText(Name);
	}
};
