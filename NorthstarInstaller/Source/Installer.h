#pragma once
#include <vector>
#include "Tabs/UITab.h"
#include <thread>

namespace Installer
{
	extern size_t SelectedTab;
	extern std::vector<UITab*> Tabs;
	extern const std::string InstallerVersion;

	extern const std::string UserAgent;

	// Name of the github package this installer is from. If the newest package is not this, we should update.
	extern const std::string GithubInstallerVersion;
	void GenerateTabs();
	void CheckForUpdates();
	void UpdateInstaller();

	void CheckForInstallerUpdate();

}