#include "SettingsTab.h"
#include "ProfileTab.h"

#include <KlemmUI/UI/UIText.h>
#include <KlemmUI/UI/UIButton.h>

#include <thread>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <filesystem>

#include "../Log.h"
#include "../UI/UIDef.h"
#include "../Game.h"
#include "../Installer.h"
#include "../BackgroundTask.h"
#include "../WindowFunctions.h"
#include "../UI/Icon.h"
#include "../Translation.h"

using namespace Translation;

SettingsTab* SettingsTab::CurrentSettingsTab = nullptr;
SettingsTab::SettingsTab()
{
	CurrentSettingsTab = this;
	Name = "settings";
	Log::Print("Loading settings tab...");

	Background->SetHorizontalAlign(UIBox::Align::Centered);
	Background->SetHorizontal(true);

	SettingsBackground = new UIBackground(false, 0, 0, Vector2f(1, 1.85));
	Background->AddChild(SettingsBackground
		->SetOpacity(0.65)
		->AddChild((new UIBackground(true, 0, 1, Vector2f(1, 0.005)))
			->SetPadding(0))
		->SetPadding(0));
	TabTitle = new UIText(0.8, 1, GetTranslation("tab_settings"), UI::Text);
	SettingsBackground->AddChild(TabTitle);
	SettingsBox = new UIScrollBox(false, 0, true);
	SettingsBox->SetMinSize(Vector2f(0, 1.78));
	SettingsBox->SetMaxSize(Vector2f(2, 1.78));
	SettingsBackground->AddChild(SettingsBox);
	GenerateSettings();
}

void SettingsTab::Tick()
{
	SettingsBackground->SetMinSize(Vector2f(1, Background->GetUsedSize().Y));
}

