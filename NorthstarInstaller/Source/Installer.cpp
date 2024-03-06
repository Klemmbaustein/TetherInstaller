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

namespace Installer
{
	std::string TitleText = "";
	std::string CurrentPath = "";
	UIBox* WindowButtonBox = nullptr;
	size_t SelectedTab = 0;
	std::vector<UIButton*> TabButtons;
	std::vector<UITab*> Tabs;
	UIBackground* SidebarBackground = nullptr;
	size_t HoveredTab = 0;
	bool UseSystemTitleBar = false;

	Vector3f32 InstallerThemeColor = Vector3f32(0.3f, 0.5f, 1);

	UIButtonStyle* TabStyles[2] = { new UIButtonStyle("Tab default style"), new UIButtonStyle("Tab selected style")};

	UIText* AppTitle;

	std::vector<unsigned int> WindowButtonsIcons;

#ifdef CI_BUILD
#define _STR(x) _XSTR(x)
#define _XSTR(x) std::string(#x)
	const std::string InstallerVersion = "Build #" + _STR(CI_BUILD);
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

		Log::Print("Generating tab bar layout");
		TabButtons.clear();

		SidebarBackground->DeleteChildren();
		for (size_t i = 0; i < Tabs.size(); i++)
		{
			Tabs[i]->Background->IsVisible = SelectedTab == i;

			auto Button = new UIButton(true, 0, TabStyles[(int)(SelectedTab == i)], [](int Index)
				{
					Tabs.at(Index)->OnClicked();
					SelectedTab = Index;
					GenerateTabs();
				}, (int)i);
			Button->SetHorizontalAlign(UIBox::Align::Centered);
			SidebarBackground->AddChild(Button
				->SetSizeMode(UIBox::SizeMode::AspectRelative)
				->SetPaddingSizeMode(UIBox::SizeMode::AspectRelative)
				->SetBorder(UIBox::BorderType::Rounded, 0.3)
				->SetPadding(0.01, 0.01, 0.01, 0.01)
				->AddChild((new UIBackground(true, 0, 0, 0.1))
					->SetUseTexture(true, Icon("itab_" + Tabs[i]->Name).TextureID)
					->SetPaddingSizeMode(UIBox::SizeMode::AspectRelative)
					->SetSizeMode(UIBox::SizeMode::AspectRelative)));
			TabButtons.push_back(Button);
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
		Window::ShowPopupError(Translation::GetTranslation("popup_linux_update"));
		return;
#endif
		BackgroundTask::SetStatus("dl_" + Translation::GetTranslation("download_update_installer"));
		if (Window::ShowPopupQuestion(
			Translation::GetTranslation("popup_windows_update_title"),
			Translation::GetTranslation("popup_windows_update"))
			!= Window::PopupReply::Yes)
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
			->AddChild((new UIText(0.4, 1, Translation::GetTranslation("tab_" + Tabs[TabIndex]->Name), UI::Text))
				->SetPadding(0.01, 0.0, 0.01, 0.01))
			->AddChild((new UIText(0.3, 0.8, Translation::GetTranslation("tab_" + Tabs[TabIndex]->Name + "_description"), UI::Text))
				->SetPadding(0, 0.01, 0.01, 0.01));
	}

#ifdef TF_PLUGIN
	bool* IsRunningPtr = nullptr;
#endif
}

void Installer::UpdateWindowFlags()
{
	Application::SetWindowFlags(UseSystemTitleBar ? 0 : Application::BORDERLESS_BIT);
	GenerateWindowButtons();
}

void Installer::SetThemeColor(Vector3f32 NewColor)
{
	InstallerThemeColor = NewColor;
	TabStyles[0]->Color = Vector3f32(1.0f, 1.0f, 1.0f);
	TabStyles[0]->HoveredColor = Vector3f32::Lerp(1.0f, InstallerThemeColor, 0.5f);
	TabStyles[0]->PressedColor = InstallerThemeColor;
	TabStyles[0]->SetPadding(0.005);
	TabStyles[0]->Border = UIBox::BorderType::Rounded;
	TabStyles[0]->BorderSize = 0.4;

	TabStyles[1]->Color = InstallerThemeColor;
	TabStyles[1]->HoveredColor = InstallerThemeColor;
	TabStyles[1]->PressedColor = InstallerThemeColor;
	TabStyles[1]->SetPadding(0.005);
	TabStyles[1]->Border = UIBox::BorderType::Rounded;
	TabStyles[1]->BorderSize = 0.4;

	GenerateTabs();
	Application::SetBorderlessWindowOutlineColor(NewColor);
}

Vector3f32 Installer::GetThemeColor()
{
	return InstallerThemeColor;
}

