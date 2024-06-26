#include "ModsTab.h"

#include <fstream>
#include <regex>
#include <iomanip>

#include <KlemmUI/Rendering/Texture.h>
#include <KlemmUI/Application.h>
#include <KlemmUI/Rendering/MarkdownRendering.h>

#include "nlohmann/json.hpp"

#include "../UI/UIDef.h"
#include "../UI/FullScreenNotify.h"
#include "../Networking.h"
#include "../Installer.h"
#include "../Log.h"
#include "../Game.h"
#include "../Thunderstore.h"
#include "../BackgroundTask.h"
#include "../Translation.h"

#include "../Markup/ModCategoryButton.hpp"
#include "../Markup/ModEntry.hpp"
#include "../Markup/TabElement.hpp"
#include "../Markup/ModPageHeader.hpp"
using namespace Translation;
using namespace KlemmUI;

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

UIBox* ModsTab::GenerateModInfoText(std::vector<std::string> Text, Vector3f Color, std::string IconName, float IconPadding)
{
	UIBox* TextBox = new UIBox(false, 0);

	ModsScrollBox->AddChild((new UIBackground(true, 0, Color, Vector2f(1.1f, 0)))
		->SetBorder(UIBox::BorderType::Rounded, 0.25)
		->SetPadding(0.0, 0.03, 0.1f, 0)
		->AddChild((new UIBackground(true, 0, 1, 35))
			->SetUseTexture(true, IconName + ".png")
			->SetSizeMode(UIBox::SizeMode::PixelRelative)
			->SetPadding(0.01, IconPadding, 0.01, 0.01))
		->AddChild(TextBox
			->SetPadding(0.01, 0.01, 0.01, 0.01)));

	for (const auto& i : Text)
	{
		TextBox->AddChild((new UIText(13.0f, 1, i, UI::Text))
			->SetTextSizeMode(UIBox::SizeMode::PixelRelative)
			->SetWrapEnabled(true, 1.8, UIBox::SizeMode::ScreenRelative)
			->SetPadding(0.0025, 0.0025, 0.01, 0.01));
	}
	return TextBox;
}

void ModsTab::InstallMod()
{
	new BackgroundTask([]() {
		BackgroundTask::SetStatus("dl_" + Format(GetTranslation("download_install_mod"), Thunderstore::SelectedMod.Name.c_str()));
		Thunderstore::InstallOrUninstallMod(Thunderstore::SelectedMod, false, false, true);
		},
		[]() {
			if (ModsTab::CurrentModsTab->IsInModInfo)
			{
				Thunderstore::LoadedSelectedMod = true;
			}
		});
}

void ModsTab::InstallCurrentMod()
{
	if (Thunderstore::SelectedMod.IsUnknownLocalMod)
	{
		std::filesystem::remove_all(Thunderstore::SelectedMod.DownloadUrl);
		Thunderstore::FoundMods = Thunderstore::GetInstalledMods().Combined();
		Thunderstore::LoadedSelectedMod = true;
		return;
	}

	if (Thunderstore::IsModInstalled(Thunderstore::SelectedMod))
	{
		Thunderstore::InstallOrUninstallMod(Thunderstore::SelectedMod, false, false, true);
		if (Thunderstore::SelectedOrdering == Thunderstore::Ordering::Installed)
		{
			Thunderstore::DownloadThunderstoreInfo(Thunderstore::SelectedOrdering, Thunderstore::CurrentlyLoadedPageID, CurrentModsTab->Filter, true);
			CurrentModsTab->DownloadingPage = true;
			CurrentModsTab->LoadedModList = true;
			Thunderstore::IsDownloading = true;
		}
		if (CurrentModsTab->IsInModInfo)
		{
			Thunderstore::LoadedSelectedMod = true;
		}
		return;
	}

	if (Thunderstore::SelectedMod.Dependencies.empty())
	{
		InstallMod();
		return;
	}

	auto DependencyNotify = new FullScreenNotify("mod_dependency_notify_title");

	DependencyNotify->ContentBox
		->AddChild((new UIText(0.6f, 0, Format(GetTranslation("mod_dependency_notify_text"), Thunderstore::SelectedMod.Name.c_str()), UI::Text))
			->SetPadding(0.01f));

	for (const auto& Dep : Thunderstore::SelectedMod.Dependencies)
	{
		DependencyNotify->ContentBox
			->AddChild((new UIText(0.6f, 0, "- " + Dep, UI::Text))
				->SetPadding(0.01f));
	}

	DependencyNotify->AddOptions({
		FullScreenNotify::NotifyOption("mod_dependency_install_all", "Download", []() {
			new BackgroundTask([]() {
				BackgroundTask::SetStatus("dl_" + GetTranslation("mod_dependency_looking_for_deps"));
				auto Deps = Thunderstore::SelectedMod.GetDependencies();

				for (const auto& Dep : Deps)
				{
					Log::Print("Installing dependency for mod '" + Thunderstore::SelectedMod.DependencyString + "': " + Dep.DependencyString);
					if (!Thunderstore::IsModInstalled(Dep))
					{
						BackgroundTask::SetStatus("dl_" + Format(GetTranslation("mod_dependency_installing"), Dep.Name.c_str()));
						Thunderstore::InstallOrUninstallMod(Dep, false, false, false);
					}
				}
				BackgroundTask::SetStatus("dl_" + Format(GetTranslation("download_install_mod"), Thunderstore::SelectedMod.Name.c_str()));
				Thunderstore::InstallOrUninstallMod(Thunderstore::SelectedMod, false, false, true);
			},
			[]() {
				if (ModsTab::CurrentModsTab->IsInModInfo)
				{
					Thunderstore::LoadedSelectedMod = true;
				}
			});
			}),
		FullScreenNotify::NotifyOption("mod_dependency_install_mod", "Download", []() {
			InstallMod();
			}),
		FullScreenNotify::NotifyOption("mod_dependency_install_cancel", "Revert", []() {
			}),
		});
}

