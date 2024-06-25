#pragma once
#include "KlemmUI/Markup/Markup.h"
#include "KlemmUI/UI/UIBackground.h"
#include "KlemmUI/UI/UIBox.h"
#include "KlemmUI/UI/UIText.h"
class SideBarTooltip : public KlemmUI::UIBox
{
	std::string Description;
	std::string Title;

public:
	KlemmUI::UIText* unnamed_0;
	KlemmUI::UIText* unnamed_1;
	SideBarTooltip() : UIBox(true)
	{
	{
	auto* e_1 = new KlemmUI::UIBackground(true, 0, 1);
	e_1->SetColor(Vector3f(float(0)));
	e_1->SetOpacity(float(0.75));
	e_1->SetMinSize(Vector2f(float(0), float(60)));
	e_1->SetSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_1->SetHorizontal(false);
	this->AddChild(e_1);
	{
	auto* e_2 = new KlemmUI::UIText(1, 1, "", nullptr);
	e_2->SetText(Title);
	e_2->SetTextSize(float(15));
	e_2->SetTextSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_2->SetFont(KlemmUI::MarkupLanguageManager::GetActive()->GetFont(""));
	e_2->SetUpPadding((float)float(5));
	e_2->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_2->SetDownPadding((float)float(2));
	e_2->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_2->SetLeftPadding((float)float(5));
	e_2->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_2->SetRightPadding((float)float(15));
	e_2->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_1->AddChild(e_2);
	unnamed_0 = e_2;
	auto* e_3 = new KlemmUI::UIText(1, 1, "", nullptr);
	e_3->SetText(Description);
	e_3->SetColor(Vector3f(float(0.8)));
	e_3->SetTextSize(float(13));
	e_3->SetTextSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_3->SetFont(KlemmUI::MarkupLanguageManager::GetActive()->GetFont(""));
	e_3->SetUpPadding((float)float(0));
	e_3->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_3->SetDownPadding((float)float(2));
	e_3->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_3->SetLeftPadding((float)float(5));
	e_3->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_3->SetRightPadding((float)float(15));
	e_3->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_1->AddChild(e_3);
	unnamed_1 = e_3;
	}
	}

	}

	void SetDescription(std::string NewValue)
	{
		Description = NewValue;
		unnamed_1->SetText(Description);
	}
	void SetTitle(std::string NewValue)
	{
		Title = NewValue;
		unnamed_0->SetText(Title);
	}
};
