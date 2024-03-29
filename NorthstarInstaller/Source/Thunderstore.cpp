#include "Thunderstore.h"
#include "Game.h"
#include "Log.h"

#include "nlohmann/json.hpp"

#include <filesystem>
#include <set>
#include <fstream>
#include "BackgroundTask.h"
#include "Networking.h"
#include "Tabs/ModsTab.h"
#include "WindowFunctions.h"
#include "Tabs/ProfileTab.h"
#include "Installer.h"
#include "TetherPlugin.h"

constexpr const char* MOD_DESCRIPTOR_FILE_FORMAT_VERSION = "v4";

Thunderstore::Ordering Thunderstore::SelectedOrdering = Ordering::Last_Updated;
std::atomic<bool> Thunderstore::IsDownloading = false;
std::atomic<bool> Thunderstore::LoadedImages = false;
std::atomic<bool> Thunderstore::ShouldStopLoadingImages = false;
std::atomic<bool> Thunderstore::LoadedSelectedMod = false;
std::atomic<bool> Thunderstore::IsInstallingMod = false;
std::vector<Thunderstore::Package> Thunderstore::FoundMods;
Thunderstore::Package Thunderstore::SelectedMod;
std::atomic<size_t> Thunderstore::CurrentlyLoadedPageID = 0;
static const std::string PackagePaths[2] = { "/packages/", "/runtime/remote/mods/" /* Called mods/, contains packages..? */ };

#define INSTALL_AS_PACKAGES 1

bool Thunderstore::IsMostRecentFileVersion(std::string VersionString)
{
	return VersionString == MOD_DESCRIPTOR_FILE_FORMAT_VERSION;
}

Thunderstore::InstalledModsResult Thunderstore::GetInstalledMods()
{
	using namespace nlohmann;

	if (!std::filesystem::exists(Game::GamePath))
	{
		return Thunderstore::InstalledModsResult();
	}

	// Mods managed by the installer
	std::set<std::string> ManagedMods;

	std::vector<Package> UnmanagedMods;
	std::vector<Package> InstalledMods;
	if (std::filesystem::exists(Installer::CurrentPath + "Data/var/modinfo/" + ProfileTab::CurrentProfile.DisplayName))
	{
		for (auto& i : std::filesystem::directory_iterator(Installer::CurrentPath + "Data/var/modinfo/" + ProfileTab::CurrentProfile.DisplayName))
		{
			auto pathstring = i.path().u8string();
			if (pathstring.substr(pathstring.find_last_of(".")) == ".png")
			{
				if (!std::filesystem::exists(pathstring.substr(0, pathstring.find_last_of(".")) + ".json"))
				{
					try
					{
						std::filesystem::remove(std::filesystem::absolute(i));
					}
					catch (std::exception& e)
					{
						WindowFunc::ShowPopupError(e.what());
					}
				}
				continue;
			}

			std::ifstream in = std::ifstream(i.path().u8string());
			std::stringstream str; str << in.rdbuf();
			in.close();

			if (str.str().empty())
			{
				continue;
			}

			auto modinfo = json::parse(str.str());
			Package p;

			try
			{
				p.Author = modinfo.at("author");
				p.Namespace = modinfo.at("namespace");
				p.Name = modinfo.at("name");
				p.Description = modinfo.at("description");
				p.Img = modinfo.at("image");
				p.Version = modinfo.at("version");
				p.UUID = modinfo.at("thunderstore_mod_identifier");
				p.FileVersion = modinfo.at("file_format_version");
				p.IsTemporary = modinfo.at("is_temporary");
				p.IsPackage = INSTALL_AS_PACKAGES;
			}
			catch (std::exception& e)
			{
				Log::Print("Error reading mod descriptor file. Possibly outdated version", Log::Warning);
				Log::Print(e.what(), Log::Warning);
				p.FileVersion = "Unknown";

			}

			bool AllFilesExist = true;
			for (auto& i : modinfo.at("mod_files"))
			{
				auto file = ProfileTab::CurrentProfile.Path + i.get<std::string>();
				if (std::filesystem::exists(file))
				{
					ManagedMods.insert(i);
				}
				else
				{
					AllFilesExist = false;
				}
			}
			if (AllFilesExist)
			{
				InstalledMods.push_back(p);
			}
			else
			{
				Log::Print("Missing or corrupted mod detected - Removing " + p.Name +" from modlist.", Log::Warning);
				InstallOrUninstallMod(p, false, false, false);
			}
		}
	}
	if (!std::filesystem::exists(ProfileTab::CurrentProfile.Path))
	{
		return InstalledModsResult();
	}
	if (!std::filesystem::exists(ProfileTab::CurrentProfile.Path + "/mods"))
	{
		std::filesystem::create_directories(ProfileTab::CurrentProfile.Path + "/mods");
	}
	if (!std::filesystem::exists(ProfileTab::CurrentProfile.Path + "/packages"))
	{
		std::filesystem::create_directories(ProfileTab::CurrentProfile.Path + "/packages");
	}
	for (auto& i : std::filesystem::directory_iterator(ProfileTab::CurrentProfile.Path + "/mods"))
	{
		std::string ModName = i.path().filename().u8string();
		std::string Author = ModName.substr(0, ModName.find_first_of("."));
		std::string Name = ModName.substr(ModName.find_first_of(".") + 1);

		if (!std::filesystem::exists(i.path().u8string() + "/mod.json"))
		{
			continue;
		}

		if (!ManagedMods.contains("/mods/" + ModName) && std::filesystem::is_directory(i) && Author != "Northstar" && Name != "autojoin")
		{
			Package p;
			p.Name = Name;
			p.Author = Author;
			p.Namespace = Author;
			p.Version = "?";
			p.IsPackage = false;
			p.IsUnknownLocalMod = true;
			p.DownloadUrl = i.path().u8string();
			UnmanagedMods.push_back(p);
		}
	}

	for (const auto& PackagePath : PackagePaths)
	{
		if (!std::filesystem::exists(ProfileTab::CurrentProfile.Path + PackagePath))
		{
			Log::Print("Path: " + ProfileTab::CurrentProfile.Path + PackagePath + " does not exist", Log::Warning);
			continue;
		}
		for (auto& i : std::filesystem::directory_iterator(ProfileTab::CurrentProfile.Path + PackagePath))
		{
			std::string ModName = i.path().filename().u8string();
			std::string Author = ModName.substr(0, ModName.find_first_of("-"));
			std::string Name = ModName.substr(ModName.find_first_of("-") + 1);
			std::string Version = Name.substr(Name.find_last_of("-") + 1);

			Name = Name.substr(0, Name.find_last_of("-"));

			if (!ManagedMods.contains(PackagePath + ModName) && std::filesystem::is_directory(i) && Author != "Northstar" && Name != "autojoin")
			{
				std::ifstream DescriptionMarkdown = std::ifstream(i.path().u8string() + "/README.md");
				std::stringstream MarkdownStream;
				MarkdownStream << DescriptionMarkdown.rdbuf();
				DescriptionMarkdown.close();
				Package p;
				p.Name = Name;
				p.Author = Author;
				p.Description = MarkdownStream.str();
				p.Namespace = Author;
				p.IsPackage = true;
				p.Version = Version;
				p.IsUnknownLocalMod = true;
				p.DownloadUrl = i.path().u8string();
				p.Img = i.path().u8string() + "/icon.png";
				UnmanagedMods.push_back(p);
			}
		}
	}

	return InstalledModsResult(InstalledMods, UnmanagedMods);
}


