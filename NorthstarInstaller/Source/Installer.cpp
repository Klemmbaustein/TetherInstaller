#include <KlemmUI/Application.h>
#include <KlemmUI/UI/UIBackground.h>
#include <KlemmUI/Rendering/Texture.h>

#include <queue>
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

#include "Tabs/LaunchTab.h"
#include "Tabs/SettingsTab.h"
#include "Tabs/ModsTab.h"
#include "Tabs/ProfileTab.h"
#include "Tabs/ServerBrowserTab.h"

#include "UI/Icon.h"
#include "UI/Download.h"

#ifdef TF_PLUGIN
#include "TetherPlugin.h"
#endif

using namespace KlemmUI;

namespace Installer
{
	Window* MainWindow = nullptr;
	std::string TitleText = "";
	std::string CurrentPath = "";
	UIBox* WindowButtonBox = nullptr;
	size_t SelectedTab = 0;
	std::vector<UIButton*> TabButtons;
	std::vector<UITab*> Tabs;
	UIBackground* SidebarBackground = nullptr;
	size_t HoveredTab = 0;
	bool UseSystemTitleBar = false;

	Vector3f InstallerThemeColor = Vector3f(0.3f, 0.5f, 1);

	UIText* AppTitle;

	std::vector<unsigned int> WindowButtonsIcons;

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
	const std::string GithubInstallerVersion = InstallerVersion;
#if DEBUG
	const std::string UserAgent = "TetherNSInstaller/" + InstallerVersion + "-dev";
#else
	const std::string UserAgent = "TetherNSInstaller/" + Installer::InstallerVersion;
#endif
	void GenerateTabs()
	{
		if (!SidebarBackground)
		{
			return;
		}

		TabButtons.clear();

		SidebarBackground->DeleteChildren();
		for (size_t i = 0; i < Tabs.size(); i++)
		{
			Tabs[i]->Background->IsVisible = SelectedTab == i;

			auto Button = new UIButton(true, 0, Installer::GetThemeColor(), [](int Index)
				{
					Tabs.at(Index)->OnClicked();
					SelectedTab = Index;
					GenerateTabs();
				}, (int)i);
			Button->SetHorizontalAlign(UIBox::Align::Centered);
			SidebarBackground->AddChild(Button
				->SetSizeMode(UIBox::SizeMode::PixelRelative)
				->SetPaddingSizeMode(UIBox::SizeMode::PixelRelative)
				->SetBorder(UIBox::BorderType::Rounded, 0.3)
				->SetPadding(6, 0, 6, 6)
				->AddChild((new UIBackground(true, 0, 0, 54))
					->SetUseTexture(true, Icon("itab_" + Tabs[i]->Name).TextureID)
					->SetPaddingSizeMode(UIBox::SizeMode::PixelRelative)
					->SetPadding(2)
					->SetSizeMode(UIBox::SizeMode::PixelRelative)));
			TabButtons.push_back(SetButtonColorIfSelected(Button, i == SelectedTab));
		}
		SidebarBackground->UpdateElement();
	}

	void CheckForUpdates()
	{
		// Don't update the game while it's running.
#if TF_PLUGIN
		return;
#endif
		Log::Print("Checking for updates...");
		BackgroundTask::SetStatus("Checking for updates");
		BackgroundTask::SetProgress(0.999);

		Thunderstore::Package ReleaseCanidate;

		ReleaseCanidate.Name = "NorthstarReleaseCandidate";
		ReleaseCanidate.Author = "northstar";
		ReleaseCanidate.Namespace = "northstar";

		if (Thunderstore::IsModInstalled(ReleaseCanidate))
		{
			return;
		}

		std::string Latest = Networking::GetLatestReleaseOf("R2Northstar/Northstar");
		
		if (Latest.empty())
		{
			Log::Print("Could not get the latest game version",  Log::Error);
			BackgroundTask::SetStatus("Could not get the latest game version");
			BackgroundTask::SetProgress(1);
			return;
		}

		std::string InstalledVersion = Game::GetCurrentVersion();

		if (Latest != InstalledVersion)
		{
			Log::Print("Game needs to be updated. Installed: " + InstalledVersion + ", latest: " + Latest, Log::Warning);
			Game::RequiresUpdate = true;
			BackgroundTask::SetStatus("Update required");
			BackgroundTask::SetProgress(1);			
			return;
		}
		BackgroundTask::SetStatus("No update required");
		Log::Print("No update required");
	}

