#pragma once
#include "KlemmUI/Markup/Markup.h"
#include "KlemmUI/UI/UIBackground.h"
#include "KlemmUI/UI/UIBox.h"
class SideBar : public KlemmUI::UIBox
{

public:
	KlemmUI::UIBackground* background;
	SideBar() : UIBox(true)
	{
	this->SetPosition(Vector2f(float(-1)));
	{
	background = new KlemmUI::UIBackground(true, 0, 1);
	background->SetColor(Vector3f(float(0)));
	background->SetOpacity(float(0.75));
	background->SetMinSize(Vector2f(float(0), float(2)));
	background->SetHorizontal(false);
	this->AddChild(background);
	}

	}

};
