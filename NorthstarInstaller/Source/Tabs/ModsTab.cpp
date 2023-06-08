#include "ModsTab.h"

#include <fstream>
#include <regex>

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

ModsTab* ModsTab::CurrentModsTab = nullptr;

namespace Thunderstore
{
	std::atomic<bool> IsDownloading = false;
	std::atomic<bool> LoadedImages = false;

	struct Package
	{
		std::string Name;
		std::string Description;
		std::string Author;
		std::string Img;
		std::string Namespace;
		std::string DownloadUrl;
		std::string Version;
		std::string PageUrl;
		size_t Downloads = 0;
		size_t Rating = 0;
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

	std::vector<std::string> OrderingStrings =
	{
		"?ordering=newest",
		"?ordering=last-updated",
		"?ordering=most-downloaded",
		"?ordering=top-rated",
		"?ordering=none"
	};

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

	std::vector<Package> GetInstalledMods()
	{
		using namespace nlohmann;

		if (!std::filesystem::exists("Data/var/modinfo"))
		{
			return std::vector<Package>();
		}

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

			InstalledMods.push_back(p);
		}
		return InstalledMods;
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

		if (ModOrdering == Ordering::Installed)
		{
			IsDownloading = true;
			Installer::BackgroundName = "Loading installed mods";
			Installer::BackgroundTask = "Loading installed mods";
			Installer::ThreadProgress = 0.5;
			try
			{
				FoundMods = GetInstalledMods();
			}
			catch (std::exception& e)
			{
				FoundMods.clear();
				Log::Print("Error parsing installed mods: " + std::string(e.what()), Log::General, Log::Error);
			}
			Installer::ThreadProgress = 1;
			IsDownloading = false;
			return;
		}

		try
		{
			if (std::filesystem::exists("temp/net/ts"))
			{
				// Get size of already loaded image folders.
				size_t size = 0;
				for (std::filesystem::recursive_directory_iterator it("temp/net/ts");
					it != std::filesystem::recursive_directory_iterator();
					++it)
				{
					if (!std::filesystem::is_directory(*it))
						size += std::filesystem::file_size(*it);
				}

				// If the already loaded images are larger than 10mb, delete them.
				if (size >= 20 * 1000 * 1000)
				{
					std::filesystem::remove_all("temp/net/ts");
				}
			}
		}
		catch (std::exception& e)
		{
			Log::Print(e.what(), Log::General, Log::Info);
			throw e;
		}

		IsDownloading = true;
		Installer::BackgroundName = "Loading Thunderstore";
		Installer::BackgroundTask = "Loading Thunderstore";
		Installer::ThreadProgress = 0.1;
		Networking::Download("https://thunderstore.io/api/experimental/frontend/c/northstar/packages/"
			+ OrderingStrings[(int)ModOrdering]
			+ "&q=" + Filter
			+ "&page=" + std::to_string(Page),
			"temp/net/tspage.txt", "");

		Installer::ThreadProgress = 0.3;

		std::filesystem::create_directories("temp/net/ts/");


