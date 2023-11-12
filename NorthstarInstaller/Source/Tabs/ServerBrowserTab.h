#pragma once
#include "UITab.h"
#include <KlemmUI/UI/UIBackground.h>
#include <KlemmUI/UI/UITextField.h>
#include <KlemmUI/UI/UIText.h>

#include <map>

#include "../UI/UIDef.h"

class ServerBrowserTab : public UITab
{
	UIBackground* ServerBackground = nullptr;
	UIScrollBox* ServerBox = nullptr;
	UIBox* ServerListBox = nullptr;
	UIBox* ServerDescriptionBox = new UIBox(false, 0);
	UITextField* ServerSearchBox = nullptr;
public:
	UIText* ServerDescriptionText = nullptr;
	UIText* ReloadText = nullptr;
	UIText* ServerHeader = nullptr;
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

	unsigned int GetMapTexture(std::string Map);

	static void LoadServers();
	void DisplayServers();
	void DisplayLoadingText();
	void Tick() override;

	void OnTranslationChanged() override;
};