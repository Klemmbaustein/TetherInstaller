#pragma once
#include <KlemmUI/UI/UIBox.h>
#include <KlemmUI/UI/UIBackground.h>

#include "../Tabs/UITab.h"

class Sidebar
{
	static KlemmUI::UIBackground* SidebarBackground;
	static KlemmUI::UIBackground* HoveredTabName;
	static void GenerateTabName(size_t TabIndex);
public:
	static std::vector<UITab*> Tabs;
	static size_t SelectedTab;
	static size_t HoveredTab;
	static std::vector<KlemmUI::UIButton*> TabButtons;

	static void GenerateTabs();
	static void Load();
	static void Update();

	static float GetSize();
};