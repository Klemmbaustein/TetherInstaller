#pragma once
#include <string>
#include <filesystem>

namespace Game
{
	std::string GetTitanfallLocation();
	void SaveGameDir(std::string FoundGameDir);
	bool IsValidTitanfallLocation(std::filesystem::path p);
	
	std::string GetCurrentVersion();
	void SetCurrentVersion(std::string ver);

	void UpdateGame();
	void UpdateGameAsync();

	std::string GetLaunchArgs();
	void SetLaunchArgs(std::string NewArgs);

	extern bool RequiresUpdate;
	extern std::string GamePath;
}