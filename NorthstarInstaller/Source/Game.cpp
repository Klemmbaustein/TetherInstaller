#include "Game.h"
#include "JSON/json.hpp"
#include <iostream>
#include "Log.h"
#include <fstream>
#include <sstream>
#include "Installer.h"
#include "Networking.h"

std::string Game::GamePath;
bool Game::RequiresUpdate = false;
const std::string GameDirTxtPath = "Data/var/GameDir.txt";


std::string CommonTitanfallLocations[] =
{
	"C:/Program Files (x86)/Steam/steamapps/common/Titanfall2",
	"C:/Program Files (x86)/Origin Games/Titanfall2",
	// "D:/EA/Titanfall2", // My Titanfall 2 path for testing
};

std::string Game::GetTitanfallLocation()
{
	if (std::filesystem::exists(GameDirTxtPath))
	{
		std::ifstream in = std::ifstream(GameDirTxtPath);
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
		if (!std::filesystem::exists("Data/var"))
		{
			std::filesystem::create_directories("Data/var");
		}
		Log::Print("Saving path '" + FoundGameDir + "' to '" + GameDirTxtPath + "'");
		std::ofstream out = std::ofstream(GameDirTxtPath, std::ios::out);
		out.exceptions(std::ios::failbit);
		out << FoundGameDir;
		out.close();
	}
	catch (std::exception& e)
	{
		Log::Print("Could not save path '" + FoundGameDir + "' to '" + GameDirTxtPath + "': " + std::string(e.what()),  Log::Error);
	}
}

bool Game::IsValidTitanfallLocation(std::filesystem::path p)
{
	return std::filesystem::exists(p.string() + "/Titanfall2.exe") && std::filesystem::exists(p.string() + "/gameversion.txt");
}

std::string Game::GetCurrentVersion()
{
	std::ifstream in = std::ifstream("Data/var/version.txt");
	std::stringstream insstream;
	insstream << in.rdbuf();
	in.close();
	return insstream.str();
}

void Game::SetCurrentVersion(std::string ver)
{
	if (!std::filesystem::exists("Data/var"))
	{
		std::filesystem::create_directories("Data/var");
	}
	Log::Print("Saving game version to 'Data/var/version.txt'");
	std::ofstream out = std::ofstream("Data/var/version.txt", std::ios::out);
	out.exceptions(std::ios::failbit);
	out << ver;
	out.close();
}

void Game::UpdateGame()
{
	try
	{
		Installer::BackgroundName = "Updating Northstar";
		Installer::BackgroundTask = "Downloading Northstar";
		Installer::ThreadProgress = 0.5;
		auto result = Networking::DownloadLatestReleaseOf("R2Northstar/Northstar");
		Installer::ThreadProgress = 0.7;
		Installer::BackgroundTask = "Installing Northstar from " + result;
		Log::Print("Extracting zip: " + result);
		Networking::ExtractZip(result, Game::GamePath);
		Installer::BackgroundTask = "Removing temporary files";
		Installer::ThreadProgress = 0.9;
		std::filesystem::remove_all("temp/net");
		Log::Print("Removed temporary files");
		Game::SetCurrentVersion(Networking::GetLatestReleaseOf("R2Northstar/Northstar"));
		Game::RequiresUpdate = false;
		Installer::ThreadProgress = 1;
	}
	catch (std::exception& e)
	{
		Installer::BackgroundTask = e.what();
		Installer::ThreadProgress = 1;
		Log::Print(e.what(), Log::Error);
	}
}

void Game::UpdateGameAsync()
{
	if (!Installer::CurrentBackgroundThread)
	{
		Installer::CurrentBackgroundThread = new std::thread(UpdateGame);
	}
}