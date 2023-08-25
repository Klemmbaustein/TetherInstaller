#include "ProfileTab.h"
#include "ModsTab.h"

#include <KlemmUI/UI/UIText.h>
#include <KlemmUI/UI/UIButton.h>

#include <fstream>
#include <filesystem>

#include "../Game.h"
#include "../UIDef.h"
#include "../Log.h"
#include "../Installer.h"
#include "../BackgroundTask.h"
#include "../Thunderstore.h"
#include "../WindowFunctions.h"

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
	Name = "Profiles";

	Background->BoxAlign = UIBox::Align::Centered;
	Background->SetHorizontal(true);

	ProfileBackground = new UIBackground(false, 0, 0, Vector2f(1.5, 1.85));
	Background->AddChild(ProfileBackground
		->SetOpacity(0.6)
		->AddChild((new UIBackground(true, 0, 1, Vector2f(1.5, 0.005)))
			->SetPadding(0))
		->SetPadding(0));
	ProfileBackground->BoxAlign = UIBox::Align::Reverse;
	ProfileBackground->AddChild(new UIText(0.8, 1, "Profiles", UI::Text));

	ProfileList = new UIScrollBox(false, 0, true);
	ProfileList->BoxAlign = UIBox::Align::Reverse;

	ProfileInfoBox = new UIBox(false, 0);
	ProfileInfoBox->BoxAlign = UIBox::Align::Reverse;

	ProfileBackground->AddChild((new UIBox(true, 0))
		->AddChild(ProfileList
			->SetMinSize(Vector2f(0.85, 1.4)))
		->AddChild(ProfileInfoBox
			->SetMinSize(Vector2f(0, 1.4))));

	NewProfileTextField = new UITextField(true, 0, 0, UI::Text, nullptr);

	ProfileBackground->AddChild((new UIBackground(false, 0, 0))
		->SetOpacity(0.5)
		->AddChild((new UIBox(true, 0))
			->AddChild(NewProfileTextField
				->SetTextSize(0.3)
				->SetHintText("New profile name")
				->SetPadding(0.01, 0.02, 0.01, 0.01)
				->SetMinSize(Vector2f(0.6, 0.055)))
			->AddChild((new UIButton(true, 0, Vector3f32(0, 0.5, 1), []()
				{
					CurrentProfileTab->CreateNewProfile(CurrentProfileTab->NewProfileTextField->GetText()); 
				}))
				->SetMinSize(Vector2f(0, 0.055))
				->SetBorder(UIBox::BorderType::Rounded, 0.5)
				->AddChild((new UIText(0.3, 0, "Create", UI::Text))
					->SetPadding(0.02))))
		->AddChild(new UIText(0.4, 1, "New profile", UI::Text)));
		
	CurrentProfileTab = this;

	DetectProfiles();
	
	if (std::filesystem::exists("Data/var/SelectedProfile.txt"))
	{
		Log::Print("Reading profile from SelectedProfile.txt");
		std::ifstream InProfile = std::ifstream("Data/var/SelectedProfile.txt");
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
	Installer::UpdateCheckedOnce = false;
	ModsTab::Reload();
	new BackgroundTask(ModsTab::CheckForModUpdates);
	CurrentProfileTab->UpdateProfilesList();
	CurrentProfileTab->DisplayProfileInfo();
	CurrentProfileTab->SaveSelectedProfile();
}

void ProfileTab::DetectProfiles()
{
	AllProfiles.clear();
	if (!std::filesystem::exists(Game::GamePath))
	{
		Log::Print("Search for northstar profiles failed - Game path does not exist.", Log::Warning);
		return;
	}

	Log::Print("-- Searching for Northstar profiles --");

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

			LOG_PRINTF("Found profile: {}", NewProfile.DisplayName);
			AllProfiles.push_back(NewProfile);
		}
	}
}

