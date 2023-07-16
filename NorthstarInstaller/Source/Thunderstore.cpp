#include "Thunderstore.h"
#include "Game.h"
#include "Log.h"

#include "JSON/json.hpp"

#include <filesystem>
#include <set>
#include <fstream>
#include "BackgroundTask.h"
#include "Networking.h"
#include "Tabs/ModsTab.h"

constexpr const char* MOD_DESCRIPTOR_FILE_FORMAT_VERSION = "v2";

Thunderstore::Ordering Thunderstore::SelectedOrdering = Ordering::Last_Updated;
std::atomic<bool> Thunderstore::IsDownloading = false;
std::atomic<bool> Thunderstore::LoadedImages = false;
std::atomic<bool> Thunderstore::ShouldStopLoadingImages = false;
std::atomic<bool> Thunderstore::LoadedSelectedMod = false;
std::vector<Thunderstore::Package> Thunderstore::FoundMods;
Thunderstore::Package Thunderstore::SelectedMod;

bool Thunderstore::IsMostRecentFileVersion(std::string VersionString)
{
	return VersionString == MOD_DESCRIPTOR_FILE_FORMAT_VERSION;
}

Thunderstore::InstalledModsResult Thunderstore::GetInstalledMods()
{
	using namespace nlohmann;
	// Mods managed by the installer
	std::set<std::string> ManagedMods;

	std::vector<Package> UnmanagedMods;
	std::vector<Package> InstalledMods;
	if (std::filesystem::exists("Data/var/modinfo"))
	{
		for (auto& i : std::filesystem::directory_iterator("Data/var/modinfo"))
		{
			auto pathstring = i.path().string();
			if (pathstring.substr(i.path().string().find_last_of(".")) == ".png")
			{
				continue;
			}

			std::ifstream in = std::ifstream(i.path().string());
			std::stringstream str; str << in.rdbuf();
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
				p.UUID = modinfo.at("UUID");
				p.FileVersion = modinfo.at("file_format_version");
				p.IsTemporary = modinfo.at("is_temporary");
			}
			catch (std::exception& e)
			{
				Log::Print("Error reading mod descriptor file. Possibly outdated version", Log::Warning);
				Log::Print(e.what(), Log::Warning);
				p.FileVersion = "Unknown";

			}


			for (auto& i : modinfo.at("mod_files"))
			{
				auto file = Game::GamePath + "/R2Northstar/mods/" + i.get<std::string>();
				if (std::filesystem::exists(file))
				{
					ManagedMods.insert(i.get<std::string>());
				}
			}
			InstalledMods.push_back(p);
		}
	}

	if (!std::filesystem::exists(Game::GamePath + "/R2Northstar/mods"))
	{
		std::filesystem::create_directories(Game::GamePath + "/R2Northstar/mods");
	}

	for (auto& i : std::filesystem::directory_iterator(Game::GamePath + "/R2Northstar/mods"))
	{
		std::string ModName = i.path().filename().string();
		std::string Author = ModName.substr(0, ModName.find_first_of("."));
		std::string Name = ModName.substr(ModName.find_first_of(".") + 1);

		if (!ManagedMods.contains(ModName) && std::filesystem::is_directory(i) && Author != "Northstar" && Name != "autojoin")
		{
			Package p;
			p.Name = Name;
			p.Author = Author;
			p.Namespace = Author;
			p.Version = "?";
			p.IsUnknownLocalMod = true;
			p.DownloadUrl = i.path().string();
			UnmanagedMods.push_back(p);
		}
	}

	return InstalledModsResult(InstalledMods, UnmanagedMods);
}


