#pragma once
#include <string>
#include <filesystem>
#include <set>

namespace Game
{
	extern std::set<std::string> CoreModNames;
	
	std::string GetTitanfallLocation();
	void SaveGameDir(std::string FoundGameDir);
	bool IsValidTitanfallLocation(std::filesystem::path p);
	
	std::string GetCurrentVersion();

	void UpdateGame();
	void UpdateGameAsync();
	int GetFileVersion(const char* filename, char* ver);

	std::string GetLaunchArgs();
	void SetLaunchArgs(std::string NewArgs);

	extern bool RequiresUpdate;
	extern std::string GamePath;
}