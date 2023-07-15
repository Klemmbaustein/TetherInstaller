#include "ServerBrowserTab.h"
#include "LaunchTab.h"


#include <fstream>
#include <sstream>

#include <UI/UIButton.h>
#include <UI/UIScrollBox.h>
#include <Rendering/MarkdownRendering.h>
#include <Rendering/Texture.h>

#include "../JSON/json.hpp"

#include "../Networking.h"
#include "../Installer.h"
#include "../Log.h"
#include "../BackgroundTask.h"
#include "../Thunderstore.h"

constexpr unsigned int MaxServerNameSize = 40;
const std::map<std::string, std::string> KNOWN_GAMEMODES = {
	std::pair("private_match", "Private Match"),
	std::pair("aitdm" , "Attrition"),
	std::pair("at" , "Bounty Hunt"),
	std::pair("coliseum" , "Coliseum"),
	std::pair("cp" , "Amped Hardpoint"),
	std::pair("ctf" , "Capture the Flag"),
	std::pair("fd_easy" , "Frontier Defense (Easy)"),
	std::pair("fd_hard" , "Frontier Defense (Hard)"),
	std::pair("fd_insane" , "Frontier Defense (Insane)"),
	std::pair("fd_master" , "Frontier Defense (Master)"),
	std::pair("fd_normal" , "Frontier Defense (Regular)"),
	std::pair("lf" , "Live Fire"),
	std::pair("lts" , "Last Titan Standing"),
	std::pair("mfd" , "Marked for Death"),
	std::pair("ps" , "Pilots vs. Pilots"),
	std::pair("solo" , "Campaign"),
	std::pair("tdm" , "Skirmish"),
	std::pair("ttdm" , "Titan Brawl"),

	std::pair("alts" , "Aegis Last Titan Standing"),
	std::pair("attdm" , "Aegis Titan Brawl"),
	std::pair("ffa" , "Free For All"),
	std::pair("fra" , "Free Agents"),
	std::pair("holopilot_lf" , "The Great Bamboozle"),
	std::pair("rocket_lf" , "Rocket Arena"),
	std::pair("turbo_lts" , "Turbo Last Titan Standing"),
	std::pair("turbo_ttdm" , "Turbo Titan Brawl"),

	std::pair("chamber" , "One in the Chamber"),
	std::pair("ctf_comp" , "Competitive CTF"),
	std::pair("fastball" , "Fastball"),
	std::pair("gg" , "Gun Game"),
	std::pair("hidden" , "The Hidden"),
	std::pair("hs" , "Hide and Seek"),
	std::pair("inf" , "Infection"),
	std::pair("kr" , "Amped Killrace"),
	std::pair("sbox" , "Sandbox"),
	std::pair("sns" , "Sticks and Stones"),
	std::pair("tffa" , "Titan FFA"),
	std::pair("tt" , "Titan Tag"),
	std::pair("fw" , "Frontier War"),
	std::pair("pk", "Parkour"),

	std::pair("sp_coop" , "Campaign Coop"),
};


void RefreshServerBrowser()
{
	if (!BackgroundTask::IsFunctionRunningAsTask(ServerBrowserTab::LoadServers))
	{
		ServerBrowserTab::CurrentServerTab->DisplayLoadingText();
		new BackgroundTask(ServerBrowserTab::LoadServers, []() {ServerBrowserTab::CurrentServerTab->DisplayServers(); });
	}
}

