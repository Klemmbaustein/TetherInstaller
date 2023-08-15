#include "SettingsTab.h"

#include <KlemmUI/UI/UIText.h>
#include <KlemmUI/UI/UIButton.h>

#include <thread>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <filesystem>

#include "../Log.h"
#include "../UIDef.h"
#include "../Game.h"
#include "../Installer.h"
#include "../BackgroundTask.h"
#include "../WindowFunctions.h"
#include "ProfileTab.h"

SettingsTab* SettingsTab::CurrentSettingsTab = nullptr;
SettingsTab::SettingsTab()
{
	CurrentSettingsTab = this;
	Name = "Settings";

	Background->BoxAlign = UIBox::Align::Centered;
	Background->SetHorizontal(true);

	SettingsBackground = new UIBackground(false, 0, 0, Vector2f(0.8, 1.85));
	Background->AddChild(SettingsBackground
		->SetOpacity(0.5)
		->AddChild((new UIBackground(true, 0, 1, Vector2f(0.8, 0.005)))
			->SetPadding(0))
		->SetPadding(0));
	SettingsBackground->BoxAlign = UIBox::Align::Reverse;
	SettingsBackground->AddChild(new UIText(0.8, 1, "Settings", UI::Text));
	SettingsBox = new UIScrollBox(false, 0, true);
	SettingsBox->SetMinSize(Vector2f(0, 1.65));
	SettingsBox->SetMaxSize(Vector2f(2, 1.65));
	SettingsBox->BoxAlign = UIBox::Align::Reverse;
	SettingsBackground->AddChild(SettingsBox);
	GenerateSettings();
}

void DeleteAllMods()
{
	// Do not uninstall the core mods. That's a very bad idea.
	std::set<std::string> CoreModNames =
	{
		"Northstar.CustomServers",
		"Northstar.Custom",
		"Northstar.Client",
		"Northstar.Coop", // soooooon
	};

	for (const auto& m : std::filesystem::directory_iterator(ProfileTab::CurrentProfile.Path + "/mods/"))
	{
		if (CoreModNames.find(m.path().filename().u8string()) == CoreModNames.end() && std::filesystem::is_directory(m))
		{
			std::filesystem::remove_all(m);
			Log::Print("Removing mod: " + m.path().filename().u8string(),  Log::Warning);
		}
	}
	std::filesystem::remove_all("Data/var/modinfo");
}

void LocateTitanfall()
{
	std::string NewPath = Window::ShowSelectFolderDialog();
	if (Game::IsValidTitanfallLocation(NewPath))
	{
		Game::SaveGameDir(NewPath);
		Game::GamePath = Game::GetTitanfallLocation();
	}
	SettingsTab::CurrentSettingsTab->GenerateSettings();
}

constexpr uint16_t MAX_GAMEPATH_SIZE = 30;

void AddCategoryHeader(std::string Text, UIBox* Parent)
{
	Parent->AddChild((new UIText(0.5, 1, Text, UI::Text))
		->SetPadding(0.05, 0.01, 0.01, 0.01));
	Parent->AddChild((new UIBackground(true, 0, 1, Vector2f(0.76, 0.005)))
		->SetPadding(0.0, 0.02, 0, 0));
}

