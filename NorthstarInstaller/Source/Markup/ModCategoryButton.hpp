#pragma once
#include "KlemmUI/Markup/Markup.h"
#include "KlemmUI/UI/UIBox.h"
#include "KlemmUI/UI/UIButton.h"
#include "KlemmUI/UI/UIText.h"
class ModCategoryButton : public KlemmUI::UIBox
{
	std::string Name;

public:
	KlemmUI::UIButton* button;
	KlemmUI::UIText* unnamed_0;
	ModCategoryButton() : UIBox(true)
	{
	{
	button = new KlemmUI::UIButton(true, 0, 1, nullptr);
	button->BoxBorder = KlemmUI::UIBox::BorderType::Rounded;
	button->BorderRadius = float(4);
	button->BorderSizeMode = KlemmUI::UIBox::SizeMode::PixelRelative;
	button->SetPadding((float)float(2));
	button->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	this->AddChild(button);
	{
	auto* e_2 = new KlemmUI::UIText(1, 1, "", nullptr);
	e_2->SetText(Name);
	e_2->SetColor(Vector3f(float(0)));
	e_2->SetTextSize(float(13));
	e_2->SetTextSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	e_2->SetFont(KlemmUI::MarkupLanguageManager::GetActive()->GetFont(""));
	e_2->SetPadding((float)float(4));
	e_2->SetPaddingSizeMode(KlemmUI::UIBox::SizeMode::PixelRelative);
	button->AddChild(e_2);
	unnamed_0 = e_2;
	}
	}

	}

	void SetName(std::string NewValue)
	{
		Name = NewValue;
		unnamed_0->SetText(Name);
	}
};
