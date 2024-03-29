#include "LaunchTab.h"
#include "ProfileTab.h"
#include <KlemmUI/UI/UIText.h>
#include <KlemmUI/UI/UIBackground.h>

#include "../UI/UIDef.h"
#include "../Log.h"
#include "../Game.h"
#include "../Installer.h"
#include "../BackgroundTask.h"
#include "../Thunderstore.h"
#include "../Translation.h"

#include <thread>
#include <atomic>
#include <map>
#include <algorithm>

#if _WIN32
#include <Windows.h>
#endif

#ifdef TF_PLUGIN
#include "../TetherPlugin.h"
#endif

#include "ModsTab.h"

static bool CheckedForVanillaPlus = false;
static std::string NorthstarLaunchArgs;
bool LaunchTab::IsGameRunning = false;

using namespace KlemmUI;

static void NorthstarLaunchTask()
{
#ifndef TF_PLUGIN
	BackgroundTask::SetStatus("Northstar is running");
	Log::Print("Game has started");

	std::string GamePath = Game::GamePath;

	Thunderstore::Package ReleaseCanidate;

	ReleaseCanidate.Name = "NorthstarReleaseCandidate";
	ReleaseCanidate.Author = "northstar";
	ReleaseCanidate.Namespace = "northstar";

	if (Thunderstore::IsModInstalled(ReleaseCanidate))
	{
		GamePath.append("/NorthstarLauncherRC.exe");
	}
	else
	{
		GamePath.append("/NorthstarLauncher.exe");
	}

#if __linux__

	std::string Command = "steam://run/1237970/-northstar -profile=\\\""
		+ ProfileTab::CurrentProfile.DisplayName
		+ "\\\" "
		+ NorthstarLaunchArgs
		+ " "
		+ Game::GetLaunchArgs();

	// std::string Command = "https://northstar.tf/";

	system(("xdg-open \"" + Command + "\"").c_str());
#elif _WIN32
		STARTUPINFOA Startup;
		PROCESS_INFORMATION pi;
		ZeroMemory(&Startup, sizeof(Startup));
		Startup.cb = sizeof(Startup);
		ZeroMemory(&pi, sizeof(pi));

		CreateProcessA(GamePath.c_str(), 
			(LPSTR)(" -profile=\""
			+ ProfileTab::CurrentProfile.DisplayName
			+ "\" "
			+ NorthstarLaunchArgs
			+ " "
			+ Game::GetLaunchArgs()).c_str(),
			NULL,
			NULL,
			FALSE,
			0,
			NULL,
			NULL,
			&Startup,
			&pi);

		WaitForSingleObject(pi.hProcess, INFINITE);

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

#endif
	Log::Print("Game has finished running");
	ModsTab::CheckForModUpdates();
#endif
}

typedef void (*StoppingFunctionPtr)();

std::map<StoppingFunctionPtr, std::string> LaunchStoppingTasks =
{
	std::pair(&Game::UpdateGame, "play_updating_game"),
	std::pair(&NorthstarLaunchTask, "play_game_running"),
	std::pair(&Installer::CheckForUpdates, "play_update_check"),
	std::pair(&Installer::CheckForInstallerUpdate, "play_update_check"),
	std::pair(&ModsTab::CheckForModUpdates, "play_update_check"),
	std::pair(&Installer::UpdateInstaller, "play_updating_installer"),
	std::pair(&Thunderstore::TSModFunc::InstallOrUninstallMod, "play_downloading_mod")
};

void LaunchTab::LaunchNorthstar()
{
	LaunchNorthstar("");
}

void LaunchTab::LaunchNorthstar(std::string Args)
{
	NorthstarLaunchArgs = Args;
	if (Thunderstore::VanillaPlusInstalled())
	{
		NorthstarLaunchArgs.append(" -vanilla ");
	}
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

void LaunchTab::OnClicked()
{
	VanillaPlus = Thunderstore::VanillaPlusInstalled();
}

LaunchTab::LaunchTab()
{
	Name = "play";
	Log::Print("Loading launch tab...");
	Background->SetVerticalAlign(UIBox::Align::Default);
	auto TextBox = (new UIBackground(true, 0, 0, 0))->SetOpacity(0.7);
	TextBox->SetHorizontalAlign(UIBox::Align::Centered);

	LaunchButton = new UIButton(true, 0, Installer::GetThemeColor(), LaunchNorthstar);
	LaunchText = new UIText(1.4f, 0, "", UI::Text);

	Background->AddChild(TextBox
		->SetMinSize(Vector2f(2, 0.2))
		->SetPadding(0)
		->SetVerticalAlign(UIBox::Align::Centered)
		->SetHorizontalAlign(UIBox::Align::Centered)
#ifndef TF_PLUGIN
		->AddChild(LaunchButton
			->SetPadding(0.03)
			->SetBorder(UIBox::BorderType::Rounded, 0.5)
#endif
			->AddChild(LaunchText))
#ifndef TF_PLUGIN
	)
#endif
		;
#ifdef TF_PLUGIN
	LaunchText->SetColor(1);
	LaunchText->SetTextSize(0.5f);
#endif
}

void LaunchTab::Tick()
{
	using namespace Translation;

#ifndef TF_PLUGIN

	if (!CheckedForVanillaPlus)
	{
		CheckedForVanillaPlus = true;
		OnClicked();
	}

	LaunchButton->SetColor(1.0f);
	LaunchButton->SetHoveredColor(Vector3f::Lerp(1.0f, Installer::GetThemeColor(), 0.5f));
	LaunchButton->SetPressedColor(Installer::GetThemeColor());

	IsGameRunning = BackgroundTask::IsFunctionRunningAsTask(NorthstarLaunchTask);
	if (Game::GamePath.empty())
	{
		LaunchText->SetText(GetTranslation("play_game_not_found"));
		return;
	}
	for (auto& i : LaunchStoppingTasks)
	{
		if (BackgroundTask::IsFunctionRunningAsTask(i.first))
		{
			LaunchText->SetText(GetTranslation(i.second));
			return;
		}
	}
	if (Thunderstore::IsInstallingMod)
	{
		LaunchText->SetText(GetTranslation("play_downloading_mod"));
	}
	else if (Game::RequiresUpdate)
	{
		LaunchText->SetText(GetTranslation("play_update_required"));
	}
	else if (VanillaPlus)
	{
		LaunchText->SetText(Format(GetTranslation("play_launch"), "Vanilla+"));
	}
	else
	{
		LaunchText->SetText(Format(GetTranslation("play_launch"), "Northstar"));
	}
#else
	LaunchText->SetText(Format(GetTranslation("plugin_status_format"),
		GetMapName(Plugin::GetCurrentMap()).c_str(),
		GetGameModeName(Plugin::GetCurrentMode()).c_str(),
		Plugin::GetCurrentServer().c_str()));
#endif
}

LaunchTab::~LaunchTab()
{
}
