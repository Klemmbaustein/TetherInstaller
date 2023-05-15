#include "Game.h"
#include "JSON/json.hpp"
#include <iostream>
#include "Log.h"
#include <fstream>
#include <sstream>
#include "Installer.h"
#include "Networking.h"
#include <zip.h>

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
		Log::Print("'" + GameDirTxtPath + "' does not contain a valid Titanfall 2 install location!", Log::General, Log::Warning);
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

	Log::Print("Could not find game dir", Log::General, Log::Warning);

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
		Log::Print("Could not save path '" + FoundGameDir + "' to '" + GameDirTxtPath + "': " + std::string(e.what()), Log::General, Log::Error);
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

void ExtractZip(std::string Zip, std::string Target)
{
	int err = 0;
	zip* z = zip_open(Zip.c_str(), 0, &err);
	std::string TargetDir = Target + "/";

	struct zip_stat* finfo = NULL;
	finfo = (struct zip_stat*)calloc(256, sizeof(int));
	zip_stat_init(finfo);
	zip_file_t* fd = NULL;
	char* txt = NULL;
	int count = 0;
	while ((zip_stat_index(z, count, 0, finfo)) == 0) {

		// allocate room for the entire file contents
		txt = (char*)calloc(finfo->size + 1, sizeof(char));
		fd = zip_fopen_index(
			z, count, 0); // opens file at count index
		// reads from fd finfo->size
		// bytes into txt buffer
		zip_fread(fd, txt, finfo->size);

		size_t slash = std::string(finfo->name).find_last_of("/\\");
		if (slash != std::string::npos)
		{
			std::filesystem::create_directories(TargetDir + std::string(finfo->name).substr(0, slash));
		}
		std::ofstream(TargetDir + std::string(finfo->name), std::ios::out | std::ios::binary).write(txt, finfo->size);

		// frees allocated buffer, will
		// reallocate on next iteration of loop
		free(txt);
		zip_fclose(fd);
		// increase index by 1 and the loop will
		// stop when files are not found
		count++;
	}
	zip_close(z);
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
		ExtractZip(result, Game::GamePath);
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
		Log::Print(e.what(), Log::Install, Log::Error);
	}
}

void Game::UpdateGameAsync()
{
	if (!Installer::CurrentBackgroundThread)
	{
		Installer::CurrentBackgroundThread = new std::thread(UpdateGame);
	}
}