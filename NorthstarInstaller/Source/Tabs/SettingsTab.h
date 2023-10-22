#pragma once
#include "UITab.h"
#include <KlemmUI/UI/UITextField.h>
#include <KlemmUI/UI/UIBackground.h>
#include <KlemmUI/UI/UIScrollBox.h>

class SettingsTab : public UITab
{
	UIBackground* SettingsBackground = nullptr;
	UIScrollBox* SettingsBox = nullptr;
public:
	UITextField* LaunchArgsText = nullptr;
	SettingsTab();
	void Tick() override;
	void GenerateSettings();
	static SettingsTab* CurrentSettingsTab;
};