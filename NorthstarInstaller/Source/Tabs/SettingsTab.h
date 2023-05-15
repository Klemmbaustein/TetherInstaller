#pragma once
#include "UITab.h"
#include <UI/UIBackground.h>
#include <UI/UIScrollBox.h>

class SettingsTab : public UITab
{
	UIBackground* SettingsBackground = nullptr;
	UIScrollBox* SettingsBox = nullptr;
public:
	SettingsTab();
	void GenerateSettings();
};