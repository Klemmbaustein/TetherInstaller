#ifdef TF_PLUGIN
#include "TetherPlugin.h"
#include "Log.h"

static bool* ReloadModsPtr = nullptr;
static char* ConnectToServerPtr = nullptr;

void Plugin::SetReloadModsBoolPtr(bool* ptr)
{
	ReloadModsPtr = ptr;
}

void Plugin::SetConnectToServerPtr(char* ptr)
{
	ConnectToServerPtr = ptr;
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
void Plugin::Connect(const std::string& ToUid)
{
	strcpy_s(ConnectToServerPtr, 128, ToUid.c_str());
}
#endif