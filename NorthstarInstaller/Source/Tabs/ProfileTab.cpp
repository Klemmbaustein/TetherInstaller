#include "ProfileTab.h"
#include "ModsTab.h"
#include "SettingsTab.h"

#include <KlemmUI/UI/UIText.h>
#include <KlemmUI/UI/UIButton.h>

#include <fstream>
#include <filesystem>

#include "../UI/Icon.h"
#include "../UI/UIDef.h"
#include "../Game.h"
#include "../Log.h"
#include "../Installer.h"
#include "../BackgroundTask.h"
#include "../Thunderstore.h"
#include "../WindowFunctions.h"
#include "../Translation.h"

#ifdef TF_PLUGIN
#include "../TetherPlugin.h"
#endif

using namespace Translation;
using namespace KlemmUI;

ProfileTab::Profile ProfileTab::CurrentProfile;
std::vector<ProfileTab::Profile> ProfileTab::AllProfiles;
ProfileTab* ProfileTab::CurrentProfileTab = nullptr;

// https://github.com/R2NorthstarTools/FlightCore/blob/b61eeba6e552fa0b6fb96f3eee6202200821826b/src-tauri/src/northstar/profile.rs#L5-L28
// Thanks flightcore.

const std::set<std::string> SKIP_PATHS = 
{
	"__overlay",
	"bin",
	"Core",
	"r2",
	"vpk",
	"platform",
	"Support",
};

// A profile may have one of these to be detected
const std::set<std::string> MAY_CONTAIN = 
{
	"mods/",
	"plugins/",
	"packages/",
	"logs/",
	"runtime/",
	"save_data/",
	"Northstar.dll",
	"enabledmods.json",
};

ProfileTab::ProfileTab()
{
	Name = "profiles";
	Log::Print("Loading profile tab...");

	Background->SetHorizontalAlign(UIBox::Align::Centered);
	Background->SetHorizontal(true);

	ProfileBackground = new UIBackground(false, 0, 0, Vector2f(1.5, 1.85));
	Background->AddChild(ProfileBackground
		->SetOpacity(0.65)
		->AddChild((new UIBackground(true, 0, 1, Vector2f(1.5, 0.005)))
			->SetPadding(0))
		->SetPadding(0));

	TabTitle = new UIText(1.4f, 1, "Profiles", UI::Text);

	ProfileBackground->AddChild(TabTitle);

	ProfileList = new UIScrollBox(false, 0, true);

	ProfileInfoBox = new UIBox(false, 0);

	ProfileBackground->AddChild((new UIBox(true, 0))
		->AddChild(ProfileList
			->SetMinSize(Vector2f(0.85, 1.55)))
		->AddChild(ProfileInfoBox
			->SetMinSize(Vector2f(0, 1.55))));
	ProfileCreationBox = new UIBackground(false, 0, 0);
	ProfileBackground->AddChild(ProfileCreationBox);
		
	GenerateProfileCreationBox();

	CurrentProfileTab = this;

	DetectProfiles();
	
	if (std::filesystem::exists(Installer::CurrentPath + "Data/var/SelectedProfile.txt"))
	{
		Log::Print("- Using profile from SelectedProfile.txt");
		std::ifstream InProfile = std::ifstream(Installer::CurrentPath + "Data/var/SelectedProfile.txt");
		char LineBuffer[2048];
		InProfile.getline(LineBuffer, 2048);
		CurrentProfile.DisplayName = LineBuffer;
		InProfile.getline(LineBuffer, 2048);
		CurrentProfile.Path = LineBuffer;
		InProfile.close();
	}
	
	UpdateProfilesList();
	DisplayProfileInfo();

}

bool ProfileTab::IsFolderValidProfilePath(std::string FolderPath)
{
	if (!std::filesystem::is_directory(FolderPath))
	{
		return false;
	}

	if (SKIP_PATHS.find(std::filesystem::path(FolderPath).filename().u8string()) != SKIP_PATHS.end())
	{
		return false;
	}

	for (auto& File : MAY_CONTAIN)
	{
		if (std::filesystem::exists(FolderPath + "/" + File))
		{
			return true;
		}
	}
	return false;
}