bool Thunderstore::IsModInstalled(Package m)
{
	if (!std::filesystem::exists(Game::GamePath))
	{
		return false;
	}
	// As a failsafe also check for mod files that could've been installed using another method.
	// This way we hopefully won't install a mod twice. (Oh no)
	// This won't always work.
	std::string ModInfoFile = Installer::CurrentPath + "Data/var/modinfo/" + ProfileTab::CurrentProfile.DisplayName + "/" + m.Namespace + "." + m.Name + ".json";
	if ((std::filesystem::exists(ModInfoFile) && !std::filesystem::is_empty(ModInfoFile))
		|| std::filesystem::exists(ProfileTab::CurrentProfile.Path + "/mods/" + m.Namespace + "." + m.Name))
	{
		return true;
	}
	if (std::filesystem::exists(ProfileTab::CurrentProfile.Path + "/mods"))
	{
		for (auto& i : std::filesystem::directory_iterator(ProfileTab::CurrentProfile.Path + "/mods"))
		{
			Package fm;
			std::string FileName = i.path().filename().u8string();

			if (FileName.find(".") != std::string::npos)
			{
				fm.Name = FileName.substr(FileName.find_first_of(".") + 1);
				fm.Namespace = FileName.substr(0, FileName.find_first_of("."));
				fm.Author = m.Namespace;
			}
			else
			{
				fm.Name = FileName;
			}
			if ((fm.Namespace == m.Namespace || m.Namespace.empty() || fm.Namespace.empty())
				&& (fm.Name == m.Name))
			{
				return true;
			}
		}
	}


	for (const auto& PackagePath : PackagePaths)
	{
		if (!std::filesystem::exists(ProfileTab::CurrentProfile.Path + PackagePath))
		{
			continue;
		}
		for (auto& i : std::filesystem::directory_iterator(ProfileTab::CurrentProfile.Path + PackagePath))
		{
			Package FoundMod;
			std::string FileName = i.path().filename().u8string();

			if (FileName.find("-") != std::string::npos)
			{
				FoundMod.Name = FileName.substr(FileName.find_first_of("-") + 1);
				FoundMod.Namespace = FileName.substr(0, FileName.find_first_of("-"));
				FoundMod.Author = m.Namespace;

				FoundMod.Version = FoundMod.Name.substr(FoundMod.Name.find_last_of("-") + 1);

				FoundMod.Name = FoundMod.Name.substr(0, FoundMod.Name.find_last_of("-"));

			}
			else
			{
				FoundMod.Name = FileName;
			}
			if ((FoundMod.Namespace == m.Namespace || m.Namespace.empty() || FoundMod.Namespace.empty())
				&& (FoundMod.Name == m.Name))
			{
				return true;
			}
		}
	}

	if (!std::filesystem::exists(Installer::CurrentPath + "Data/var/" + ProfileTab::CurrentProfile.DisplayName + "/modinfo"))
	{
		return false;
	}

	for (auto& i : std::filesystem::directory_iterator(Installer::CurrentPath + "Data/var/" + ProfileTab::CurrentProfile.DisplayName + "/modinfo"))
	{
		std::string Name = i.path().filename().u8string();
		std::string ModName = Name.substr(Name.find_first_of(".") + 1);
		if (ModName == m.Name + ".json")
		{
			return true;
		}
	}

	return false;
}
namespace Thunderstore::TSDownloadThunderstoreInfo
{
	Ordering ModOrdering;
	size_t Page;
	std::string Filter;
	void DownloadThunderstoreInfoInternal()
	{
		using namespace nlohmann;
		
		LOG_PRINTF("Loading page {} of ordering {} with filter '{}'", Page, (int)ModOrdering, Filter);

		size_t ModLoadID = ++CurrentlyLoadedPageID;
		ShouldStopLoadingImages = true;

		std::transform(Filter.begin(), Filter.end(), Filter.begin(),
			[](unsigned char c) { return std::tolower(c); });

		std::filesystem::create_directories(Installer::CurrentPath + "Data/temp/net");

		if (ModOrdering == Ordering::Installed)
		{
			IsDownloading = true;
			BackgroundTask::SetStatus("Loading installed mods");
			BackgroundTask::SetProgress(0.5);
			try
			{
				FoundMods = GetInstalledMods().Combined();
			}
			catch (std::exception& e)
			{
				FoundMods.clear();
				Log::Print("Error parsing installed mods: " + std::string(e.what()), Log::Error);
			}
			IsDownloading = false;
			return;
		}

		try
		{
			if (std::filesystem::exists(Installer::CurrentPath + "Data/temp/net/ts"))
			{
				// Get size of already loaded image folders.
				size_t size = 0;
				for (std::filesystem::recursive_directory_iterator it(Installer::CurrentPath + "Data/temp/net/ts");
					it != std::filesystem::recursive_directory_iterator();
					++it)
				{
					if (!std::filesystem::is_directory(*it))
						size += std::filesystem::file_size(*it);
				}

				// If the already loaded images are larger than 10mb, delete them.
				if (size >= 20ull * 1000ull * 1000ull)
				{
					std::filesystem::remove_all(Installer::CurrentPath + "Data/temp/net/ts");
				}
			}
		}
		catch (std::exception& e)
		{
			Log::Print(e.what(), Log::Info);
			throw e;
		}

		IsDownloading = true;
		BackgroundTask::SetStatus("Loading Thunderstore");
		BackgroundTask::SetProgress(0.1);
		Networking::Download("https://thunderstore.io/c/northstar/api/v1/package/", Installer::CurrentPath + "Data/temp/net/tspage.txt", "");

		BackgroundTask::SetProgress(0.3);

		std::filesystem::create_directories(Installer::CurrentPath + "Data/temp/net/ts/");

		try
		{
			std::ifstream in = std::ifstream(Installer::CurrentPath + "Data/temp/net/tspage.txt");
			std::stringstream str; str << in.rdbuf();
			auto response = json::parse(str.str());

			std::vector<Package> NewFoundMods;
			size_t Start = ModsTab::ModsPerPage * (Page);
			size_t FoundCount = 0;

			struct ModElement
			{
				ModElement(size_t CurrentIndex, size_t Rating, bool SortByLowest)
				{
					this->CurrentIndex = CurrentIndex;
					this->Rating = Rating;
					this->SortByLowest = SortByLowest;
				}
				size_t CurrentIndex = 0;
				size_t Rating = 0;
				bool SortByLowest = true;

				static bool compare(const ModElement& a, const ModElement& b)
				{
					if (a.SortByLowest)
						return a.Rating < b.Rating;
					else
						return a.Rating > b.Rating;
				}

			};

			std::vector<ModElement> RatedMods;

			for (size_t i = 0; i < response.size(); i++)
			{
				size_t Rating = 0;
				bool SortByLowest = true;

				switch (SelectedOrdering)
				{
				case Ordering::Newest:
				{
					// https://stackoverflow.com/questions/21021388/how-to-parse-a-date-string-into-a-c11-stdchrono-time-point-or-similar
					std::tm tm = {};
					std::stringstream ss(response[i].at("date_created").get<std::string>());
					ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
					auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
					Rating = tp.time_since_epoch().count();
					SortByLowest = false;
				}
				break;
				case Ordering::Last_Updated:
					Rating = i;
					break;
				case Ordering::Most_Downloaded:
					for (auto& i : response.at(i).at("versions"))
					{
						Rating += i.at("downloads").get<size_t>();
					}
					SortByLowest = false;
					break;
				case Ordering::Top_Rated:
					Rating = response[i].at("rating_score");
					SortByLowest = false;
					break;
				default:
					break;
				}

				RatedMods.push_back(ModElement(i, Rating, SortByLowest));
			}

			std::sort(RatedMods.begin(), RatedMods.end(), ModElement::compare);


			for (size_t i = 0; i < RatedMods.size(); i++)
			{
				// Kwality code
				if (FoundCount == ModsTab::ModsPerPage + Start - (Page == 0 ? 0 : 1))
				{
					break;
				}
				Package Mod;
				auto& elem = response[RatedMods[i].CurrentIndex];
				if (elem.at("is_pinned"))
				{
					continue;
				}
				Mod.Name = elem.at("versions")[0].at("name");
				Mod.Author = elem.at("owner");

				std::string LowerCaseName = Mod.Name;
				std::string LowerCaseAuthor = Mod.Author;


				std::transform(LowerCaseName.begin(), LowerCaseName.end(), LowerCaseName.begin(),
					[](unsigned char c) { return std::tolower(c); });
				std::transform(LowerCaseAuthor.begin(), LowerCaseAuthor.end(), LowerCaseAuthor.begin(),
					[](unsigned char c) { return std::tolower(c); });

				if (!Filter.empty() && (LowerCaseName.find(Filter) == std::string::npos && LowerCaseAuthor.find(Filter) == std::string::npos))
				{
					continue;
				}
				Mod.DependencyString = elem.at("full_name");
				Mod.UUID = elem.at("uuid4");
				Mod.Img = elem.at("versions")[0].at("icon");
				Mod.Namespace = elem.at("owner");
				Mod.IsNSFW = elem.at("has_nsfw_content");


				FoundCount++;

				if (FoundCount < Start)
				{
					continue;
				}
				NewFoundMods.push_back(Mod);
			}

			if (ModLoadID == CurrentlyLoadedPageID)
			{
				FoundMods = NewFoundMods;
				IsDownloading = false;
			}
			else
			{
				return;
			}

			size_t i = 0;
			ShouldStopLoadingImages = false;
			size_t FirstSize = FoundMods.size();
			for (auto& Elem : FoundMods)
			{
				BackgroundTask::SetProgress(0.3f + ((float)i++ / (float)FoundMods.size()) * 0.5f);

				std::string TargetName = Installer::CurrentPath + "Data/temp/net/ts/" + Elem.Name + ".png";
				// Windows file size monent
				if (!std::filesystem::exists(TargetName) || std::filesystem::file_size(TargetName) < 1000)
				{
					Networking::Download(Elem.Img, TargetName, "X-CSRFToken: ");
					if (FoundMods.size() != FirstSize)
					{
						break;
					}
					Elem.Img = TargetName;
					LoadedImages = true;
				}
				Elem.Img = TargetName;
				if (ShouldStopLoadingImages)
				{
					return;
				}
			}
		}
		catch (std::exception& e)
		{
			Log::Print("Thunderstore response has an invalid layout.", Log::Error);
			Log::Print(e.what(), Log::Error);

			if (!std::filesystem::exists(Installer::CurrentPath + "Data/temp"))
			{
				std::filesystem::create_directories(Installer::CurrentPath + "Data/temp");
			}

			Log::Print("Writing response to Data/temp/invalidresponse.txt", Log::Error);
			if (std::filesystem::exists(Installer::CurrentPath + "Data/temp/invalidresponse.txt"))
			{
				std::filesystem::remove(Installer::CurrentPath + "Data/temp/invalidresponse.txt");
			}

			std::filesystem::copy(Installer::CurrentPath + "Data/temp/net/tspage.txt", Installer::CurrentPath + "Data/temp/invalidresponse.txt");
		}
		BackgroundTask::SetProgress(1);
		IsDownloading = false;
		LoadedImages = true;
	}
}

