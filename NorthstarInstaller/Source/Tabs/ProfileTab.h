#pragma once
#include "UITab.h"
#include <KlemmUI/UI/UIBackground.h>
#include <KlemmUI/UI/UIScrollBox.h>
#include <KlemmUI/UI/UITextField.h>

class ProfileTab : public UITab
{
	UIBackground* ProfileBackground;
	UIScrollBox* ProfileList;
	UIBox* ProfileInfoBox;
	UITextField* NewProfileTextField;
public:
	ProfileTab();

	struct Profile
	{
		std::string DisplayName = "";
		std::string Path = "";
		
		bool operator==(Profile b)
		{
			return DisplayName == b.DisplayName && Path == b.Path;
		}
	};

	static ProfileTab* CurrentProfileTab;

	static bool IsFolderValidProfilePath(std::string FolderPath);

	static void OnProfileSwitched();
	static void DetectProfiles();

	void DisplayProfileInfo();
	void UpdateProfilesList();

	void CreateNewProfile(std::string Name);
	void SaveSelectedProfile();

	static std::vector<Profile> AllProfiles;

	static Profile CurrentProfile;
};