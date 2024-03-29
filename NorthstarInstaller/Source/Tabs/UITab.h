#pragma once
#include <KlemmUI/UI/UIBox.h>
#include <KlemmUI/UI/UIText.h>

class UITab
{
public:
	KlemmUI::UIBox* Background = nullptr;
	KlemmUI::UIText* TabTitle = nullptr;

	std::string Name = "Tab";
	std::string Description = "Default Description";

	virtual void OnClicked();

	UITab();
	virtual void Tick();
	virtual ~UITab();

	virtual void OnTranslationChanged();
};