void ProfileTab::OnProfileSwitched()
{
	LOG_PRINTF("Switched profile to \"{}\"", CurrentProfile.DisplayName);
	ModsTab::Reload();
	new BackgroundTask(Installer::CheckForUpdates);
	if (!BackgroundTask::IsFunctionRunningAsTask(ModsTab::CheckForModUpdates))
	{
		new BackgroundTask(ModsTab::CheckForModUpdates);
	}
	CurrentProfileTab->UpdateProfilesList();
	CurrentProfileTab->DisplayProfileInfo();
	CurrentProfileTab->SaveSelectedProfile();
}

void ProfileTab::DetectProfiles()
{
	AllProfiles.clear();
	if (!std::filesystem::exists(Game::GamePath))
	{
		Log::Print("- Looking for Northstar profiles failed - Game path does not exist.", Log::Warning);
		return;
	}

	Log::Print("- Looking for Northstar profiles");

	for (const auto& f : std::filesystem::directory_iterator(Game::GamePath))
	{
		if (IsFolderValidProfilePath(f.path().u8string()))
		{
			Profile NewProfile;
			NewProfile.Path = f.path().u8string();
			NewProfile.DisplayName = f.path().filename().u8string();

			if (NewProfile.DisplayName == "R2Northstar" && CurrentProfile.Path.empty())
			{
				CurrentProfile = NewProfile;
			}

			LOG_PRINTF("- * Found profile: {}", NewProfile.DisplayName);
			AllProfiles.push_back(NewProfile);
		}
	}
}

void ProfileTab::Tick()
{
	ProfileBackground->SetMinSize(Vector2f(0, Background->GetUsedSize().Y));
	ProfileBackground->SetMaxSize(Vector2f(2, Background->GetUsedSize().Y));
}

