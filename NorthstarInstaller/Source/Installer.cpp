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
#include "UI/Download.h"

#include "Tabs/LaunchTab.h"
#include "Tabs/SettingsTab.h"
#include "Tabs/ModsTab.h"
#include "Tabs/ProfileTab.h"
#include "Tabs/ServerBrowserTab.h"
#include "UI/Icon.h"

namespace Installer
{
	UIBox* WindowButtonBox = nullptr;
	size_t SelectedTab = 0;
	std::vector<UIButton*> TabButtons;
	std::vector<UITab*> Tabs;
	UIBackground* SidebarBackground = nullptr;
	size_t HoveredTab = 0;
	UIButtonStyle* TabStyles[2] = { new UIButtonStyle("Tab default style"), new UIButtonStyle("Tab selected style")};

	UIText* AppTitle;

	std::vector<unsigned int> WindowButtonsIcons;

#ifdef CI_BUILD
#define _STR(x) _XSTR(x)
#define _XSTR(x) std::string(#x)
	const std::string InstallerVersion = "CI Build #" + _STR(CI_BUILD);
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
			Button->BoxAlign = UIBox::Align::Centered;
			SidebarBackground->AddChild(Button
				->SetSizeMode(UIBox::SizeMode::AspectRelative)
				->SetPaddingSizeMode(UIBox::SizeMode::AspectRelative)
				->SetBorder(UIBox::BorderType::Rounded, 0.3)
				->SetPadding(0.01, 0.01, 0.01, 0.01)
				->AddChild((new UIBackground(true, 0, 0, 0.1))
					->SetUseTexture(true, Icon("Tab_" + Tabs[i]->Name).TextureID)
					->SetPaddingSizeMode(UIBox::SizeMode::AspectRelative)
					->SetSizeMode(UIBox::SizeMode::AspectRelative)));
			TabButtons.push_back(Button);
		}
	}

	bool UpdateCheckedOnce = false;

	void CheckForUpdates()
	{
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
			if (!UpdateCheckedOnce)
			{
				UpdateCheckedOnce = true;
			}
			else
			{
				Window::ShowPopupError("Could not get the latest game version. Possible rate limit.");
			}
			return;
		}

		std::string InstalledVersion = Game::GetCurrentVersion();

		if (Latest != InstalledVersion)
		{
			Log::Print("Game needs to be updated. Installed: " + InstalledVersion + ", latest: " + Latest, Log::Warning);
			Game::RequiresUpdate = true;
			BackgroundTask::SetStatus("Update required");
			BackgroundTask::SetProgress(1);
			if (!UpdateCheckedOnce)
			{
				UpdateCheckedOnce = true;
			}
			else
			{
				Window::ShowPopup("Northstar update", "Northstar update required.");
			}
			return;
		}
		BackgroundTask::SetStatus("No update required");
		Log::Print("No update required");
		if (!UpdateCheckedOnce)
		{
			UpdateCheckedOnce = true;
		}
		else
		{
			Window::ShowPopup("Northstar update", "No Northstar update required.");
		}
	}

	std::atomic<bool> RequiresUpdate = false;

	void CheckForInstallerUpdate()
	{
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
		Window::ShowPopupError("A new version of Tether is avaliable but updating the installer is not yet supported on linux.\nPlease update manually.");
		return;
#endif
		BackgroundTask::SetStatus("dl_Updating installer");
		if (Window::ShowPopupQuestion("Update", "An update for the launcher is avaliabe.\nWould you like to install it?") != Window::PopupReply::Yes)
		{
			return;
		}
		Networking::DownloadLatestReleaseOf("Klemmbaustein/TetherInstaller", "Windows");
		Networking::ExtractZip("Data/temp/net/latest.zip", "Data/temp/install");

		system("start update.bat");
		exit(0);
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
			->AddChild((new UIText(0.3, 0.8, Tabs[TabIndex]->Description, UI::Text))
				->SetPadding(0, 0.01, 0.01, 0.01))
			->AddChild((new UIText(0.4, 1, Tabs[TabIndex]->Name, UI::Text))
				->SetPadding(0, 0.01, 0.01, 0.01));
	}
}

