#pragma once

namespace Install
{
	void InstallApp(bool CreateStartShortcut, bool CreateDesktopShortcut);

	bool IsInstalling();

	void LaunchApp();
}