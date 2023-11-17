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
#include "../UI/Icon.h"
#include "../Translation.h"

using namespace Translation;

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

static std::string ColorToRgb(Vector3f32 Color)
{
	Vector3<uint8_t> RgbColor = Color * 255;
	return "#" + UintToHex(RgbColor.X) + UintToHex(RgbColor.Y) + UintToHex(RgbColor.Z);
}

static Vector3f32 RgbToColor(std::string Rgb)
{
	if (Rgb.empty() || Rgb[0] != '#')
	{
		return -1;
	}
	Rgb.resize(7, '0');

	uint8_t x = CharHex(Rgb.substr(1, 2));
	uint8_t y = CharHex(Rgb.substr(3, 2));
	uint8_t z = CharHex(Rgb.substr(5, 2));

	return Vector3f32(x / 255.0f, y / 255.0f, z / 255.0f);
}


static void SaveSettings()
{
	std::ofstream out = std::ofstream("Data/var/appearance.txt");
	out << Installer::GetThemeColor().X << " " << Installer::GetThemeColor().Y << " " << Installer::GetThemeColor().Z << std::endl;
	out << (Installer::UseSystemTitleBar ? "system" : "custom");
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
	Installer::SetThemeColor(Vector3f32::FromString(std::string(Buffer)));
	in.getline(Buffer, 2048);
	Installer::UseSystemTitleBar = std::string("system") == Buffer;
	Installer::UpdateWindowFlags();
	in.close();
}

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
	LoadSettings();
	
	GenerateSettings();
}

void SettingsTab::Tick()
{
	SettingsBackground->SetMinSize(Vector2f(1, Background->GetUsedSize().Y));
}

static void DeleteAllMods()
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
	std::string NewPath = Window::ShowSelectFileDialog(true);
	if (Game::IsValidTitanfallLocation(NewPath))
	{
		Game::SaveGameDir(NewPath);
		Game::GamePath = Game::GetTitanfallLocation();
		Installer::UpdateCheckedOnce = false;
		new BackgroundTask(Installer::CheckForUpdates);
		ProfileTab::CurrentProfileTab->DetectProfiles();
		if (ProfileTab::AllProfiles.size())
		{
			ProfileTab::CurrentProfile = ProfileTab::AllProfiles[0];
		}
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

	AddCategoryHeader("settings_category_appearance", "Settings/App", SettingsBox);

	AddSettingsButton("settings_appearance_background", "Settings/Image", []() {
		std::string NewPicture = Window::ShowSelectFileDialog(false);
		if (!std::filesystem::exists(NewPicture))
		{
			return;
		}

		try
		{
			std::filesystem::copy(NewPicture, "Data/var/custom_background.png", std::filesystem::copy_options::overwrite_existing);
			Installer::SetInstallerBackgroundImage("Data/var/custom_background.png");
		}
		catch (std::exception& e)
		{
			Log::Print(e.what(), Log::Error);
		}
		CurrentSettingsTab->GenerateSettings();
		}, SettingsBox);


	if (std::filesystem::exists("Data/var/custom_background.png"))
	{
		SettingsBox->AddChild((new UIButton(true, 0, 0.75, []() 
			{
				std::filesystem::remove("Data/var/custom_background.png.png");
				Installer::SetInstallerBackgroundImage("Data/Game.png");
				CurrentSettingsTab->GenerateSettings();
			}))
			->SetMinSize(Vector2f(0.6, 0))
			->SetPadding(0, 0.05, 0.06, 0)
			->SetBorder(UIBox::BorderType::Rounded, 0.25)
			->AddChild((new UIBackground(true, 0, 0, 0.04))
				->SetUseTexture(true, Icon("Revert").TextureID)
				->SetSizeMode(UIBox::SizeMode::AspectRelative)
				->SetPadding(0.01, 0.01, 0.01, 0.01))
			->AddChild((new UIText(0.3, 0, GetTranslation("settings_appearance_background_revert"), UI::Text))));
	}

	std::vector<UIDropdown::Option> TitleBarOptions = 
	{
		UIDropdown::Option(GetTranslation("settings_title_bar_custom")),
		UIDropdown::Option(GetTranslation("settings_title_bar_system"))
	};

	SettingsBox->AddChild((new UIBox(true, 0))
		->SetPadding(0.05, 0.01, 0.06, 0.01)
		->AddChild((new UIBackground(true, 0, 1, 0.05))
			->SetUseTexture(true, Icon("Settings/App").TextureID)
			->SetPadding(0.01, 0.01, 0, 0)
			->SetSizeMode(UIBox::SizeMode::AspectRelative))
		->AddChild((new UIText(0.4, 1, GetTranslation("settings_title_bar"), UI::Text))
			->SetPadding(0.01)));

	TitleBarDropdown = new UIDropdown(0, 0.6, 1, 0, TitleBarOptions, []()
		{
			Installer::UseSystemTitleBar = CurrentSettingsTab->TitleBarDropdown->SelectedIndex == 1;
			Installer::UpdateWindowFlags();
			SaveSettings();
		}, UI::Text);
	SettingsBox->AddChild(TitleBarDropdown
		->SetTextSize(0.3, 0.005)
		->SetPadding(0.01, 0.01, 0.06, 0.01));
	TitleBarDropdown->SelectOption(Installer::UseSystemTitleBar, false);

	SettingsBox->AddChild((new UIBox(true, 0))
		->SetPadding(0.05, 0.01, 0.06, 0.01)
		->AddChild((new UIBackground(true, 0, 1, 0.05))
			->SetUseTexture(true, Icon("Settings/Color").TextureID)
			->SetPadding(0.01, 0.01, 0, 0)
			->SetSizeMode(UIBox::SizeMode::AspectRelative))
		->AddChild((new UIText(0.4, 1, GetTranslation("settings_appearance_color"), UI::Text))
			->SetPadding(0.01)));

	ColorText = new UITextField(true, 0, 1, UI::MonoText, []() 
		{ 
			auto Color = RgbToColor(CurrentSettingsTab->ColorText->GetText());
			if (Color == Vector3f32(-1))
			{
				CurrentSettingsTab->ColorText->SetText(ColorToRgb(Installer::GetThemeColor()));
				return;
			}
			Installer::SetThemeColor(Color);
			SaveSettings();
			CurrentSettingsTab->ColorText->SetText(ColorToRgb(Color));
		});

	SettingsBox->AddChild(ColorText
		->SetTextColor(0)
		->SetTextSize(0.3)
		->SetText(ColorToRgb(Installer::GetThemeColor()))
		->SetPadding(0.01, 0.01, 0.06, 0.01)
		->SetMinSize(Vector2f(0.6, 0.05)));


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