ServerBrowserTab* ServerBrowserTab::CurrentServerTab = nullptr;
std::vector<ServerBrowserTab::ServerEntry> ServerBrowserTab::Servers;
ServerBrowserTab::ServerBrowserTab()
{
	CurrentServerTab = this;
	Name = "Servers";

	Background->Align = UIBox::E_CENTERED;
	Background->SetHorizontal(true);

	ServerBackground = new UIBackground(false, 0, 0, Vector2f(1.5, 1.85));
	Background->AddChild(ServerBackground
		->SetOpacity(0.5)
		->SetMaxSize(Vector2f(1.5, 1.85))
		->AddChild((new UIBackground(true, 0, 1, Vector2f(1.5, 0.005)))
			->SetPadding(0))
		->SetPadding(0));
	ServerBackground->Align = UIBox::E_REVERSE;
	ServerBackground->AddChild(new UIText(0.8, 1, "Server browser", UI::Text));
	
	ServerBackground->AddChild((new UIBox(true, 0))
		->AddChild(PlayerCountText->SetPadding(0.02, 0.02, 0.01, 0.02))
		->AddChild((new UIButton(true, 0, 1, RefreshServerBrowser))
			->AddChild(new UIText(0.3, 0, "Refresh", UI::Text))));

	ServerSearchBox = new UITextField(true, 0, 0, UI::Text, []() {
		if (CurrentServerTab->SearchText != CurrentServerTab->ServerSearchBox->GetText())
		{
			CurrentServerTab->SearchText = CurrentServerTab->ServerSearchBox->GetText();
			RefreshServerBrowser();
		}});
	ServerSearchBox->SetTextSize(0.3);
	ServerSearchBox->SetMinSize(Vector2f(0.8, 0.02));
	ServerSearchBox->SetHintText("Search");
	ServerBackground->AddChild(ServerSearchBox);

	ServerBox = new UIScrollBox(false, 0, 10);
	ServerBox->Align = UIBox::E_REVERSE;
	ServerBackground->AddChild(new UIBackground(true, 0, 1, Vector2f(1.46, 0.005)));
	ServerBackground->AddChild((new UIText(0.25, 1, "Region     Name                                      Players  Map", UI::MonoText))->SetPadding(0.05, 0, 0.02, 0.02));
	ServerBackground->AddChild((new UIBox(true, 0))
		->SetPadding(0)
		->AddChild(ServerBox
			->SetMaxSize(Vector2f(0.825, 1.5))
			->SetMinSize(Vector2f(0.825, 1.5)))
		->AddChild(ServerDescriptionBox
			->SetMinSize(Vector2f(0.3, 1.5))));
	ServerDescriptionBox->Align = UIBox::E_REVERSE;
	new BackgroundTask(LoadServers, []() {CurrentServerTab->DisplayServers(); });
}

