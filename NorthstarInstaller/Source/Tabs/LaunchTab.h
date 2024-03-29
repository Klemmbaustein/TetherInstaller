#pragma once
#include "UITab.h"
#include <KlemmUI/UI/UIButton.h>
#include <KlemmUI/UI/UIText.h>

class LaunchTab : public UITab
{
	bool VanillaPlus = false;
public:

	void OnClicked() override;
	KlemmUI::UIButton* LaunchButton = nullptr;
	KlemmUI::UIText* LaunchText = nullptr;
	LaunchTab();
	void Tick() override;
	virtual ~LaunchTab();
	static bool IsGameRunning;
	static void LaunchNorthstar();
	static void LaunchNorthstar(std::string Args);
};