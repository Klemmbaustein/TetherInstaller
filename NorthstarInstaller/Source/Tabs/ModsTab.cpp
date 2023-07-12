#include "ModsTab.h"

#include <fstream>
#include <regex>
#include <iomanip>

#include <Rendering/Texture.h>
#include <Application.h>
#include <Rendering/MarkdownRendering.h>

#include "../JSON/json.hpp"

#include "../Networking.h"
#include "../Installer.h"
#include "../UIDef.h"
#include "../Log.h"
#include "../Game.h"

std::vector<UIButton*> ModsTab::ModButtons;
std::vector<UIButton*> ModsTab::PageButtons;
std::vector<UIButton*> ModsTab::CategoryButtons;

std::atomic<unsigned int> ModsTab::ModsPerPage = 20;
ModsTab* ModsTab::CurrentModsTab = nullptr;

namespace Thunderstore
{
	std::atomic<bool> IsDownloading = false;
	std::atomic<bool> LoadedImages = false;
	std::atomic<bool> ShouldStopLoadingImages = false;

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
		size_t Downloads = 0;
		size_t Rating = 0;
		bool IsUnknownLocalMod = false;
		bool IsNSFW = false;
	};


	std::vector<Package> FoundMods;

	enum class Ordering
	{
		Newest,
		Last_Updated,
		Most_Downloaded,
		Top_Rated,
		Installed
	};

	Ordering SelectedOrdering = Ordering::Last_Updated;


	struct Category
	{
		std::string Name;
		Ordering o;
	};

	std::vector<Thunderstore::Category> CategoryFunctions =
	{
		Thunderstore::Category("Installed", Thunderstore::Ordering::Installed),
		Thunderstore::Category("Last Updated", Thunderstore::Ordering::Last_Updated),
		Thunderstore::Category("Latest", Thunderstore::Ordering::Newest),
		Thunderstore::Category("Most Downloaded", Thunderstore::Ordering::Most_Downloaded),
		Thunderstore::Category("Top Rated", Thunderstore::Ordering::Top_Rated),
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
	InstalledModsResult GetInstalledMods()
	{
		using namespace nlohmann;

		if (!std::filesystem::exists("Data/var/modinfo"))
		{
			return InstalledModsResult();
		}

		// Mods managed by the installer
		std::set<std::string> ManagedMods;
		std::vector<Package> UnmanagedMods;
		std::vector<Package> InstalledMods;
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
			p.Author = modinfo.at("author");
			p.Namespace = modinfo.at("namespace");
			p.Name = modinfo.at("name");
			p.Description = modinfo.at("description");
			p.Img = modinfo.at("image");
			p.Version = modinfo.at("version");

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

		for (auto& i : std::filesystem::directory_iterator(Game::GamePath + "/R2Northstar/mods"))
		{
			std::string ModName = i.path().filename().string();
			std::string Author = ModName.substr(0, ModName.find_first_of("."));
			std::string Name = ModName.substr(ModName.find_first_of(".") + 1);

			if (!ManagedMods.contains(ModName) && std::filesystem::is_directory(i) && Author != "Northstar")
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

	bool IsModInstalled(Package m)
	{
		// As a failsafe also check for mod files that could've been installed using another method.
		// This way we hopefully won't install a mod twice. (Oh no)
		// This won't always work.
		return std::filesystem::exists("Data/var/modinfo/" + m.Namespace + "." + m.Name + ".json")
			|| std::filesystem::exists(Game::GamePath + "/R2Northstar/mods/" + m.Namespace + "." + m.Name);
	}


	// Gets the thunderstore mod page with the given ordering, filter and page index.
	// The result will be put into 'FoundMods' because this function is meant to be run as a new thread.
	void DownloadThunderstoreInfo(Ordering ModOrdering, size_t Page, std::string Filter)
	{
		using namespace nlohmann;
		std::transform(Filter.begin(), Filter.end(), Filter.begin(),
			[](unsigned char c) { return std::tolower(c); });

		if (ModOrdering == Ordering::Installed)
		{
			IsDownloading = true;
			Installer::BackgroundName = "Loading installed mods";
			Installer::BackgroundTask = "Loading installed mods";
			Installer::ThreadProgress = 0.5;
			try
			{
				FoundMods = GetInstalledMods().Combined();
			}
			catch (std::exception& e)
			{
				FoundMods.clear();
				Log::Print("Error parsing installed mods: " + std::string(e.what()),  Log::Error);
			}
			Installer::ThreadProgress = 1;
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
			Log::Print(e.what(),  Log::Info);
			throw e;
		}

		IsDownloading = true;
		Installer::BackgroundName = "Loading Thunderstore";
		Installer::BackgroundTask = "Loading Thunderstore";
		Installer::ThreadProgress = 0.1;
		Networking::Download("https://thunderstore.io/c/northstar/api/v1/package/", "Data/temp/net/tspage.txt", "");

		Installer::ThreadProgress = 0.3;

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
				ModElement(size_t CurrentIndex,size_t Rating, bool SortByLowest)
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
				if (Mod.Author == "northstar")
				{
					continue;
				}

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
				Installer::ThreadProgress = 0.3f + ((float)i++ / (float)FoundMods.size()) * 0.5f;

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
		Installer::ThreadProgress = 1;
		IsDownloading = false;
		LoadedImages = true;
	}

	std::atomic<bool> LoadedSelectedMod = false;
	Package SelectedMod;

	// Sets the Thunderstore::SelectedMod variable to a more detailed version of the given package.
	void GetModInfo(Package m)
	{
		using namespace nlohmann;
		if (m.UUID.empty())
		{
			SelectedMod = m;
			LoadedSelectedMod = true;
			Installer::ThreadProgress = 1;
			return;
		}
		Installer::ThreadProgress = 0;
		Installer::BackgroundName = "Loading Thunderstore mod";
		Installer::BackgroundTask = "Loading Thunderstore mod";
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

			for (auto& i : response.at("versions"))
			{
				m.Downloads += i.at("downloads");

			}

			// I'm using the experimental API anyways because there doesn't seem to be another way to do this.
			// Fuck this.
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
		Installer::ThreadProgress = 1;
	}

	// Downloads the given package into "Data/temp/net/{m.Author}.{m.Name}.zip,
	// extracts it's contents into "Data/temp/mod",
	// then extracts the content of the extracted zip file's "mods" Folder into the Titanfall 2
	// mods folder.
	// TODO: Extract other elements of the mod. (I've seen some mods have different Folders that
	// need to be extracted somewhere else)
	void InstallMod(Package m, bool Async = true)
	{
		using namespace nlohmann;
		if (Async)
		{
			Installer::ThreadProgress = 0;
		}
		try
		{
			if (Thunderstore::IsModInstalled(m))
			{
				auto InfoFile = "Data/var/modinfo/" + m.Namespace + "." + m.Name + ".json";
				std::ifstream in = std::ifstream(InfoFile);
				std::stringstream str; str << in.rdbuf();
				auto modinfo = json::parse(str.str());
				in.close();
				if (std::filesystem::exists(InfoFile))
				{
					std::filesystem::remove_all(InfoFile);
				}
				if (std::filesystem::exists(modinfo.at("image")))
				{
					std::filesystem::remove(modinfo.at("image"));
				}
				for (auto& i : modinfo.at("mod_files"))
				{
					auto file = Game::GamePath + "/R2Northstar/mods/" + i.get<std::string>();
					if (std::filesystem::exists(file))
					{
						std::filesystem::remove_all(file);
					}
				}
				Thunderstore::LoadedSelectedMod = true;
				try
				{
					FoundMods = GetInstalledMods().Combined();
				}
				catch (std::exception& e)
				{
					FoundMods.clear();
					Log::Print("Error parsing installed mods: " + std::string(e.what()), Log::Error);
				}
				if (Async)
				{
					Installer::ThreadProgress = 1;
				}
				return;
			}
			Installer::BackgroundName = "Downloading mod";
			Installer::BackgroundTask = "Downloading mod";
			std::string TargetZipName = "Data/temp/net/" + m.Author + "." + m.Name + ".zip";
			Networking::Download(m.DownloadUrl, TargetZipName, "");

			std::filesystem::remove_all("Data/temp/mod");
			std::filesystem::create_directories("Data/temp/mod");
			Networking::ExtractZip(TargetZipName, "Data/temp/mod/");
			std::filesystem::remove(TargetZipName);
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

			auto descr(json::object({ 
				{"version", m.Version},
				{"author", m.Author},
				{"namespace", m.Namespace},
				{"name", m.Name},
				{"mod_files", Files},
				{"description", m.Description},
				{"image", Image}
				}));

			out << descr.dump();
			out.close();
		}
		catch (std::exception& e)
		{
			Log::Print(e.what());
		}
		Thunderstore::LoadedSelectedMod = true;
		if (Async)
		{
			Installer::ThreadProgress = 1;
		}
	}
}

void ModsTab::GenerateModInfo()
{
	UpdateClickedCategoryButton();

	bool IsInstalled = Thunderstore::IsModInstalled(Thunderstore::SelectedMod);

	ModsScrollBox->GetScrollObject()->Percentage = 0;
	ModButtons.clear();
	ModsScrollBox->SetMaxScroll(50);
	ModsScrollBox->DeleteChildren();

	ModsScrollBox->AddChild((new UIButton(true, 0, 1, []() {CurrentModsTab->GenerateModPage(); }))
		->AddChild(new UIText(0.4, 0, "Back", UI::Text)));

	ModsScrollBox->AddChild((new UIBackground(true, 0, 1, Vector2f(1.15, 0.005)))
			->SetPadding(0));

	std::string DescriptionText = "By: " + Thunderstore::SelectedMod.Author
		+ " | Downloads: " + std::to_string(Thunderstore::SelectedMod.Downloads)
		+ " | Ratings: " + std::to_string(Thunderstore::SelectedMod.Rating);

	if (Thunderstore::SelectedMod.IsUnknownLocalMod)
	{
		DescriptionText = "By: " + Thunderstore::SelectedMod.Author
			+ " | Downloads: ???"
			+ " | Ratings: ???";
	}

	ModsScrollBox->AddChild((new UIBox(true, 0))
		->SetPadding(0)
		->AddChild((new UIBox(false, 0))
			->SetPadding(0)

			->AddChild((new UIBox(true, 0))

				->AddChild((new UIButton(true, 0, 1, []() {
						if (Thunderstore::SelectedMod.IsUnknownLocalMod)
						{
							std::filesystem::remove_all(Thunderstore::SelectedMod.DownloadUrl);
							Thunderstore::FoundMods = Thunderstore::GetInstalledMods().Combined();
							Thunderstore::LoadedSelectedMod = true;
							return;
						}
						if (!Installer::CurrentBackgroundThread)
						{
							Installer::CurrentBackgroundThread = new std::thread(Thunderstore::InstallMod, Thunderstore::SelectedMod, true);
						}}))
					->SetPadding(0.01, 0.07, 0.01, 0.01)
					->AddChild(new UIText(0.4, 0, IsInstalled ? "Uninstall" : (Game::GamePath.empty() ? "Install (No game path!)" : "Install"), UI::Text)))

				->AddChild((new UIButton(true, 0, 1, []() {
							system(("start https://northstar.thunderstore.io/package/"
								+ Thunderstore::SelectedMod.Namespace
								+ "/"
								+ Thunderstore::SelectedMod.Name).c_str());
					}))
					->SetPadding(0.01, 0.07, 0.01, 0.01)
					->AddChild(new UIText(0.4, 0, "Open In Browser", UI::Text))))

			->AddChild(new UIText(0.4, 1, DescriptionText, UI::Text))
			->AddChild(new UIText(0.7, 1, Thunderstore::SelectedMod.Name + (IsInstalled ? " (Installed)" : ""), UI::Text)))

		->AddChild((new UIBackground(true, 0, 1, Vector2(0.35)))
			->SetUseTexture(true, Texture::LoadTexture(Thunderstore::SelectedMod.Img))
			->SetPadding(0.02, 0.02, 0.1, 0.02)
			->SetSizeMode(UIBox::E_PIXEL_RELATIVE)));


	ModsScrollBox->AddChild((new UIBackground(true, 0, 1, Vector2f(1.15, 0.005)))
		->SetPadding(0, 0.05, 0, 0));

	UIBox* MarkdownBackground = new UIBox(false, 0);
	MarkdownBackground->SetPadding(0);
	MarkdownBackground->Align = UIBox::E_REVERSE;
	ModsScrollBox->AddChild(MarkdownBackground);

	Markdown::RenderMarkdown(Thunderstore::SelectedMod.Description, 1.1, MarkdownBackground, 1, UI::Text);
}

void ModsTab::GenerateModPage()
{
	UpdateClickedCategoryButton();
	ModImages.clear();
	ModsScrollBox->GetScrollObject()->Percentage = 0;
	ModsScrollBox->SetMaxScroll(10);
	ModsScrollBox->DeleteChildren();

	ClearLoadedTextures();

	std::vector<UIBox*> Rows;

	SearchBar = new UITextField(true, 0, 0, UI::Text, []() 
		{				
			if (CurrentModsTab->Filter != CurrentModsTab->SearchBar->GetText())
			{
				CurrentModsTab->Filter = CurrentModsTab->SearchBar->GetText();
				CurrentModsTab->SelectedPage = 0;
				CurrentModsTab->LoadedModList = false;
				CurrentModsTab->ShowLoadingText();
			}
		});

	SearchBar->SetTextSize(0.3);
	SearchBar->SetMinSize(Vector2f(0.6, 0.02));
	SearchBar->SetHintText("Search");
	SearchBar->SetText(Filter);
	ModsScrollBox->AddChild(SearchBar);

	for (size_t i = 0; i < 20; i++)
	{
		auto NewRow = new UIBox(true, 0);
		NewRow->SetPadding(0.001);
		Rows.push_back(NewRow);
		ModsScrollBox->AddChild(NewRow);
	}

	size_t SlotsPerRow = std::round((Application::AspectRatio / (16.0f / 9.0f)) * 6.9f);

	size_t it = 0;
	ModButtons.clear();
	for (const auto& i : Thunderstore::FoundMods)
	{
		bool UseTexture = std::filesystem::exists(i.Img) && !i.IsNSFW;
		UIBackground* Image = new UIBackground(false, 0, 1, Vector2(0.25));
		UIButton* b = new UIButton(false, 0, 1, []() {
			for (size_t i = 0; i < ModButtons.size(); i++)
			{
				if (ModButtons[i]->GetIsHovered())
				{
					const auto& m = Thunderstore::FoundMods[i];

					if (m.IsUnknownLocalMod)
					{
						Thunderstore::SelectedMod = m;
						Thunderstore::LoadedSelectedMod = true;
						return;
					}

					if (!Installer::CurrentBackgroundThread)
					{
						CurrentModsTab->ShowLoadingText();
						Installer::CurrentBackgroundThread = new std::thread(Thunderstore::GetModInfo, m);
					}
					else
					{
						Installer::CurrentBackgroundThread->join();
						delete Installer::CurrentBackgroundThread;
						Installer::CurrentBackgroundThread = nullptr;
						Installer::ThreadProgress = 0;
						CurrentModsTab->ShowLoadingText();
						Installer::CurrentBackgroundThread = new std::thread(Thunderstore::GetModInfo, m);
					}
				}
			}
			});

		b->SetMinSize(Vector2f(0, 0.34));
		b->Align = UIBox::E_REVERSE;
		b->SetPadding(0.005 * Application::AspectRatio, 0.005 * Application::AspectRatio, 0.005, 0.005);
		unsigned int tex = 0;
		if (UseTexture)
		{
			tex = Texture::LoadTexture(i.Img);
			ModTextures.push_back(tex);
		}
		auto NameText = new UIText(0.225, 0, i.Name, UI::Text);
		Rows[it++ / SlotsPerRow]->AddChild(b
			->SetBorder(UIBox::E_ROUNDED, 0.5)
			->AddChild(Image
				->SetUseTexture(UseTexture, tex)
				->SetPadding(0)
				->SetSizeMode(UIBox::E_PIXEL_RELATIVE))
			->AddChild(NameText
					->SetPadding(0.005)));

		NameText->Wrap = true;
		NameText->WrapDistance = 0.8 / Application::AspectRatio;

		if (!UseTexture)
		{
			Image->Align = UIBox::E_CENTERED;
			Image->SetColor(0.1);
			Image->AddChild(new UIText(0.4, 1, i.IsUnknownLocalMod ? " Unknown" : (i.IsNSFW ? "    NSFW" : "Loading..."), UI::Text));
		}
		ModImages.push_back(Image);
		ModButtons.push_back(b);
	}

	Rows[19]->AddChild(new UIText(0.4, 1, "Pages: ", UI::Text));
	PageButtons.clear();
	for (int i = 0; i < 6; i++)
	{
		auto b = new UIButton(true, 0, SelectedPage == (i) ? 0.5 : 1, []() {			
		for (size_t i = 0; i < PageButtons.size(); i++)
		{
			if (PageButtons[i]->GetIsHovered() && !Installer::CurrentBackgroundThread)
			{
				CurrentModsTab->ShowLoadingText();
				CurrentModsTab->SelectedPage = i;
				CurrentModsTab->LoadedModList = false;
			}
		} });
		Rows[19]->AddChild(b
			->SetPadding(0.005)
			->AddChild(new UIText(0.4, 0, std::to_string(i + 1), UI::MonoText)));
		PageButtons.push_back(b);
	}
}

void ModsTab::ShowLoadingText()
{
	ModButtons.clear();
	ModImages.clear();

	ModsScrollBox->DeleteChildren();
	ModsScrollBox->AddChild((new UIText(0.5, 1, "Loading...", UI::Text))->SetPadding(0.5));
	UpdateClickedCategoryButton();
}

void ModsTab::UpdateClickedCategoryButton()
{
	for (size_t i = 0; i < CategoryButtons.size(); i++)
	{
		if (Thunderstore::CategoryFunctions[i].o == Thunderstore::SelectedOrdering)
		{
			CategoryButtons[i]->SetColor(0.5);
		}
		else
		{
			CategoryButtons[i]->SetColor(1);
		}
	}
}

void ModsTab::GenerateModImages()
{
	ClearLoadedTextures();
	for (size_t i = 0; i < ModImages.size(); i++)
	{
		if (Thunderstore::FoundMods.size() <= i)
		{
			break;
		}		
		bool UseTexture = std::filesystem::exists(Thunderstore::FoundMods[i].Img) && !Thunderstore::FoundMods[i].IsNSFW;
		if (UseTexture)
		{
			unsigned int tex = Texture::LoadTexture(Thunderstore::FoundMods[i].Img);
			ModTextures.push_back(tex);
			ModImages[i]->SetUseTexture(true, tex);
			ModImages[i]->SetColor(1);
			ModImages[i]->DeleteChildren();
		}
	}
}

void ModsTab::CheckForModUpdates()
{
	using namespace nlohmann;

	Installer::BackgroundName = "Checking for mod updates";
	Installer::BackgroundTask = "Checking for mod updates";
	Installer::ThreadProgress = 0;

	std::vector<Thunderstore::Package> Mods = Thunderstore::GetInstalledMods().Managed;

	size_t it = 0;
	for (const auto& m : Mods)
	{
		Networking::Download("https://thunderstore.io/api/experimental/frontend/c/northstar/p/" + m.Namespace + "/" + m.Name, "Data/temp/net/mod.txt", "");

		try
		{
			std::ifstream in = std::ifstream("Data/temp/net/mod.txt");
			std::stringstream str; str << in.rdbuf();
			json response = json::parse(str.str());
			if (m.Version != response.at("versions")[0].at("version_number").get<std::string>())
			{
				Log::Print("Mod '" + m.Name + "' is outdated!", Log::Warning);
				Log::Print(response.at("versions")[0].at("version_number").get<std::string>() + " != " + m.Version, Log::Warning);
				// Uninstall mod, then install mod again. (InstallMod() uninstalls a mod if it's already installed)

				Thunderstore::Package NewMod = m;
				NewMod.DownloadUrl = response.at("download_url");
				Thunderstore::InstallMod(NewMod, false);
				Thunderstore::InstallMod(NewMod, false);
			}
			else
			{
				Log::Print("Mod '" + m.Name + "' is not outdated.");
			}
		}
		catch (std::exception& e)
		{
			Log::Print("Thunderstore response has an invalid layout.", Log::Error);
			Log::Print(e.what(), Log::Error);

			Log::Print("Writing response to Data/temp/invalidresponse.txt", Log::Error);
			std::filesystem::copy("Data/temp/net/mod.txt", "Data/temp/invalidresponse.txt");
		}
		Installer::ThreadProgress += (float)it / (float)Mods.size();
		Installer::ThreadProgress = std::min(0.95f, Installer::ThreadProgress);
		it++;
	}
	Installer::ThreadProgress = 1;
}

ModsTab::ModsTab()
{
	PrevAspectRatio = Application::AspectRatio;
	CurrentModsTab = this;
	Name = "Mods";

	Background->Align = UIBox::E_CENTERED;
	Background->SetHorizontal(true);

	ModsBackground = new UIBackground(false, 0, 0, Vector2f(1.2, 1.85));
	Background->AddChild(ModsBackground
		->SetOpacity(0.5)
		->AddChild((new UIBackground(true, 0, 1, Vector2f(1.2, 0.005)))
			->SetPadding(0))
		->SetPadding(0));
	ModsBackground->Align = UIBox::E_REVERSE;

	auto TitleBox = new UIBox(true, 0);
	ModsBackground->AddChild(TitleBox);
	TitleBox->SetPadding(0);
	TitleBox->AddChild((new UIText(0.8, 1, "Mods", UI::Text))->SetPadding(0.01, 0.01, 0.01, 0.05));
	ModsScrollBox = new UIScrollBox(false, 0, 25);
	ModsScrollBox->Align = UIBox::E_REVERSE;

	ModsScrollBox->SetMinSize(Vector2f(1.15, 1.7));
	ModsScrollBox->SetMaxSize(Vector2f(1.15, 1.7));
	ShowLoadingText();
	ModsBackground->AddChild(ModsScrollBox);


	for (auto& i : Thunderstore::CategoryFunctions)
	{
		auto b = new UIButton(true, 0, 1, []() {
			for (size_t i = 0; i < CategoryButtons.size(); i++)
			{
				if (CategoryButtons[i]->GetIsHovered() && !Installer::CurrentBackgroundThread)
				{
					Thunderstore::SelectedOrdering = Thunderstore::CategoryFunctions[i].o;
					CurrentModsTab->SelectedPage = 0;
					CurrentModsTab->LoadedModList = false;
					CurrentModsTab->ShowLoadingText();
				}
			}
			});

		TitleBox->AddChild(b
			->SetPadding(0, 0.025, 0.001, 0.001)
			->AddChild(new UIText(0.3, 0, i.Name, UI::Text)));

		CategoryButtons.push_back(b);
	}
}

void ModsTab::ClearLoadedTextures()
{
	for (auto i : ModTextures)
	{
		Texture::UnloadTexture(i);
	}
}

int ModsTab::GetModsPerPage(float Aspect)
{
	return std::round((Aspect / (16.0f / 9.0f)) * 6.9f) * 4;;
}

void ModsTab::Tick()
{
	ModsPerPage = GetModsPerPage(Application::AspectRatio);
	if (!LoadedModList && Background->IsVisible && !Thunderstore::IsDownloading && !Installer::CurrentBackgroundThread)
	{
		Installer::CurrentBackgroundThread = new std::thread(Thunderstore::DownloadThunderstoreInfo, Thunderstore::SelectedOrdering, SelectedPage, Filter);
		DownloadingPage = true;
		LoadedModList = true;
		Thunderstore::IsDownloading = true;
	}
	if (DownloadingPage && !Thunderstore::IsDownloading)
	{
		DownloadingPage = false;
		GenerateModPage();
	}
	if (Thunderstore::LoadedImages)
	{
		GenerateModImages();
		Thunderstore::LoadedImages = false;
	}
	if (Thunderstore::LoadedSelectedMod)
	{
		GenerateModInfo();
		Thunderstore::LoadedSelectedMod = false;
	}

	if (PrevAspectRatio != Application::AspectRatio && ModButtons.size() && GetModsPerPage(Application::AspectRatio) != GetModsPerPage(PrevAspectRatio))
	{
		if (!Installer::CurrentBackgroundThread)
		{
			ShowLoadingText();
			Installer::CurrentBackgroundThread = new std::thread(Thunderstore::DownloadThunderstoreInfo, Thunderstore::SelectedOrdering, SelectedPage, Filter);
			DownloadingPage = true;
			LoadedModList = true;
			Thunderstore::IsDownloading = true;
			PrevAspectRatio = Application::AspectRatio;
		}
	}
	else
	{
		PrevAspectRatio = Application::AspectRatio;
	}
}

ModsTab::~ModsTab()
{
}
