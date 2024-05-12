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
	this->SetPosition(Vector2f(-1));
	{
	background = new KlemmUI::UIBackground(true, 0, 1);
	background->SetColor(Vector3f(0));
	background->SetOpacity(0.75);
	background->SetMinSize(Vector2f(0,2));
	background->SetHorizontal(false);
	this->AddChild(background);
	}

	}

};
