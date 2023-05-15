#pragma once
#include <string>

namespace Networking
{
	// Gets the latest release of the given github repo.
	// RepoName = Owner/Name. Example: "R2Northstar/Northstar"
	std::string GetLatestReleaseOf(std::string RepoName);

	// Downloads the latest release of the given github repo.
	// RepoName = Owner/Name. Example: "R2Northstar/Northstar"
	std::string DownloadLatestReleaseOf(std::string RepoName);


	void Init();

	void Cleanup();
}