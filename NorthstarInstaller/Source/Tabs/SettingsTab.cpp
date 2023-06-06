#include "SettingsTab.h"
#include <UI/UIText.h>
#include <UI/UIButton.h>

#include <thread>
#include <sstream>
#include <zip.h>
#include <fstream>
#include <stdlib.h>
#include <filesystem>

#include "../Log.h"
#include "../UIDef.h"
#include "../Game.h"
#include "../Installer.h"

#if _WIN32
#include <Shlobj.h>
#include <shobjidl.h> 

//https://www.cplusplus.com/forum/windows/74644/
std::string wstrtostr(const std::wstring& wstr)
{
	std::string strTo;
	char* szTo = new char[wstr.length() + 1];
	szTo[wstr.size()] = '\0';
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, szTo, (int)wstr.length(), NULL, NULL);
	strTo = szTo;
	delete[] szTo;
	return strTo;
}

std::string ShowSelectFolderDialog()
{
	std::string FilePath = "";
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
		COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog* pFileOpen;

		// Create the FileOpenDialog object.
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
			IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr))
		{
			// Show the Open dialog box.
			pFileOpen->SetOptions(FOS_PICKFOLDERS);
			hr = pFileOpen->Show(NULL);
			// Get the file name from the dialog box.
			if (SUCCEEDED(hr))
			{
				IShellItem* pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr))
				{
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
					// Display the file name to the user.
					if (SUCCEEDED(hr))
					{
						FilePath = wstrtostr(pszFilePath);
						return FilePath;
					}
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
		CoUninitialize();
	}
	return FilePath;
}

#endif

SettingsTab* CurrentSettingsTab = nullptr;

SettingsTab::SettingsTab()
{
	CurrentSettingsTab = this;
	Name = "Settings";

	Background->Align = UIBox::E_CENTERED;
	Background->SetHorizontal(true);

	SettingsBackground = new UIBackground(false, 0, 0, Vector2f(0.8, 1.85));
	Background->AddChild(SettingsBackground
		->SetOpacity(0.3)
		->AddChild((new UIBackground(true, 0, 1, Vector2f(0.8, 0.005)))
			->SetPadding(0))
		->SetPadding(0));
	SettingsBackground->Align = UIBox::E_REVERSE;
	SettingsBackground->AddChild(new UIText(0.8, 1, "Settings", UI::Text));
	SettingsBox = new UIScrollBox(false, 0, 10);
	SettingsBox->Align = UIBox::E_REVERSE;
	SettingsBackground->AddChild(SettingsBox);
	GenerateSettings();
}

void DeleteAllMods()
{
	// Do not uninstall the core mods, that's a very bad idea.
	std::set<std::string> CoreModNames =
	{
		"Northstar.CustomServers",
		"Northstar.Custom",
		"Northstar.Client"
	};

	for (const auto& m : std::filesystem::directory_iterator(Game::GamePath + "/R2Northstar/mods/"))
	{
		if (CoreModNames.find(m.path().filename().string()) == CoreModNames.end() && std::filesystem::is_directory(m))
		{
			std::filesystem::remove_all(m);
			Log::Print("Removing mod: " + m.path().filename().string(), Log::General, Log::Warning);
		}
	}
	std::filesystem::remove_all("Data/var/modinfo");
}

void LocateTitanfall()
{
	std::string NewPath = ShowSelectFolderDialog();
	if (Game::IsValidTitanfallLocation(NewPath))
	{
		Game::SaveGameDir(NewPath);
		Game::GamePath = Game::GetTitanfallLocation();
	}
	CurrentSettingsTab->GenerateSettings();
}

constexpr uint16_t MAX_GAMEPATH_SIZE = 30;

void SettingsTab::GenerateSettings()
{
	SettingsBox->DeleteChildren();

	std::string ShortGamePath = Game::GamePath;
	if (ShortGamePath.size() > MAX_GAMEPATH_SIZE)
	{
		ShortGamePath = ShortGamePath.substr(0, MAX_GAMEPATH_SIZE - 3) + "...";
	}

	SettingsBox->AddChild((new UIButton(true, 0, 1, LocateTitanfall))
		->AddChild(new UIText(0.4, 0, Game::GamePath.empty() ? "Locate Titanfall 2 (No path!)" : "Locate Titanfall (" + ShortGamePath + ")", UI::Text)));

	SettingsBox->AddChild((new UIButton(true, 0, 1, []() {
		if (!Installer::CurrentBackgroundThread)
		{
			Installer::CurrentBackgroundThread = new std::thread(Installer::CheckForUpdates);
		}
		}))
		->AddChild(new UIText(0.4, 0, "Re-check for updates", UI::Text)));

	SettingsBox->AddChild((new UIButton(true, 0, 1, Game::UpdateGameAsync))
		->AddChild(new UIText(0.4, 0, "Force reinstall Northstar", UI::Text)));

	SettingsBox->AddChild((new UIButton(true, 0, 1, DeleteAllMods))
		->AddChild(new UIText(0.4, 0, "Delete all mods", UI::Text)));

	SettingsBox->AddChild((new UIButton(true, 0, 1, []() {
		if (!Installer::CurrentBackgroundThread)
		{
			Log::Print("Un-fucking installation...", Log::General, Log::Warning);
			std::filesystem::remove_all("temp");
			Log::Print("Deleted ./temp/", Log::General, Log::Warning);
			DeleteAllMods();
			Game::UpdateGameAsync();
		}
		}))
		->AddChild(new UIText(0.4, 0, "Try to unfuck installation", UI::Text)));
}
