#include "SettingsTab.h"
#include "ProfileTab.h"

#include <KlemmUI/UI/UIText.h>
#include <KlemmUI/UI/UIButton.h>

#include <thread>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <cmath>
#include <filesystem>

#include "../Log.h"
#include "../UI/UIDef.h"
#include "../Game.h"
#include "../Installer.h"
#include "../BackgroundTask.h"
#include "../WindowFunctions.h"
#include "../UI/TitleBar.h"
#include "../Translation.h"
#include "../AppUpdate.h"

#include "../Markup/SettingsSection.hpp"
#include "../Markup/TabElement.hpp"

using namespace Translation;
using namespace KlemmUI;

static char HexChar(uint8_t val)
{
	if (val < 10)
	{
		return val + '0';
	}
	return val - 10 + 'a';
}

uint8_t CharHex(std::string hex)
{
	uint8_t result = 0;
	for (int i = 0; i < hex.length(); i++)
	{
		if (hex[i] >= 48 && hex[i] <= 57)
		{
			result += (hex[i] - 48) * pow(16, hex.length() - i - 1);
		}
		else if (hex[i] >= 65 && hex[i] <= 70)
		{
			result += (hex[i] - 55) * pow(16, hex.length() - i - 1);
		}
		else if (hex[i] >= 97 && hex[i] <= 102)
		{
			result += (hex[i] - 87) * pow(16, hex.length() - i - 1);
		}
	}
	return result;
}


static std::string UintToHex(uint8_t Uint)
{
	return std::string({ HexChar(Uint / 16), HexChar(Uint % 16)});
}

static std::string ColorToRgb(Vector3f Color)
{
	Vector3<uint8_t> RgbColor = Color * 255;
	return "#" + UintToHex(RgbColor.X) + UintToHex(RgbColor.Y) + UintToHex(RgbColor.Z);
}

static Vector3f RgbToColor(std::string Rgb)
{
	if (Rgb.empty() || Rgb[0] != '#')
	{
		return -1;
	}
	Rgb.resize(7, '0');

	uint8_t x = CharHex(Rgb.substr(1, 2));
	uint8_t y = CharHex(Rgb.substr(3, 2));
	uint8_t z = CharHex(Rgb.substr(5, 2));

	return Vector3f(x / 255.0f, y / 255.0f, z / 255.0f);
}


static void SaveSettings()
{
	std::ofstream out = std::ofstream(Installer::CurrentPath + "Data/var/appearance.txt");
	out << Installer::GetThemeColor().X << " " << Installer::GetThemeColor().Y << " " << Installer::GetThemeColor().Z << std::endl;
	out << (TitleBar::GetUseSystemTitleBar() ? "system" : "custom");
	out.close();
}

static void LoadSettings()
{
	if (!std::filesystem::exists("Data/var/appearance.txt"))
	{
		return;
	}

	std::ifstream in = std::ifstream("Data/var/appearance.txt");
	char Buffer[2048];
	in.getline(Buffer, 2048);
	Installer::SetThemeColor(Vector3f::FromString(std::string(Buffer)));
	in.getline(Buffer, 2048);
	TitleBar::SetUseSystemTitleBar(std::string("system") == Buffer);
	Installer::UpdateWindowFlags();
	in.close();
}

SettingsTab* SettingsTab::CurrentSettingsTab = nullptr;
SettingsTab::SettingsTab()
{
	CurrentSettingsTab = this;
	Name = "settings";
	Log::Print("Loading settings tab...");

	Background
		->SetHorizontalAlign(UIBox::Align::Centered)
		->SetHorizontal(true);
	auto TabElem = new TabElement();
	TabElem->SetTabName(GetTranslation("tab_settings"));
	Background->AddChild(TabElem);

	TabTitle = TabElem->title;
	SettingsBox = new UIScrollBox(false, 0, true);
	SettingsBox->SetMinSize(Vector2f(1.0f, 1.78));
	SettingsBox->SetMaxSize(Vector2f(2, 1.78));
	TabElem->contentBox->AddChild(SettingsBox);
	LoadSettings();
	
	GenerateSettings();
}

