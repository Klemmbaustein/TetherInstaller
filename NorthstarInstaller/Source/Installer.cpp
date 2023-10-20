#include <KlemmUI/Application.h>
#include <KlemmUI/UI/UIBackground.h>
#include <KlemmUI/Rendering/Texture.h>

#include <queue>
#include <fstream>

#include "Game.h"
#include "Log.h"
#include "Installer.h"
#include "UIDef.h"
#include "Networking.h"
#include "BackgroundTask.h"
#include "WindowFunctions.h"
#include "Thunderstore.h"

#include "Tabs/LaunchTab.h"
#include "Tabs/SettingsTab.h"
#include "Tabs/ModsTab.h"
#include "Tabs/ProfileTab.h"
#include "Tabs/ServerBrowserTab.h"

/*
* 
* TODO: 
* 
* - Implement https://github.com/LazyJazz/tinyfiledialogs/blob/master/tinyfiledialogs.h instead of regular win32-api calls
* - Linux??!???!?
* 
*/


namespace Installer
{
	UIBox* WindowButtonBox = nullptr;
	size_t SelectedTab = 0;
	std::vector<UIButton*> TabButtons;
	UIBackground* TabBackground;
	std::vector<UITab*> Tabs;

	UIButtonStyle* TabStyles[2] = { new UIButtonStyle("Tab default style"), new UIButtonStyle("Tab selected style")};

	std::vector<unsigned int> WindowButtonsIcons;

