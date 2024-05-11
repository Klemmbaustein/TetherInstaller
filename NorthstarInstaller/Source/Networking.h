#pragma once
#include <string>

namespace Networking
{
	// Gets the latest release of the given github repo.
	// RepoName = Owner/Name. Example: "R2Northstar/Northstar"
	std::string GetLatestReleaseOf(std::string RepoName);

	// Downloads the latest release of the given github repo.
	// RepoName = Owner/Name. Example: "R2Northstar/Northstar"
	// Puts the response into Data/temp/net/latest.zip
	std::string DownloadLatestReleaseOf(std::string RepoNam, std::string NecessaryAssetNamee = "");

	void Download(std::string url, std::string target, std::string UserAgent, bool IsDownload = false);
	std::string DownloadString(std::string Url, std::string UserAgent);

	bool IsProcessRunning(std::string Name);

	void Init();

	void Cleanup();

	void ExtractZip(std::string File, std::string TargetFolder);
}