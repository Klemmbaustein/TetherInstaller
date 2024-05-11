#pragma once
#include <KlemmUI/UI/UIBox.h>
#include <KlemmUI/UI/UIButton.h>
#include <KlemmUI/UI/UIText.h>

class TitleBarElement;

class TitleBar
{
	static TitleBarElement* WindowBox;
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