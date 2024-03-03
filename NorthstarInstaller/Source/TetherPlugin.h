#pragma once

#ifdef TF_PLUGIN
namespace Plugin
{
	void SetReloadModsBoolPtr(bool* ptr);
	// Reloads all northstar mods.
	void ReloadMods();
}
#endif