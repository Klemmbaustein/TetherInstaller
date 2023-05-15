#pragma once
#include <vector>
#include "Tabs/UITab.h"
#include <thread>

namespace Installer
{
	extern size_t SelectedTab;
	extern std::vector<UITab*> Tabs;
	void GenerateTabs();
	void CheckForUpdates();

	extern std::thread* CurrentBackgroundThread;
	extern std::string BackgroundTask;
	extern std::string BackgroundName;
	extern float ThreadProgress;
}