#pragma once
#include <string>

#ifdef TF_PLUGIN
namespace Plugin
{
	void SetReloadModsBoolPtr(bool* ptr);
	void SetConnectToServerPtr(char* ptr);
	// Reloads all northstar mods.
	void ReloadMods();
	void Connect(const std::string& ToUid);

	void HideWindow();
	void ShowWindow();
	void ShowWindowFromThread();
	void Update();

	std::string GetCurrentProfile();
	std::string GetCurrentMap();
	std::string GetCurrentServer();
	std::string GetCurrentMode();
}
#endif