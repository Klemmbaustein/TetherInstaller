#include "ModsTab.h"

#include <fstream>
#include <regex>
#include <iomanip>

#include <KlemmUI/Rendering/Texture.h>
#include <KlemmUI/Application.h>
#include <KlemmUI/Rendering/MarkdownRendering.h>

#include "nlohmann/json.hpp"

#include "../UI/Icon.h"
#include "../UI/UIDef.h"
#include "../Networking.h"
#include "../Installer.h"
#include "../Log.h"
#include "../Game.h"
#include "../Thunderstore.h"
#include "../BackgroundTask.h"
#include "../Translation.h"
using namespace Translation;

std::vector<UIButton*> ModsTab::ModButtons;
std::vector<UIButton*> ModsTab::PageButtons;
std::vector<UIButton*> ModsTab::CategoryButtons;

std::atomic<unsigned int> ModsTab::ModsPerPage = 20;
ModsTab* ModsTab::CurrentModsTab = nullptr;

namespace Thunderstore
{
	std::vector<Thunderstore::Category> TSCategoryNames =
	{
		Thunderstore::Category("installed", Thunderstore::Ordering::Installed),
		Thunderstore::Category("last_updated", Thunderstore::Ordering::Last_Updated),
		Thunderstore::Category("latest", Thunderstore::Ordering::Newest),
		Thunderstore::Category("most_downloaded", Thunderstore::Ordering::Most_Downloaded),
		Thunderstore::Category("top_rated", Thunderstore::Ordering::Top_Rated),
	};

	size_t CurrentlyLoadedMod = 0;
}

UIBox* ModsTab::GenerateModInfoText(std::vector<std::string> Text, Vector3f32 Color, std::string IconName, double IconPadding)
{
	UIBox* TextBox = new UIBox(false, 0);

	ModsScrollBox->AddChild((new UIBackground(true, 0, Color, Vector2f(1, 0)))
		->SetBorder(UIBox::BorderType::Rounded, 0.25)
		->SetPadding(0.0, 0.03, 0.06, 0)
		->AddChild((new UIBackground(true, 0, 1, 0.075))
			->SetUseTexture(true, Icon(IconName).TextureID)
			->SetPadding(0.01, IconPadding, 0.01, 0.01)
			->SetSizeMode(UIBox::SizeMode::AspectRelative))
		->AddChild(TextBox
			->SetPadding(0.01, 0.01, 0.01, 0.01)));

	for (const auto& i : Text)
	{
		TextBox->AddChild((new UIText(0.35, 1, i, UI::Text))
			->SetWrapEnabled(true, 0.5, UIBox::SizeMode::ScreenRelative)
			->SetPadding(0.0025, 0.0025, 0.01, 0.01));
	}
	return TextBox;
}

