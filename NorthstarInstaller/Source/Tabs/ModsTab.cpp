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
#include "../Thunderstore.h"
#include "../BackgroundTask.h"

std::vector<UIButton*> ModsTab::ModButtons;
std::vector<UIButton*> ModsTab::PageButtons;
std::vector<UIButton*> ModsTab::CategoryButtons;

std::atomic<unsigned int> ModsTab::ModsPerPage = 20;
ModsTab* ModsTab::CurrentModsTab = nullptr;

namespace Thunderstore
{
	std::vector<Thunderstore::Category> TSCategoryNames =
	{
		Thunderstore::Category("Installed", Thunderstore::Ordering::Installed),
		Thunderstore::Category("Last Updated", Thunderstore::Ordering::Last_Updated),
		Thunderstore::Category("Latest", Thunderstore::Ordering::Newest),
		Thunderstore::Category("Most Downloaded", Thunderstore::Ordering::Most_Downloaded),
		Thunderstore::Category("Top Rated", Thunderstore::Ordering::Top_Rated),
	};
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
						new BackgroundTask([]() {
								Thunderstore::InstallOrUninstallMod(Thunderstore::SelectedMod, false, false);
							},
							[]() {
								Thunderstore::LoadedSelectedMod = true;
							});
					}))
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
	if (Thunderstore::SelectedMod.IsDeprecated)
	{
		ModsScrollBox->AddChild((new UIBackground(false, 0, Vector3f32(1, 0.5, 0), Vector2(1.15, 0.1)))
			->SetBorder(UIBox::E_DARKENED_EDGE, 0.2)
			->AddChild((new UIText(0.3, 0, "Deprecated mods are no longer supported and might have issues.", UI::Text))
				->SetPadding(0, 0.01, 0.01, 0.01))
			->AddChild((new UIText(0.4, 0, "This mod is deprecated", UI::Text))
				->SetPadding(0.01, 0, 0.01, 0.01))
			->SetPadding(0.0, 0.03, 0, 0));
	}

	if (Thunderstore::SelectedMod.Name == "NorthstarReleaseCandidate")
	{
		ModsScrollBox->AddChild((new UIBackground(false, 0, Vector3f32(0, 0.5, 1), Vector2(1.15, 0.1)))
			->SetBorder(UIBox::E_DARKENED_EDGE, 0.2)
			->AddChild((new UIBox(true, 0))
				->SetPadding(0)
				->AddChild((new UIButton(true, 0, 1, []() {system("start https://discord.com/channels/920776187884732556/951461326478262292"); }))
					->AddChild(new UIText(0.3, 0, "Open discord channel", UI::Text))))
			->AddChild((new UIText(0.3, 0, "If you notice any issues with it, please report them to github or the northstar discord server.", UI::Text))
				->SetPadding(0, 0.01, 0.01, 0.01))
			->AddChild((new UIText(0.3, 0, "Release canidates are versions of northstar that are almost ready to release.", UI::Text))
				->SetPadding(0, 0, 0.01, 0.01))
			->AddChild((new UIText(0.4, 0, "This is a northstar release canidate", UI::Text))
				->SetPadding(0.01, 0, 0.01, 0.01))
			->SetPadding(0.0, 0.03, 0, 0));
	}

	UIBox* MarkdownBackground = new UIBox(false, 0);
	MarkdownBackground->SetPadding(0);
	MarkdownBackground->Align = UIBox::E_REVERSE;
	ModsScrollBox->AddChild(MarkdownBackground);

	Markdown::RenderMarkdown(Thunderstore::SelectedMod.Description, 1.15, MarkdownBackground, 1, UI::Text);
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

	if (Thunderstore::SelectedOrdering != Thunderstore::Ordering::Installed)
	{
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
	}

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

					CurrentModsTab->ShowLoadingText();
					Thunderstore::GetModInfo(m, true);
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
			Image->AddChild(new UIText(0.4, 1, i.IsUnknownLocalMod ? " Unknown" : (i.IsNSFW ? "    NSFW" : (i.IsTemporary ? "Temporary" : "Loading...")), UI::Text));
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
			if (PageButtons[i]->GetIsHovered())
			{
				CurrentModsTab->ShowLoadingText();
				CurrentModsTab->SelectedPage = i;
				CurrentModsTab->LoadedModList = false;
			}
		} });
		Rows[19]->AddChild(b
			->SetPadding(0.005)
			->SetBorder(UIBox::E_ROUNDED, 0.25)
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
		if (Thunderstore::TSCategoryNames[i].o == Thunderstore::SelectedOrdering)
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

	BackgroundTask::SetStatus("Checking for mod updates");

	std::vector<Thunderstore::Package> Mods = Thunderstore::GetInstalledMods().Managed;

	size_t it = 0;
	for (const auto& m : Mods)
	{
		Networking::Download("https://thunderstore.io/api/experimental/frontend/c/northstar/p/" + m.Namespace + "/" + m.Name, "Data/temp/net/mod.txt", "");
		float Progress = 0;
		try
		{
			std::ifstream in = std::ifstream("Data/temp/net/mod.txt");
			std::stringstream str; str << in.rdbuf();
			json response = json::parse(str.str());
			if (m.Version != response.at("versions")[0].at("version_number") || !Thunderstore::IsMostRecentFileVersion(m.FileVersion))
			{
				Log::Print("Mod '" + m.Name + "' is outdated!", Log::Warning);
				Log::Print(response.at("versions")[0].at("version_number").get<std::string>() + " != " + m.Version, Log::Warning);
				// Uninstall mod, then install mod again.
				Thunderstore::Package NewMod = m;
				NewMod.DownloadUrl = response.at("download_url");
				Thunderstore::InstallOrUninstallMod(NewMod, false, false);
				Thunderstore::InstallOrUninstallMod(NewMod, false, false);
			}
			else
			{
				Log::Print("Mod '" + m.Name + "' is not outdated.");
			}

			if (m.IsTemporary)
			{
				Thunderstore::InstallOrUninstallMod(m, true, false);
			}
		}
		catch (std::exception& e)
		{
			Log::Print("Thunderstore response has an invalid layout.", Log::Error);
			Log::Print(e.what(), Log::Error);

			Log::Print("Writing response to Data/temp/invalidresponse.txt", Log::Error);
			std::filesystem::copy("Data/temp/net/mod.txt", "Data/temp/invalidresponse.txt");
		}
		Progress += (float)it / (float)Mods.size();
		BackgroundTask::SetProgress(std::min(0.95f, Progress));
		it++;
	}
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
		->SetMaxSize(Vector2f(1.2, 1.85))
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


	for (auto& i : Thunderstore::TSCategoryNames)
	{
		auto b = new UIButton(true, 0, 1, []() {
			for (size_t i = 0; i < CategoryButtons.size(); i++)
			{
				if (CategoryButtons[i]->GetIsHovered())
				{
					Thunderstore::SelectedOrdering = Thunderstore::TSCategoryNames[i].o;
					CurrentModsTab->SelectedPage = 0;
					CurrentModsTab->LoadedModList = false;
					CurrentModsTab->ShowLoadingText();
				}
			}
			});

		TitleBox->AddChild(b
			->SetBorder(UIBox::E_ROUNDED, 0.25)
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
	ModTextures.clear();
}

int ModsTab::GetModsPerPage(float Aspect)
{
	return std::round((Aspect / (16.0f / 9.0f)) * 6.9f) * 4;
}

void ModsTab::Tick()
{
	if (!Background->IsVisible)
	{
		return;
	}
	ModsPerPage = GetModsPerPage(Application::AspectRatio);
	if (!LoadedModList && Background->IsVisible && !Thunderstore::IsDownloading)
	{
		Thunderstore::DownloadThunderstoreInfo(Thunderstore::SelectedOrdering, SelectedPage, Filter, true);
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
		ShowLoadingText();
		Thunderstore::DownloadThunderstoreInfo(Thunderstore::SelectedOrdering, SelectedPage, Filter, true);
		DownloadingPage = true;
		LoadedModList = true;
		Thunderstore::IsDownloading = true;
		PrevAspectRatio = Application::AspectRatio;
	}
	else
	{
		PrevAspectRatio = Application::AspectRatio;
	}
}

ModsTab::~ModsTab()
{
}
