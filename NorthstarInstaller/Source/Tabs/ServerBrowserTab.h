#pragma once
#include "UITab.h"
#include <KlemmUI/UI/UIBackground.h>
#include <KlemmUI/UI/UITextField.h>
#include <KlemmUI/UI/UIText.h>

#include <map>

#include "../UIDef.h"

class ServerBrowserTab : public UITab
{
	UIBackground* ServerBackground = nullptr;
	UIScrollBox* ServerBox = nullptr;
	UIBox* ServerListBox = nullptr;
	UIBox* ServerDescriptionBox = new UIBox(false, 0);
	UITextField* ServerSearchBox = nullptr;
public:
	UIText* ServerDescriptionText = nullptr;
	static bool ShouldLaunchGame;
	struct ServerEntry
	{
		std::string Region;
		std::string Name;
		std::string Map;
		std::string MapName;
		std::string ServerID;
		std::string GameMode;
		std::string GameModeName;
		std::string Description;
		struct ServerMod
		{
			bool IsRequired = false;
			std::string ModName;
		};

		std::vector<ServerMod> RequiredMods;
		int PlayerCount = 0, MaxPlayerCount = 0;
	};

	ServerEntry SelectedServer;

	std::vector<ServerEntry> DisplayedServerEntries;
	std::vector<UIButton*> ServerBrowserButtons;
	std::string SearchText = "";

	void DisplayServerDescription(ServerEntry e);

	UIText* PlayerCountText = new UIText(0.3, 1, "Players in game: ??", UI::Text);
	static std::vector<ServerEntry> Servers;
	static ServerBrowserTab* CurrentServerTab;
	ServerBrowserTab();

	std::map<std::string, unsigned int> MapTextures =
	{
	};

	bool IsInstalledAsServerMod(std::string Name);

	static inline const std::map<std::string, std::string> MapNames = {
		std::pair("mp_angel_city", "Angel City"),
		std::pair("mp_black_water_canal" , "Black Water Canal"),
		std::pair("mp_box" , "Box"),
		std::pair("mp_coliseum" , "Coliseum"),
		std::pair("mp_coliseum_column" , "Pillars"),
		std::pair("mp_colony02" , "Colony"),
		std::pair("mp_complex3" , "Complex"),
		std::pair("mp_crashsite3" , "Crash Site"),
		std::pair("mp_drydock" , "Drydock"),
		std::pair("mp_eden" , "Eden"),
		std::pair("mp_forwardbase_kodai" , "Forwardbase Kodai"),
		std::pair("mp_glitch" , "Glitch"),
		std::pair("mp_grave" , "Boomtown"),
		std::pair("mp_homestead" , "Homestead"),
		std::pair("mp_lf_deck" , "Deck"),
		std::pair("mp_lf_meadow" , "Meadow"),
		std::pair("mp_lf_stacks" , "Stacks"),
		std::pair("mp_lf_township" , "Township"),
		std::pair("mp_lf_traffic" , "Traffic"),
		std::pair("mp_lf_uma" , "UMA"),
		std::pair("mp_lobby" , "Lobby"),
		std::pair("mp_relic02" , "Relic"),
		std::pair("mp_rise" , "Rise"),
		std::pair("mp_thaw" , "Exoplanet"),
		std::pair("mp_wargames" , "War Games"),
	};

	unsigned int GetMapTexture(std::string Map);

	static void LoadServers();
	void DisplayServers();
	void DisplayLoadingText();
	void Tick() override;
};