void SettingsTab::Tick()
{
}

static void DeleteAllMods()
{
	for (const auto& m : std::filesystem::directory_iterator(ProfileTab::CurrentProfile.Path + "/mods/"))
	{
		if (Game::CoreModNames.find(m.path().filename().u8string()) == Game::CoreModNames.end() && std::filesystem::is_directory(m))
		{
			std::filesystem::remove_all(m);
			Log::Print("Removing mod: " + m.path().filename().u8string(), Log::Warning);
		}
	}
	std::filesystem::remove_all(Installer::CurrentPath + "Data/var/modinfo");
}

void LocateTitanfall()
{
	std::string NewPath = WindowFunc::ShowSelectFileDialog(true);
	if (Game::IsValidTitanfallLocation(NewPath))
	{
		Game::SaveGameDir(NewPath);
		Game::GamePath = Game::GetTitanfallLocation();
		new BackgroundTask(AppUpdate::CheckForUpdates);
		ProfileTab::CurrentProfileTab->DetectProfiles();
		if (ProfileTab::AllProfiles.size())
		{
			ProfileTab::CurrentProfile = ProfileTab::AllProfiles[0];
		}
	}
	else if (!NewPath.empty())
	{
		WindowFunc::ShowPopupError(NewPath + " is not a valid Titanfall 2 path.");
	}
	SettingsTab::CurrentSettingsTab->GenerateSettings();
}

constexpr uint16_t MAX_GAMEPATH_SIZE = 30;


void AddSettingsButton(std::string Text, std::string IconName, void(*OnClicked)(), UIBox* Parent)
{
	Parent->AddChild((new UIButton(true, 0, 1, OnClicked))
		->SetPadding(5, 5, 5, 0)
		->SetPaddingSizeMode(UIBox::SizeMode::PixelRelative)
		->SetVerticalAlign(UIBox::Align::Centered)
		->SetBorder(UIBox::BorderType::Rounded, 0.25)
		->SetMinSize(Vector2f(0.8f, 0))
		->AddChild((new UIBackground(true, 0, 0, 20))
			->SetUseTexture(true, IconName)
			->SetSizeMode(UIBox::SizeMode::PixelRelative)
			->SetPadding(5, 5, 5, 0)
			->SetPaddingSizeMode(UIBox::SizeMode::PixelRelative))
		->AddChild((new UIText(12, 0, GetTranslation(Text), UI::Text))
			->SetTextSizeMode(UIBox::SizeMode::PixelRelative)
			->SetPaddingSizeMode(UIBox::SizeMode::PixelRelative)
			->SetPadding(5, 5, 5, 0)));
}

void AddSettingsText(std::string Text, std::string IconName, UIBox* Parent)
{
	Parent->AddChild((new UIBox(true, 0))
		->SetPadding(0.05f, 0.01f, 0.01f, 0.01f)
		->AddChild((new UIBackground(true, 0, 1, 25))
			->SetUseTexture(true, IconName)
			->SetPadding(0, 0, 0, 0.01f)
			->SetSizeMode(UIBox::SizeMode::PixelRelative))
		->AddChild((new UIText(15, 1, GetTranslation(Text), UI::Text))
			->SetTextSizeMode(UIBox::SizeMode::PixelRelative)));
}

void AddSettingsDropdown(std::vector<UIDropdown::Option> Options, void(*OnClicked)(int SelectedIndex), size_t DefaultOption, UIBox* Parent)
{
	auto TitleBarDropdown = new UIDropdown(0, 0.8, 1, 0, Options, OnClicked, UI::Text);
	Parent->AddChild(TitleBarDropdown
		->SetTextSizeMode(UIBox::SizeMode::PixelRelative)
		->SetTextSize(11, 0.005)
		->SetPadding(0.01f, 0.01f, 0.01f, 0.01f));
	TitleBarDropdown->SelectOption(DefaultOption, false);
}