std::vector<Thunderstore::Package> Thunderstore::Package::GetDependencies()
{
	using namespace nlohmann;
	std::vector<Package> OutPackages;

	Networking::Download("https://thunderstore.io/c/northstar/api/v1/package/", Installer::CurrentPath + "Data/temp/net/tspage.txt", "");
	try
	{
		std::ifstream in = std::ifstream(Installer::CurrentPath + "Data/temp/net/tspage.txt");
		std::stringstream str; str << in.rdbuf();
		auto response = json::parse(str.str());
		for (const auto& dep : Dependencies)
		{
			for (const auto& i : response)
			{
				if (i.at("full_name") != dep)
				{
					continue;
				}
				Package Mod;
				Mod.Name = i.at("versions")[0].at("name");
				Mod.Author = i.at("owner");
				Mod.DependencyString = i.at("full_name");
				Mod.UUID = i.at("uuid4");
				Mod.Img = i.at("versions")[0].at("icon");
				Mod.Namespace = i.at("owner");
				Mod.Version = i.at("versions")[0].at("version_number");
				Mod.IsNSFW = i.at("has_nsfw_content");
				Mod.DownloadUrl = i.at("versions")[0].at("download_url");

				OutPackages.push_back(Mod);
				break;
			}
		}
		in.close();
	}
	catch (std::exception& e)
	{
		Log::Print(e.what(), Log::Error);
	}
	return OutPackages;
}

