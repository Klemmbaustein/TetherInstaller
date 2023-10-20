#include "LaunchTab.h"
#include "ProfileTab.h"
#include <KlemmUI/UI/UIText.h>
#include <KlemmUI/UI/UIBackground.h>

#include "../UIDef.h"
#include "../Log.h"
#include "../Game.h"
#include "../Installer.h"
#include "../BackgroundTask.h"
#include "../Thunderstore.h"

#include <thread>
#include <atomic>
#include <map>
#include <regex>

#include "ModsTab.h"

std::string NorthstarLaunchArgs;
bool LaunchTab::IsGameRunning = false;
void NorthstarLaunchTask()
{
	BackgroundTask::SetStatus("Northstar is running");
	Log::Print("Game has started");

	std::string UTF8GameDir = std::regex_replace(Game::GamePath, std::regex(" "), "^ ");

	Thunderstore::Package ReleaseCanidate;

	ReleaseCanidate.Name = "NorthstarReleaseCandidate";
	ReleaseCanidate.Author = "northstar";
	ReleaseCanidate.Namespace = "northstar";

	if (Thunderstore::IsModInstalled(ReleaseCanidate))
	{
		UTF8GameDir.append("/NorthstarLauncherRC.exe");
	}
	else
	{
		UTF8GameDir.append("/NorthstarLauncher.exe");
	}

#if __linux__
#endif

	system((UTF8GameDir + " -profile=\""
		+ ProfileTab::CurrentProfile.DisplayName 
		+ "\" "
		+ NorthstarLaunchArgs 
		+ " " 
		+ Game::GetLaunchArgs()).c_str());
	Log::Print("Game has finished running");
	ModsTab::CheckForModUpdates();
}

std::map<void (*)(), std::string> LaunchStoppingTasks =
{
	std::pair(Game::UpdateGame, "Updating northstar"),
	std::pair(NorthstarLaunchTask, "Northstar is running"),
	std::pair(Installer::CheckForUpdates, "Checking for updates (1/3)"),
	std::pair(Installer::CheckForInstallerUpdate, "Checking for updates (2/3)"),
	std::pair(ModsTab::CheckForModUpdates, "Checking for updates (3/3)"),
	std::pair(Installer::UpdateInstaller, "Updating installer"),
	std::pair(Thunderstore::TSModFunc::InstallOrUninstallMod, "Downloading mod")
};

void LaunchTab::LaunchNorthstar()
{
	LaunchNorthstar("");
}

void LaunchTab::LaunchNorthstar(std::string Args)
{
	NorthstarLaunchArgs = Args;
	for (auto& i : LaunchStoppingTasks)
	{
		if (BackgroundTask::IsFunctionRunningAsTask(i.first))
		{
			return;
		}
	}
	if (Thunderstore::IsInstallingMod)
	{
		return;
	}
	if (!Game::GamePath.empty() && !Game::RequiresUpdate)
	{
		Log::Print("Starting game...");

		new BackgroundTask(NorthstarLaunchTask);
	}
	if (Game::RequiresUpdate && !Game::GamePath.empty())
	{
		Game::UpdateGameAsync();
	}
}

LaunchTab::LaunchTab()
{
	Name = "Play";
	Log::Print("Loading launch tab...");
	Background->BoxAlign = UIBox::Align::Default;
	auto TextBox = (new UIBackground(true, 0, 0, 0))->SetOpacity(0.3);
	TextBox->BoxAlign = UIBox::Align::Centered;

	LaunchButton = new UIButton(true, 0, 1, LaunchNorthstar);
	LaunchText = new UIText(0.7, 0, "Launch", UI::Text);

	Background->AddChild(TextBox
		->SetMinSize(Vector2f(2, 0.2))
		->SetPadding(0)
		->AddChild(LaunchButton
			->SetPadding(0.03)
			->SetBorder(UIBox::BorderType::Rounded, 1)
			->AddChild(LaunchText)));


}

void LaunchTab::Tick()
{
	IsGameRunning = BackgroundTask::IsFunctionRunningAsTask(NorthstarLaunchTask);
	if (Game::GamePath.empty())
	{
		LaunchText->SetText("Titanfall 2 not found");
		return;
	}
	for (auto& i : LaunchStoppingTasks)
	{
		if (BackgroundTask::IsFunctionRunningAsTask(i.first))
		{
			LaunchText->SetText(i.second);
			return;
		}
	}
	if (Thunderstore::IsInstallingMod)
	{
		LaunchText->SetText("Downloading mod");
	}
	else if (Game::RequiresUpdate)
	{
		LaunchText->SetText("Update northstar");
	}
	else
	{
		LaunchText->SetText("Launch Northstar");
	}
}

Launch