void SettingsTab::GenerateSettings()
{
	SettingsBox->DeleteChildren();

	std::string ShortGamePath = Game::GamePath;
	if (ShortGamePath.size() > MAX_GAMEPATH_SIZE)
	{
		ShortGamePath = ShortGamePath.substr(0, MAX_GAMEPATH_SIZE - 3) + "...";
	}

	auto GeneralSection = new SettingsSection();
	GeneralSection->SetTitle(GetTranslation("settings_category_general"));
	GeneralSection->SetIcon("itab_settings.png");
	SettingsBox->AddChild(GeneralSection);

	AddSettingsButton("settings_locate_game", "Settings/Folder.png", LocateTitanfall, GeneralSection->contentBox);

	bool PathValid = true;
	std::string PathString = Format(GetTranslation("settings_game_path"), ShortGamePath.c_str());
	if (!std::filesystem::exists(Game::GamePath))
	{
		PathString = GetTranslation("settings_game_no_path");
		PathValid = false;
	}

	GeneralSection->contentBox->AddChild((new UIBox(true, 0))
		->SetPadding(0.01f, 0.01f, 0.03f, 0.0f)
		->SetVerticalAlign(UIBox::Align::Centered)
		->AddChild((new UIBackground(true, 0, PathValid ? Vector3f(0, 1, 0.5f) : Vector3f(1, 0.5f, 0), 22))
			->SetUseTexture(true, PathValid ? "Enabled.png" : "Settings/Warning.png")
			->SetSizeMode(UIBox::SizeMode::PixelRelative)
			->SetPadding(0.01, 0.01, 0.01, 0.01))
		->AddChild((new UIText(12, 0.9, PathString, UI::Text))
			->SetTextSizeMode(UIBox::SizeMode::PixelRelative)));

	if (Game::IsValidTitanfallLocation(Game::GamePath))
	{
		AddSettingsButton("settings_recheck_updates", "Settings/Reload.png", []() {
			new BackgroundTask([]()
				{
					AppUpdate::CheckForUpdates();
					if (AppUpdate::RequiresUpdate)
					{
						WindowFunc::ShowPopup(GetTranslation("app_name"), GetTranslation("settings_update_required"));
					}
					else
					{
						WindowFunc::ShowPopup(GetTranslation("app_name"), GetTranslation("settings_update_not_required"));
					}
				});
				}, GeneralSection->contentBox);


		AddSettingsButton("settings_reinstall_northstar", "Download.png", Game::UpdateGameAsync, GeneralSection->contentBox);

		AddSettingsText("settings_launch_arg", "Settings/Arguments.png", GeneralSection->contentBox);

		LaunchArgsText = new UITextField(0, 1, UI::MonoText, []() {Game::SetLaunchArgs(CurrentSettingsTab->LaunchArgsText->GetText()); });
		GeneralSection->contentBox->AddChild(LaunchArgsText
			->SetTextColor(0)
			->SetTextSize(11)
			->SetTextSizeMode(UIBox::SizeMode::PixelRelative)
			->SetText(Game::GetLaunchArgs())
			->SetPadding(0.01f, 0.01f, 0.01f, 0.01f)
			->SetMinSize(Vector2f(0.8f, 0)));
	}


	AddSettingsText("settings_language", "Settings/Language.png", GeneralSection->contentBox);

	std::string SelectedLanguage = LoadedTranslation;

	auto PossibleLanguages = GetAvailableTranslations();

	size_t Selected = 0;

	LanguageOptions.clear();
	for (size_t i = 0; i < PossibleLanguages.size(); i++)
	{
		LanguageOptions.push_back(UIDropdown::Option(PossibleLanguages[i]));
		if (PossibleLanguages[i] == SelectedLanguage)
		{
			Selected = i;
		}
	}

	AddSettingsDropdown(LanguageOptions,
		[](int Index)
		{
			Translation::LoadTranslation(CurrentSettingsTab->LanguageOptions[Index].Name);
			CurrentSettingsTab->GenerateSettings();
		},
		Selected, GeneralSection->contentBox);

	auto AppearanceSection = new SettingsSection();
	AppearanceSection->SetTitle(GetTranslation("settings_category_appearance"));
	AppearanceSection->SetIcon("Settings/App.png");
	SettingsBox->AddChild(AppearanceSection);

	AddSettingsButton("settings_appearance_background", "Settings/Image.png", []() {
		std::string NewPicture = WindowFunc::ShowSelectFileDialog(false);
		if (!std::filesystem::exists(NewPicture))
		{
			return;
		}

		try
		{
			std::filesystem::copy(NewPicture, Installer::CurrentPath + "Data/var/custom_background.png", std::filesystem::copy_options::overwrite_existing);
			Installer::SetInstallerBackgroundImage(Installer::CurrentPath + "Data/var/custom_background.png");
		}
		catch (std::exception& e)
		{
			Log::Print(e.what(), Log::Error);
		}
		CurrentSettingsTab->GenerateSettings();
		}, AppearanceSection->contentBox);


	if (std::filesystem::exists(Installer::CurrentPath + "Data/var/custom_background.png"))
	{
		AppearanceSection->contentBox->AddChild((new UIButton(true, 0, 0.75, []()
			{
				std::filesystem::remove(Installer::CurrentPath + "Data/var/custom_background.png");
				Installer::SetInstallerBackgroundImage(Installer::CurrentPath + "Data/Game.png");
				CurrentSettingsTab->GenerateSettings();
			}))
			->SetMinSize(Vector2f(0.6, 0))
			->SetPadding(0, 0.05, 0.06, 0)
			->SetBorder(UIBox::BorderType::Rounded, 0.25)
			->AddChild((new UIBackground(true, 0, 0, 0.04))
				->SetUseTexture(true, "Revert.png")
				->SetSizeMode(UIBox::SizeMode::AspectRelative)
				->SetPadding(0.01, 0.01, 0.01, 0.01))
			->AddChild((new UIText(0.6f, 0, GetTranslation("settings_appearance_background_revert"), UI::Text))));
	}


	AddSettingsText("settings_title_bar", "Settings/App.png", AppearanceSection->contentBox);

	AddSettingsDropdown(
		{
		UIDropdown::Option(GetTranslation("settings_title_bar_custom")),
		UIDropdown::Option(GetTranslation("settings_title_bar_system"))
		},
		[](int Index)
		{
			TitleBar::SetUseSystemTitleBar(Index == 1);
			Installer::UpdateWindowFlags();
			SaveSettings();
		}, TitleBar::GetUseSystemTitleBar(), AppearanceSection->contentBox);

	AddSettingsText("settings_appearance_color", "Settings/Color.png", AppearanceSection->contentBox);

	ColorText = new UITextField(0, 1, UI::MonoText, []() 
		{ 
			auto Color = RgbToColor(CurrentSettingsTab->ColorText->GetText());
			if (Color == Vector3f(-1))
			{
				CurrentSettingsTab->ColorText->SetText(ColorToRgb(Installer::GetThemeColor()));
				return;
			}
			Installer::SetThemeColor(Color);
			SaveSettings();
			CurrentSettingsTab->ColorText->SetText(ColorToRgb(Color));
		});

	AppearanceSection->contentBox->AddChild(ColorText
		->SetText(ColorToRgb(Installer::GetThemeColor()))
		->SetTextColor(0)
		->SetTextSize(11)
		->SetTextSizeMode(UIBox::SizeMode::PixelRelative)
		->SetPadding(0.01f, 0.01f, 0.01f, 0.01f)
		->SetMinSize(Vector2f(0.8f, 0)));

	if (Game::IsValidTitanfallLocation(Game::GamePath))
	{
		auto LogsSection = new SettingsSection();
		LogsSection->SetTitle(GetTranslation("settings_category_appearance"));
		LogsSection->SetIcon("Settings/App.png");
		SettingsBox->AddChild(LogsSection);

		AddSettingsButton("settings_open_log_directory", "Settings/Folder.png", []()
			{
				if (!std::filesystem::exists(ProfileTab::CurrentProfile.Path + "/logs"))
				{
					WindowFunc::ShowPopupError("Log folder does not exist.");
					return;
				}
#if _WIN32
				system(("cd \"" + ProfileTab::CurrentProfile.Path + "/logs\" && explorer .").c_str());
#else
				system(("xdg-open \"" + ProfileTab::CurrentProfile.Path + "/logs\"").c_str());
#endif
			}, LogsSection->contentBox);

		AddSettingsButton("settings_open_latest_log", "Settings/Logs.png", []()
			{
				if (!std::filesystem::exists(ProfileTab::CurrentProfile.Path + "/logs"))
				{
					WindowFunc::ShowPopupError("Log folder does not exist.");
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
					WindowFunc::ShowPopupError("Could not find a log file.");
					return;
				}
#if _WIN32
				system(("start \"\" \"" + LatestLog.path().u8string() + "\"").c_str());
#else
				system(("xdg-open \"\" \"" + LatestLog.path().u8string() + "\"").c_str());
#endif
			}, LogsSection->contentBox);

		auto DangerSection = new SettingsSection();
		DangerSection->SetTitle(GetTranslation("settings_category_danger"));
		DangerSection->SetIcon("Settings/Warning.png");
		SettingsBox->AddChild(DangerSection);

		AddSettingsButton("settings_delete_all_mods", "Delete.png", DeleteAllMods, DangerSection->contentBox);

		AddSettingsButton("settings_reset_installer", "Settings/Reload.png", []() {
			try
			{
				std::filesystem::remove_all(Installer::CurrentPath + "Data/temp");
				Log::Print("Deleted ./Data/temp/", Log::Warning);
				std::filesystem::remove_all("./Data/var");
				Log::Print("Deleted ./Data/var/", Log::Warning);
				std::filesystem::create_directories(Installer::CurrentPath + "./Data/var");
				std::filesystem::create_directories(Installer::CurrentPath + "./Data/temp");
			}
			catch (std::exception& e)
			{
				Log::Print(e.what());
			}
			}, DangerSection->contentBox);
	}

#if _WIN32
	constexpr const char* OS = "Windows";
#elif __linux__
	constexpr const char* OS = "Linux";
#else
	constexpr const char* OS = "Unknown";
#endif
	auto AboutSection = new SettingsSection();
	AboutSection->SetTitle(GetTranslation("settings_category_about"));
	AboutSection->SetIcon("Settings/About.png");
	SettingsBox->AddChild(AboutSection);

	AboutSection->contentBox->AddChild((new UIText(12, 1, GetTranslation("settings_about_ns_version") + ": " + Game::GetCurrentVersion(), UI::Text))
		->SetTextSizeMode(UIBox::SizeMode::PixelRelative)
		->SetPaddingSizeMode(UIBox::SizeMode::PixelRelative)
		->SetPadding(5));
	AboutSection->contentBox->AddChild((new UIText(12, 1, GetTranslation("settings_about_version")
		+ ": Tether "
		+ Installer::InstallerVersion 
		+ " ("
		+ std::string(OS) 
		+ ")", UI::Text))
		->SetTextSizeMode(UIBox::SizeMode::PixelRelative)
		->SetPaddingSizeMode(UIBox::SizeMode::PixelRelative)
		->SetPadding(5));
}