void Thunderstore::DownloadThunderstoreInfo(Ordering ModOrdering, size_t Page, std::string Filter, bool Async)
{
	TSDownloadThunderstoreInfo::ModOrdering = ModOrdering;
	TSDownloadThunderstoreInfo::Page = Page;
	TSDownloadThunderstoreInfo::Filter = Filter;
	if (Async)
	{
		new BackgroundTask(TSDownloadThunderstoreInfo::DownloadThunderstoreInfoInternal);
	}
	else
	{
		TSDownloadThunderstoreInfo::DownloadThunderstoreInfoInternal();
	}
}

namespace Thunderstore::TSGetModInfoFunc
{
	Package m;
	void AsyncGetModInfoInternal()
	{
		using namespace nlohmann;

		// To avoid loading a page while another page loads.
		if (m.UUID.empty())
		{
			SelectedMod = m;
			LoadedSelectedMod = true;
			return;
		}
		BackgroundTask::SetStatus("Loading Thunderstore mod");

		Networking::Download("https://thunderstore.io/c/northstar/api/v1/package/" + m.UUID, Installer::CurrentPath + "Data/temp/net/mod.txt", "");
		try
		{
			std::ifstream in = std::ifstream(Installer::CurrentPath + "Data/temp/net/mod.txt");
			std::stringstream str; str << in.rdbuf();
			auto response = json::parse(str.str());
			auto& version = response.at("versions")[0];
			m.Description = version.at("description");
			m.DownloadUrl = version.at("download_url");
			m.Version = response.at("versions")[0].at("version_number").get<std::string>();
			m.Rating = response.at("rating_score");
			m.IsDeprecated = response.at("is_deprecated");
			m.DependencyString = response.at("full_name");

			bool CheckedDependencies = false;
			for (auto& i : response.at("versions"))
			{
				m.Downloads += i.at("downloads").get<size_t>();

				if (CheckedDependencies)
				{
					continue;
				}
				CheckedDependencies = true;
				for (const std::string& dep : i.at("dependencies"))
				{
					if (dep.substr(0, dep.find_first_of("-")) == "northstar")
					{
						continue;
					}
					m.Dependencies.push_back(dep.substr(0, dep.find_last_of("-")));
				}
			}

			// I'm using the experimental API anyways because there doesn't seem to be another way to do this.
			// If any of this fails, (as it probably will), it shouldn't matter that much.
			try
			{
				Networking::Download("https://thunderstore.io/api/experimental/frontend/c/northstar/p/" 
					+ m.Namespace
					+ "/"
					+ m.Name 
					+ "/", Installer::CurrentPath + "Data/temp/net/expmod.txt", "");
				std::ifstream in = std::ifstream(Installer::CurrentPath + "Data/temp/net/expmod.txt");
				std::stringstream str; str << in.rdbuf();
				response = json::parse(str.str());
				m.Description = response.at("markdown");
			}
			catch (std::exception& e)
			{
				Log::Print("Error reading thunderstore response.", Log::Warning);
			}
		}
		catch (std::exception& e)
		{
			Log::Print("Thunderstore response has an invalid layout.", Log::Error);
			Log::Print(e.what(), Log::Error);
			if (std::filesystem::exists(Installer::CurrentPath + "Data/temp/invalidresponse.txt"))
			{
				std::filesystem::remove(Installer::CurrentPath + "Data/temp/invalidresponse.txt");
			}
			Log::Print("address: https://thunderstore.io/c/northstar/api/v1/package/" + m.UUID, Log::Error);
			Log::Print("Writing response to Data/temp/invalidresponse.txt", Log::Error);
			std::filesystem::copy(Installer::CurrentPath + "Data/temp/net/mod.txt", Installer::CurrentPath + "Data/temp/invalidresponse.txt");
		}

		SelectedMod = m;
		LoadedSelectedMod = true;
	}
}