	std::atomic<bool> RequiresUpdate = false;

	void CheckForInstallerUpdate()
	{
#if DEBUG || TF_PLUGIN
		return;
#endif
		Log::Print("Checking for installer updates...");
		BackgroundTask::SetStatus("Checking for installer updates");
		BackgroundTask::SetProgress(0.999);

		std::string Ver = Networking::GetLatestReleaseOf("Klemmbaustein/TetherInstaller");
		if (Ver.empty())
		{
			return;
		}
		if (GithubInstallerVersion != Ver)
		{
			RequiresUpdate = true;
			Log::Print("Launcher requires update. " + GithubInstallerVersion + " -> " + Ver , Log::Warning);
		}
	}
	std::queue<void (*)()> LaunchTasks =
	std::queue<void (*)()>({
		CheckForUpdates,
		CheckForInstallerUpdate,
		ModsTab::CheckForModUpdates
	});

	void UpdateInstaller()
	{
#if __linux__
		WindowFunc::ShowPopupError(Translation::GetTranslation("popup_linux_update"));
		return;
#endif
		BackgroundTask::SetStatus("dl_" + Translation::GetTranslation("download_update_installer"));
		if (WindowFunc::ShowPopupQuestion(
			Translation::GetTranslation("popup_windows_update_title"),
			Translation::GetTranslation("popup_windows_update"))
			!= WindowFunc::PopupReply::Yes)
		{
			return;
		}
		Networking::DownloadLatestReleaseOf("Klemmbaustein/TetherInstaller", "Windows");
		Networking::ExtractZip("Data/temp/net/latest.zip", "Data/temp/install");

		system("start update.bat");
		exit(0);
	}

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

	UIBackground* HoveredTabName = nullptr;

	void GenerateTabName(size_t TabIndex)
	{
		if (HoveredTabName)
		{
			delete HoveredTabName;
			HoveredTabName = nullptr;
		}
		if (TabIndex == SIZE_MAX)
		{
			return;
		}

		HoveredTabName = new UIBackground(false,
			Vector2f(SidebarBackground->GetPosition().X + SidebarBackground->GetUsedSize().X, TabButtons[TabIndex]->GetPosition().Y),
			0,
			Vector2f(0, TabButtons[TabIndex]->GetUsedSize().Y));

		HoveredTabName
			->SetOpacity(0.75)
			->SetBorder(UIBox::BorderType::Rounded, 0.25)
			->AddChild((new UIText(16, 1, Translation::GetTranslation("tab_" + Tabs[TabIndex]->Name), UI::Text))
				->SetTextSizeMode(UIBox::SizeMode::PixelRelative)
				->SetPaddingSizeMode(UIBox::SizeMode::PixelRelative)
				->SetPadding(5, 0, 5, 15))
			->AddChild((new UIText(12, 0.8, Translation::GetTranslation("tab_" + Tabs[TabIndex]->Name + "_description"), UI::Text))
				->SetTextSizeMode(UIBox::SizeMode::PixelRelative)
				->SetPaddingSizeMode(UIBox::SizeMode::PixelRelative)
				->SetPadding(0, 5, 5, 15));
	}

#ifdef TF_PLUGIN
	bool* IsRunningPtr = nullptr;
#endif
}

void Installer::UpdateWindowFlags()
{
	MainWindow->SetWindowFlags(UseSystemTitleBar ? Window::WindowFlag::Resizable : (Window::WindowFlag::Borderless | Window::WindowFlag::Resizable));
	GenerateWindowButtons();
}

void Installer::SetThemeColor(Vector3f NewColor)
{
	InstallerThemeColor = NewColor;

	GenerateTabs();
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
	return Button;
}