void ProfileTab::DisplayProfileInfo()
{
	ProfileInfoBox->DeleteChildren();

	ProfileInfoBox->AddChild((new UIText(0.7f, 1, GetTranslation("profile_current"), UI::Text))
		->SetPadding(0, 0.01, 0.01, 0));

	ProfileInfoBox->AddChild((new UIBackground(true, 0, 1, Vector2f(0.575, 0.005)))
		->SetPadding(0.0, 0, 0, 0));
	ProfileInfoBox->AddChild(new UIText(1.5f, 1, CurrentProfile.DisplayName, UI::Text));
	ProfileInfoBox->AddChild((new UIBackground(true, 0, 1, Vector2f(0.575, 0.005)))
		->SetPadding(0.0, 0.05, 0, 0));

	auto Mods = Thunderstore::GetInstalledMods().Combined();

	ProfileInfoBox->AddChild(new UIText(1.0f, 1, GetTranslation("profile_mods"), UI::Text));

	for (auto& i : Mods)
	{
		std::vector<TextSegment> ModString = { TextSegment("- " + i.Namespace + "." + i.Name, 1) };
		if (!Thunderstore::GetModEnabled(i))
		{
			ModString.push_back(TextSegment("  " + GetTranslation("profile_disabled"), Vector3f(1, 0.5f, 0)));
		}

		ProfileInfoBox->AddChild((new UIText(0.6f, ModString, UI::Text))
			->SetPadding(0.0f, 0.005f, 0.02f, 0.01f));
	}

	if (Mods.empty())
	{
		ProfileInfoBox->AddChild((new UIText(0.6f, 1, GetTranslation("profile_no_mods"), UI::Text))
			->SetPadding(0, 0.005f, 0.02f, 0.01f));
	}

	ProfileInfoBox->AddChild(new UIText(1.0f, 1, GetTranslation("profile_about"), UI::Text));

	if (!std::filesystem::exists(Game::GamePath + "/Northstar.dll"))
	{
		ProfileInfoBox->AddChild((new UIText(0.6f, 1, "Northstar is not installed", UI::Text))
			->SetPadding(0, 0.005, 0.02, 0.01));
	}
	else
	{
		std::string DLLPath = CurrentProfile.Path + "/Northstar.dll";

		if (!std::filesystem::exists(DLLPath))
		{
			DLLPath = Game::GamePath + "/Northstar.dll";
		}
		auto p = std::filesystem::current_path();
		std::filesystem::current_path(Game::GamePath + "/..");
		std::vector<std::string> AboutStrings =
		{
			GetTranslation("profile_dll_used") + std::filesystem::relative(DLLPath).u8string()
		};

		if (CurrentProfile.DisplayName == "R2Northstar")
		{
			AboutStrings.push_back(GetTranslation("profile_default"));
		}

		if (Thunderstore::VanillaPlusInstalled())
		{
			AboutStrings.push_back(GetTranslation("profile_vanillaplus"));
		}

		std::filesystem::current_path(p);

		for (auto& i : AboutStrings)
		{
			ProfileInfoBox->AddChild((new UIText(0.6f, 1, i, UI::Text))
				->SetPadding(0, 0.005, 0.02, 0.01));
		}
	}

	if (CurrentProfile.DisplayName != "R2Northstar")
	{
		auto OptionsBox = (new UIBox(true, 0));
		ProfileInfoBox->AddChild(OptionsBox);
		if (!Thunderstore::VanillaPlusInstalled())
		{
			
			OptionsBox->AddChild((new UIButton(true, 0, Installer::GetThemeColor(), []()
				{
					new BackgroundTask([]()
						{
							if (Game::RequiresUpdate)
							{
								Game::UpdateGame();
							}
							BackgroundTask::SetStatus("Updating profile...");
							BackgroundTask::SetProgress(0.8);
							UpdateProfile(CurrentProfile);
						},
						[]()
						{
							SettingsTab::CurrentSettingsTab->GenerateSettings();
						});
				}))
				->SetPadding(0.1, 0, 0.01, 0.01)
				->AddChild((new UIBackground(true, 0, 0, 0.045))
					->SetUseTexture(true, Icon("Settings/Reload").TextureID)
					->SetPadding(0.01, 0.01, 0.01, 0)
					->SetSizeMode(UIBox::SizeMode::AspectRelative))
				->SetBorder(UIBox::BorderType::Rounded, 0.5)
				->AddChild((new UIText(0.5f, 0, "Update profile", UI::Text))
					->SetPadding(0.015)));
		}
		OptionsBox->AddChild((new UIButton(true, 0, Vector3f(1, 0.5, 0), []()
			{
				if (WindowFunc::ShowPopupQuestion(GetTranslation("profile_delete"),
					Format(GetTranslation("profile_delete_message"), CurrentProfile.DisplayName.c_str()))
					 != WindowFunc::PopupReply::Yes)
				{ 
					return;
				}
				try
				{
					std::filesystem::remove_all(CurrentProfile.Path);
					CurrentProfile = Profile();
					DetectProfiles();
					OnProfileSwitched();
				}
				catch (std::exception& e)
				{
					WindowFunc::ShowPopupError(e.what());
				}
			}))
			->SetPadding(0.1f, 0, 0.01f, 0.01f)
			->SetBorder(UIBox::BorderType::Rounded, 0.5f)
			->AddChild((new UIBackground(true, 0, 0, 0.045f))
				->SetUseTexture(true, Icon("Delete").TextureID)
				->SetPadding(0.01f, 0.01f, 0.01f, 0)
				->SetSizeMode(UIBox::SizeMode::AspectRelative))
			->AddChild((new UIText(0.5f, 0, GetTranslation("profile_delete"), UI::Text))
				->SetPadding(0.015)));
	}
}

void ProfileTab::UpdateProfilesList()
{
	ProfileList->DeleteChildren();
	for (size_t i = 0; i < AllProfiles.size(); i++)
	{

		ProfileList->AddChild(Installer::SetButtonColorIfSelected((UIButton*)
			((new UIButton(true, 0, Installer::GetThemeColor(),
				[](int Index)
				{
					CurrentProfile = AllProfiles[Index];
					OnProfileSwitched();
				},
				i))
				->SetMinSize(Vector2f(0.8, 0))
				->SetPadding(0.005)
				->SetBorder(UIBox::BorderType::Rounded, 0.3f)
				->AddChild(new UIText(0.6f, 0, AllProfiles[i].DisplayName, UI::MonoText))),
			CurrentProfile.Path == AllProfiles[i].Path));
	}
}

void ProfileTab::OnClicked()
{
	UpdateProfilesList();
	DisplayProfileInfo();
	GenerateProfileCreationBox();
}