void ModsTab::GenerateModInfo()
{
	IsInModInfo = true;
	UpdateClickedCategoryButton();

	Log::Print("Displaying info for mod: " + Thunderstore::SelectedMod.Name);

	bool IsInstalled = Thunderstore::IsModInstalled(Thunderstore::SelectedMod);

	ModsScrollBox->GetScrollObject()->Percentage = 0;
	ModsScrollBox->SetDisplayScrollBar(true);
	ModButtons.clear();
	ModsScrollBox->DeleteChildren();

	ModsScrollBox->AddChild((new UIButton(true, 0, 1, []() 
		{
		if (Thunderstore::SelectedOrdering == Thunderstore::Ordering::Installed)
		{
			Thunderstore::DownloadThunderstoreInfo(Thunderstore::SelectedOrdering, Thunderstore::CurrentlyLoadedPageID, CurrentModsTab->Filter, true);
			CurrentModsTab->DownloadingPage = true;
			CurrentModsTab->LoadedModList = true;
			Thunderstore::IsDownloading = true;
		}
		else
		{
			CurrentModsTab->GenerateModPage();
		}
		}))
		->SetBorder(UIBox::BorderType::Rounded, 0.25)
		->AddChild((new UIBackground(true, 0, 0, 0.055))
			->SetUseTexture(true, Icon("Back").TextureID)
			->SetPadding(0.01, 0.01, 0.01, 0)
			->SetSizeMode(UIBox::SizeMode::AspectRelative))
		->AddChild(new UIText(0.4, 0, GetTranslation("mod_back"), UI::Text)));

	ModsScrollBox->AddChild((new UIBackground(true, 0, 1, Vector2f(1.15, 0.005)))
			->SetPadding(0));

	std::string DescriptionText = Format(GetTranslation("mod_info_string"), Thunderstore::SelectedMod.Author.c_str(),
		std::to_string(Thunderstore::SelectedMod.Downloads).c_str(),
		std::to_string(Thunderstore::SelectedMod.Rating).c_str());

	if (Thunderstore::SelectedMod.IsUnknownLocalMod)
	{
		DescriptionText = Format(GetTranslation("mod_info_string"), Thunderstore::SelectedMod.Author.c_str(),
			"???",
			"???");
	}

	UIBox* ModActionsBox = new UIBox(true, 0);

	if (ModPreviewTexture)
	{
		Texture::UnloadTexture(ModPreviewTexture);
	}

	ModPreviewTexture = Texture::LoadTexture(Thunderstore::SelectedMod.Img);

	ModsScrollBox->AddChild((new UIBox(true, 0))
		->SetPadding(0)
		->AddChild((new UIBox(false, 0))
			->SetPadding(0)

			->AddChild((new UIText(0.7, 1, Thunderstore::SelectedMod.Name + (IsInstalled ? " (Installed)" : ""), UI::Text))
				->SetWrapEnabled(true, 0.2, UIBox::SizeMode::ScreenRelative))

			->AddChild((new UIText(0.4, 1, DescriptionText, UI::Text))
				->SetWrapEnabled(true, 0.35f, UIBox::SizeMode::ScreenRelative))

			->AddChild(ModActionsBox))

		->AddChild((new UIBackground(true, 0, 1, Vector2(0.35)))
			->SetUseTexture(true, ModPreviewTexture)
			->SetPadding(0.02, 0.02, 0.1, 0.02)
			->SetSizeMode(UIBox::SizeMode::AspectRelative)));

	ModActionsBox->AddChild((new UIButton(true, 0, 1, []() {
		if (Thunderstore::SelectedMod.IsUnknownLocalMod)
		{
			std::filesystem::remove_all(Thunderstore::SelectedMod.DownloadUrl);
			Thunderstore::FoundMods = Thunderstore::GetInstalledMods().Combined();
			Thunderstore::LoadedSelectedMod = true;
			return;
		}
		new BackgroundTask([]() {
			if (!Thunderstore::IsModInstalled(Thunderstore::SelectedMod))
			{
				BackgroundTask::SetStatus("dl_" + Format(GetTranslation("download_install_mod"), Thunderstore::SelectedMod.Name.c_str()));
			}
			Thunderstore::InstallOrUninstallMod(Thunderstore::SelectedMod, false, false);
			},
			[]() {
				if (!Thunderstore::IsModInstalled(Thunderstore::SelectedMod) && Thunderstore::SelectedOrdering == Thunderstore::Ordering::Installed)
				{
					Thunderstore::DownloadThunderstoreInfo(Thunderstore::SelectedOrdering, Thunderstore::CurrentlyLoadedPageID, CurrentModsTab->Filter, true);
					CurrentModsTab->DownloadingPage = true;
					CurrentModsTab->LoadedModList = true;
					Thunderstore::IsDownloading = true;
				}
				else if (CurrentModsTab->IsInModInfo)
				{
					Thunderstore::LoadedSelectedMod = true;
				}
			});
		}))
		->SetPadding(0.01, 0.07, 0.01, 0.01)
		->SetBorder(UIBox::BorderType::Rounded, 0.25)
		->AddChild((new UIBackground(true, 0, 0, 0.055))
			->SetUseTexture(true, IsInstalled ? Icon("Delete").TextureID : Icon("Download").TextureID)
			->SetPadding(0.01, 0.01, 0.01, 0)
			->SetSizeMode(UIBox::SizeMode::AspectRelative))
		->AddChild(new UIText(0.4, 0,
			(Thunderstore::IsInstallingMod) ?
			GetTranslation("mod_installing")
			: (IsInstalled ? GetTranslation("mod_uninstall") : (Game::GamePath.empty() ? GetTranslation("mod_install_no_path") : GetTranslation("mod_install"))),
			UI::Text)));

	bool IsEnabled = true; 

	if (IsInstalled && Thunderstore::SelectedMod.Name != "VanillaPlus")
	{
		IsEnabled = Thunderstore::GetModEnabled(Thunderstore::SelectedMod); 
		
		ModActionsBox->AddChild((new UIButton(true, 0, 1, []() {
				Thunderstore::SetModEnabled(Thunderstore::SelectedMod, !Thunderstore::GetModEnabled(Thunderstore::SelectedMod));
				CurrentModsTab->GenerateModInfo();
			}))
			->SetPadding(0.01, 0.07, 0.01, 0.01)
			->SetBorder(UIBox::BorderType::Rounded, 0.25)
			->AddChild((new UIBackground(true, 0, 0, 0.055))
				->SetUseTexture(true, IsEnabled ? Icon("Disabled").TextureID : Icon("Enabled").TextureID)
				->SetPadding(0.01, 0.01, 0.01, 0)
				->SetSizeMode(UIBox::SizeMode::AspectRelative))
			->AddChild(new UIText(0.4, 0, IsEnabled ? GetTranslation("mod_disable") : GetTranslation("mod_enable"), UI::Text)));
	}
	else
	{
		ModActionsBox->AddChild((new UIButton(true, 0, 1, []() {
#if _WIN32
			system(("start https://northstar.thunderstore.io/package/"
#else
			system(("xdg-open https://northstar.thunderstore.io/package/"
#endif
				+ Thunderstore::SelectedMod.Namespace
				+ "/"
				+ Thunderstore::SelectedMod.Name).c_str());
			}))
			->SetPadding(0.01, 0.07, 0.01, 0.01)
			->SetBorder(UIBox::BorderType::Rounded, 0.25)
			->AddChild((new UIBackground(true, 0, 0, 0.055))
				->SetUseTexture(true, Icon("Open").TextureID)
				->SetPadding(0.01, 0.01, 0.01, 0)
				->SetSizeMode(UIBox::SizeMode::AspectRelative))
			->AddChild(new UIText(0.4, 0, GetTranslation("mod_open_browser"), UI::Text)));
	}
	if (Thunderstore::SelectedMod.IsDeprecated)
	{
		GenerateModInfoText(
			{
			NewLineStringToStringArray(GetTranslation("mod_deprecation_warning"))
			}, Vector3f32(0.6f, 0.15f, 0), "Settings/Warning");
	}

	if (IsInstalled)
	{
		if (IsEnabled)
		{
			GenerateModInfoText({ GetTranslation("mod_is_enabled") }, Vector3f32(0, 0.5f, 0.25f), "Enabled");
		}
		else
		{
			GenerateModInfoText({ GetTranslation("mod_is_disabled") }, Vector3f32(0.6f, 0.15f, 0), "Disabled");
		}
	}

	if (Thunderstore::SelectedMod.Name == "NorthstarReleaseCandidate")
	{
		GenerateModInfoText(
			{
				NewLineStringToStringArray(GetTranslation("mod_release_canidate_message"))
			}, Vector3f32(0.1f, 0.2f, 0.6f), "Settings/About", 0.15)
			->AddChild((new UIButton(true, 0, 1, []() {system("start https://discord.com/channels/920776187884732556/951461326478262292"); }))
				->SetBorder(UIBox::BorderType::Rounded, 0.25)
				->AddChild((new UIBackground(true, 0, 0, 0.04))
					->SetUseTexture(true, Icon("Open").TextureID)
					->SetPadding(0.01, 0.01, 0.01, 0)
					->SetSizeMode(UIBox::SizeMode::AspectRelative))
				->AddChild(new UIText(0.3, 0, GetTranslation("mod_release_canidate_open_discord"), UI::Text)));
	}

	if (Thunderstore::SelectedMod.Name == "VanillaPlus")
	{
		GenerateModInfoText(
			{
				NewLineStringToStringArray(GetTranslation("mod_vanillaplus_message"))

			}, Vector3f32(0.1f, 0.2f, 0.6f), "Settings/About", 0.1);
	}

	ModsScrollBox->AddChild((new UIBackground(true, 0, 1, Vector2f(1.15, 0.005)))
		->SetPadding(0, 0.05, 0, 0));

	UIBox* MarkdownBackground = new UIBox(false, 0);
	MarkdownBackground->SetPadding(0);
	ModsScrollBox->AddChild(MarkdownBackground);

	Markdown::MarkdownStyling Style;
	Style.Text = UI::Text;
	Style.Width = 1.1;
	Markdown::MarkdownStyling::CodeStyling CodeStyle;
	CodeStyle.BackgroundColor = 0;
	CodeStyle.Color = 1;
	CodeStyle.Rounding = 0.25f;
	CodeStyle.CodeText = UI::MonoText;
	Style.Code = CodeStyle;

	Markdown::RenderMarkdown(Thunderstore::SelectedMod.Description, MarkdownBackground, Style);
}

void ModsTab::GenerateModPage()
{
	IsInModInfo = false;
	UpdateClickedCategoryButton();
	ModImages.clear();
	ModsScrollBox->GetScrollObject()->Percentage = 0;
	ModsScrollBox->DeleteChildren();
	ModsScrollBox->SetDisplayScrollBar(false);
	ClearLoadedTextures();

	std::vector<UIBox*> Rows;

	if (Thunderstore::SelectedOrdering != Thunderstore::Ordering::Installed)
	{
		SearchBar = new UITextField(true, 0, 0, UI::Text, []()
			{
				if (CurrentModsTab->Filter != CurrentModsTab->SearchBar->GetText() && !Thunderstore::IsDownloading)
				{
					CurrentModsTab->Filter = CurrentModsTab->SearchBar->GetText();
					CurrentModsTab->SelectedPage = 0;
					CurrentModsTab->LoadedModList = false;
					CurrentModsTab->ShowLoadingText();
				}
			});

		SearchBar->SetTextSize(0.3);
		SearchBar->SetMinSize(Vector2f(0.6, 0.02));
		SearchBar->SetHintText(GetTranslation("search"));
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
		UIBackground* Image = new UIBackground(false, 0, 1, Vector2f(0.25));
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

		if (Thunderstore::SelectedOrdering == Thunderstore::Ordering::Installed && !Thunderstore::GetModEnabled(i))
		{
			b->SetColor(Vector3f32(1, 0.75, 0));
			b->SetHoveredColor(Vector3f32(1, 0.75, 0) * 0.75f);
			b->SetPressedColor(Vector3f32(1, 0.75, 0) * 0.5f);
		}

		b->SetMinSize(Vector2f(0, 0.34));
		b->SetMaxSize(Vector2f(1, 0.34));
		b->SetPadding(0.005 * Application::AspectRatio, 0.005 * Application::AspectRatio, 0.005, 0.005);
		unsigned int tex = 0;
		if (UseTexture)
		{
			tex = Texture::LoadTexture(i.Img);
			ModTextures.push_back(tex);
		}

		auto NameText = new UIText(0.225, 0, i.Name, UI::Text);
		Rows[it++ / SlotsPerRow]->AddChild(b
			->SetBorder(UIBox::BorderType::Rounded, 0.5)
			->AddChild((new UIBackground(false, 0, 0.1))
				->SetPadding(0)
				->AddChild(Image
					->SetUseTexture(UseTexture, tex)
					->SetPadding(0)
					->SetSizeMode(UIBox::SizeMode::AspectRelative)))
			->AddChild(NameText
				->SetPaddingSizeMode(UIBox::SizeMode::AspectRelative)
				->SetPadding(0.005)));

		NameText->Wrap = true;
		NameText->WrapDistance = 0.19f / Application::AspectRatio;

		if (!UseTexture)
		{
			Image->SetVerticalAlign(UIBox::Align::Centered);
			Image->SetHorizontalAlign(UIBox::Align::Centered);
			Image->SetColor(0.1);
			Image->AddChild(new UIText(0.4, 1, i.IsUnknownLocalMod ? GetTranslation("mod_unknown") 
				: (i.IsNSFW ? GetTranslation("mod_nsfw") : (i.IsTemporary ? GetTranslation("mod_temporary") : GetTranslation("loading"))), UI::Text));
		}
		ModImages.push_back(Image);
		ModButtons.push_back(b);
	}

	PageButtons.clear();
	if (Thunderstore::SelectedOrdering == Thunderstore::Ordering::Installed)
	{
		return;
	}
	Rows[19]->AddChild(new UIText(0.4, 1, GetTranslation("mod_pages"), UI::Text));
	for (int i = 0; i < 6; i++)
	{
		auto b = new UIButton(true, 0, SelectedPage == (i) ? 0.5 : 1, []() {
			for (size_t i = 0; i < PageButtons.size(); i++)
			{
				if (PageButtons[i]->GetIsHovered() && !Thunderstore::IsDownloading)
				{
					CurrentModsTab->ShowLoadingText();
					CurrentModsTab->SelectedPage = i;
					CurrentModsTab->LoadedModList = false;
				}
			} });

		if (i == SelectedPage)
		{
			b->SetColor(Installer::GetThemeColor());
			b->SetHoveredColor(Installer::GetThemeColor());
			b->SetPressedColor(Installer::GetThemeColor());
		}
		else
		{
			b->SetColor(Vector3f32(1.0f, 1.0f, 1.0f));
			b->SetHoveredColor(Vector3f32::Lerp(1.0f, Installer::GetThemeColor(), 0.5f));
			b->SetPressedColor(Installer::GetThemeColor());
		}

		Rows[19]->AddChild(b
			->SetPadding(0.005)
			->SetBorder(UIBox::BorderType::Rounded, 0.25)
			->AddChild(new UIText(0.4, 0, std::to_string(i + 1), UI::MonoText)));
		PageButtons.push_back(b);
	}
}

void ModsTab::ShowLoadingText()
{
	ModButtons.clear();
	ModImages.clear();

	ModsScrollBox->DeleteChildren();
	ModsScrollBox->AddChild((new UIText(0.5, 1, GetTranslation("loading"), UI::Text))->SetPadding(0.5));
	UpdateClickedCategoryButton();
}

void ModsTab::Reload()
{
	CurrentModsTab->ShowLoadingText();
	CurrentModsTab->LoadedModList = false;
}

void ModsTab::OnTranslationChanged()
{
	size_t i = 0;
	for (auto& cat : Thunderstore::TSCategoryNames)
	{
		CategoryButtons[i]->DeleteChildren();
		CategoryButtons[i]->AddChild(new UIText(0.3, 0, GetTSOrderingName(cat.Name), UI::Text));
		i++;
	}

	if (ModButtons.size())
	{
		GenerateModPage();
	}
	else
	{
		GenerateModInfo();
	}
}

void ModsTab::UpdateClickedCategoryButton()
{
	for (size_t i = 0; i < CategoryButtons.size(); i++)
	{
		if (Thunderstore::TSCategoryNames[i].o == Thunderstore::SelectedOrdering)
		{
			CategoryButtons[i]->SetColor(Installer::GetThemeColor());
			CategoryButtons[i]->SetHoveredColor(Installer::GetThemeColor());
			CategoryButtons[i]->SetPressedColor(Installer::GetThemeColor());
		}
		else
		{
			CategoryButtons[i]->SetColor(Vector3f32(1.0f, 1.0f, 1.0f));
			CategoryButtons[i]->SetHoveredColor(Vector3f32::Lerp(1.0f, Installer::GetThemeColor(), 0.5f));
			CategoryButtons[i]->SetPressedColor(Installer::GetThemeColor());
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
		if (m.UUID.empty())
		{
			Thunderstore::InstallOrUninstallMod(m, false, false);
			continue;
		}
		Networking::Download("https://thunderstore.io/c/northstar/api/v1/package/" + m.UUID,
			"Data/temp/net/mod.json",
			"UserAgent: " + Installer::UserAgent);
		float Progress = 0;
		try
		{
			if (m.IsTemporary)
			{
				Thunderstore::InstallOrUninstallMod(m, true, false);
				continue;
			}
			std::ifstream in = std::ifstream("Data/temp/net/mod.json");
			std::stringstream str; str << in.rdbuf();
			json response = json::parse(str.str());
			if (m.Version != response.at("versions")[0].at("version_number").get<std::string>() || !Thunderstore::IsMostRecentFileVersion(m.FileVersion))
			{
				Log::Print("Mod '" + m.Name + "' is outdated!", Log::Warning);
				Log::Print(response.at("versions")[0].at("version_number").get<std::string>() + " != " + m.Version, Log::Warning);

				// Uninstall mod, then install mod again.
				Thunderstore::Package NewMod = m;
				NewMod.DownloadUrl = response.at("versions")[0].at("download_url");
				NewMod.Version = response.at("versions")[0].at("version_number").get<std::string>();
				BackgroundTask::SetStatus("dl_Updating " + m.Name);
				Thunderstore::InstallOrUninstallMod(NewMod, true, false);
				Thunderstore::InstallOrUninstallMod(NewMod, false, false);
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
			if (std::filesystem::exists("Data/temp/net/mod.json"))
			{
				if (std::filesystem::exists("Data/temp/invalidresponse.txt"))
				{
					std::filesystem::remove("Data/temp/invalidresponse.txt");
				}
				std::filesystem::copy("Data/temp/net/mod.json", "Data/temp/invalidresponse.txt");
			}
		}
		std::filesystem::remove("Data/temp/net/mod.json");

		Progress += (float)it / (float)Mods.size();
		BackgroundTask::SetProgress(std::min(0.95f, Progress));
		it++;
	}
}

ModsTab::ModsTab()
{
	PrevAspectRatio = Application::AspectRatio;
	CurrentModsTab = this;
	Name = "mods";
	Log::Print("Loading mods tab...");

	Background->SetHorizontalAlign(UIBox::Align::Centered);
	Background->SetHorizontal(true);

	ModsBackground = new UIBackground(false, 0, 0, Vector2f(1.2, 1.85));
	Background->AddChild(ModsBackground
		->SetOpacity(0.65)
		->AddChild((new UIBackground(true, 0, 1, Vector2f(1.2, 0.005)))
			->SetPadding(0))
		->SetPadding(0));
	ModsBackground->SetVerticalAlign(UIBox::Align::Reverse);

	TabTitle = new UIText(0.8, 1, GetTranslation("tab_mods"), UI::Text);
	auto TitleBox = new UIBox(true, 0);
	ModsBackground->AddChild(TitleBox);
	TitleBox->SetPadding(0);
	TitleBox->SetVerticalAlign(UIBox::Align::Default);
	TitleBox->AddChild(TabTitle->SetPadding(0.01, 0.01, 0.01, 0.05));
	ModsScrollBox = new UIScrollBox(false, 0, true);

	ModsScrollBox->SetMinSize(Vector2f(1.15, 1.77));
	ModsScrollBox->SetMaxSize(Vector2f(1.15, 1.77));
	ShowLoadingText();
	ModsBackground->AddChild(ModsScrollBox);


	for (auto& i : Thunderstore::TSCategoryNames)
	{
		auto b = new UIButton(true, 0, 1, []() {
			for (size_t i = 0; i < CategoryButtons.size(); i++)
			{
				if (CategoryButtons[i]->GetIsHovered() && !Thunderstore::IsDownloading)
				{
					Thunderstore::SelectedOrdering = Thunderstore::TSCategoryNames[i].o;
					CurrentModsTab->SelectedPage = 0;
					CurrentModsTab->LoadedModList = false;
					CurrentModsTab->ShowLoadingText();
				}
			}
			});

		TitleBox->AddChild(b
			->SetBorder(UIBox::BorderType::Rounded, 0.25)
			->SetPadding(0, 0.025, 0.001, 0.001)
			->AddChild(new UIText(0.3, 0, GetTSOrderingName(i.Name), UI::Text)));

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

bool InstallingMod = false;
void ModsTab::Tick()
{
	if (!Background->IsVisible)
	{
		return;
	}
	ModsBackground->SetMinSize(Vector2f(1.2, Background->GetUsedSize().Y));
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

	if (Thunderstore::IsInstallingMod)
	{
		if (!InstallingMod)
		{
			GenerateModInfo();
		}
		InstallingMod = true;
	}
	else
	{
		InstallingMod = false;
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