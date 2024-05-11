#pragma once
#include <KlemmUI/UI/UIBox.h>
#include <KlemmUI/UI/UIBackground.h>

#include "../Tabs/UITab.h"

class SideBar;

class SidebarClass
{
	static SideBar* SidebarBackground;
	static KlemmUI::UIBox* HoveredTabName;
	static void GenerateTabName(size_t TabIndex);
public:
	static std::vector<UITab*> Tabs;
	static size_t SelectedTab;
	static size_t HoveredTab;
	static std::vector<KlemmUI::UIBox*> TabButtons;

	static void GenerateTabs();
	static void Load();
	static void Update();

	static float GetSize();
};