void ProfileTab::GenerateProfileCreationBox()
{
	NewProfileTextField = new UITextField(0, 0, UI::Text, nullptr);

	ProfileCreationBox->DeleteChildren();
	ProfileCreationBox->SetOpacity(0.5)
		->AddChild(new UIText(0.8f, 1.0f, GetTranslation("profile_create_new"), UI::Text))
		->AddChild((new UIBox(true, 0))
			->SetVerticalAlign(UIBox::Align::Centered)
			->AddChild(NewProfileTextField
				->SetTextSize(0.6f)
				->SetHintText(GetTranslation("profile_create_name"))
				->SetPadding(0.01f, 0.01f, 0.01f, 0.01f)
				->SetMinSize(Vector2f(0.5f, 0.055f)))
			->AddChild((new UIButton(true, 0, Installer::GetThemeColor(), []()
				{
					CurrentProfileTab->CreateNewProfile(CurrentProfileTab->NewProfileTextField->GetText());
				}))
				->AddChild((new UIBackground(true, 0, 0, 0.06))
					->SetUseTexture(true, Icon("Add").TextureID)
					->SetPadding(0.01f, 0.01f, 0.01f, 0)
					->SetSizeMode(UIBox::SizeMode::AspectRelative))
				->SetMinSize(Vector2f(0, 0.055f))
				->SetBorder(UIBox::BorderType::Rounded, 0.5f)
				->AddChild((new UIText(0.6f, 0, GetTranslation("profile_create"), UI::Text))
					->SetPadding(0.02))));
}

void ProfileTab::CreateNewProfile(std::string Name)
{
	if (Name.empty())
	{
		WindowFunc::ShowPopupError(GetTranslation("profile_create_no_name_entered"));
		return;
	}

	if (!std::filesystem::exists(Game::GamePath))
	{
		return;
	}
	if (std::filesystem::exists(Game::GamePath + "/" + Name))
	{
		WindowFunc::ShowPopupError(Format(GetTranslation("profile_create_already_exists"), Name.c_str()));
		return;
	}

	std::filesystem::create_directories(Game::GamePath + "/" + Name + "/mods");

	for (auto& i : Game::CoreModNames)
	{
		if (std::filesystem::exists(Game::GamePath + "/R2Northstar/mods/" + i))
		{
			std::filesystem::copy(
				Game::GamePath
				+ "/R2Northstar/mods/"
				+ i,
				
				Game::GamePath 
				+ Name
				+ "/mods/"
				+ i, 
				
				std::filesystem::copy_options::recursive);
		}
		else if (std::filesystem::exists(CurrentProfile.Path + "/mods/" + i))
		{
			std::filesystem::copy(
				CurrentProfile.Path 
				+ "/mods/"
				+ i,
				
				Game::GamePath
				+ Name
				+ "/mods/"
				+ i, 
				
				std::filesystem::copy_options::recursive);
		}
	}
	
	DetectProfiles();
	UpdateProfilesList();
	DisplayProfileInfo();
}

void ProfileTab::SaveSelectedProfile()
{
#ifndef TF_PLUGIN
	std::ofstream out = std::ofstream(Installer::CurrentPath + "Data/var/SelectedProfile.txt");
	out << CurrentProfile.DisplayName << std::endl << CurrentProfile.Path << std::endl;
	out.close();
#endif
}

void ProfileTab::UpdateProfile(Profile Target, bool Silent)
{
	Log::Print("Updating profile mods: " + Target.DisplayName);

	if (!std::filesystem::exists(Target.Path + "/mods/"))
	{
		return;
	}

	for (auto& mod : std::filesystem::directory_iterator(Target.Path + "/mods/"))
	{
		std::string ModString = mod.path().filename().u8string();
		if (Game::CoreModNames.contains(ModString))
		{
			std::filesystem::remove_all(mod);
		}
	}


	for (auto& m : Game::CoreModNames)
	{
		if (std::filesystem::exists(Game::GamePath + "/R2Northstar/mods/" + m))
		{
			std::filesystem::copy(
				Game::GamePath
				+ "/R2Northstar/mods/"
				+ m,

				Target.Path
				+ "/mods/"
				+ m,

				std::filesystem::copy_options::recursive);
		}
	}
	if (!Silent)
	{
		WindowFunc::ShowPopup(GetTranslation("app_name"),
			Format(GetTranslation("profile_update_complete"), CurrentProfile.DisplayName.c_str()));
	}
}
