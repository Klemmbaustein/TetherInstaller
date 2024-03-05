#ifdef TF_PLUGIN
#include "TetherPlugin.h"
#include "Log.h"
#include <SDL.h>
#include <KlemmUI/Application.h>

#pragma comment(lib, "Shlwapi.lib")

static bool* ReloadModsPtr = nullptr;
static char* ConnectToServerPtr = nullptr;
static std::atomic<bool> ShouldShowWindow = false;

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
void Plugin::HideWindow()
{
	SDL_Window* Window = static_cast<SDL_Window*>(Application::GetSDLWindowPtr());
	SDL_HideWindow(Window);
}
void Plugin::ShowWindow()
{
	SDL_Window* Window = static_cast<SDL_Window*>(Application::GetSDLWindowPtr());
	SDL_ShowWindow(Window);
}
void Plugin::ShowWindowFromThread()
{
	ShouldShowWindow = true;
}
void Plugin::Update()
{
	if (ShouldShowWindow)
	{
		ShowWindow();
	}
}
#include <Windows.h>
#include <Shlwapi.h>

std::string Plugin::GetCurrentProfile()
{
	// Code pretty much copy-pasted from NorthstarLauncher.exe to determine the current profile.
	std::wstring strProfile = L"R2Northstar";
	wchar_t* clArgChar = StrStrW(GetCommandLineW(), L"-profile=");
	if (clArgChar)
	{
		std::wstring cla = clArgChar;
		if (cla.substr(9, 1) != L"\"")
		{
			size_t space = cla.find(L" ");
			std::wstring dirname = cla.substr(9, space - 9);
			strProfile = dirname;
		}
		else
		{
			std::wstring quote = L"\"";
			size_t quote1 = cla.find(quote);
			size_t quote2 = (cla.substr(quote1 + 1)).find(quote);
			std::wstring dirname = cla.substr(quote1 + 1, quote2);
			strProfile = dirname;
		}
	}

	char* Buffer = new char[strProfile.size() + 1]();

	WideCharToMultiByte(CP_UTF8, 0, strProfile.c_str(), strProfile.size(), Buffer, sizeof(Buffer), NULL, NULL);
	Log::Print(Buffer);
	std::string ProfleString = Buffer;
	delete[] Buffer;
	return ProfleString;
}
#endif