void SettingsTab::GenerateSettings()
{
	SettingsBox->DeleteChildren();

	std::string ShortGamePath = Game::GamePath;
	if (ShortGamePath.size() > MAX_GAMEPATH_SIZE)
	{
		ShortGamePath = ShortGamePath.substr(0, MAX_GAMEPATH_SIZE - 3) + "...";
	}

	AddCategoryHeader("General", SettingsBox);
	SettingsBox->AddChild((new UIButton(true, 0, 1, LocateTitanfall))
		->AddChild(new UIText(0.35, 0, Game::GamePath.empty() ? "Locate Titanfall 2 (No path!)" : "Locate Titanfall (" + ShortGamePath + ")", UI::Text)));

	if (Game::IsValidTitanfallLocation(Game::GamePath))
	{


		SettingsBox->AddChild((new UIButton(true, 0, 1, []() {
			new BackgroundTask(Installer::CheckForUpdates);
			}))
			->AddChild(new UIText(0.35, 0, "Re-check for updates", UI::Text)));

		SettingsBox->AddChild((new UIButton(true, 0, 1, Game::UpdateGameAsync))
			->AddChild(new UIText(0.35, 0, "Reinstall Northstar", UI::Text)));

		LaunchArgsText = new UITextField(true, 0, 0, UI::MonoText, []() {Game::SetLaunchArgs(CurrentSettingsTab->LaunchArgsText->GetText()); });

		SettingsBox->AddChild(new UIText(0.35, 1, "Launch arguments", UI::Text));
		SettingsBox->AddChild(LaunchArgsText
			->SetHintText("Launch arguments")
			->SetTextSize(0.3)
			->SetText(Game::GetLaunchArgs())
			->SetMinSize(Vector2f(0.75, 0.05)));

		AddCategoryHeader("Logs", SettingsBox);

		SettingsBox->AddChild((new UIButton(true, 0, 1, []() 
			{
				if (!std::filesystem::exists(ProfileTab::CurrentProfile.Path + "/logs"))
				{
					Window::ShowPopupError("Log folder does not exist.");
					return;
				}
				system(("cd " + ProfileTab::CurrentProfile.Path + "\\logs && explorer .").c_str());
			}))
			->AddChild(new UIText(0.35, 0, "Open log folder", UI::Text)));

		SettingsBox->AddChild((new UIButton(true, 0, 1, []() 
			{
				if (!std::filesystem::exists(ProfileTab::CurrentProfile.Path + "/logs"))
				{
					Window::ShowPopupError("Log folder does not exist.");
					return;
				}
				std::filesystem::directory_entry LatestLog;
				for (auto& i : std::filesystem::directory_iterator(ProfileTab::CurrentProfile.Path + "/logs"))
				{
					if (i.path().extension().u8string() == ".txt"
						&& (!LatestLog.exists() 
							|| i.last_write_time() > LatestLog.last_write_time()))
					{
						LatestLog = i;
					}
				}
				if (!std::filesystem::exists(LatestLog))
				{
					Window::ShowPopupError("Could not find a log file.");
					return;
				}
				system(("start \"\" \"" + LatestLog.path().u8string() + "\"").c_str());
			}))
			->AddChild(new UIText(0.35, 0, "View latest log", UI::Text)));


		AddCategoryHeader("Danger zone", SettingsBox);
		SettingsBox->AddChild((new UIButton(true, 0, 1, DeleteAllMods))
			->AddChild(new UIText(0.35, 0, "Delete all mods", UI::Text)));

		SettingsBox->AddChild((new UIButton(true, 0, 1, []() {
			try
			{
				std::filesystem::remove_all("Data/temp");
				Log::Print("Deleted ./Data/temp/", Log::Warning);
				std::filesystem::remove_all("./Data/var");
				Log::Print("Deleted ./Data/var/", Log::Warning);
				std::filesystem::create_directories("./Data/var");
				std::filesystem::create_directories("./Data/temp");
			}
			catch (std::exception& e)
			{

			}
			}))
			->AddChild(new UIText(0.35, 0, "Reset launcher", UI::Text)));
	}

	AddCategoryHeader("About", SettingsBox);
	SettingsBox->AddChild(new UIText(0.35, 1, "Installed Northstar version: " + Game::GetCurrentVersion(), UI::Text));
	SettingsBox->AddChild(new UIText(0.35, 1, "Launcher version: " + Installer::InstallerVersion, UI::Text));
#if _WIN32
	SettingsBox->AddChild(new UIText(0.35, 1, "Operating system: Windows", UI::Text));
#elif __linux__
	SettingsBox->AddChild(new UIText(0.35, 1, "Operating system: Linux", UI::Text));
#endif
}