void Thunderstore::SaveModInfo(Package m, std::vector<std::string> ModFiles, bool Temporary)
{
	using namespace nlohmann;

	std::filesystem::create_directories(Installer::CurrentPath + "Data/var/modinfo/" + ProfileTab::CurrentProfile.DisplayName);
	std::ofstream out = std::ofstream(Installer::CurrentPath + "Data/var/modinfo/" + ProfileTab::CurrentProfile.DisplayName + "/" + m.Namespace + "." + m.Name + ".json");

	std::string Image;
	if (std::filesystem::exists(m.Img))
	{
		std::string ImagePath = Installer::CurrentPath + "Data/var/modinfo/" + ProfileTab::CurrentProfile.DisplayName + "/" + m.Namespace + "." + m.Name + ".png";
		if (std::filesystem::exists(ImagePath))
		{
			std::filesystem::remove(ImagePath);
		}
		std::filesystem::copy(m.Img, ImagePath);
		Image = ImagePath;
	}
	else
	{
		std::string ImagePath = Installer::CurrentPath + "Data/var/modinfo/"
			+ ProfileTab::CurrentProfile.DisplayName
			+ "/"
			+ m.Namespace
			+ "."
			+ m.Name
			+ ".png";
		Networking::Download(m.Img, ImagePath, "");
		Image = ImagePath;
	}

	auto descr(json::object({
		{"version", m.Version},
		{"author", m.Author},
		{"namespace", m.Namespace},
		{"name", m.Name},
		{"mod_files", ModFiles},
		{"description", m.Description},
		{"image", Image},
		{"is_temporary", Temporary},
		{"file_format_version", MOD_DESCRIPTOR_FILE_FORMAT_VERSION},
		{"thunderstore_mod_identifier", m.UUID},
		{"mod_source", "thunderstore"}
		}));
	Log::Print("Saving mod info - Data/var/modinfo/" + ProfileTab::CurrentProfile.DisplayName + "/" + m.Namespace + "." + m.Name + ".json");

	out << descr.dump();
	out.close();
}

