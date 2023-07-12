#pragma once
#include <vector>
#include "Tabs/UITab.h"
#include <thread>

namespace Installer
{
	extern size_t SelectedTab;
	extern std::vector<UITab*> Tabs;
	extern const std::string InstallerVersion;
	void GenerateTabs();
	void CheckForUpdates();

}