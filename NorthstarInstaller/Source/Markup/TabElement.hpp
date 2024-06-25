#pragma once
#include "KlemmUI/Markup/Markup.h"
#include "KlemmUI/UI/UIBackground.h"
#include "KlemmUI/UI/UIBox.h"
#include "KlemmUI/UI/UIText.h"
class TabElement : public KlemmUI::UIBox
{
	std::string TabName;

public:
	KlemmUI::UIBackground* tabBackground;
	KlemmUI::UIBox* contentBox;
	KlemmUI::UIText* title;
	TabElement() : UIBox(true)
	{
	this->SetTryFill((bool)true);
	{
	tabBackground = new KlemmUI::UIBackground(true, 0, 1);
	tabBackground->SetColor(Vector3f(float(0)));
	tabBackground->SetOpacity(float(0.7));
	tabBackground->SetMinSize(Vector2f(float(1.0), float(0)));
	tabBackground->SetTryFill((bool)true);
	tabBackground->SetHorizontal(false);
	this->AddChild(tabBackground);
	{
	auto* e_2 = new KlemmUI::UIBackground(true, 0, 1);
	e_2->SetMinSize(Vector2f(float(1)));
	e_2->SetSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_2->SetTryFill((bool)true);
	tabBackground->AddChild(e_2);
	title = new KlemmUI::UIText(1, 1, "", nullptr);
	title->SetText(TabName);
	title->SetTextSize(float(20));
	title->SetTextSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	title->SetFont(KlemmUI::MarkupLanguageManager::GetActive()->GetFont(""));
	title->SetPadding((float)float(5));
	title->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	tabBackground->AddChild(title);
	contentBox = new KlemmUI::UIBox(true);
	contentBox->SetHorizontal(false);
	tabBackground->AddChild(contentBox);
	}
	}

	}

	void SetTabName(std::string NewValue)
	{
		TabName = NewValue;
		title->SetText(TabName);
	}
};