void Installer::GenerateWindowButtons()
{
	WindowButtonBox->DeleteChildren();
	if (UseSystemTitleBar)
	{
		return;
	}

	std::vector<int> Buttons;
	if (MainWindow->GetWindowFullScreen())
	{
		Buttons = { 0, 2, 3 };
	}
	else
	{
		Buttons = { 0, 1, 3 };
	}
	for (int i : Buttons)
	{
		Vector3f HoveredColor = 0.3f;
		if (i == 0)
		{
			HoveredColor = Vector3f(0.5, 0, 0);
		}
		WindowButtonBox->AddChild((new UIButton(true, 0, 0.1f, [](int Index) {
			switch (Index)
			{
			case 0:
				MainWindow->Close();
				break;
			case 1:
			case 2:
				MainWindow->SetWindowFullScreen(!MainWindow->GetWindowFullScreen());
				GenerateWindowButtons();
				break;
			case 3:
				//Application::Minimize();
				break;
			default:
				break;
			}
			}, i))
			->SetHoveredColor(HoveredColor)
			->SetPressedColor(HoveredColor * 0.5)
			->SetMinSize(0)
			->SetPadding(0)
			->SetSizeMode(UIBox::SizeMode::PixelRelative)
			->AddChild((new UIBackground(true, 0, 1, 16))
				->SetUseTexture(true, WindowButtonsIcons[i])
				->SetSizeMode(UIBox::SizeMode::PixelRelative)
				->SetPaddingSizeMode(UIBox::SizeMode::PixelRelative)
				->SetPadding(MainWindow->GetWindowFullScreen() ? 18 : 10,
					10,
					16,
					(MainWindow->GetWindowFullScreen() && i == 0) ? 24 : 16)));
	}

	if (!DownloadWindow::IsDownloading)
	{
		return;
	}

	WindowButtonBox->AddChild((new UIButton(true, 0, Vector3f(0.1), []() {
		DownloadWindow::SetWindowVisible(true);
		}))
		->SetHoveredColor(0.3f)
		->SetPressedColor(0.15f)
		->SetPadding(0, 0, 0, 0.25)
		->AddChild((new UIBackground(true, 0, 1, Vector2(0.045)))
			->SetUseTexture(true, Icon("Download").TextureID)
			->SetSizeMode(UIBox::SizeMode::PixelRelative)
			->SetPadding(true ? 0.0175 : 0.005,
				0.005,
				0.015,
				0.015)));
}


