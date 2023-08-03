#pragma once
#include <KlemmUI/UI/UIBox.h>

class UITab
{
public:
	UIBox* Background = nullptr;

	std::string Name = "Tab";

	UITab();
	virtual void Tick();
	virtual ~UITab();
};