void DeleteAllMods()
{
	for (const auto& m : std::filesystem::directory_iterator(ProfileTab::CurrentProfile.Path + "/mods/"))
	{
		if (Game::CoreModNames.find(m.path().filename().u8string()) == Game::CoreModNames.end() && std::filesystem::is_directory(m))
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
	else if (!NewPath.empty())
	{
		Window::ShowPopupError(NewPath + " is not a valid Titanfall 2 path.");
	}
	SettingsTab::CurrentSettingsTab->GenerateSettings();
}

constexpr uint16_t MAX_GAMEPATH_SIZE = 30;

void AddCategoryHeader(std::string Text, std::string IconName, UIBox* Parent)
{
	Parent->AddChild((new UIBox(true, 0))
		->SetVerticalAlign(UIBox::Align::Default)
		->AddChild((new UIBackground(true, 0, 1, 0.075))
			->SetUseTexture(true, Icon(IconName).TextureID)
			->SetPadding(0.01, 0.005, 0, 0)
			->SetSizeMode(UIBox::SizeMode::AspectRelative))
		->AddChild((new UIText(0.5, 1, GetTranslation(Text), UI::Text))
		->SetPadding(0.05, 0.005, 0.01, 0.01)));
	Parent->AddChild((new UIBackground(true, 0, 1, Vector2f(0.98, 0.005)))
		->SetPadding(0.0, 0.02, 0, 0));
}

void AddSettingsButton(std::string Text, std::string IconName, void(*OnClicked)(), UIBox* Parent)
{
	Parent->AddChild((new UIButton(true, 0, 1, OnClicked))
		->SetPadding(0.01, 0.01, 0.06, 0)
		->SetVerticalAlign(UIBox::Align::Reverse)
		->SetBorder(UIBox::BorderType::Rounded, 0.25)
		->SetMinSize(Vector2f(0.6, 0))
		->AddChild((new UIBackground(true, 0, 0, 0.05))
			->SetUseTexture(true, Icon(IconName).TextureID)
			->SetPadding(0.01, 0.01, 0.01, 0)
			->SetSizeMode(UIBox::SizeMode::AspectRelative))
		->AddChild((new UIText(0.35, 0, GetTranslation(Text), UI::Text))
			->SetPadding(0.01, 0.01, 0.01, 0.01)));
}


void SettingsTab::GenerateSettings()
{
	SettingsBox->DeleteChildren();

	std::string ShortGamePath = Game::GamePath;
	if (ShortGamePath.size() > MAX_GAMEPATH_SIZE)
	{
		ShortGamePath = ShortGamePath.substr(0, MAX_GAMEPATH_SIZE - 3) + "...";
	}

	AddCategoryHeader("settings_category_general", "itab_settings", SettingsBox);
	AddSettingsButton("settings_locate_game", "Settings/Folder", LocateTitanfall, SettingsBox);

	bool PathValid = true;
	std::string PathString = Format(GetTranslation("settings_game_path"), ShortGamePath.c_str());
	if (!std::filesystem::exists(Game::GamePath))
	{
		PathString = GetTranslation("settings_game_no_path");
		PathValid = false;
	}

	SettingsBox->AddChild((new UIBox(true, 0))
		->SetPadding(0.01, 0.05, 0.09, 0)
		->AddChild((new UIBackground(true, 0, PathValid ? Vector3f32(0, 1, 0.5) : Vector3f32(1, 0.5, 0), 0.05))
			->SetUseTexture(true, PathValid ? Icon("Enabled").TextureID : Icon("Settings/Warning").TextureID)
			->SetSizeMode(UIBox::SizeMode::AspectRelative)
			->SetPadding(0.01, 0.01, 0, 0))
		->AddChild((new UIText(0.35, 0.9, PathString, UI::Text))));

	if (Game::IsValidTitanfallLocation(Game::GamePath))
	{
		AddSettingsButton("settings_recheck_updates", "Settings/Reload", []() {
			new BackgroundTask(Installer::CheckForUpdates);
			}, SettingsBox);


		AddSettingsButton("settings_reinstall_northstar", "Download", Game::UpdateGameAsync, SettingsBox);

		LaunchArgsText = new UITextField(true, 0, 1, UI::MonoText, []() {Game::SetLaunchArgs(CurrentSettingsTab->LaunchArgsText->GetText()); });

		SettingsBox->AddChild((new UIBox(true, 0))
			->SetPadding(0.05, 0.01, 0.06, 0.01)
			->AddChild((new UIBackground(true, 0, 1, 0.05))
				->SetUseTexture(true, Icon("Settings/Arguments").TextureID)
				->SetPadding(0.01, 0.01, 0, 0)
				->SetSizeMode(UIBox::SizeMode::AspectRelative))
			->AddChild((new UIText(0.4, 1, GetTranslation("settings_launch_arg"), UI::Text))
				->SetPadding(0.01)));
		SettingsBox->AddChild(LaunchArgsText
			->SetTextColor(0)
			->SetTextSize(0.3)
			->SetText(Game::GetLaunchArgs())
			->SetPadding(0.01, 0.01, 0.06, 0.01)
			->SetMinSize(Vector2f(0.6, 0.05)));
	}


	SettingsBox->AddChild((new UIBox(true, 0))
		->SetPadding(0.05, 0.01, 0.06, 0.01)
		->AddChild((new UIBackground(true, 0, 1, 0.05))
			->SetUseTexture(true, Icon("Settings/Language").TextureID)
			->SetPadding(0.01, 0.01, 0, 0)
			->SetSizeMode(UIBox::SizeMode::AspectRelative))
		->AddChild((new UIText(0.4, 1, GetTranslation("settings_language"), UI::Text))
			->SetPadding(0.01)));

	std::string SelectedLangauge = LoadedTranslation;

	auto PossibleLangauges = GetAvaliableTranslations();

	std::vector<UIDropdown::Option> LanguageOption;
	size_t Selected = 0;

	for (size_t i = 0; i < PossibleLangauges.size(); i++)
	{
		LanguageOption.push_back(UIDropdown::Option(PossibleLangauges[i]));
		if (PossibleLangauges[i] == SelectedLangauge)
		{
			Selected = i;
		}
	}

	LanguageDropdown = new UIDropdown(0, 0.6, 1, 0, LanguageOption, []() 
		{
			Translation::LoadTranslation(CurrentSettingsTab->LanguageDropdown->SelectedOption.Name);
			CurrentSettingsTab->GenerateSettings();
		}, UI::Text);

	SettingsBox->AddChild(LanguageDropdown
		->SetTextSize(0.3, 0.005)
		->SetPadding(0.01, 0.01, 0.06, 0.01));
	LanguageDropdown->SelectOption(Selected, false);

	if (Game::IsValidTitanfallLocation(Game::GamePath))
	{
		AddCategoryHeader("settings_category_logs", "Settings/Logs", SettingsBox);

		AddSettingsButton("settings_open_log_directory", "Settings/Folder", []()
			{
				if (!std::filesystem::exists(ProfileTab::CurrentProfile.Path + "/logs"))
				{
					Window::ShowPopupError("Log folder does not exist.");
					return;
				}
#if _WIN32
				system(("cd \"" + ProfileTab::CurrentProfile.Path + "/logs\" && explorer .").c_str());
#else
				system(("xdg-open \"" + ProfileTab::CurrentProfile.Path + "/logs\"").c_str());
#endif
			}, SettingsBox);

		AddSettingsButton("settings_open_latest_log", "Settings/Logs", []()
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
#if _WIN32
				system(("start \"\" \"" + LatestLog.path().u8string() + "\"").c_str());
#else
				system(("xdg-open \"\" \"" + LatestLog.path().u8string() + "\"").c_str());
#endif
			}, SettingsBox);


		AddCategoryHeader("settings_category_danger", "Settings/Warning", SettingsBox);
		AddSettingsButton("settings_delete_all_mods", "Delete", DeleteAllMods, SettingsBox);

		AddSettingsButton("settings_reset_installer", "Settings/Reload", []() {
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
				Log::Print(e.what());
			}
			}, SettingsBox);
	}

#if _WIN32
	constexpr const char* OS = "Windows";
#elif __linux__
	constexpr const char* OS = "Linux";
#else
	constexpr const char* OS = "Unknown";
#endif

	AddCategoryHeader("settings_category_about", "Settings/About", SettingsBox);
	SettingsBox->AddChild((new UIText(0.35, 1, GetTranslation("settings_about_ns_version") + ": " + Game::GetCurrentVersion(), UI::Text))
		->SetPadding(0.01, 0.01, 0.06, 0.01));
	SettingsBox->AddChild((new UIText(0.35, 1, GetTranslation("settings_about_version")
		+ ": "
		+ Installer::InstallerVersion 
		+ " ("
		+ std::string(OS) 
		+ ")", UI::Text))
		->SetPadding(0.01, 0.01, 0.06, 0.01));
}
