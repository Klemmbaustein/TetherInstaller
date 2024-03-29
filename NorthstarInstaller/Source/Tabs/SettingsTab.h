#pragma once
#include "UITab.h"
#include <KlemmUI/UI/UITextField.h>
#include <KlemmUI/UI/UIBackground.h>
#include <KlemmUI/UI/UIScrollBox.h>
#include <KlemmUI/UI/UIDropdown.h>

class SettingsTab : public UITab
{
	KlemmUI::UIBackground* SettingsBackground = nullptr;
	KlemmUI::UIScrollBox* SettingsBox = nullptr;
public:
	KlemmUI::UITextField* LaunchArgsText = nullptr;
	KlemmUI::UITextField* ColorText = nullptr;
	std::vector<KlemmUI::UIDropdown::Option> LanguageOptions;
	SettingsTab();
	void Tick() override;
	void GenerateSettings();
	static SettingsTab* CurrentSettingsTab;
};