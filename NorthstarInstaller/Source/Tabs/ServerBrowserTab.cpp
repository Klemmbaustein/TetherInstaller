#include "ServerBrowserTab.h"
#include "LaunchTab.h"

#include <fstream>
#include <sstream>

#include <KlemmUI/UI/UIButton.h>
#include <KlemmUI/UI/UIScrollBox.h>
#include <KlemmUI/Rendering/MarkdownRendering.h>
#include <KlemmUI/Rendering/Texture.h>

#include "nlohmann/json.hpp"

#include "../Networking.h"
#include "../Installer.h"
#include "../Log.h"
#include "../BackgroundTask.h"
#include "../Thunderstore.h"
#include "../WindowFunctions.h"
#include "../Game.h"
#include "ProfileTab.h"


bool ServerBrowserTab::ShouldLaunchGame;
ServerBrowserTab* ServerBrowserTab::CurrentServerTab = nullptr;
std::vector<ServerBrowserTab::ServerEntry> ServerBrowserTab::Servers;
constexpr unsigned int MaxServerNameSize = 40;
const std::map<std::string, std::string> KNOWN_GAMEMODES = 
{
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

std::string GetFile(std::string InFile)
{
	std::ifstream i = std::ifstream(InFile);
	std::stringstream istream;
	istream << i.rdbuf();
	return istream.str();
}

std::string ToLowerCase(std::string Target)
{
	std::transform(Target.begin(), Target.end(), Target.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return Target;
}

std::string RemoveSpaces(std::string Target)
{
	Target.erase(remove(Target.begin(), Target.end(), ' '), Target.end());
	return Target;
}

bool InstallRequiredModsForServer(ServerBrowserTab::ServerEntry e)
{
	using namespace nlohmann;

	BackgroundTask::SetStatus("Loading required mods");

	Networking::Download(
		"https://thunderstore.io/c/northstar/api/v1/package/",
		"Data/temp/net/allmods.json",
		"User-Agent: " + Installer::UserAgent);

	json Response = json::parse(GetFile("Data/temp/net/allmods.json"));

	std::vector<ServerBrowserTab::ServerEntry::ServerMod> FailedMods;


	if (std::filesystem::exists(ProfileTab::CurrentProfile.Path + "/mods/autojoin"))
	{
		std::filesystem::remove_all(ProfileTab::CurrentProfile.Path + "/mods/autojoin");
	}

	std::filesystem::copy("Data/autojoin", ProfileTab::CurrentProfile.Path + "/mods/autojoin",
		std::filesystem::copy_options::recursive);

	float Progress = 0;

	for (auto& RequiredMod : e.RequiredMods)
	{
		bool HasInstalledMod = false;
		Thunderstore::Package m;

		if (RequiredMod.ModName.find(".") != std::string::npos)
		{
			m.Name = RequiredMod.ModName.substr(RequiredMod.ModName.find_first_of(".") + 1);
			m.Namespace = RequiredMod.ModName.substr(0, RequiredMod.ModName.find_first_of("."));
			m.Author = m.Namespace;
		}
		else
		{
			m.Name = RequiredMod.ModName;
		}

		Progress += 1.0f / e.RequiredMods.size();
		BackgroundTask::SetProgress(Progress - 0.001);
		if (Thunderstore::IsModInstalled(m))
		{
			if (m.Author != "Northstar")
			{
				Thunderstore::SetModEnabled(m, true);
			}
			continue;
		}
		
		Thunderstore::Package ClosestMod;
		size_t ClosestModScore = SIZE_MAX;
		size_t Age = 0;
		for (auto& item : Response)
		{
			if (RemoveSpaces(ToLowerCase(m.Name)).find(RemoveSpaces(ToLowerCase(item.at("name")))) != std::string::npos
				|| RemoveSpaces(ToLowerCase(item.at("name"))).find(RemoveSpaces(ToLowerCase(m.Name))) != std::string::npos)
			{
				if (item.at("is_deprecated"))
				{
					continue;
				}

				size_t NewScore = std::llabs(m.Name.size() - item.at("name").get<std::string>().size()) + Age++;
				
				if (ClosestModScore <= NewScore)
				{
					continue;
				}
				ClosestModScore = NewScore;
				m.Author = item.at("owner");
				m.Namespace = item.at("owner");
				m.Name = item.at("name");
				m.UUID = item.at("uuid4");
				m.Version = item.at("versions")[0].at("version_number");
				m.DownloadUrl = item.at("versions")[0].at("download_url");

				LOG_PRINTF("Found possible canidate: {}.{} with UUID of {}. Score: {}.",
					item.at("owner").get<std::string>(), 
					item.at("name").get<std::string>(), 
					item.at("uuid4").get<std::string>(),
					NewScore);
			}

		}
		if (ClosestModScore != SIZE_MAX)
		{
			BackgroundTask::SetStatus("Installing " + m.Name);
			Thunderstore::InstallOrUninstallMod(m, true, false);
			HasInstalledMod = true;
		}
		if (!HasInstalledMod)
		{
			FailedMods.push_back(RequiredMod);
		}
	}

	if (FailedMods.empty())
	{
		return true;
	}

	std::string FailedModsString = "The following mods could not be installed:\n";

	for (auto& i : FailedMods)
	{
		FailedModsString.append("- " + i.ModName);
		if (i.IsRequired)
		{
			FailedModsString.append(" (Required)");
		}
		FailedModsString.append("\n");
	}
	FailedModsString.append("Continue anyways?");

	return Window::ShowPopupQuestion("Joining " + e.Name, FailedModsString) == Window::PopupReply::Yes;
}


void RefreshServerBrowser()
{
	if (!BackgroundTask::IsFunctionRunningAsTask(ServerBrowserTab::LoadServers))
	{
		ServerBrowserTab::CurrentServerTab->DisplayLoadingText();
		new BackgroundTask(ServerBrowserTab::LoadServers, []() {ServerBrowserTab::CurrentServerTab->DisplayServers(); });
	}
}

ServerBrowserTab::ServerBrowserTab()
{
	CurrentServerTab = this;
	Name = "Servers";
	Description = "Browse northstar servers";
	Log::Print("Loading server browser tab...");

	Background->BoxAlign = UIBox::Align::Centered;
	Background->SetHorizontal(true);

	ServerBackground = new UIBackground(false, 0, 0, Vector2f(1.5, 1.85));
	Background->AddChild(ServerBackground
		->SetOpacity(0.65)
		->SetMaxSize(Vector2f(1.5, 1.85))
		->AddChild((new UIBackground(true, 0, 1, Vector2f(1.5, 0.005)))
			->SetPadding(0))
		->SetPadding(0));
	ServerBackground->BoxAlign = UIBox::Align::Reverse;
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

	ServerBox = new UIScrollBox(false, 0, true);
	ServerBox->BoxAlign = UIBox::Align::Reverse;
	ServerBackground->AddChild(new UIBackground(true, 0, 1, Vector2f(1.46, 0.005)));
	ServerBackground->AddChild((new UIText(0.25,
		1,
		"Region     Name                                      Players",
		UI::MonoText))
		->SetPadding(0.05, 0, 0.02, 0.02));

	ServerBackground->AddChild((new UIBox(true, 0))
		->SetPadding(0)
		->AddChild(ServerBox
			->SetMaxSize(Vector2f(0.825, 1.5))
			->SetMinSize(Vector2f(0.825, 1.5)))
		->AddChild(ServerDescriptionBox
			->SetMinSize(Vector2f(0.3, 1.4))));
	ServerDescriptionBox->BoxAlign = UIBox::Align::Reverse;
	new BackgroundTask(LoadServers, []() {CurrentServerTab->DisplayServers(); });
}

bool ServerBrowserTab::IsInstalledAsServerMod(std::string Name)
{
	auto Mods = Thunderstore::GetInstalledMods().Combined();
	for (auto& i : Mods)
	{
		if (RemoveSpaces(ToLowerCase(i.Name)).find(RemoveSpaces(ToLowerCase(Name))) != std::string::npos
			|| RemoveSpaces(ToLowerCase(Name)).find(RemoveSpaces(ToLowerCase(i.Name))) != std::string::npos)
		{
			return true;
		}

	}
	return false;
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
	ServerListBox->BoxAlign = UIBox::Align::Reverse;

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

		TotalPlayers += i.PlayerCount;

		if (!Filter.empty() && ToLowerCase(Name).find(Filter) == std::string::npos)
		{
			continue;
		}
		Region.resize(10, ' ');
		if (Name.size() > MaxServerNameSize)
		{
			Name = Name.substr(0, MaxServerNameSize - 3u) + "...";
		}
		Name.resize(MaxServerNameSize, ' ');

		std::string PlayerCount = std::to_string(i.PlayerCount) + "/" + std::to_string(i.MaxPlayerCount);
		PlayerCount.resize(7, ' ');

		UIButton* b = new UIButton(true, 0, Installer::TabStyles[0], []() {
			for (size_t i = 0; i < CurrentServerTab->ServerBrowserButtons.size(); i++)
			{
				if (CurrentServerTab->ServerBrowserButtons[i]->IsBeingHovered())
				{
					Installer::TabStyles[1]->ApplyTo(CurrentServerTab->ServerBrowserButtons[i]);
					CurrentServerTab->ServerBrowserButtons[i]->SetMinSize(Vector2f(0.8, 0));
					CurrentServerTab->DisplayServerDescription(CurrentServerTab->DisplayedServerEntries[i]);
				}
				else
				{
					Installer::TabStyles[0]->ApplyTo(CurrentServerTab->ServerBrowserButtons[i]);
					CurrentServerTab->ServerBrowserButtons[i]->SetMinSize(Vector2f(0.8, 0));
				}
			}
			});

		ServerListBox->AddChild(b
			->SetMinSize(Vector2f(0.8, 0))
			->SetPadding(0.005)
			->AddChild((new UIText(0.25, { 
				TextSegment(Region, 0.25),
				TextSegment(" " + Name + "  ", 0),
				TextSegment(PlayerCount, 0.25),
				}, UI::MonoText))
				->SetPadding(0.005)));
		DisplayedServerEntries.push_back(i);
		ServerBrowserButtons.push_back(b);
	}

	if (ServerBrowserButtons.empty())
	{
		ServerListBox->AddChild(new UIText(0.3, 1, "No servers found", UI::Text));
	}

	PlayerCountText->SetText("Players in game: " + std::to_string(TotalPlayers));
}

void JoinCurrentServer()
{
	if (Thunderstore::VanillaPlusInstalled())
	{
		Window::ShowPopupError("Cannot join northstar servers with Vanilla+");
		ServerBrowserTab::ShouldLaunchGame = false;
		return;
	}
	ServerBrowserTab::ShouldLaunchGame = InstallRequiredModsForServer(ServerBrowserTab::CurrentServerTab->SelectedServer);
}

void ServerBrowserTab::Tick()
{
	ServerBackground->SetMinSize(Vector2f(1.2, Background->GetUsedSize().Y));
	ServerBackground->SetMaxSize(Vector2f(2, Background->GetUsedSize().Y));

	if (!ServerDescriptionText)
		return;

	if (BackgroundTask::IsFunctionRunningAsTask(JoinCurrentServer))
	{
		ServerDescriptionText->SetText("Joining server...");
	}
	else if (LaunchTab::IsGameRunning)
	{
		ServerDescriptionText->SetText("Game is running");
	}
	else
	{
		ServerDescriptionText->SetText("Join server");

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
	SelectedServer = e;
	ServerDescriptionBox->DeleteChildren();
	ServerDescriptionText = nullptr;
	if (e.Name.empty())
	{
		return;
	}

	UIText* Descr = new UIText(0.3, 1, e.Description, UI::Text);
	Descr->Wrap = true;
	Descr->WrapDistance = 0.4;
	std::string PlayerCount = "Players: " +
	 std::to_string(e.PlayerCount) + "/" + std::to_string(e.MaxPlayerCount);

	UIText* Title = new UIText(0.5, 1, e.Name, UI::Text);
	Title->Wrap = true;
	Title->WrapDistance = 0.25;

	ServerDescriptionBox->AddChild(Title);
	
	ServerDescriptionBox->AddChild(new UIBackground(true, 0, 1, Vector2f(0.6, 0.005)));

	ServerDescriptionBox->AddChild(Descr);

	ServerDescriptionBox->AddChild(new UIBackground(true, 0, 1, Vector2f(0.6, 0.005)));

	auto MapDescr = new UIBox(false, 0);
	MapDescr->BoxAlign = UIBox::Align::Reverse;
	MapDescr->SetPadding(0);
	MapDescr->SetMinSize(9 * 0.025);

	ServerDescriptionBox->AddChild((new UIText(0.5, 1, "Playing on: ", UI::Text)));
	ServerDescriptionBox->AddChild((new UIBox(true, 0))
		->AddChild((new UIBackground(true, 0, 1, Vector2f(16, 9) * 0.025))
			->SetUseTexture(true, GetMapTexture(e.Map))
			->SetSizeMode(UIBox::SizeMode::AspectRelative))
		->AddChild(MapDescr
			->AddChild((new UIText(0.5, 1, e.MapName, UI::Text))
				->SetPadding(0, 0, 0.01, 0.01))
			->AddChild((new UIText(0.3, 1, e.GameModeName, UI::Text))
				->SetPadding(0, 0, 0.01, 0.01))
			->AddChild((new UIText(0.3, 1, PlayerCount, UI::Text))
				->SetPadding(0, 0, 0.01, 0.01))));

	ServerDescriptionBox->AddChild(new UIBackground(true, 0, 1, Vector2f(0.6, 0.005)));
	ServerDescriptionText = new UIText(0.4, 0, "Join server", UI::Text);
	ServerDescriptionBox->AddChild((new UIBox(true, 0))
		->AddChild((new UIButton(true, 0, Vector3f32(0.5, 0.6, 1), []() { new BackgroundTask(JoinCurrentServer, []() {

				ServerBrowserTab::CurrentServerTab->DisplayServerDescription(ServerBrowserTab::CurrentServerTab->SelectedServer);
				if (ServerBrowserTab::ShouldLaunchGame)
				{ 
					LOG_PRINTF("Joining server \"{}\" (id: {})",
						ServerBrowserTab::CurrentServerTab->SelectedServer.Name,
						ServerBrowserTab::CurrentServerTab->SelectedServer.ServerID);
					LaunchTab::LaunchNorthstar("+AutoJoinServer " + ServerBrowserTab::CurrentServerTab->SelectedServer.ServerID); 
				}
			});
			}))
			->SetBorder(UIBox::BorderType::Rounded, 0.5)
			->AddChild(ServerDescriptionText)));

	ServerDescriptionBox->AddChild(new UIText(0.5, 1, "Mods: ", UI::Text));

	for (const auto& i : e.RequiredMods)
	{
		Thunderstore::Package m;

		if (i.ModName.find(".") != std::string::npos)
		{
			m.Name = i.ModName.substr(i.ModName.find_first_of(".") + 1);
			m.Namespace = i.ModName.substr(0, i.ModName.find_first_of("."));
			m.Author = m.Namespace;
		}
		else
		{
			m.Name = i.ModName;
		}

		std::string ModName = " - " + i.ModName;

		if (IsInstalledAsServerMod(m.Name) || Thunderstore::IsModInstalled(m))
		{
			ModName.append(" (installed)");
		}
		ServerDescriptionBox->AddChild((new UIText(0.3, 1, ModName, UI::Text))
			->SetPadding(0.0, 0.005, 0.01, 0.01));
	}
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

	if (!std::filesystem::exists("Data/temp"))
	{
		return;
	}

	BackgroundTask::SetStatus("Loading servers");
	Networking::Download("https://northstar.tf/client/servers", "Data/temp/net/servers.json", "User-Agent: " + Installer::UserAgent);
	json ServerResponse = json::parse(GetFile("Data/temp/net/servers.json"));

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
		e.ServerID = i.at("id");
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
			ServerEntry::ServerMod RequiredMod;
			RequiredMod.ModName = mod.at("Name");
			RequiredMod.IsRequired = mod.at("RequiredOnClient");
			e.RequiredMods.push_back(RequiredMod);
		}

		e.Description = i.at("description");

		e.PlayerCount = i.at("playerCount");
		e.MaxPlayerCount = i.at("maxPlayers");
		Servers.push_back(e);
	}
}