void Installer::GenerateWindowButtons()
{
	WindowButtonBox->DeleteChildren();
	if (UseSystemTitleBar)
	{
		return;
	}


	std::vector<int> Buttons;
	if (Application::GetFullScreen())
	{
		Buttons = { 0, 2, 3 };
	}
	else
	{
		Buttons = { 0, 1, 3 };
	}
	for (int i : Buttons)
	{
		Vector3f32 HoveredColor = 0.3f;
		if (i == 0)
		{
			HoveredColor = Vector3f32(0.5, 0, 0);
		}
		WindowButtonBox->AddChild((new UIButton(true, 0, 0.1, [](int Index) {
			switch (Index)
			{
			case 0:
				Application::Quit = true;
				break;
			case 1:
			case 2:
				Application::SetFullScreen(!Application::GetFullScreen());
				GenerateWindowButtons();
				break;
			case 3:
				Application::Minimize();
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
			->AddChild((new UIBackground(true, 0, 1, Vector2(0.03)))
				->SetUseTexture(true, WindowButtonsIcons[i])
				->SetSizeMode(UIBox::SizeMode::PixelRelative)
				->SetPadding(Application::GetFullScreen() ? 0.0275 : 0.015,
					0.015,
					0.015,
					(Application::GetFullScreen() && i == 0) ? 0.02 : 0.015)));
	}

	if (!DownloadWindow::IsDownloading)
	{
		return;
	}

	WindowButtonBox->AddChild((new UIButton(true, 0, Vector3f32(0.1), []() {
		DownloadWindow::SetWindowVisible(true);
		}))
		->SetHoveredColor(0.3f)
		->SetPressedColor(0.15f)
		->SetPadding(0, 0, 0, 0.25)
		->AddChild((new UIBackground(true, 0, 1, Vector2(0.045)))
			->SetUseTexture(true, Icon("Download").TextureID)
			->SetSizeMode(UIBox::SizeMode::PixelRelative)
			->SetPadding(Application::GetFullScreen() ? 0.0175 : 0.005,
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

	SetThemeColor(GetThemeColor());

	LoadTranslation(GetLastTranslation());

	Application::SetBorderlessWindowOutlineColor(InstallerThemeColor);
	Application::SetShaderPath(CurrentPath + "Data/shaders");
	Application::SetErrorMessageCallback([](std::string Message)
		{
			Window::ShowPopupError("-- Internal UI Error --\n\n" + Message);
		});
	Application::Initialize("TetherInstaller " + Installer::InstallerVersion, UseSystemTitleBar ? 0 : Application::BORDERLESS_BIT);
	Application::SetWindowMovableCallback([]() 
		{ 
			return WindowButtonBox->IsBeingHovered();
		});
	Log::Print("Created app window");
	UI::LoadFonts();


	WindowButtonsIcons =
	{
		Texture::LoadTexture(Installer::CurrentPath + "Data/WindowX.png"),
		Texture::LoadTexture(Installer::CurrentPath + "Data/WindowResize.png"),
		Texture::LoadTexture(Installer::CurrentPath + "Data/WindowResize2.png"),
		Texture::LoadTexture(Installer::CurrentPath + "Data/WindowMin.png"),
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
	
	AppTitle = new UIText(0.3, 1, "TetherInstaller - Loading...", UI::Text);
	AppTitle->SetPosition(Vector2f(-1, 0.9));
	AppTitle->SetTextSizeMode(UIBox::SizeMode::PixelRelative);

	Application::UpdateWindow();
	InstallerBackground->SetPosition(Vector2f(0.0) - InstallerBackground->GetUsedSize() / 2);
	AppTitle->SetPosition(Vector2f(-1, WindowButtonBox->GetPosition().Y));

	Application::UpdateWindow();

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
#ifndef TF_PLUGIN // Don't implement anything relating to plugin yet.
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
	float PrevAspect = Application::AspectRatio;

#ifdef TF_PLUGIN
	while (true)
#else
	while (!Application::Quit)
#endif
	{
		for (auto i : Tabs)
		{
			i->Background->SetPosition(Vector2f(
				SidebarBackground->GetPosition().X + SidebarBackground->GetUsedSize().X,
				-1));
			i->Background->SetMinSize(Vector2f(
				2 - SidebarBackground->GetUsedSize().X,
				2 - WindowButtonBox->GetUsedSize().Y));
			i->Background->SetMaxSize(Vector2f(
				2 - SidebarBackground->GetUsedSize().X,
				2 - WindowButtonBox->GetUsedSize().Y));
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
			Application::SetApplicationTitle(Title);
		}
		else if (!UseSystemTitleBar)
		{
			AppTitle->SetText(Title);
		}
		AppTitle->SetPosition(Vector2f(SidebarBackground->GetUsedSize().X - 0.99, WindowButtonBox->GetPosition().Y + 0.001));
		DownloadWindow::Update(WindowButtonBox->GetUsedSize().Y);

		if (Application::AspectRatio != PrevAspect)
		{
			PrevAspect = Application::AspectRatio;
			AppTitle->SetTextSize(Application::GetFullScreen() ? 0.4 : 0.3);
			GenerateWindowButtons();
		}


		if (!BackgroundTask::IsRunningTask && !LaunchTasks.empty())
		{
			new BackgroundTask(LaunchTasks.front());
			LaunchTasks.pop();
		}

		Application::UpdateWindow();
		Application::SetActiveMouseCursor(Application::GetMouseCursorFromHoveredButtons());
#ifndef TF_PLUGIN
		if (Application::Quit && BackgroundTask::IsRunningTask)
		{
			Application::Quit = false;
		}
#else
		Plugin::Update();
		if (Application::Quit && !BackgroundTask::IsRunningTask)
		{
			Plugin::HideWindow();
			if (IsRunningPtr)
			{
				*IsRunningPtr = false;
			}
			Application::Quit = false;
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
		Window::ShowPopupError(e.what());
	}
}
#endif