		try
		{
			std::ifstream in = std::ifstream("temp/net/tspage.txt");
			std::stringstream str; str << in.rdbuf();
			auto response = json::parse(str.str());
			FoundMods.clear();
			for (auto& elem : response["packages"])
			{
				Package Mod;
				Mod.Name = elem.at("package_name").get<std::string>();
				Mod.Img = elem.at("image_src").get<std::string>();
				Mod.Namespace = elem.at("namespace").get<std::string>();
				Mod.Author = elem.at("team_name").get<std::string>();
				if (elem.at("is_pinned").get<bool>())
				{
					continue;
				}

				FoundMods.push_back(Mod);
			}

			std::vector<Package> ModsCopy = FoundMods;
			IsDownloading = false;

			for (auto& Elem : FoundMods)
			{
				Installer::ThreadProgress += 0.0275;

				std::string TargetName = "temp/net/ts/" + Elem.Name + ".png";
				// Windows file size monent
				if (!std::filesystem::exists(TargetName) || std::filesystem::file_size(TargetName) < 1000)
				{
					Networking::Download(Elem.Img, TargetName, "X-CSRFToken: nan");
					Elem.Img = TargetName;
					LoadedImages = true;
				}
				Elem.Img = TargetName;
			}
		}
		catch (std::exception& e)
		{
			Log::Print("Thunderstore response has an invalid layout.", Log::Install, Log::Error);
			Log::Print(e.what(), Log::Install, Log::Error);

			Log::Print("Writing response to temp/invalidresponse.txt", Log::Install, Log::Error);
			std::filesystem::copy("temp/net/tspage.txt", "temp/invalidresponse.txt");
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

		Installer::ThreadProgress = 0;
		Installer::BackgroundName = "Loading Thunderstore mod";
		Installer::BackgroundTask = "Loading Thunderstore mod";
		Networking::Download("https://thunderstore.io/api/experimental/frontend/c/northstar/p/" + m.Namespace + "/" + m.Name, "temp/net/mod.txt", "");
		try
		{
			std::ifstream in = std::ifstream("temp/net/mod.txt");
			std::stringstream str; str << in.rdbuf();
			auto response = json::parse(str.str());
			m.Description = response.at("markdown");
			m.Downloads = response.at("download_count");
			m.Rating = response.at("rating_score");
			m.DownloadUrl = response.at("download_url");
			m.Version = response.at("versions")[0].at("version_number").get<std::string>();
		}
		catch (std::exception& e)
		{
			Log::Print("Thunderstore response has an invalid layout.", Log::Install, Log::Error);
			Log::Print(e.what(), Log::Install, Log::Error);

			Log::Print("Writing response to temp/invalidresponse.txt", Log::Install, Log::Error);
			std::filesystem::copy("temp/net/mod.txt", "temp/invalidresponse.txt");
		}
		SelectedMod = m;
		LoadedSelectedMod = true;
		Installer::ThreadProgress = 1;
	}

	// Downloads the given package into "temp/net/{m.Author}.{m.Name}.zip,
	// extracts it's contents into "temp/mod",
	// then extracts the content of the extracted zip file's "mods" Folder into the Titanfall 2
	// mods folder.
	// TODO: Extract other elements of the mod. (I've seen some mods have different Folders that
	// need to be extracted somewhere else)
	void InstallMod(Package m)
	{
		using namespace nlohmann;
		Installer::ThreadProgress = 0;

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
					FoundMods = GetInstalledMods();
				}
				catch (std::exception& e)
				{
					FoundMods.clear();
					Log::Print("Error parsing installed mods: " + std::string(e.what()), Log::General, Log::Error);
				}
				Installer::ThreadProgress = 1;
				return;
			}
			Installer::BackgroundName = "Downloading mod";
			Installer::BackgroundTask = "Downloading mod";
			std::string TargetZipName = "temp/net/" + m.Author + "." + m.Name + ".zip";
			Networking::Download(m.DownloadUrl, TargetZipName, "");

			std::filesystem::remove_all("temp/mod");
			std::filesystem::create_directories("temp/mod");
			Networking::ExtractZip(TargetZipName, "temp/mod/");
			std::filesystem::remove(TargetZipName);
			std::filesystem::copy("temp/mod/mods/", Game::GamePath + "/R2Northstar/mods",
				std::filesystem::copy_options::overwrite_existing
				| std::filesystem::copy_options::recursive);

			std::vector<std::string> Files;
			for (auto& i : std::filesystem::directory_iterator("temp/mod/mods"))
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
		Installer::ThreadProgress = 1;
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

	ModsScrollBox->AddChild((new UIBox(true, 0))
		->SetPadding(0)
		->AddChild((new UIBox(false, 0))
			->SetPadding(0)

			->AddChild((new UIBox(true, 0))

				->AddChild((new UIButton(true, 0, 1, []() {
						if (!Installer::CurrentBackgroundThread)
						{
							Installer::CurrentBackgroundThread = new std::thread(Thunderstore::InstallMod, Thunderstore::SelectedMod);
						}}))
					->SetPadding(0.01, 0.07, 0.01, 0.01)
					->AddChild(new UIText(0.4, 0, IsInstalled ? "Uninstall" : "Install", UI::Text)))

				->AddChild((new UIButton(true, 0, 1, []() {
							system(("start https://northstar.thunderstore.io/package/"
								+ Thunderstore::SelectedMod.Namespace
								+ "/"
								+ Thunderstore::SelectedMod.Name).c_str());
					}))
					->SetPadding(0.01, 0.07, 0.01, 0.01)
					->AddChild(new UIText(0.4, 0, "Open In Browser", UI::Text))))

			->AddChild(new UIText(0.4, 1, 
				"By: " + Thunderstore::SelectedMod.Author
				+ " | Downloads: " + std::to_string(Thunderstore::SelectedMod.Downloads)
				+ " | Ratings: " + std::to_string(Thunderstore::SelectedMod.Rating), UI::Text))
			->AddChild(new UIText(0.7, 1, Thunderstore::SelectedMod.Name + (IsInstalled ? " (Installed)" : ""), UI::Text)))

		->AddChild((new UIBackground(true, 0, 1, Vector2(0.35)))
			->SetUseTexture(true, Texture::LoadTexture(Thunderstore::SelectedMod.Img))
			->SetPadding(0.02, 0.02, 0.1, 0.02)
			->SetSizeMode(UIBox::E_PIXEL_RELATIVE)));