void ModsTab::GenerateAvailabilityMessage()
{
	GenerateModInfoText(
		{
			NewLineStringToStringArray(GetTranslation("plugin_mod_not_available"))

		}, Vector3f(0.6f, 0.15f, 0), "Settings/About", 0.1);

}

void ModsTab::GenerateModInfo()
{
	IsInModInfo = true;
	UpdateClickedCategoryButton();

	Log::Print("Displaying info for mod: " + Thunderstore::SelectedMod.Name);

	bool IsInstalled = Thunderstore::IsModInstalled(Thunderstore::SelectedMod);

	ModsScrollBox->GetScrollObject()->Percentage = 0;
	ModsScrollBox->SetDisplayScrollBar(true);
	ModsScrollBox->DeleteChildren();

	ModsScrollBox->UpdateElement();

	auto Header = new ModPageHeader();

	Header->backButton->button->OnClickedFunction = []() {
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
		};

	ModsScrollBox->AddChild(Header);

	std::string DescriptionText = Format(GetTranslation("mod_info_string"), Thunderstore::SelectedMod.Author.c_str(),
		std::to_string(Thunderstore::SelectedMod.Downloads).c_str(),
		std::to_string(Thunderstore::SelectedMod.Rating).c_str());

	if (Thunderstore::SelectedMod.IsUnknownLocalMod)
	{
		DescriptionText = Format(GetTranslation("mod_info_string"), Thunderstore::SelectedMod.Author.c_str(),
			"???",
			"???");
	}

	Header->SetDescription(DescriptionText);
	Header->SetName(Thunderstore::SelectedMod.Name + (IsInstalled ? " (Installed)" : ""));

	if (ModPreviewTexture)
	{
		Texture::UnloadTexture(ModPreviewTexture);
	}

	ModPreviewTexture = Texture::LoadTexture(Thunderstore::SelectedMod.Img);

	Header->modImage->SetUseTexture(true, ModPreviewTexture);

	auto InstallButton = new ImageButton();

	InstallButton->button->OnClickedFunction = &InstallCurrentMod;
	InstallButton->SetImage(IsInstalled ? "Delete.png" : "Download.png");
	InstallButton->SetText(Thunderstore::IsInstallingMod ?
		GetTranslation("mod_installing")
		: (IsInstalled ? GetTranslation("mod_uninstall") : (Game::GamePath.empty() ? GetTranslation("mod_install_no_path") : GetTranslation("mod_install"))));
	Header->actionsBox->AddChild(InstallButton);

	bool IsEnabled = true; 

	if (IsInstalled && Thunderstore::SelectedMod.Name != "VanillaPlus")
	{
		IsEnabled = Thunderstore::GetModEnabled(Thunderstore::SelectedMod); 
		
		auto EnableButton = new ImageButton();
		EnableButton->SetImage(IsEnabled ? "Disabled.png" : "Enabled.png");
		EnableButton->SetText(IsEnabled ? GetTranslation("mod_disable") : GetTranslation("mod_enable"));

		Header->actionsBox->AddChild(EnableButton);
		EnableButton->button->OnClickedFunction = []() {
			Thunderstore::SetModEnabled(Thunderstore::SelectedMod, !Thunderstore::GetModEnabled(Thunderstore::SelectedMod));
			CurrentModsTab->GenerateModInfo();
			};
	}
	else
	{
		auto WebOpenButton = new ImageButton();
		WebOpenButton->SetImage("Open.png");
		WebOpenButton->SetText(GetTranslation("mod_open_browser"));

		Header->actionsBox->AddChild(WebOpenButton);

		WebOpenButton->button->OnClickedFunction = []() {
#if _WIN32
			system(("start https://northstar.thunderstore.io/package/"
#else
			system(("xdg-open https://northstar.thunderstore.io/package/"
#endif
				+ Thunderstore::SelectedMod.Namespace
				+ "/"
				+ Thunderstore::SelectedMod.Name).c_str());
				};
	}
	if (Thunderstore::SelectedMod.IsDeprecated)
	{
		GenerateModInfoText(
			{
			NewLineStringToStringArray(GetTranslation("mod_deprecation_warning"))
			}, Vector3f(0.6f, 0.15f, 0), "Settings/Warning");
	}

	if (IsInstalled)
	{
		if (IsEnabled)
		{
			GenerateModInfoText({ GetTranslation("mod_is_enabled") }, Vector3f(0, 0.5f, 0.25f), "Enabled");
		}
		else
		{
			GenerateModInfoText({ GetTranslation("mod_is_disabled") }, Vector3f(0.6f, 0.15f, 0), "Disabled");
		}
	}

	if (Thunderstore::SelectedMod.Name == "NorthstarReleaseCandidate")
	{
#ifndef TF_PLUGIN
		if (true)
#else
		if (IsInstalled)
#endif
		{
			GenerateModInfoText(
				{
					NewLineStringToStringArray(GetTranslation("mod_release_candidate_message"))
				}, Vector3f(0.1f, 0.2f, 0.6f), "Settings/About", 0.15)
				->AddChild((new UIButton(true, 0, 1, []() {system("start https://discord.com/channels/920776187884732556/951461326478262292"); }))
					->SetBorder(UIBox::BorderType::Rounded, 0.25)
					->AddChild((new UIBackground(true, 0, 0, 0.04))
						->SetUseTexture(true, "Open.png")
						->SetPadding(0.01, 0.01, 0.01, 0)
						->SetSizeMode(UIBox::SizeMode::AspectRelative))
					->AddChild(new UIText(0.3, 0, GetTranslation("mod_release_candidate_open_discord"), UI::Text)));
		}
		else
		{
			GenerateAvailabilityMessage();
		}
	}

	if (Thunderstore::SelectedMod.Name == "VanillaPlus")
	{
#ifndef TF_PLUGIN
		GenerateModInfoText(
			{
				NewLineStringToStringArray(GetTranslation("mod_vanillaplus_message"))

			}, Vector3f(0.1f, 0.2f, 0.6f), "Settings/About", 0.1);
#else
		GenerateAvailabilityMessage();
#endif
	}

	UIBox* MarkdownBackground = new UIBox(false, 0);
	MarkdownBackground->SetPadding(0, 0, 0.1f, 0.1f);
	ModsScrollBox->AddChild(MarkdownBackground);

	Markdown::MarkdownStyling Style;
	Style.Text = UI::Text;
	Style.Width = 1.1;
	Style.TextSize = 0.6f;
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
	ClearLoadedTextures();

	std::vector<UIBox*> Rows;

	if (Thunderstore::SelectedOrdering != Thunderstore::Ordering::Installed)
	{
		SearchBar = new UITextField(0, 0, UI::Text, []()
			{
				if (CurrentModsTab->Filter != CurrentModsTab->SearchBar->GetText() && !Thunderstore::IsDownloading)
				{
					CurrentModsTab->Filter = CurrentModsTab->SearchBar->GetText();
					CurrentModsTab->SelectedPage = 0;
					CurrentModsTab->LoadedModList = false;
					CurrentModsTab->ShowLoadingText();
				}
			});

		SearchBar->SetTextSize(12)
			->SetHintText(GetTranslation("search"))
			->SetText(Filter)
			->SetTextSizeMode(UIBox::SizeMode::PixelRelative)
			->SetPadding(5)
			->SetPaddingSizeMode(UIBox::SizeMode::PixelRelative)
			->SetMinSize(Vector2f(1.0f, 0));
		ModsScrollBox->AddChild(SearchBar);
	}

	for (size_t i = 0; i < 20; i++)
	{
		auto NewRow = new UIBox(true, 0);
		NewRow->SetPadding(0.001);
		Rows.push_back(NewRow);
		ModsScrollBox->AddChild(NewRow);
	}

	size_t SlotsPerRow = GetModsPerPage().X;

	size_t it = 0;

	int ModIndex = 0;
	for (const auto& i : Thunderstore::FoundMods)
	{
		bool UseTexture = std::filesystem::exists(i.Img) && !i.IsNSFW;

		unsigned int tex = 0;
		if (UseTexture)
		{
			tex = Texture::LoadTexture(i.Img);
			ModTextures.push_back(tex);
		}

		auto Entry = new ModEntry();

		if (Thunderstore::SelectedOrdering == Thunderstore::Ordering::Installed && !Thunderstore::GetModEnabled(i))
		{
			Entry->SetColor(Vector3f(1, 0.5f, 0));
		}

		Entry->entryButton->OnClickedFunctionIndex = [](int i) {
			const auto& m = Thunderstore::FoundMods[i];

			if (m.IsUnknownLocalMod)
			{
				Thunderstore::SelectedMod = m;
				Thunderstore::LoadedSelectedMod = true;
				return;
			}

			CurrentModsTab->ShowLoadingText();
			Thunderstore::GetModInfo(m, true);
		};
		Entry->entryButton->ButtonIndex = ModIndex++;

		Entry->SetName(i.Name);
		Entry->entryImage->SetUseTexture(UseTexture, tex);

		if (!UseTexture)
		{
			Entry->entryImage->SetColor(0.1);
			Entry->entryImage->AddChild(new UIText(0.8f, 1, i.IsUnknownLocalMod ? GetTranslation("mod_unknown")
				: (i.IsNSFW ? GetTranslation("mod_nsfw") : (i.IsTemporary ? GetTranslation("mod_temporary") : GetTranslation("loading"))), UI::Text));
		}
		ModImages.push_back(Entry->entryImage);
		Rows[it++ / SlotsPerRow]->AddChild(Entry);
	}

	PageButtons.clear();
	if (Thunderstore::SelectedOrdering == Thunderstore::Ordering::Installed)
	{
		return;
	}
	Rows[19]->AddChild((new UIText(12, 1, GetTranslation("mod_pages"), UI::Text))
		->SetTextSizeMode(UIBox::SizeMode::PixelRelative)
		->SetPadding(5)
		->SetPaddingSizeMode(UIBox::SizeMode::PixelRelative));
	Rows[19]->SetVerticalAlign(UIBox::Align::Centered);
	for (int i = 0; i < 8; i++)
	{
		auto b = new ModCategoryButton();
		b->button->OnClickedFunctionIndex = [](int i) {
			if (!Thunderstore::IsDownloading)
			{
				CurrentModsTab->ShowLoadingText();
				CurrentModsTab->SelectedPage = i;
				CurrentModsTab->LoadedModList = false;
			}
		};
		b->button->ButtonIndex = i;
		b->SetName(" " + std::to_string(i + 1) + " ");

		Installer::SetButtonColorIfSelected(b->button, i == SelectedPage);

		Rows[19]->AddChild(b);
		PageButtons.push_back(b->button);
	}
}

void ModsTab::ShowLoadingText()
{
	ModImages.clear();

	ModsScrollBox->DeleteChildren();
	ModsScrollBox->AddChild((new UIText(1.0f, 1, GetTranslation("loading"), UI::Text))->SetPadding(0.5));
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
	Log::Print("abc");
	for (auto& cat : Thunderstore::TSCategoryNames)
	{
		Log::Print("abc");
		static_cast<UIText*>(CategoryButtons[i]->GetChildren()[0])->SetText(GetTSOrderingName(cat.Name));
		i++;
	}

	if (!IsInModInfo)
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
			CategoryButtons[i]->SetColor(Vector3f(1.0f, 1.0f, 1.0f));
			CategoryButtons[i]->SetHoveredColor(Vector3f::Lerp(1.0f, Installer::GetThemeColor(), 0.5f));
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
			ModImages.at(i)->SetUseTexture(true, tex);
			ModImages.at(i)->SetColor(1);
			ModImages.at(i)->DeleteChildren();
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
			Thunderstore::InstallOrUninstallMod(m, false, false, false);
			continue;
		}
		float Progress = 0;
		try
		{
			if (m.IsTemporary)
			{
				Thunderstore::InstallOrUninstallMod(m, true, false, false);
				continue;
			}
			json response = json::parse(Networking::DownloadString("https://thunderstore.io/c/northstar/api/v1/package/" + m.UUID,
				Installer::UserAgent));
			if (m.Version != response.at("versions")[0].at("version_number").get<std::string>() || !Thunderstore::IsMostRecentFileVersion(m.FileVersion))
			{
				Log::Print("Mod '" + m.Name + "' is outdated!", Log::Warning);
				Log::Print(response.at("versions")[0].at("version_number").get<std::string>() + " != " + m.Version, Log::Warning);

				// Uninstall mod, then install mod again.
				Thunderstore::Package NewMod = m;
				NewMod.DownloadUrl = response.at("versions")[0].at("download_url");
				NewMod.Version = response.at("versions")[0].at("version_number").get<std::string>();
				BackgroundTask::SetStatus("dl_Updating " + m.Name);
				Thunderstore::InstallOrUninstallMod(NewMod, true, false, false);
				Thunderstore::InstallOrUninstallMod(NewMod, false, false, false);
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
		}

		Progress += (float)it / (float)Mods.size();
		BackgroundTask::SetProgress(std::min(0.95f, Progress));
		it++;
	}
}

ModsTab::ModsTab()
{
	PrevAspectRatio = Installer::MainWindow->GetAspectRatio();
	CurrentModsTab = this;
	Name = "mods";
	Log::Print("Loading mods tab...");

	Background->SetHorizontalAlign(UIBox::Align::Centered);
	Background->SetHorizontal(true);

	auto TabElem = new TabElement();
	TabElem->SetTabName(GetTranslation("tab_mods"));
	TabTitle = TabElem->title;

	Background->AddChild(TabElem);

	UIBox* CategoryBox = new UIBox(true, 0);
	CategoryBox
		->SetPadding(10)
		->SetPaddingSizeMode(UIBox::SizeMode::PixelRelative);

	ModsScrollBox = new UIScrollBox(false, 0, true);

	TabElem->contentBox->AddChild(CategoryBox);

	ModsScrollBox->SetMinSize(Vector2f(1.40f, 1.77f));
	ModsScrollBox->SetMaxSize(Vector2f(2, 1.77f));
	ShowLoadingText();
	TabElem->contentBox->AddChild(ModsScrollBox);


	for (size_t i = 0; i < Thunderstore::TSCategoryNames.size(); i++)
	{
		auto Category = new ModCategoryButton();
		Category->button->OnClickedFunctionIndex = [](int i) {
			Thunderstore::SelectedOrdering = Thunderstore::TSCategoryNames[i].o;
			CurrentModsTab->SelectedPage = 0;
			CurrentModsTab->LoadedModList = false;
			CurrentModsTab->ShowLoadingText();
			};

		Category->button->ButtonIndex = (int)i;
		Category->SetName(GetTSOrderingName(Thunderstore::TSCategoryNames[i].Name));
		Installer::SetButtonColorIfSelected(Category->button, false);
		CategoryBox->AddChild(Category);
		CategoryButtons.push_back(Category->button);
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

Vector2ui ModsTab::GetModsPerPage()
{
	Vector2ui VerticalPixels = CurrentModsTab->Background->GetUsedSize() * Installer::MainWindow->GetSize();

	return (VerticalPixels / 400).Clamp(1, 18) - Vector2ui(0, 1);
}

bool InstallingMod = false;
void ModsTab::Tick()
{
	if (!Background->IsVisible)
	{
		return;
	}
	Vector2ui ModPageDimensions = GetModsPerPage();
	ModsPerPage = ModPageDimensions.X * ModPageDimensions.Y;
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

	if (PrevAspectRatio != Installer::MainWindow->GetAspectRatio() && !IsInModInfo)
	{
		ShowLoadingText();
		Thunderstore::DownloadThunderstoreInfo(Thunderstore::SelectedOrdering, SelectedPage, Filter, true);
		DownloadingPage = true;
		LoadedModList = true;
		Thunderstore::IsDownloading = true;
	}
	PrevAspectRatio = Installer::MainWindow->GetAspectRatio();
}

ModsTab::~ModsTab()
{
}