bool Thunderstore::VanillaPlusInstalled()
{
	Package VanillaPlus;
	VanillaPlus.Name = "VanillaPlus";
	return Thunderstore::IsModInstalled(VanillaPlus);
}

void Thunderstore::SetModEnabled(Package m, bool IsEnabled)
{
	using namespace nlohmann;

	std::string EnabledModsJson = ProfileTab::CurrentProfile.Path + "/enabledmods.json";

	if (!std::filesystem::exists(EnabledModsJson) || std::filesystem::is_empty(EnabledModsJson))
	{
		std::ofstream out = std::ofstream(EnabledModsJson);
		out << "{}";
		out.close();
	}

	if (std::filesystem::exists(EnabledModsJson))
	{
		std::vector<std::string> TargetMods = GetLocalModNames(m);
		std::ifstream in = std::ifstream(EnabledModsJson);
		std::stringstream str;
		str << in.rdbuf();
		in.close();
		json EnabledMods = json::parse(str.str());


		for (const auto& i : TargetMods)
		{
			if (EnabledMods.contains(i))
			{
				if (IsEnabled)
				{
					Log::Print("Enabled existing key: " + i);
				}
				else
				{
					Log::Print("Disabled existing key: " + i);
				}
				EnabledMods.at(i) = IsEnabled;
			}
			else
			{
				try
				{
					if (IsEnabled)
					{
						Log::Print("Enabled new key: " + i);
					}
					else
					{
						Log::Print("Disabled new key: " + i);
					}
					EnabledMods.update({ { i, IsEnabled } });
				}
				catch (std::exception& e)
				{
					Log::Print(e.what());
				}

			}
		}

		std::ofstream out = std::ofstream(EnabledModsJson);
		out << EnabledMods.dump();
		out.close();
	}

}

bool Thunderstore::GetModEnabled(Package m)
{
	using namespace nlohmann;
	std::string EnabledModsJson = ProfileTab::CurrentProfile.Path + "/enabledmods.json";

	if (!std::filesystem::exists(EnabledModsJson) || std::filesystem::is_empty(EnabledModsJson))
	{
		std::ofstream out = std::ofstream(EnabledModsJson);
		out << "{}";
		out.close();
	}

	if (std::filesystem::exists(EnabledModsJson) && !std::filesystem::is_empty(EnabledModsJson))
	{
		bool AllModsEnabled = true;
		std::vector<std::string> TargetMods = GetLocalModNames(m);
		std::ifstream in = std::ifstream(EnabledModsJson);
		std::stringstream str;
		str << in.rdbuf();
		in.close();
		json EnabledMods = json::parse(str.str());

		for (const auto& i : TargetMods)
		{
			if (EnabledMods.contains(i) && !EnabledMods.at(i))
			{
				AllModsEnabled = false;
			}
		}
		return AllModsEnabled;
	}
	return false;
}

void Thunderstore::GetModInfo(Package m, bool Async)
{
	TSGetModInfoFunc::m = m;
	if (Async)
	{
		new BackgroundTask(TSGetModInfoFunc::AsyncGetModInfoInternal);
	}
	else
	{
		TSGetModInfoFunc::AsyncGetModInfoInternal();
	}
}