	ModsScrollBox->AddChild((new UIBackground(true, 0, 1, Vector2f(1.15, 0.005)))
		->SetPadding(0, 0.05, 0, 0));

	// Somewhat fucky markdown parser. TODO: Unfuck.

	UIBox* MarkdownBackground = new UIBox(false, 0);
	MarkdownBackground->SetPadding(0);
	MarkdownBackground->Align = UIBox::E_REVERSE;
	ModsScrollBox->AddChild(MarkdownBackground);

	Markdown::RenderMarkdown(Thunderstore::SelectedMod.Description, MarkdownBackground, 1, UI::Text);
}

void ModsTab::GenerateModPage()
{
	UpdateClickedCategoryButton();

	ModsScrollBox->GetScrollObject()->Percentage = 0;
	ModsScrollBox->SetMaxScroll(10);
	ModsScrollBox->DeleteChildren();

	for (auto i : ModTextures)
	{
		Texture::UnloadTexture(i);
	}

	std::vector<UIBox*> Rows;

	SearchBar = new UITextField(true, 0, 0, UI::Text, []() 
		{				
			if (CurrentModsTab->Filter != CurrentModsTab->SearchBar->GetText())
			{
				CurrentModsTab->Filter = CurrentModsTab->SearchBar->GetText();
				CurrentModsTab->SelectedPage = 1;
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
		bool UseTexture = std::filesystem::exists(i.Img);
		UIBackground* Image = new UIBackground(false, 0, 1, Vector2(0.25));
		UIButton* b = new UIButton(false, 0, 1, []() {
			for (size_t i = 0; i < ModButtons.size(); i++)
			{
				if (ModButtons[i]->GetIsHovered())
				{
					const auto& m = Thunderstore::FoundMods[i];
					if (!Installer::CurrentBackgroundThread)
					{
						CurrentModsTab->ShowLoadingText();
						Installer::CurrentBackgroundThread = new std::thread(Thunderstore::GetModInfo, m);
					}
				}
			}
			});

		b->SetMinSize(Vector2f(0, 0.34));
		b->Align = UIBox::E_REVERSE;
		unsigned int tex = 0;
		if (UseTexture)
		{
			tex = Texture::LoadTexture(i.Img);
			ModTextures.push_back(tex);
		}
		auto NameText = new UIText(0.225, 0, i.Name, UI::Text);
		Rows[it++ / SlotsPerRow]->AddChild(b
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
			Image->AddChild(new UIText(0.4, 1, "Loading...", UI::Text));
		}
		ModButtons.push_back(b);
	}

	Rows[19]->AddChild(new UIText(0.4, 1, "Pages: ", UI::Text));
	PageButtons.clear();
	for (int i = 0; i < 6; i++)
	{
		auto b = new UIButton(true, 0, SelectedPage == (i + 1) ? 0.5 : 1, []() {			
		for (size_t i = 0; i < PageButtons.size(); i++)
		{
			if (PageButtons[i]->GetIsHovered() && !Installer::CurrentBackgroundThread)
			{
				CurrentModsTab->ShowLoadingText();
				CurrentModsTab->SelectedPage = i + 1;
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

ModsTab::ModsTab()
{
	AspectRatio = Application::AspectRatio;
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
					CurrentModsTab->SelectedPage = 1;
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

void ModsTab::Tick()
{
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
		GenerateModPage();
		Thunderstore::LoadedImages = false;
	}
	if (Thunderstore::LoadedSelectedMod)
	{
		GenerateModInfo();
		Thunderstore::LoadedSelectedMod = false;
	}

	if (AspectRatio != Application::AspectRatio)
	{
		if (ModButtons.size())
		{
			AspectRatio = Application::AspectRatio;
			GenerateModPage();
		}
	}
}

ModsTab::~ModsTab()
{
}
