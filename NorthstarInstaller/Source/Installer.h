#pragma once
#include <vector>
#include "Tabs/UITab.h"
#include <KlemmUI/Window.h>
#include <thread>
#include <string>

namespace Installer
{
	extern KlemmUI::Window* MainWindow;

	extern std::string CurrentPath;

	extern size_t SelectedTab;
	extern std::vector<UITab*> Tabs;
	extern const std::string InstallerVersion;
	extern const std::string UserAgent;
	extern bool UseSystemTitleBar;

	void UpdateWindowFlags();

	void SetThemeColor(Vector3f NewColor);
	Vector3f GetThemeColor();

	KlemmUI::UIButton* SetButtonColorIfSelected(KlemmUI::UIButton* Button, bool IsSelected);

	void GenerateWindowButtons();

	// Name of the github package this installer is from. If the newest package is not this, we should update.
	extern const std::string GithubInstallerVersion;
	void GenerateTabs();
	void CheckForUpdates();
	void UpdateInstaller();

	void SetInstallerBackgroundImage(std::string Image);

	void CheckForInstallerUpdate();

}
