#ifdef TF_PLUGIN
#include "TetherPlugin.h"
#include "Log.h"

static bool* ReloadModsPtr = nullptr;

void Plugin::SetReloadModsBoolPtr(bool* ptr)
{
	ReloadModsPtr = ptr;
}

void Plugin::ReloadMods()
{
	if (ReloadModsPtr)
	{
		*ReloadModsPtr = true;
	}
	else
	{
		Log::Print("Could not reload mods - value is nullptr", Log::Error);
	}
}
#endif