void ServerBrowserTab::DisplayServers()
{
	DisplayServerDescription(ServerEntry());
	ServerBrowserButtons.clear();
	DisplayedServerEntries.clear();
	ServerListBox = nullptr;
	ServerBox->DeleteChildren();

	ServerListBox = new UIBox(false, 0);
	ServerBox->AddChild(ServerListBox
		->SetPadding(0));
	ServerListBox->Align = UIBox::E_REVERSE;

	std::string Filter = ServerSearchBox->GetText();
	std::transform(Filter.begin(), Filter.end(), Filter.begin(),
		[](unsigned char c) { return std::tolower(c); });

	struct SortingElement
	{
		SortingElement(size_t CurrentIndex, size_t Rating, bool SortByLowest)
		{
			this->CurrentIndex = CurrentIndex;
			this->Rating = Rating;
			this->SortByLowest = SortByLowest;
		}
		size_t CurrentIndex = 0;
		size_t Rating = 0;
		bool SortByLowest = true;

		static bool compare(const SortingElement& a, const SortingElement& b)
		{
			if (a.SortByLowest)
				return a.Rating < b.Rating;
			else
				return a.Rating > b.Rating;
		}

	};

	std::vector<SortingElement> ServerSorting;

	for (size_t i = 0; i < Servers.size(); i++)
	{
		size_t Rating = 0;
		bool SortByLowest = false;

		Rating = Servers[i].PlayerCount;

		ServerSorting.push_back(SortingElement(i, Rating, SortByLowest));
	}

	std::sort(ServerSorting.begin(), ServerSorting.end(), SortingElement::compare);
	
	std::vector<ServerEntry> SortedServers;

	for (size_t i = 0; i < ServerSorting.size(); i++)
	{
		SortedServers.push_back(Servers[ServerSorting[i].CurrentIndex]);
	}
	size_t TotalPlayers = 0;

	ServerEntry entr;

	for (const auto& i : SortedServers)
	{
		std::string Name = i.Name;
		std::string Region = i.Region;
		std::string LowerCaseName = Name;

		std::transform(LowerCaseName.begin(), LowerCaseName.end(), LowerCaseName.begin(),
			[](unsigned char c) { return std::tolower(c); });
		TotalPlayers += i.PlayerCount;

		if (!Filter.empty() && LowerCaseName.find(Filter) == std::string::npos)
		{
			continue;
		}
		Region.resize(10, ' ');
		if (Name.size() > MaxServerNameSize)
		{
			Name = Name.substr(0, MaxServerNameSize - 3) + "...";
		}
		Name.resize(MaxServerNameSize, ' ');

		std::string PlayerCount = std::format("{}/{}", i.PlayerCount, i.MaxPlayerCount);
		PlayerCount.resize(7, ' ');

		UIButton* b = new UIButton(true, 0, 1, []() {
			for (size_t i = 0; i < CurrentServerTab->ServerBrowserButtons.size(); i++)
			{
				if (CurrentServerTab->ServerBrowserButtons[i]->IsBeingHovered())
				{
					CurrentServerTab->ServerBrowserButtons[i]->SetColor(Vector3f32(0.5, 0.6, 1));
					CurrentServerTab->DisplayServerDescription(CurrentServerTab->DisplayedServerEntries[i]);
				}
				else
				{
					CurrentServerTab->ServerBrowserButtons[i]->SetColor(1);
				}
			}
			});

		ServerListBox->AddChild(b
			->SetPadding(0.005)
			->SetMinSize(Vector2f(0.8, 0))
			->AddChild((new UIText(0.25, { 
				TextSegment(Region, 0.25),
				TextSegment(" " + Name + "  ", 0),
				TextSegment(PlayerCount, 0.25),
				TextSegment("  " + i.MapName, 0)
				}, UI::MonoText))
				->SetPadding(0.005)));
		DisplayedServerEntries.push_back(i);
		ServerBrowserButtons.push_back(b);
	}
	PlayerCountText->SetText(std::format("Players in game: {}", TotalPlayers));
}

void ServerBrowserTab::Tick()
{
	if (ServerListBox)
	{
		ServerBox->SetMaxScroll(std::max(ServerListBox->GetUsedSize().Y * 10 - 10.0f, 0.0));
	}
	else
	{
		ServerBox->SetMaxScroll(0);
		ServerBox->GetScrollObject()->Percentage = 0;
	}
}

void ServerBrowserTab::DisplayLoadingText()
{
	ServerListBox = nullptr;
	ServerBox->DeleteChildren();
	ServerBox->AddChild((new UIText(0.4, 1, "Loading...", UI::Text))->SetPadding(0.5));
}