void ProfileTab::DisplayProfileInfo()
{
	ProfileInfoBox->DeleteChildren();

	ProfileInfoBox->AddChild((new UIText(0.35, 1, "Current profile:", UI::Text))
		->SetPadding(0, 0.01, 0.01, 0));

	ProfileInfoBox->AddChild((new UIBackground(true, 0, 1, Vector2f(0.575, 0.005)))
		->SetPadding(0.0, 0, 0, 0));
	ProfileInfoBox->AddChild(new UIText(0.75, 1, CurrentProfile.DisplayName, UI::Text));
	ProfileInfoBox->AddChild((new UIBackground(true, 0, 1, Vector2f(0.575, 0.005)))
		->SetPadding(0.0, 0.05, 0, 0));

	auto Mods = Thunderstore::GetInstalledMods().Combined();

	ProfileInfoBox->AddChild(new UIText(0.5, 1, "Mods: ", UI::Text));

	for (auto& i : Mods)
	{
		ColoredText ModString = { TextSegment("- " + i.Namespace + "." + i.Name, 1) };
		if (!Thunderstore::GetModEnabled(i))
		{
			ModString.push_back(TextSegment("  (disabled)", Vector3f32(1, 0.5f, 0)));
		}

		ProfileInfoBox->AddChild((new UIText(0.3, ModString, UI::Text))
			->SetPadding(0, 0.005, 0.02, 0.01));
	}

	if (Mods.empty())
	{
		ProfileInfoBox->AddChild((new UIText(0.3, 1, "No mods installed.", UI::Text))
			->SetPadding(0, 0.005, 0.02, 0.01));
	}

	ProfileInfoBox->AddChild(new UIText(0.5, 1, "About: ", UI::Text));

	if (!std::filesystem::exists(Game::GamePath + "/Northstar.dll"))
	{
		ProfileInfoBox->AddChild((new UIText(0.3, 1, "Northstar is not installed", UI::Text))
			->SetPadding(0, 0.005, 0.02, 0.01));
	}
	else
	{
		std::string DLLPath = CurrentProfile.Path + "/Northstar.dll";

		if (!std::filesystem::exists(DLLPath))
		{
			DLLPath = Game::GamePath + "/Northstar.dll";
		}

		char Ver[100];
		Game::GetFileVersion(DLLPath.c_str(), Ver);
		auto p = std::filesystem::current_path();
		std::filesystem::current_path(Game::GamePath + "/..");
		std::vector<std::string> AboutStrings =
		{
			"DLL file used: " + std::filesystem::relative(DLLPath).u8string()
		};

		if (CurrentProfile.DisplayName == "R2Northstar")
		{
			AboutStrings.push_back("This is the default profile.");
		}

		std::filesystem::current_path(p);

		for (auto& i : AboutStrings)
		{
			ProfileInfoBox->AddChild((new UIText(0.3, 1, i, UI::Text))
				->SetPadding(0, 0.005, 0.02, 0.01));
		}
	}

	if (CurrentProfile.DisplayName != "R2Northstar")
	{
		ProfileInfoBox->AddChild((new UIButton(true, 0, Vector3f32(1, 0.5, 0), []()
			{
				if (Window::ShowPopupQuestion("Delete profile",
				"Are you sure you want to delete the profile \"" + CurrentProfile.DisplayName + "\"?")
					 != Window::PopupReply::Yes)
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
					Window::ShowPopupError(e.what());
				}
			}))
			->SetPadding(0.1, 0, 0.01, 0.01)
			->SetBorder(UIBox::BorderType::Rounded, 0.5)
			->AddChild((new UIText(0.25, 0, "Delete profile", UI::Text))
				->SetPadding(0.015)));
	}
}

void ProfileTab::UpdateProfilesList()
{
	ProfileList->DeleteChildren();
	for (size_t i = 0; i < AllProfiles.size(); i++)
	{ 

		ProfileList->AddChild((new UIButton(true, 0, Installer::TabStyles[(int)(AllProfiles[i] == CurrentProfile)],
			[](int Index) 
			{
				CurrentProfile = AllProfiles[Index];
				OnProfileSwitched();
			},
			i))
			->SetMinSize(Vector2f(0.8, 0))
			->SetPadding(0.005)
			->AddChild(new UIText(0.3, 0, AllProfiles[i].DisplayName, UI::MonoText)));
	}
}

void ProfileTab::CreateNewProfile(std::string Name)
{
	if (Name.empty())
	{
		Window::ShowPopupError("Please enter a profile name");
		return;
	}

	if (!std::filesystem::exists(Game::GamePath))
	{
		return;
	}
	std::filesystem::create_directories(Game::GamePath + Name + "/mods");

	std::set<std::string> CoreModNames =
	{
		"Northstar.CustomServers",
		"Northstar.Custom",
		"Northstar.Client",
		"Northstar.Coop", // soooooon
	};
	for (auto& i : CoreModNames)
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
	std::ofstream out = std::ofstream("Data/var/SelectedProfile.txt");
	out << CurrentProfile.DisplayName << std::endl << CurrentProfile.Path << std::endl;
	out.close();
}
