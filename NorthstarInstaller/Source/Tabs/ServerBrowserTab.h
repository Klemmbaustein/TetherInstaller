#pragma once
#include "UITab.h"
#include <KlemmUI/UI/UIBackground.h>
#include <KlemmUI/UI/UITextField.h>
#include <KlemmUI/UI/UIText.h>
#include <KlemmUI/UI/UIButton.h>

#include <map>

#include "../UI/UIDef.h"
#include "../Thunderstore.h"

class ServerBrowserTab : public UITab
{
	KlemmUI::UIScrollBox* ServerBox = nullptr;
	KlemmUI::UIBox* ServerListBox = nullptr;
	KlemmUI::UIBox* ServerDescriptionBox = new KlemmUI::UIBox(false, 0);
	KlemmUI::UITextField* ServerSearchBox = nullptr;
public:
	void JoinServerDirect();
	KlemmUI::UIText* ServerDescriptionText = nullptr;
	KlemmUI::UIText* ReloadText = nullptr;
	KlemmUI::UIText* ServerHeader = nullptr;
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

			Thunderstore::Package GetTsPackage() const;
		};

		std::vector<ServerMod> RequiredMods;
		int PlayerCount = 0, MaxPlayerCount = 0;
	};

	ServerEntry SelectedServer;

	std::vector<ServerEntry> DisplayedServerEntries;
	std::vector<KlemmUI::UIButton*> ServerBrowserButtons;
	std::string SearchText = "";

	void DisplayServerDescription(ServerEntry e);

	static void AddSeperator(KlemmUI::UIBox* Parent);

	KlemmUI::UIText* PlayerCountText = new KlemmUI::UIText(12, 1, "Players in game: ??", UI::Text);
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