bool Thunderstore::IsModInstalled(Package m)
{
	// As a failsafe also check for mod files that could've been installed using another method.
	// This way we hopefully won't install a mod twice. (Oh no)
	// This won't always work.
	if (std::filesystem::exists("Data/var/modinfo/" + m.Namespace + "." + m.Name + ".json")
		|| std::filesystem::exists(Game::GamePath + "/R2Northstar/mods/" + m.Namespace + "." + m.Name))
	{
		return true;
	}
	if (!std::filesystem::exists(Game::GamePath + "/R2Northstar/mods"))
	{
		return false;
	}
	for (auto& i : std::filesystem::directory_iterator(Game::GamePath + "R2Northstar/mods"))
	{
		Package fm;
		std::string FileName = i.path().filename().string();

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

	if (!std::filesystem::exists("Data/var/modinfo"))
	{
		return false;
	}

	for (auto& i : std::filesystem::directory_iterator("Data/var/modinfo"))
	{
		std::string Name = i.path().filename().string();
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
		std::transform(Filter.begin(), Filter.end(), Filter.begin(),
			[](unsigned char c) { return std::tolower(c); });

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
			if (std::filesystem::exists("Data/temp/net/ts"))
			{
				// Get size of already loaded image folders.
				size_t size = 0;
				for (std::filesystem::recursive_directory_iterator it("Data/temp/net/ts");
					it != std::filesystem::recursive_directory_iterator();
					++it)
				{
					if (!std::filesystem::is_directory(*it))
						size += std::filesystem::file_size(*it);
				}

				// If the already loaded images are larger than 10mb, delete them.
				if (size >= 20ull * 1000ull * 1000ull)
				{
					std::filesystem::remove_all("Data/temp/net/ts");
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
		Networking::Download("https://thunderstore.io/c/northstar/api/v1/package/", "Data/temp/net/tspage.txt", "");

		BackgroundTask::SetProgress(0.3);

		std::filesystem::create_directories("Data/temp/net/ts/");

		try
		{
			// The thunderstore API sucks.
			std::ifstream in = std::ifstream("Data/temp/net/tspage.txt");
			std::stringstream str; str << in.rdbuf();
			auto response = json::parse(str.str());
			FoundMods.clear();
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
						Rating += i.at("downloads");
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
				Mod.UUID = elem.at("uuid4");
				Mod.Img = elem.at("versions")[0].at("icon");
				Mod.Namespace = elem.at("owner");
				Mod.IsNSFW = elem.at("has_nsfw_content");


				FoundCount++;

				if (FoundCount < Start)
				{
					continue;
				}
				FoundMods.push_back(Mod);
			}

			std::vector<Package> ModsCopy = FoundMods;
			IsDownloading = false;

			size_t i = 0;
			ShouldStopLoadingImages = false;
			for (auto& Elem : FoundMods)
			{
				BackgroundTask::SetProgress(0.3f + ((float)i++ / (float)FoundMods.size()) * 0.5f);

				std::string TargetName = "Data/temp/net/ts/" + Elem.Name + ".png";
				// Windows file size monent
				if (!std::filesystem::exists(TargetName) || std::filesystem::file_size(TargetName) < 1000)
				{
					Networking::Download(Elem.Img, TargetName, "X-CSRFToken: nan");
					Elem.Img = TargetName;
					LoadedImages = true;
				}
				Elem.Img = TargetName;
				if (ShouldStopLoadingImages)
				{
					break;
				}
			}
		}
		catch (std::exception& e)
		{
			Log::Print("Thunderstore response has an invalid layout.", Log::Error);
			Log::Print(e.what(), Log::Error);

			Log::Print("Writing response to Data/temp/invalidresponse.txt", Log::Error);
			if (std::filesystem::exists("Data/temp/invalidresponse.txt"))
			{
				std::filesystem::remove("Data/temp/invalidresponse.txt");
			}
			std::filesystem::copy("Data/temp/net/tspage.txt", "Data/temp/invalidresponse.txt");
		}
		BackgroundTask::SetProgress(1);
		IsDownloading = false;
		LoadedImages = true;
	}
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
		if (m.UUID.empty())
		{
			SelectedMod = m;
			LoadedSelectedMod = true;
			return;
		}
		BackgroundTask::SetStatus("Loading Thunderstore mod");
		BackgroundTask::SetStatus("Loading Thunderstore mod");
		Networking::Download("https://thunderstore.io/c/northstar/api/v1/package/" + m.UUID, "Data/temp/net/mod.txt", "");
		try
		{
			std::ifstream in = std::ifstream("Data/temp/net/mod.txt");
			std::stringstream str; str << in.rdbuf();
			auto response = json::parse(str.str());
			auto& version = response.at("versions")[0];
			m.Description = version.at("description");
			m.DownloadUrl = version.at("download_url");
			m.Version = response.at("versions")[0].at("version_number").get<std::string>();
			m.Rating = response.at("rating_score");
			m.IsDeprecated = response.at("is_deprecated");

			for (auto& i : response.at("versions"))
			{
				m.Downloads += i.at("downloads");
			}

			// I'm using the experimental API anyways because there doesn't seem to be another way to do this.
			// If any of this fails, (as it probably will), it shouldn't matter that much.
			try
			{
				Networking::Download("https://thunderstore.io/api/experimental/frontend/c/northstar/p/" + m.Namespace + "/" + m.Name + "/", "Data/temp/net/expmod.txt", "");
				std::ifstream in = std::ifstream("Data/temp/net/expmod.txt");
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
			if (std::filesystem::exists("Data/temp/invalidresponse.txt"))
			{
				std::filesystem::remove("Data/temp/invalidresponse.txt");
			}
			Log::Print("address: https://thunderstore.io/c/northstar/api/v1/package/" + m.UUID, Log::Error);
			Log::Print("Writing response to Data/temp/invalidresponse.txt", Log::Error);
			std::filesystem::copy("Data/temp/net/mod.txt", "Data/temp/invalidresponse.txt");
		}
		SelectedMod = m;
		LoadedSelectedMod = true;
	}
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
	void InstallOrUninstallMod()
	{
		using namespace nlohmann;
		try
		{
			if (Thunderstore::IsModInstalled(m))
			{
				auto InfoFile = "Data/var/modinfo/" + m.Namespace + "." + m.Name + ".json";
				std::ifstream in = std::ifstream(InfoFile);
				std::stringstream str; str << in.rdbuf();
				json modinfo = json::parse(str.str());
				in.close();
				if (std::filesystem::exists(InfoFile))
				{
					std::filesystem::remove_all(InfoFile);
				}
				if (std::filesystem::exists(modinfo.at("image")))
				{
					std::filesystem::remove(modinfo.at("image"));
				}

				if (m.Name != "NorthstarReleaseCandidate")
				{
					for (auto& i : modinfo.at("mod_files"))
					{
						auto file = Game::GamePath + "/R2Northstar/mods/" + i.get<std::string>();
						if (std::filesystem::exists(file))
						{
							std::filesystem::remove_all(file);
						}
					}
				}
				else
				{
					Game::UpdateGameAsync();
				}
				try
				{
					FoundMods = GetInstalledMods().Combined();
				}
				catch (std::exception& e)
				{
					FoundMods.clear();
					Log::Print("Error parsing installed mods: " + std::string(e.what()), Log::Error);
				}
				return;
			}
			if (Async)
			{
				BackgroundTask::SetStatus("Downloading mod");
				BackgroundTask::SetProgress(0.999);
			}
			std::string TargetZipName = "Data/temp/net/" + m.Author + "." + m.Name + ".zip";
			Networking::Download(m.DownloadUrl, TargetZipName, "");

			std::filesystem::remove_all("Data/temp/mod");
			std::filesystem::create_directories("Data/temp/mod");
			Networking::ExtractZip(TargetZipName, "Data/temp/mod/");
			std::filesystem::remove(TargetZipName);
			
			if (m.Name == "NorthstarReleaseCandidate")
			{
				std::filesystem::copy("Data/temp/mod/Northstar", Game::GamePath,
					std::filesystem::copy_options::overwrite_existing
					| std::filesystem::copy_options::recursive);

				std::string Image;
				if (std::filesystem::exists(m.Img))
				{
					std::filesystem::copy(m.Img, "Data/var/modinfo/" + m.Namespace + "." + m.Name + ".png");
					Image = "Data/var/modinfo/" + m.Namespace + "." + m.Name + ".png";
				}
				else if (!IsTemporary)
				{
					Networking::Download(m.Img, "Data/var/modinfo/" + m.Namespace + "." + m.Name + ".png", "");
				}

				json descr(json::object({
					{"version", m.Version},
					{"author", m.Author},
					{"namespace", m.Namespace},
					{"name", m.Name},
					{"mod_files", ""},
					{"description", m.Description},
					{"image", Image},
					{"is_temporary", IsTemporary},
					{"file_format_version", MOD_DESCRIPTOR_FILE_FORMAT_VERSION},
					{"UUID", m.UUID},
					}));

				std::ofstream out = std::ofstream("Data/var/modinfo/" + m.Namespace + "." + m.Name + ".json");
				out << descr.dump();
				out.close();
				return;
			}

			std::filesystem::copy("Data/temp/mod/mods/", Game::GamePath + "/R2Northstar/mods",
				std::filesystem::copy_options::overwrite_existing
				| std::filesystem::copy_options::recursive);

			std::vector<std::string> Files;
			for (auto& i : std::filesystem::directory_iterator("Data/temp/mod/mods"))
			{
				Files.push_back(i.path().filename().string());
			}

			std::filesystem::create_directories("Data/var/modinfo/");
			std::ofstream out = std::ofstream("Data/var/modinfo/" + m.Namespace + "." + m.Name + ".json");

			std::string Image;
			if (std::filesystem::exists(m.Img))
			{
				std::filesystem::copy(m.Img, "Data/var/modinfo/" + m.Namespace + "." + m.Name + ".png");
				Image = "Data/var/modinfo/" + m.Namespace + "." + m.Name + ".png";
			}
			else
			{
				Networking::Download(m.Img, "Data/var/modinfo/" + m.Namespace + "." + m.Name + ".png", "");
			}

			auto descr(json::object({
				{"version", m.Version},
				{"author", m.Author},
				{"namespace", m.Namespace},
				{"name", m.Name},
				{"mod_files", Files},
				{"description", m.Description},
				{"image", Image},
				{"is_temporary", IsTemporary},
				{"file_format_version", MOD_DESCRIPTOR_FILE_FORMAT_VERSION},
				{"UUID", m.UUID}
				}));

			out << descr.dump();
			out.close();
		}
		catch (std::exception& e)
		{
			Log::Print(e.what());
		}
		Thunderstore::LoadedSelectedMod = true;
	}
}

void Thunderstore::InstallOrUninstallMod(Package m, bool IsTemporary, bool Async)
{
	TSModFunc::m = m;
	TSModFunc::Async = Async;
	TSModFunc::IsTemporary = IsTemporary;
	
	if (Async)
	{
		new BackgroundTask(TSModFunc::InstallOrUninstallMod);
	}
	else
	{
		TSModFunc::InstallOrUninstallMod();
	}
}