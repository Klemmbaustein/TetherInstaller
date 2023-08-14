#pragma once
#include <atomic>
#include <string>
#include <vector>

namespace Thunderstore
{
	// Describes a mod on thunderstore.
	struct Package
	{
		std::string Name;
		std::string Description;
		std::string Author;
		std::string Img;
		std::string Namespace;
		std::string DownloadUrl;
		std::string Version;
		std::string UUID;
		std::string PageUrl;
		std::string FileVersion;
		size_t Downloads = 0;
		size_t Rating = 0;
		bool IsDeprecated = false;
		bool IsUnknownLocalMod = false;
		bool IsNSFW = false;
		bool IsTemporary = false;
		bool IsPackage = false;
	};

	bool IsMostRecentFileVersion(std::string VersionString);

	enum class Ordering
	{
		Newest,
		Last_Updated,
		Most_Downloaded,
		Top_Rated,
		Installed
	};

	struct InstalledModsResult
	{
		std::vector<Package> Managed;
		std::vector<Package> Unmanaged;

		std::vector<Package> Combined() const
		{
			std::vector<Package> comb = Managed;
			for (auto& i : Unmanaged)
			{
				comb.push_back(i);
			}
			return comb;
		}
	};

	struct Category
	{
		std::string Name;
		Ordering o;
	};

	extern Ordering SelectedOrdering;
	extern std::atomic<bool> IsDownloading;
	extern std::atomic<bool> LoadedImages;
	extern std::atomic<bool> ShouldStopLoadingImages;
	extern std::vector<Package> FoundMods;
	extern std::atomic<bool> LoadedSelectedMod;
	extern Package SelectedMod;

	InstalledModsResult GetInstalledMods();

	// Tries to return if a given package is installed locally using a bunch of different checks.
	// Will not work if it's named differently.
	// TODO: Check UUID too. This should still return true if an installed mod has a different name but the same UUID.
	bool IsModInstalled(Package m);

	// Gets the thunderstore mod page with the given ordering, filter and page index.
	// The result will be put into 'FoundMods'
	void DownloadThunderstoreInfo(Ordering ModOrdering, size_t Page, std::string Filter, bool Async);

	// Downloads the given package into "Data/temp/net/{m.Author}.{m.Name}.zip,
	// extracts it's contents into "Data/temp/mod",
	// then extracts the content of the extracted zip file's "mods" Folder into the Titanfall 2
	// mods folder.
	// TODO: Extract other elements of the mod. (Plugins, ...)
	void InstallOrUninstallMod(Package m, bool IsTemporary, bool Async);



	// Returns the mod names of an installed mod package. Can be /mods or /packages
	std::vector<std::string> GetLocalModNames(Package m);

	// Returns all paths that are a mod from an installed package.
	// For example: GlitchOverhaul package -> [Titanfall2]/R2Northstar/packages/S2Mods-GlitchOverhaul-1.2.11/mods/*
	std::vector<std::string> GetLocalMods(Package m);

	namespace TSModFunc
	{
		void InstallOrUninstallMod();
	}

	void SetModEnabled(Package m, bool IsEnabled);
	bool GetModEnabled(Package m);

	// Sets the Thunderstore::SelectedMod variable to a more detailed version of the given package.
	void GetModInfo(Package m, bool Async);

	extern std::atomic<bool> IsInstallingMod;
	extern std::atomic<size_t> CurrentlyLoadedPageID;
}