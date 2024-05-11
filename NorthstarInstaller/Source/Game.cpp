#include "Game.h"
#include "nlohmann/json.hpp"
#include <iostream>
#include "Log.h"
#include <fstream>
#include <sstream>
#include "Networking.h"
#include "BackgroundTask.h"
#include "Tabs/SettingsTab.h"
#include "Linux_PE.h"
#include "Translation.h"
#include "WindowFunctions.h"
#include "Tabs/ProfileTab.h"
#include "Installer.h"
using namespace Translation;

std::string Game::GamePath;
bool Game::RequiresUpdate = false;
const std::string GameDirTxtPath = "Data/var/GameDir.txt";

std::set<std::string> Game::CoreModNames =
{
	"Northstar.CustomServers",
	"Northstar.Custom",
	"Northstar.Client",
	"Northstar.Coop",
};

std::string CommonTitanfallLocations[] =
{
	"C:/Program Files (x86)/Steam/steamapps/common/Titanfall2",
	"C:/Program Files (x86)/Origin Games/Titanfall2",
};

#if _WIN32

#define NOMINMAX
#include <Windows.h>
#include "Tabs/SettingsTab.h"


std::string wstrtostr(const std::wstring& wstr);

LONG GetStringRegKey(HKEY hKey, const std::wstring& strValueName, std::wstring& strValue, const std::wstring& strDefaultValue)
{
	strValue = strDefaultValue;
	WCHAR szBuffer[512] = {};
	DWORD dwBufferSize = sizeof(szBuffer);
	ULONG nError;
	nError = RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
	if (ERROR_SUCCESS == nError)
	{
		strValue = szBuffer;
	}
	return nError;
}

#endif

std::string Game::GetTitanfallLocation()
{
#if TF_PLUGIN
	return std::filesystem::current_path().u8string();
#endif

	if (std::filesystem::exists(Installer::CurrentPath + GameDirTxtPath))
	{
		std::ifstream in = std::ifstream(Installer::CurrentPath + GameDirTxtPath);
		std::stringstream insstream;
		insstream << in.rdbuf();
		in.close();
		std::string str = insstream.str();
		if (IsValidTitanfallLocation(str))
		{
			Log::Print("Found game dir from '" + GameDirTxtPath + "': " + str);
			return str;
		}
		Log::Print("'" + GameDirTxtPath + "' does not contain a valid Titanfall 2 install location!",  Log::Warning);
	}
	Log::Print("Could not find game dir from '" + GameDirTxtPath + "'");

#if _WIN32

	HKEY RegKey;
	LONG lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Respawn\\Titanfall2", 0, KEY_READ, &RegKey);
	if (lRes == ERROR_SUCCESS)
	{
		std::wstring GameDir, GameDirDefault;
		LONG KeyResult = GetStringRegKey(RegKey, L"Install Dir", GameDir, GameDirDefault);
		if (KeyResult == ERROR_SUCCESS && IsValidTitanfallLocation(GameDir))
		{
			std::string UTF8GameDir = wstrtostr(GameDir);
			Log::Print("Found game dir through Windows registry: " + UTF8GameDir);
			SaveGameDir(UTF8GameDir);
			return UTF8GameDir;
		}
	}
	else
	{
		Log::Print("Cannot open reg key 'SOFTWARE\\Respawn\\Titanfall2'");
	}
#endif

	for (auto& i : CommonTitanfallLocations)
	{
		if (IsValidTitanfallLocation(i))
		{
			Log::Print("Found game dir: " + i);
			SaveGameDir(i);
			return i;
		}
	}

	Log::Print("Could not find game dir",  Log::Warning);

	return "";
}

// Super lazy way of saving/loading game dir.
void Game::SaveGameDir(std::string FoundGameDir)
{
	try
	{
		if (!std::filesystem::exists(Installer::CurrentPath + "Data/var"))
		{
			std::filesystem::create_directories(Installer::CurrentPath + "Data/var");
		}
		Log::Print("Saving path '" + FoundGameDir + "' to '" + Installer::CurrentPath + GameDirTxtPath + "'");
		std::ofstream out = std::ofstream(Installer::CurrentPath + GameDirTxtPath, std::ios::out);
		out.exceptions(std::ios::failbit);
		out << FoundGameDir;
		out.close();
	}
	catch (std::exception& e)
	{
		Log::Print("Could not save path '" + FoundGameDir + "' to '" + Installer::CurrentPath + GameDirTxtPath + "': " + std::string(e.what()),  Log::Error);
	}
}

