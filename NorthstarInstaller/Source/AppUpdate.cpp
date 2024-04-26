#include "AppUpdate.h"
#include "Translation.h"
#include "BackgroundTask.h"
#include "Installer.h"
#include "Log.h"
#include <queue>
#include "Tabs/ModsTab.h"
#include "WindowFunctions.h"
#include "Networking.h"
#include "Game.h"


void AppUpdate::CheckForUpdates()
{
	// Don't update the game while it's running.
#if TF_PLUGIN
	return;
#endif
	Log::Print("Checking for updates...");
	BackgroundTask::SetStatus("Checking for updates");
	BackgroundTask::SetProgress(0.999);

	Thunderstore::Package ReleaseCanidate;

	ReleaseCanidate.Name = "NorthstarReleaseCandidate";
	ReleaseCanidate.Author = "northstar";
	ReleaseCanidate.Namespace = "northstar";

	if (Thunderstore::IsModInstalled(ReleaseCanidate))
	{
		return;
	}

	std::string Latest = Networking::GetLatestReleaseOf("R2Northstar/Northstar");

	if (Latest.empty())
	{
		Log::Print("Could not get the latest game version", Log::Error);
		BackgroundTask::SetStatus("Could not get the latest game version");
		BackgroundTask::SetProgress(1);
		return;
	}

	std::string InstalledVersion = Game::GetCurrentVersion();

	if (Latest != InstalledVersion)
	{
		Log::Print("Game needs to be updated. Installed: " + InstalledVersion + ", latest: " + Latest, Log::Warning);
		Game::RequiresUpdate = true;
		BackgroundTask::SetStatus("Update required");
		BackgroundTask::SetProgress(1);
		return;
	}
	BackgroundTask::SetStatus("No update required");
	Log::Print("No update required");
}

static std::atomic<bool> RequiresUpdate = false;

void AppUpdate::CheckForInstallerUpdate()
{
#if DEBUG || TF_PLUGIN
	return;
#endif
	Log::Print("Checking for installer updates...");
	BackgroundTask::SetStatus("Checking for installer updates");
	BackgroundTask::SetProgress(0.999);

	std::string Ver = Networking::GetLatestReleaseOf("Klemmbaustein/TetherInstaller");
	if (Ver.empty())
	{
		return;
	}
	if (Installer::GitHubInstallerVersion != Ver)
	{
		RequiresUpdate = true;
		Log::Print("Launcher requires update. " + Installer::GitHubInstallerVersion + " -> " + Ver, Log::Warning);
	}
}
static auto LaunchTasks = 
std::queue<void (*)()
>({
	AppUpdate::CheckForUpdates,
	AppUpdate::CheckForInstallerUpdate,
	ModsTab::CheckForModUpdates
});

void AppUpdate::UpdateInstaller()
{
#if __linux__
	WindowFunc::ShowPopupError(Translation::GetTranslation("popup_linux_update"));
	return;
#endif
	BackgroundTask::SetStatus("dl_" + Translation::GetTranslation("download_update_installer"));
	if (WindowFunc::ShowPopupQuestion(
		Translation::GetTranslation("popup_windows_update_title"),
		Translation::GetTranslation("popup_windows_update"))
		!= WindowFunc::PopupReply::Yes)
	{
		return;
	}
	Networking::DownloadLatestReleaseOf("Klemmbaustein/TetherInstaller", "Windows");
	Networking::ExtractZip("Data/temp/net/latest.zip", "Data/temp/install");

	system("start update.bat");
	exit(0);
}

void AppUpdate::Check()
{
}

void AppUpdate::Update()
{
	if (!BackgroundTask::IsRunningTask && !LaunchTasks.empty())
	{
		new BackgroundTask(LaunchTasks.front());
		LaunchTasks.pop();
	}
}