int main(int argc, char** argv)
{
	using namespace Translation;
	using namespace Installer;

	for (int i = 0; i < argc; i++)
	{
		if (argv[i] == std::string("-systemtitlebar"))
		{
			UseSystemTitleBar = true;
		}
	}
	
	if (!std::filesystem::exists("Data/shaders/postprocess.vert"))
	{
		std::string ProgramLocation = argv[0];
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

	MainWindow = &AppWindow;

	SetThemeColor(GetThemeColor());

	Application::Error::SetErrorCallback([](std::string Message)
		{
			WindowFunc::ShowPopupError("-- Internal UI Error --\n\n" + Message);
		});

	MainWindow->IsAreaGrabbableCallback = ([](Window*)
		{ 
			return WindowButtonBox->IsBeingHovered();
		});
	Log::Print("Created app window");
	UI::LoadFonts();


	WindowButtonsIcons =
	{
		Texture::LoadTexture(Installer::CurrentPath + "/Data/WindowX.png"),
		Texture::LoadTexture(Installer::CurrentPath + "/Data/WindowResize.png"),
		Texture::LoadTexture(Installer::CurrentPath + "/Data/WindowResize2.png"),
		Texture::LoadTexture(Installer::CurrentPath + "/Data/WindowMin.png"),
	};

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
	Vector2f bgCenter = Vector2f(-0.3, 0);
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

	WindowButtonBox = (new UIBackground(true, 0, 0.1))
		->SetPadding(0)
		->SetHorizontalAlign(UIBox::Align::Reverse)
		->SetMinSize(Vector2f(2, 0.0));
	(new UIBox(false, Vector2f(-1, 0.7)))
		->SetMinSize(Vector2f(2, 0.3))
		->AddChild(WindowButtonBox);
	GenerateWindowButtons();
	
	AppTitle = new UIText(14, 1, "", UI::Text);
	AppTitle->SetTextSizeMode(UIBox::SizeMode::PixelRelative);
;

	InstallerBackground->SetPosition(Vector2f(0.0) - InstallerBackground->GetUsedSize() / 2);

	Networking::Init();

	Game::GamePath = Game::GetTitanfallLocation();

#ifdef TF_PLUGIN
	ProfileTab::DetectProfiles();

	ProfileTab::Profile NewProfile;
	NewProfile.DisplayName = Plugin::GetCurrentProfile();
	NewProfile.Path = Game::GamePath + "\\" + NewProfile.DisplayName;
	ProfileTab::CurrentProfile = NewProfile;

	std::filesystem::create_directories(Installer::CurrentPath + "Data/var");
#endif

	Log::Print("--- Loading tabs ---");
	Installer::Tabs =
	{
		new LaunchTab(),
		new ServerBrowserTab(),
		new ModsTab(),
#ifndef TF_PLUGIN // Haven't implemented anything relating to profiles with plugins yet.
		new ProfileTab(),
#endif
		new SettingsTab(),
	};

	SidebarBackground = new UIBackground(false, Vector2f(-1, -1), 0, Vector2f(0, 2));
	SidebarBackground
		->SetOpacity(0.75);
	GenerateTabs();

	if (std::filesystem::exists(ProfileTab::CurrentProfile.Path + "/mods/autojoin"))
	{
		std::filesystem::remove_all(ProfileTab::CurrentProfile.Path + "/mods/autojoin");
	}

	Log::Print("Successfully started launcher");
	float PrevAspect = MainWindow->GetAspectRatio();

	bool ShouldQuit = false;
	while (true)
	{
		ShouldQuit = !MainWindow->UpdateWindow();
		for (auto i : Tabs)
		{
			i->Background->SetPosition(Vector2f(
				SidebarBackground->GetPosition().X + SidebarBackground->GetUsedSize().X,
				-1));

			Vector2f BackgroundSize = Vector2f(2) - Vector2f(
				SidebarBackground->GetUsedSize().X,
				WindowButtonBox->GetUsedSize().Y);

			i->Background->SetMinSize(BackgroundSize);
			i->Background->SetMaxSize(BackgroundSize);
			i->Tick();
		}

		bool AnyButtonHovered = false;
		for (size_t i = 0; i < TabButtons.size(); i++)
		{
			if (TabButtons[i]->GetIsHovered())
			{
				AnyButtonHovered = true;
				if (HoveredTab != i)
				{
					HoveredTab = i;
					GenerateTabName(HoveredTab);
				}
			}
		}
		if (!AnyButtonHovered && HoveredTab != SIZE_MAX)
		{
			HoveredTab = SIZE_MAX;
			GenerateTabName(HoveredTab);
		}

		InstallerBackground->SetPosition(Vector2f(0.0) - InstallerBackground->GetUsedSize() / 2);
		BackgroundTask::UpdateTaskStatus();

		std::string ProfileName = ProfileTab::CurrentProfile.DisplayName;
		if (ProfileName.empty())
		{
			ProfileName = "None";
		}

		std::string AppNameString = GetTranslation("app_name");

#if TF_PLUGIN
		AppNameString.append(" (Northstar Plugin)");
#endif

		std::string Title = Format(GetTranslation("title_bar"),
			AppNameString.c_str(),
			GetTranslation("tab_" + Tabs[SelectedTab]->Name).c_str(),
			ProfileName.c_str());

		if (UseSystemTitleBar && TitleText != Title)
		{
			TitleText = Title;
			MainWindow->SetTitle(Title);
		}
		else if (!UseSystemTitleBar)
		{
			AppTitle->SetText(Title);
		}
		AppTitle->SetPosition(Vector2f(SidebarBackground->GetUsedSize().X - 0.99, WindowButtonBox->GetPosition().Y + 0.005));
		DownloadWindow::Update(WindowButtonBox->GetUsedSize().Y);

		if (MainWindow->GetAspectRatio() != PrevAspect)
		{
			PrevAspect = MainWindow->GetAspectRatio();
			GenerateWindowButtons();
		}


		if (!BackgroundTask::IsRunningTask && !LaunchTasks.empty())
		{
			new BackgroundTask(LaunchTasks.front());
			LaunchTasks.pop();
		}

		//Application::SetActiveMouseCursor(Application::GetMouseCursorFromHoveredButtons());
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
		if (RequiresUpdate)
		{
			RequiresUpdate = false;
			new BackgroundTask(UpdateInstaller);
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