void Installer::GenerateWindowButtons()
{
	std::vector<int> Buttons;
	if (Application::GetFullScreen())
	{
		Buttons = { 0, 2, 3 };
	}
	else
	{
		Buttons = { 0, 1, 3 };
	}
	WindowButtonBox->DeleteChildren();
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


float BackgroundFade = 0;

int main(int argc, char** argv)
{
	using namespace Installer;

	if (!std::filesystem::exists("Shaders/postprocess.vert"))
	{
		std::string ProgramLocation = argv[0];
		std::filesystem::current_path(ProgramLocation.substr(0, ProgramLocation.find_last_of("/\\")));
	}

	Application::SetBorderlessWindowOutlineColor(Vector3f32(0.3f, 0.5f, 1));
	Application::SetShaderPath("Data/shaders");
	Application::SetErrorMessageCallback([](std::string Message)
		{
			Window::ShowPopupError("-- Internal UI Error --\n\n" + Message);
		});
	Application::Initialize("TetherInstaller " + Installer::InstallerVersion, Application::BORDERLESS_BIT);
	Application::SetWindowMovableCallback([]() 
		{ 
			return WindowButtonBox->IsBeingHovered();
		});
	Log::Print("Created app window");

	TabStyles[0]->Color			= Vector3f32(1.0f, 1.0f, 1.0f);
	TabStyles[0]->HoveredColor	= Vector3f32(0.6f, 0.7f, 1.0f);
	TabStyles[0]->PressedColor	= Vector3f32(0.3f, 0.5f, 1.0f);
	TabStyles[0]->SetPadding(0.005);
	TabStyles[0]->Border = UIBox::BorderType::Rounded;
	TabStyles[0]->BorderSize = 0.4;

	TabStyles[1]->Color			= Vector3f32(0.3f, 0.5f, 1.0f);
	TabStyles[1]->HoveredColor	= Vector3f32(0.3f, 0.5f, 1.0f);
	TabStyles[1]->PressedColor	= Vector3f32(0.3f, 0.5f, 1.0f);
	TabStyles[1]->SetPadding(0.005);
	TabStyles[1]->Border = UIBox::BorderType::Rounded;
	TabStyles[1]->BorderSize = 0.4;

	UI::LoadFonts();


	WindowButtonsIcons =
	{
		Texture::LoadTexture("Data/WindowX.png"),
		Texture::LoadTexture("Data/WindowResize.png"),
		Texture::LoadTexture("Data/WindowResize2.png"),
		Texture::LoadTexture("Data/WindowMin.png"),
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
			if (std::filesystem::exists(i))
			{
				std::filesystem::remove_all(i);
			}
		}
	}
	Vector2f bgCenter = Vector2f(-0.3, 0);
	auto bg = new UIBackground(true, 0, 1, Vector2f(2.5 * (16.f/9.f), 2.5));
	bg->SetSizeMode(UIBox::SizeMode::AspectRelative);
	bg->SetUseTexture(true, Texture::LoadTexture("Data/Game.png"));

	WindowButtonBox = (new UIBackground(true, 0, 0.1))
		->SetPadding(0)
		->SetAlign(UIBox::Align::Reverse)
		->SetMinSize(Vector2f(2, 0.0));
	WindowButtonBox->BoxAlign = UIBox::Align::Reverse;
	(new UIBox(false, Vector2f(-1, 0.7)))
		->SetMinSize(Vector2f(2, 0.3))
		->AddChild(WindowButtonBox)
		->SetAlign(UIBox::Align::Reverse);

	GenerateWindowButtons();

	AppTitle = new UIText(0.3, 1, "TetherInstaller - Loading...", UI::Text);
	AppTitle->SetPosition(Vector2f(-1, 0.9));
	AppTitle->SetTextSizeMode(UIBox::SizeMode::PixelRelative);

	Application::UpdateWindow();
	bg->SetPosition(Vector2f(0.0) - bg->GetUsedSize() / 2);
	AppTitle->SetPosition(Vector2f(-1, WindowButtonBox->GetPosition().Y));

	Application::UpdateWindow();

	Networking::Init();

	Game::GamePath = Game::GetTitanfallLocation();

	Log::Print("--- Loading tabs ---");
	Installer::Tabs =
	{
		new LaunchTab(),
		new ServerBrowserTab(),
		new ModsTab(),
		new ProfileTab(),
		new SettingsTab(),
	};

	SidebarBackground = new UIBackground(false, Vector2f(-1, -1), 0, Vector2f(0, 2));
	SidebarBackground
		->SetOpacity(0.75)
		->SetAlign(UIBox::Align::Reverse);
	GenerateTabs();

	if (std::filesystem::exists(ProfileTab::CurrentProfile.Path + "/mods/autojoin"))
	{
		std::filesystem::remove_all(ProfileTab::CurrentProfile.Path + "/mods/autojoin");
	}

	Log::Print("Successfully started launcher");
	float PrevAspect = Application::AspectRatio;

	while (!Application::Quit)
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

		bg->SetPosition(Vector2f(0.0) - bg->GetUsedSize() / 2);
		BackgroundTask::UpdateTaskStatus();
		AppTitle->SetPosition(Vector2f(SidebarBackground->GetUsedSize().X - 0.99, WindowButtonBox->GetPosition().Y + 0.001));
		AppTitle->SetText("TetherInstaller - " + Tabs[SelectedTab]->Name + " - Profile: " + ProfileTab::CurrentProfile.DisplayName);
		DownloadWindow::Update(WindowButtonBox->GetUsedSize().Y);

		if (Application::AspectRatio != PrevAspect)
		{
			PrevAspect = Application::AspectRatio;
			AppTitle->SetTextSize(Application::GetFullScreen() ? 0.5 : 0.3);
			GenerateWindowButtons();
		}


		if (!BackgroundTask::IsRunningTask && !LaunchTasks.empty())
		{
			new BackgroundTask(LaunchTasks.front());
			LaunchTasks.pop();
		}

		Application::UpdateWindow();
		Application::SetActiveMouseCursor(Application::GetMouseCursorFromHoveredButtons());
		if (Application::Quit && BackgroundTask::IsRunningTask)
		{
			Application::Quit = false;
		}
#if !DEBUG
		if (RequiresUpdate)
		{
			RequiresUpdate = false;
			new BackgroundTask(UpdateInstaller);
		}
#endif
	}
	Networking::Cleanup();
	Log::Print("Application closed.");
	exit(0);
}

#if _WIN32
int WinMain()
{
	main(__argc, __argv);
}
#endif