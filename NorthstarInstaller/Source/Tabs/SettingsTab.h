#pragma once
#include "UITab.h"
#include <KlemmUI/UI/UITextField.h>
#include <KlemmUI/UI/UIBackground.h>
#include <KlemmUI/UI/UIScrollBox.h>
#include <KlemmUI/UI/UIDropdown.h>

class SettingsTab : public UITab
{
	UIBackground* SettingsBackground = nullptr;
	UIScrollBox* SettingsBox = nullptr;
public:
	UITextField* LaunchArgsText = nullptr;
	UITextField* ColorText = nullptr;
	UIDropdown* LanguageDropdown = nullptr;
	UIDropdown* TitleBarDropdown = nullptr;
	SettingsTab();
	void Tick() override;
	void GenerateSettings();
	static SettingsTab* CurrentSettingsTab;
};