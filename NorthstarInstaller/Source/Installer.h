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

	extern const std::string InstallerVersion;
	extern const std::string UserAgent;

	void UpdateWindowFlags();

	void SetThemeColor(Vector3f NewColor);
	Vector3f GetThemeColor();

	KlemmUI::UIButton* SetButtonColorIfSelected(KlemmUI::UIButton* Button, bool IsSelected);

	// Name of the GitHub package this installer is from. If the newest package is not this, we should update.
	extern const std::string GitHubInstallerVersion;

	void SetInstallerBackgroundImage(std::string Image);
}
