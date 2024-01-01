#pragma once
#include <vector>
#include "Tabs/UITab.h"
#include <thread>
#include <string>

class UIButtonStyle;

namespace Installer
{
	extern size_t SelectedTab;
	extern std::vector<UITab*> Tabs;
	extern const std::string InstallerVersion;
	extern const std::string UserAgent;
	extern UIButtonStyle* TabStyles[2];
	extern bool UseSystemTitleBar;

	void UpdateWindowFlags();

	void SetThemeColor(Vector3f32 NewColor);
	Vector3f32 GetThemeColor();

	void GenerateWindowButtons();

	// Name of the github package this installer is from. If the newest package is not this, we should update.
	extern const std::string GithubInstallerVersion;
	void GenerateTabs();
	void CheckForUpdates();
	void UpdateInstaller();

	void SetInstallerBackgroundImage(std::string Image);

	void CheckForInstallerUpdate();

}