namespace Thunderstore::TSModFunc
{
	Package m;
	bool Async;
	bool IsTemporary;
	bool Reload;
	void InstallOrUninstallMod()
	{
		using namespace nlohmann;

		if (!std::filesystem::exists(Game::GamePath))
		{
			return;
		}

		try
		{
			IsInstallingMod = true;
			std::filesystem::create_directories(Installer::CurrentPath + "Data/var/modinfo/" + ProfileTab::CurrentProfile.DisplayName);
			if (Thunderstore::IsModInstalled(m))
			{
				Log::Print("Removing: " + m.Name);
				auto InfoFile = Installer::CurrentPath + "Data/var/modinfo/" + ProfileTab::CurrentProfile.DisplayName + "/" + m.Namespace + "." + m.Name + ".json";
				std::ifstream in = std::ifstream(InfoFile);
				std::stringstream str; str << in.rdbuf();
				in.close();
				if (std::filesystem::exists(InfoFile))
				{
					std::filesystem::remove_all(InfoFile);
				}

				if (!str.str().empty())
				{
					json modinfo = json::parse(str.str());

					if (m.Name != "NorthstarReleaseCandidate")
					{
						for (auto& i : modinfo.at("mod_files"))
						{
							auto file = ProfileTab::CurrentProfile.Path + i.get<std::string>();
							if (std::filesystem::exists(file))
							{
								std::filesystem::remove_all(file);
							}
						}

						if (m.Name == "VanillaPlus")
						{
							RemoveVanillaPlus();
							IsInstallingMod = false;
							return;
						}
					}
					else if (!IsTemporary)
					{
						std::filesystem::remove(ProfileTab::CurrentProfile.Path + "/Northstar.dll");
						Game::UpdateGame();
						ProfileTab::UpdateProfile(ProfileTab::CurrentProfile, true);
					}
				}
				IsInstallingMod = false;
#ifdef TF_PLUGIN
				if (Reload)
				{
					Plugin::ReloadMods();
				}
#endif
				return;
			}
			Log::Print("Installing mod \"" + m.Name + "\"");

			std::string TargetZipName = Installer::CurrentPath + "Data/temp/net/" + m.Author + "." + m.Name + ".zip";
			Networking::Download(m.DownloadUrl, TargetZipName, "", BackgroundTask::IsBackgroundTask());

			std::filesystem::remove_all(Installer::CurrentPath + "Data/temp/mod");
			std::filesystem::create_directories(Installer::CurrentPath + "Data/temp/mod");
			Networking::ExtractZip(TargetZipName, Installer::CurrentPath + "Data/temp/mod/");
			std::filesystem::remove(TargetZipName);
			
			if (m.Name == "VanillaPlus")
			{
				InstallVanillaPlus(Installer::CurrentPath + "Data/temp/mod", m);
				if (m.UUID == SelectedMod.UUID)
					LoadedSelectedMod = true;
				IsInstallingMod = false;
				return;
			}

			if (m.Name == "NorthstarReleaseCandidate")
			{
				// Remove core mods before installing them again
				for (auto& i : std::filesystem::directory_iterator(ProfileTab::CurrentProfile.Path + "/mods"))
				{
					std::string File = i.path().filename().u8string();
					std::string Author = File.substr(0, File.find_first_of("."));
					if (Author == "Northstar")
					{
						std::filesystem::remove(File);
					}
				}

				std::filesystem::copy(Installer::CurrentPath + "Data/temp/mod/Northstar/Northstar.dll",
					ProfileTab::CurrentProfile.Path + "/Northstar.dll", 
					std::filesystem::copy_options::overwrite_existing);

				std::filesystem::remove(Installer::CurrentPath + "Data/temp/mod/Northstar/Northstar.dll");

				std::filesystem::rename(
					"Data/temp/mod/Northstar/R2Northstar", 
					"Data/temp/mod/Northstar/" + ProfileTab::CurrentProfile.DisplayName);
				std::filesystem::rename(
					"Data/temp/mod/Northstar/NorthstarLauncher.exe",
					"Data/temp/mod/Northstar/NorthstarLauncherRC.exe");


				std::filesystem::copy("Data/temp/mod/Northstar", Game::GamePath,
					std::filesystem::copy_options::overwrite_existing
					| std::filesystem::copy_options::recursive);

				SaveModInfo(m, {}, false);
				IsInstallingMod = false;
				return;
			}

#if INSTALL_AS_PACKAGES
			std::string TargetPackage = m.Author + "-" + m.Name + "-" + m.Version;
			std::filesystem::create_directories(ProfileTab::CurrentProfile.Path + "/packages/" + TargetPackage);
			std::filesystem::copy(Installer::CurrentPath + "Data/temp/mod/", ProfileTab::CurrentProfile.Path + "/packages/" + TargetPackage,
				std::filesystem::copy_options::overwrite_existing
				| std::filesystem::copy_options::recursive);

			std::vector<std::string> Files = {"/packages/" + TargetPackage};
#else
			std::filesystem::copy(Installer::CurrentPath + "Data/temp/mod/mods/", ProfileTab::CurrentProfile.Path + "/mods",
				std::filesystem::copy_options::overwrite_existing
				| std::filesystem::copy_options::recursive); 

			std::vector<std::string> Files;
			for (auto& i : std::filesystem::directory_iterator(Installer::CurrentPath + "Data/temp/mod/mods"))
			{
				Files.push_back("/mods/" + i.path().filename().u8string());
			}

			// TODO: (Or not, since it now is deprecated behavior) Handle plugins
#endif

			SaveModInfo(m, Files, IsTemporary);
#ifdef TF_PLUGIN
			if (Reload)
			{
				Plugin::ReloadMods();
			}
#endif
			// Thunderstore::SetModEnabled(m, true);
		}
		catch (std::exception& e)
		{
			Log::Print(e.what(), Log::Error);
		}
		if (m.UUID == SelectedMod.UUID)
			LoadedSelectedMod = true;
		IsInstallingMod = false;
	}
}

