#include <KlemmUI/Application.h>
#include <KlemmUI/UI/UIBackground.h>
#include <KlemmUI/Rendering/Texture.h>
#include <KlemmUI/Markup/Markup.h>

#include <fstream>

#include "Game.h"
#include "Log.h"
#include "Installer.h"
#include "UI/UIDef.h"
#include "Networking.h"
#include "BackgroundTask.h"
#include "WindowFunctions.h"
#include "Thunderstore.h"
#include "Translation.h"

#include "UI/Download.h"
#include "UI/TitleBar.h"
#include "UI/Sidebar.h"

#ifdef TF_PLUGIN
#include "TetherPlugin.h"
#endif
#include "AppUpdate.h"

using namespace KlemmUI;

namespace Installer
{
	Window* MainWindow = nullptr;
	std::string CurrentPath = "";

	Vector3f InstallerThemeColor = Vector3f(0.3f, 0.5f, 1);

#ifdef CI_BUILD
#define _STR(x) _XSTR(x)
#define _XSTR(x) std::string(#x)
#if CI_BUILD == -1
	const std::string InstallerVersion = "Build " + std::string(__DATE__) + ", " + std::string(__TIME__);
#else
	const std::string InstallerVersion = "Build #" + _STR(CI_BUILD);
#endif
#else
	const std::string InstallerVersion = "DevBuild";
#endif
	const std::string GitHubInstallerVersion = InstallerVersion;
#if DEBUG
	const std::string UserAgent = "TetherNSInstaller/" + InstallerVersion + "-dev";
#else
	const std::string UserAgent = "TetherNSInstaller/" + Installer::InstallerVersion;
#endif


	UIBackground* InstallerBackground = nullptr;
	Texture::TextureInfo BackgroundImage;

	void SetInstallerBackgroundImage(std::string Image)
	{
		if (BackgroundImage.ID)
		{
			Texture::UnloadTexture(BackgroundImage.ID);
		}
		BackgroundImage = Texture::LoadTextureWithInfo(Image);
		InstallerBackground->SetUseTexture(true, BackgroundImage.ID);
		InstallerBackground->SetMinSize(Vector2f(2.5 * ((float)BackgroundImage.Width / (float)BackgroundImage.Height), 2.5));
	}

#ifdef TF_PLUGIN
	bool* IsRunningPtr = nullptr;
#endif
}

void Installer::UpdateWindowFlags()
{
	MainWindow->SetWindowFlags(TitleBar::GetUseSystemTitleBar() ? Window::WindowFlag::Resizable : (Window::WindowFlag::Borderless | Window::WindowFlag::Resizable));
	TitleBar::GenerateWindowButtons();
}

void Installer::SetThemeColor(Vector3f NewColor)
{
	InstallerThemeColor = NewColor;

	SidebarClass::GenerateTabs();
	MainWindow->BorderColor = NewColor;
}

Vector3f Installer::GetThemeColor()
{
	return InstallerThemeColor;
}

KlemmUI::UIButton* Installer::SetButtonColorIfSelected(KlemmUI::UIButton* Button, bool IsSelected)
{
	if (IsSelected)
	{
		Button->SetColor(InstallerThemeColor);
		Button->SetHoveredColor(InstallerThemeColor);
		Button->SetPressedColor(InstallerThemeColor);
	}
	else
	{
		Button->SetColor(1);
		Button->SetHoveredColor(Vector3f::Lerp(1, InstallerThemeColor, 0.5f));
		Button->SetPressedColor(InstallerThemeColor);
	}
	Button->RedrawElement();
	return Button;
}