	UIBackground* TaskBackground = nullptr;
	UIBackground* TaskProgressBar = nullptr;
	UIText* TaskNameText = nullptr;
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
		TabBackground->DeleteChildren();
		for (size_t i = 0; i < Tabs.size(); i++)
		{
			if (SelectedTab == i)
			{
				Tabs[i]->Background->IsVisible = true;
			}
			else
			{
				Tabs[i]->Background->IsVisible = false;
			}

			auto Button = new UIButton(true, 0, TabStyles[(int)(i == SelectedTab)], []()
				{
					for (size_t i = 0; i < TabButtons.size(); i++)
					{
						if (TabButtons.at(i)->GetIsHovered())
						{
							SelectedTab = i;
							GenerateTabs();
							return;
						}
					}
				});
			Button->BoxAlign = UIBox::Align::Centered;
			TabBackground->AddChild(Button
				->SetSizeMode(UIBox::SizeMode::AspectRelative)
				->SetMinSize(Vector2f(0.3, 0))
				->SetPadding(0.01, 0.01, 0.01, 0)
				->AddChild((new UIText(0.5, 0, Tabs[i]->Name, UI::Text))
					->SetPadding(0.025, 0.025, 0, 0)));
			TabButtons.push_back(Button);
		}
	}

	void CreateTaskWindow()
	{
		TaskNameText = new UIText(0.4, 1, "No background task", UI::Text);
		TaskBackground->AddChild(TaskNameText);
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
			Log::Print("Launcher requires update. " + GithubInstallerVersion + " ->" + Ver , Log::Warning);
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
		BackgroundTask::SetStatus("Updating installer");
		if (Window::ShowPopupQuestion("Update", "An update for the launcher is avaliabe.\nWould you like to install it?") != Window::PopupReply::Yes)
		{
			return;
		}
		BackgroundTask::SetProgress(0.3);
		Networking::DownloadLatestReleaseOf("Klemmbaustein/TetherInstaller");
		Networking::ExtractZip("Data/temp/net/latest.zip", "Data/temp/install");

		system("start update.bat");
		exit(0);
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
		Vector3f32 HoveredColor = 0.2f;
		if (i == 0)
		{
			HoveredColor = Vector3f32(0.5, 0, 0);
		}
		WindowButtonBox->AddChild((new UIButton(true, 0, 0.25, [](int Index) {
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
}


float BackgroundFade = 0;

#if !_WIN32
#include <nvdialog/nvdialog.h>
#endif

int main(int argc, char** argv)
{
	using namespace Installer;

#if !_WIN32
    if (nvd_init(argv[0]) != 0) {
        std::printf("NvDialog couldn't initialize: %s", nvd_stringify_error(nvd_get_error()));
        exit(EXIT_FAILURE);
    }
#endif

	if (!std::filesystem::exists("Shaders/postprocess.vert"))
	{
		std::string ProgramLocation = argv[0];
		std::filesystem::current_path(ProgramLocation.substr(0, ProgramLocation.find_last_of("/\\")));
	}

	Application::SetErrorMessageCallback([](std::string Message)
		{
			Window::ShowPopupError("-- Internal UI Error --\n\n" + Message);
		});
	Application::Initialize("Tether installer " + Installer::InstallerVersion, Application::BORDERLESS_BIT);
	Application::SetWindowMovableCallback([]() 
		{ 
			return Installer::TabBackground
				&& Installer::TabBackground->IsBeingHovered()
			|| (Installer::TaskBackground 
				&& Installer::TaskBackground->IsBeingHovered()); 
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

	Application::UpdateWindow();
	bg->SetPosition(Vector2f(0.0) - bg->GetUsedSize() / 2);
	Application::UpdateWindow();

	TaskBackground = new UIBackground(false, Vector2f(0.5, 0.85), Vector3f32(0.15), Vector2f(0.5, 0.15));
	TaskProgressBar = new UIBackground(false, Vector2f(0.5, 0.985), Vector3f32(1, 0.5, 0), Vector2f(0.5, 0.05));
	TaskProgressBar->SetBorder(UIBox::BorderType::Rounded, 0.5);


	Networking::Init();

	CreateTaskWindow();

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

	WindowButtonsIcons =
	{
		Texture::LoadTexture("Data/WindowX.png"),
		Texture::LoadTexture("Data/WindowResize.png"),
		Texture::LoadTexture("Data/WindowResize2.png"),
		Texture::LoadTexture("Data/WindowMin.png"),
	};

	TabBackground = new UIBackground(true, Vector2f(-1, 0.85), 0, Vector2f(1.5, 0.15));
	TabBackground->SetOpacity(0.3);
	GenerateTabs();

	if (std::filesystem::exists(ProfileTab::CurrentProfile.Path + "/mods/autojoin"))
	{
		std::filesystem::remove_all(ProfileTab::CurrentProfile.Path + "/mods/autojoin");
	}

	Log::Print("Successfully started launcher");

	WindowButtonBox = (new UIBox(true, Vector2f(0.75, 0.0)))
		->SetPadding(0)
		->SetAlign(UIBox::Align::Reverse)
		->SetMinSize(Vector2f(0.25, 0.0));
	WindowButtonBox->BoxAlign = UIBox::Align::Reverse;
	(new UIBox(false, Vector2f(0.75, 0.7)))
		->SetMinSize(Vector2f(0.25, 0.3))
		->AddChild(WindowButtonBox)
		->SetAlign(UIBox::Align::Reverse);
	
	GenerateWindowButtons();
	float PrevAspect = Application::AspectRatio;

	while (!Application::Quit)
	{
		for (auto i : Tabs)
		{
			i->Tick();
		}
		bg->SetPosition(Vector2f(0.0) - bg->GetUsedSize() / 2);
		BackgroundTask::UpdateTaskStatus();
		if (BackgroundTask::IsRunningTask)
		{
			BackgroundFade = 0;
			TaskProgressBar->SetOpacity(1);
			if (BackgroundTask::CurrentTaskProgress != 1)
			{
				TaskProgressBar->SetMinSize(Vector2f(0.5 * BackgroundTask::CurrentTaskProgress, 0.05));
				TaskProgressBar->IsVisible = true;
				TaskNameText->SetText(BackgroundTask::CurrentTaskStatus);
			}
		}
		else if (BackgroundFade < 1)
		{
			TaskProgressBar->SetMinSize(Vector2f(0.5, 0.05));
			BackgroundFade += Application::DeltaTime * 2;
			TaskProgressBar->SetOpacity(1 - BackgroundFade);
			TaskProgressBar->IsVisible = true;
			TaskNameText->SetText(BackgroundTask::CurrentTaskStatus);
		}
		else
		{
			TaskProgressBar->IsVisible = false;
			TaskNameText->SetText("No background task");
		}

		if (Application::AspectRatio != PrevAspect)
		{
			PrevAspect = Application::AspectRatio;
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
}

#if _WIN32
int WinMain()
{
	main(__argc, __argv);
}
#endif