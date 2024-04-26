#pragma once
#include <KlemmUI/UI/UIBox.h>
#include <KlemmUI/UI/UIButton.h>
#include <KlemmUI/UI/UIText.h>

class TitleBar
{
	static KlemmUI::UIBackground* WindowButtonBox;
	static KlemmUI::UIText* AppTitle;
	static std::vector<unsigned int> WindowButtonsIcons;
	static bool UseSystemTitleBar;
	static std::string TitleText;
public:
	static void GenerateWindowButtons();
	
	static void Load();

	static float GetSize();

	static void SetTitle(std::string NewTitle);

	static void Update();

	static bool SetUseSystemTitleBar(bool NewVal);
	static bool GetUseSystemTitleBar();

	static bool IsHovered();
};