void Thunderstore::InstallOrUninstallMod(Package m, bool IsTemporary, bool Async, bool Reload)
{
	TSModFunc::m = m;
	TSModFunc::Async = Async;
	TSModFunc::IsTemporary = IsTemporary;
	TSModFunc::Reload = Reload;
	
	if (Async)
	{
		new BackgroundTask(TSModFunc::InstallOrUninstallMod);
	}
	else
	{
		TSModFunc::InstallOrUninstallMod();
	}
}

std::vector<std::string> Thunderstore::GetLocalModNames(Package m)
{
	using namespace nlohmann;

	auto Mods = GetLocalMods(m);

	std::vector<std::string> ModNames;
	for (auto& i : Mods)
	{
		std::string ModJson = i + "/mod.json";
		if (std::filesystem::exists(ModJson))
		{
			try
			{
				std::ifstream in = std::ifstream(ModJson);
				std::stringstream str;
				str << in.rdbuf();
				json InfoJson = json::parse(str.str());
				in.close();
				ModNames.push_back(InfoJson.at("Name"));
			}
			catch (std::exception& e)
			{
				Log::Print("Error while parsing " + ModJson + ": " + std::string(e.what()));
			}
		}
	}

	return ModNames;
}
std::string ToLowerCase(std::string Target);
std::vector<std::string> Thunderstore::GetLocalMods(Package m)
{
	using namespace nlohmann;
	// Mh yes, packages made everything so much easier.
	try
	{
		std::vector<std::string> TargetMods;

		auto InfoFile = Installer::CurrentPath + "Data/var/modinfo/" + ProfileTab::CurrentProfile.DisplayName + "/" + m.Namespace + "." + m.Name + ".json";
		std::string TargetPackage = ProfileTab::CurrentProfile.Path + "/packages/" + m.Author + "-" + m.Name + "-" + m.Version;

		if (!std::filesystem::exists(TargetPackage))
		{
			for (auto& i : std::filesystem::directory_iterator(ProfileTab::CurrentProfile.Path + "/packages"))
			{
				if (ToLowerCase(m.Name).find(ToLowerCase(i.path().u8string())) != std::string::npos
					|| ToLowerCase(i.path().u8string()).find(ToLowerCase(m.Name)) != std::string::npos)
				{
					TargetPackage = i.path().u8string();
				}
			}
		}

		if (std::filesystem::exists(InfoFile))
		{
			std::ifstream in = std::ifstream(InfoFile);
			std::stringstream str;
			str << in.rdbuf();
			json InfoJson = json::parse(str.str());
			in.close();


			for (const auto& i : InfoJson.at("mod_files"))
			{
				if (i.get<std::string>().substr(0, 4) == "mods/")
				{
					TargetMods.push_back(ProfileTab::CurrentProfile.Path + i.get<std::string>());
				}
				else
				{
					TargetPackage = ProfileTab::CurrentProfile.Path + i.get<std::string>();
				}
			}
		}

		if (!m.IsPackage)
		{
			std::vector<std::string> PossibleModPaths =
			{
				ProfileTab::CurrentProfile.Path + "/mods/" + m.Name,
				ProfileTab::CurrentProfile.Path + "/mods/" + m.Namespace + "." + m.Name,
				ProfileTab::CurrentProfile.Path + "/mods/" + m.Author + "." + m.Name
			};

			for (const auto& i : PossibleModPaths)
			{
				if (std::filesystem::exists(i))
				{
					TargetMods = { i };
				}
			}
		}
		else
		{
			if (std::filesystem::exists(TargetPackage + "/mods"))
			{
				for (auto& i : std::filesystem::directory_iterator(TargetPackage + "/mods"))
				{
					TargetMods.push_back(i.path().u8string());
				}
			}

		}		
		return TargetMods;
	}
	catch (std::exception& e)
	{
		Log::Print(e.what());
		return std::vector<std::string>();
	}
}

void Thunderstore::InstallVanillaPlus(std::string From, Package m)
{
	using namespace nlohmann;
	if (ProfileTab::CurrentProfile.DisplayName == "R2Northstar")
	{
		WindowFunc::ShowPopupError("Cannot install Vanilla+ on the default R2Northstar profile.\nPlease create and select a new profile in the profile menu.");
		return;
	}

	std::filesystem::copy(From + "/mods",
		ProfileTab::CurrentProfile.Path + "/mods", 
		std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

	for (const auto& i : Game::CoreModNames)
	{
		std::string Path = ProfileTab::CurrentProfile.Path + "/mods/" + i;
		if (std::filesystem::exists(Path))
		{
			std::filesystem::remove_all(Path);
		}
	}

	SaveModInfo(m, {"/mods/NP.VanillaPlus"}, false);
}

void Thunderstore::RemoveVanillaPlus()
{
	for (const auto& i : Game::CoreModNames)
	{
		std::string From = Game::GamePath + "/R2Northstar/mods/" + i;
		if (std::filesystem::exists(From))
		{
			std::filesystem::copy(From,
				ProfileTab::CurrentProfile.Path + "/mods/" + i,
				std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
		}
	}
}