bool Game::IsValidTitanfallLocation(std::filesystem::path p)
{
	return std::filesystem::exists(p.u8string() + "/Titanfall2.exe") && std::filesystem::exists(p.u8string() + "/gameversion.txt");
}

#if _WIN32
int Game::GetFileVersion(const char* filename, char* ver)
{
	DWORD dwHandle = 0, sz = GetFileVersionInfoSizeA(filename, &dwHandle);
	if (0 == sz)
	{
		return 1;
	}
	char* buf = new char[sz];
	if (!GetFileVersionInfoA(filename, dwHandle, sz, &buf[0]))
	{
		delete[] buf;
		return 2;
	}
	VS_FIXEDFILEINFO* pvi = nullptr;
	sz = sizeof(VS_FIXEDFILEINFO);
	if (!VerQueryValueA(&buf[0], "\\", (LPVOID*)&pvi, (unsigned int*)&sz))
	{
		delete[] buf;
		return 3;
	}
	sprintf(ver, "v%d.%d.%d",
		pvi->dwProductVersionMS >> 16, 
		pvi->dwFileVersionMS & 0xFFFF,
		pvi->dwFileVersionLS >> 16
	);
	delete[] buf;
	return 0;
}


#endif

std::string Game::GetCurrentVersion()
{
	if (GamePath.empty())
	{
		return "None";
	}
#if _WIN32
	char Ver[100];
	GetFileVersion(std::filesystem::path(GamePath + "/NorthstarLauncher.exe").u8string().c_str(), Ver);
	return Ver;
#endif
#if __linux__
	return getV(std::filesystem::path(GamePath + "/NorthstarLauncher.exe").u8string().c_str());
#endif
	return "Unknown";
}

void Game::UpdateGame()
{
	try
	{
		BackgroundTask::SetStatus("dl_" + GetTranslation("download_northstar"));
		auto result = Networking::DownloadLatestReleaseOf("R2Northstar/Northstar");
		BackgroundTask::SetProgress(0.9);
		Log::Print("Extracting zip: " + result);

		if (std::filesystem::exists(Game::GamePath + "/R2Northstar/mods"))
		{
			// Remove core mods before installing them again
			for (const auto& i : std::filesystem::directory_iterator(Game::GamePath + "/R2Northstar/mods"))
			{
				std::string File = i.path().filename().u8string();
				std::string Author = File.substr(0, File.find_first_of("."));
				if (Author == "Northstar")
				{
					std::filesystem::remove(File);
				}
			}
		}
		Networking::ExtractZip(result, Game::GamePath);
		Game::RequiresUpdate = false;
		ProfileTab::DetectProfiles();
	}
	catch (std::exception& e)
	{
		WindowFunc::ShowPopupError(Format("Error updating game: %s", e.what()));
	}
}

void Game::UpdateGameAsync()
{
	new BackgroundTask(UpdateGame, []() {SettingsTab::CurrentSettingsTab->GenerateSettings(); });
}

std::string Game::GetLaunchArgs()
{
	if (!std::filesystem::exists("Data/var/launch_args.txt"))
	{
		return "-novid -multiple";
	}
	std::ifstream LaunchArgStream = std::ifstream(Installer::CurrentPath + "Data/var/launch_args.txt");
	std::stringstream s;
	s << LaunchArgStream.rdbuf();
	LaunchArgStream.close();
	return s.str();
}

void Game::SetLaunchArgs(std::string NewArgs)
{
	std::ofstream LaunchArgStream = std::ofstream(Installer::CurrentPath + "Data/var/launch_args.txt");
	LaunchArgStream << NewArgs;
	LaunchArgStream.close();
}