void ServerBrowserTab::DisplayServerDescription(ServerEntry e)
{
	ServerDescriptionBox->DeleteChildren();

	if (e.Name.empty())
	{
		return;
	}

	UIText* Descr = new UIText(0.3, 1, e.Description, UI::Text);
	Descr->Wrap = true;
	Descr->WrapDistance = 1.6;
	std::string PlayerCount = std::format("Players: {}/{}", e.PlayerCount, e.MaxPlayerCount);

	ServerDescriptionBox->AddChild(new UIText(0.5, 1, e.Name, UI::Text));
	
	ServerDescriptionBox->AddChild(new UIBackground(true, 0, 1, Vector2f(0.6, 0.005)));

	ServerDescriptionBox->AddChild(Descr);

	ServerDescriptionBox->AddChild(new UIBackground(true, 0, 1, Vector2f(0.6, 0.005)));

	auto MapDescr = new UIBox(false, 0);
	MapDescr->Align = UIBox::E_REVERSE;
	MapDescr->SetPadding(0);
	MapDescr->SetMinSize(9 * 0.025);

	ServerDescriptionBox->AddChild(new UIText(0.5, 1, "Playing on: ", UI::Text));
	ServerDescriptionBox->AddChild((new UIBox(true, 0))
		->AddChild((new UIBackground(true, 0, 1, Vector2f(16, 9) * 0.025))
			->SetUseTexture(true, GetMapTexture(e.Map))
			->SetSizeMode(UIBox::E_PIXEL_RELATIVE))
		->AddChild(MapDescr
			->AddChild(new UIText(0.5, 1, e.MapName, UI::Text))
			->AddChild(new UIText(0.3, 1, "Game mode: " + e.GameModeName, UI::Text))
			->AddChild(new UIText(0.3, 1, PlayerCount, UI::Text))));

	ServerDescriptionBox->AddChild(new UIBackground(true, 0, 1, Vector2f(0.6, 0.005)));
	
	ServerDescriptionBox->AddChild(new UIText(0.5, 1, "Required mods: ", UI::Text));

	for (const auto& i : e.RequiredMods)
	{
		Thunderstore::Package m;

		if (i.find(".") != std::string::npos)
		{
			m.Name = i.substr(i.find_first_of(".") + 1);
			m.Namespace = i.substr(0, i.find_first_of("."));
			m.Author = m.Namespace;
		}
		else
		{
			m.Name = i;
		}

		std::string ModName = " - " + i;

		if (Thunderstore::IsModInstalled(m))
		{
			ModName.append(" (installed)");
		}
		ServerDescriptionBox->AddChild(new UIText(0.3, 1, ModName, UI::Text));
	}
	ServerDescriptionBox->AddChild((new UIBox(true, 0))
		->AddChild((new UIButton(true, 0, 1, []() {LaunchTab::LaunchNorthstar(); }))
			->AddChild(new UIText(0.25, 0, "Launch game", UI::Text))));
}

unsigned int ServerBrowserTab::GetMapTexture(std::string Map)
{
	auto tex = MapTextures.find(Map);
	if (tex == MapTextures.end())
	{
		std::string Texture = "Data/maps/" + Map + "_widescreen.png";
		if (std::filesystem::exists(Texture))
		{
			unsigned int LoadedTexture = Texture::LoadTexture(Texture);
			MapTextures.insert(std::pair(Map, LoadedTexture));
			return LoadedTexture;
		}
		else
		{
			Log::Print("Error: " + Texture + " does not exist!", Log::Error);
			return 0;
		}
	}

	return tex->second;
}

void ServerBrowserTab::LoadServers()
{
	using namespace nlohmann;

	BackgroundTask::SetStatus("Loading servers");
	Networking::Download("https://northstar.tf/client/servers", "Data/temp/net/servers.json", "User-Agent: " + Installer::UserAgent);
	std::ifstream i = std::ifstream("Data/temp/net/servers.json");
	std::stringstream istream;
	istream << i.rdbuf();
	json ServerResponse = json::parse(istream.str());

	Servers.clear();
	for (const auto& i : ServerResponse)
	{
		ServerEntry e;
		e.Name = i.at("name");
		e.Map = i.at("map");
		e.MapName = e.Map;
		e.GameMode = i.at("playlist");
		e.GameModeName = e.GameMode;

		if (MapNames.contains(e.MapName))
		{
			e.MapName = MapNames.at(e.MapName);
		}

		if (KNOWN_GAMEMODES.contains(e.GameModeName))
		{
			e.GameModeName = KNOWN_GAMEMODES.at(e.GameModeName);
		}
		e.Region = "Unknown";
		try
		{
			e.Region = i.at("region");
		}
		catch (std::exception& e)
		{
		}

		for (auto& mod : i.at("modInfo").at("Mods"))
		{
			e.RequiredMods.push_back(mod.at("Name"));
		}

		e.Description = i.at("description");

		e.PlayerCount = i.at("playerCount");
		e.MaxPlayerCount = i.at("maxPlayers");
		Servers.push_back(e);
	}
}
