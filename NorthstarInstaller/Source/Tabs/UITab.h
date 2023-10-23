#pragma once
#include <KlemmUI/UI/UIBox.h>

class UITab
{
public:
	UIBox* Background = nullptr;

	std::string Name = "Tab";
	std::string Description = "Default Description";

	virtual void OnClicked();

	UITab();
	virtual void Tick();
	virtual ~UITab();
};