int main(int argc, char** argv)
{
	using namespace Translation;
	using namespace Installer;

	for (int i = 0; i < argc; i++)
	{
		if (argv[i] == std::string("-systemtitlebar"))
		{
			TitleBar::SetUseSystemTitleBar(true);
		}
	}
	
	if (!std::filesystem::exists("Data/shaders/postprocess.vert"))
	{
#if _WIN32
		std::string ProgramLocation = WindowFunc::GetAppPath();
#else
		std::string ProgramLocation = argv[0];
#endif
		CurrentPath = (ProgramLocation.substr(0, ProgramLocation.find_last_of("/\\")));
	}
	else
	{
		CurrentPath = std::filesystem::current_path().u8string();
	}
	CurrentPath.append("/");

	Application::Initialize(CurrentPath + "Data/shaders");
	
	LoadTranslation(GetLastTranslation());

	Window AppWindow = Window("TetherInstaller", Window::WindowFlag::Resizable | Window::WindowFlag::Borderless);
	AppWindow.SetMinSize(Vector2ui(640, 480));
	AppWindow.UI.SetTexturePath("Data/icons");

	MainWindow = &AppWindow;

	SetThemeColor(GetThemeColor());

	Application::Error::SetErrorCallback([](std::string Message)
		{
			WindowFunc::ShowPopupError("-- Internal UI Error --\n\n" + Message);
		});

	MainWindow->IsAreaGrabbableCallback = ([](Window*)
		{ 
			return TitleBar::IsHovered();
		});
	Log::Print("Created app window");
	UI::LoadFonts();
	AppWindow.Markup.SetDefaultFont(UI::Text);

	Log::Print("Cleaning up temp directory");
	{
		std::vector<std::string> TargetDirs = 
		{
			"Data/temp/mod",
			"Data/temp/net/tspage.txt",
			"Data/temp/net/allmods.json",
			"Data/temp/net/servers.json"
		};

		for (const auto& i : TargetDirs)
		{
			if (std::filesystem::exists(Installer::CurrentPath + i))
			{
				std::filesystem::remove_all(Installer::CurrentPath + i);
			}
		}
	}
	InstallerBackground = new UIBackground(true, 0, 1, 0);
	InstallerBackground->SetSizeMode(UIBox::SizeMode::AspectRelative);
	if (std::filesystem::exists(Installer::CurrentPath + "Data/var/custom_background.png"))
	{
		SetInstallerBackgroundImage(Installer::CurrentPath + "Data/var/custom_background.png");
	}
	else
	{
		SetInstallerBackgroundImage(Installer::CurrentPath + "Data/Game.png");
	}

	InstallerBackground->SetPosition(Vector2f(0.0) - InstallerBackground->GetUsedSize() / 2);

	Networking::Init();
	TitleBar::Load();

	Game::GamePath = Game::GetTitanfallLocation();

#ifdef TF_PLUGIN
	ProfileTab::DetectProfiles();

	ProfileTab::Profile NewProfile;
	NewProfile.DisplayName = Plugin::GetCurrentProfile();
	NewProfile.Path = Game::GamePath + "\\" + NewProfile.DisplayName;
	ProfileTab::CurrentProfile = NewProfile;

	std::filesystem::create_directories(Installer::CurrentPath + "Data/var");
#endif

	SidebarClass::Load();
	AppUpdate::Check();

	Log::Print("Successfully started launcher");
	float PrevAspect = MainWindow->GetAspectRatio();

	bool ShouldQuit = false;
	while (true)
	{
		ShouldQuit = !MainWindow->UpdateWindow();

		UITab::UpdateAll();

		InstallerBackground->SetPosition(Vector2f(0.0) - InstallerBackground->GetUsedSize() / 2);
		BackgroundTask::UpdateTaskStatus();

		TitleBar::Update();
		SidebarClass::Update();

		DownloadWindow::Update();

		if (MainWindow->GetAspectRatio() != PrevAspect)
		{
			PrevAspect = MainWindow->GetAspectRatio();
			TitleBar::GenerateWindowButtons();
		}

		AppUpdate::Update();

#ifndef TF_PLUGIN
		if (ShouldQuit && !DownloadWindow::IsDownloading)
		{
			break;
		}
#else
		Plugin::Update();
		if (ShouldQuit && !DownloadWindow::IsDownloading)
		{
			Plugin::HideWindow();
			if (IsRunningPtr)
			{
				*IsRunningPtr = false;
			}
			ShouldQuit = false;
		}
#endif
#if !DEBUG
		if (AppUpdate::RequiresUpdate)
		{
			AppUpdate::RequiresUpdate = false;
			new BackgroundTask(AppUpdate::UpdateInstaller);
		}
#endif
	}
	Networking::Cleanup();
}

#ifdef TF_PLUGIN
#include <Windows.h>

std::thread* TetherThread = nullptr;

extern "C" _declspec(dllexport) void LoadInstaller(Log::LogFn fn, bool* ReloadPtr, char* ServerConnectPtr, bool* InIsRunningPtr)
{

	Installer::IsRunningPtr = InIsRunningPtr;

	Log::OverrideLogFunction(fn);
	Plugin::SetReloadModsBoolPtr(ReloadPtr);
	Plugin::SetConnectToServerPtr(ServerConnectPtr);

	if (TetherThread)
	{
		Plugin::ShowWindow();
		return;
	}

	static char path[MAX_PATH];
	HMODULE hm = NULL;

	if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)&LoadInstaller, &hm) == 0)
	{
		int ret = GetLastError();
		fprintf(stderr, "GetModuleHandle failed, error = %d\n", ret);
		// Return or however you want to handle an error.
	}
	if (GetModuleFileNameA(hm, path, sizeof(path)) == 0)
	{
		int ret = GetLastError();
		fprintf(stderr, "GetModuleFileName failed, error = %d\n", ret);
		// Return or however you want to handle an error.
	}

	char** argv = new char* [] { path };
	TetherThread = new std::thread(main, 1, argv);
}

#elif _WIN32
int WinMain()
{
	try
	{
		main(__argc, __argv);
	}
	catch (std::exception& e)
	{
		WindowFunc::ShowPopupError(e.what());
	}
}
#endif