#include <Application.h>
#include <UI/UIBackground.h>
#include <Rendering/Texture.h>

#include <queue>
#include <fstream>

#include "Game.h"
#include "Log.h"
#include "Installer.h"
#include "UIDef.h"
#include "Networking.h"
#include "BackgroundTask.h"
#include "WindowFunctions.h"

#include "Tabs/LaunchTab.h"
#include "Tabs/SettingsTab.h"
#include "Tabs/ModsTab.h"

/*
* 
* TODO:
* 
*/

namespace Installer
{
	size_t SelectedTab = 0;
	std::vector<UIButton*> TabButtons;
	UIBackground* TabBackground;
	std::vector<UITab*> Tabs;

	UIBackground* TaskBackground = nullptr;
	UIBackground* TaskProgressBar = nullptr;
	UIText* TaskNameText = nullptr;
	const std::string InstallerVersion = "v0.1.3";
	const std::string GithubInstallerVersion = InstallerVersion;

	void GenerateTabs()
	{
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

			auto Button = new UIButton(true, 0, SelectedTab == i ? Vector3f32(0.2, 0.4, 1) : 0.8, []()
				{
					for (size_t i = 0; i < TabButtons.size(); i++)
					{
						if (TabButtons[i]->GetIsHovered())
						{
							SelectedTab = i;
							GenerateTabs();
							return;
						}
					}
				});
			Button->Align = UIBox::E_CENTERED;
			TabBackground->AddChild(Button
				->SetBorder(UIBox::E_ROUNDED, 0.5)
				->SetPadding(0.02)
				->SetSizeMode(UIBox::E_PIXEL_RELATIVE)
				->SetMinSize(Vector2f(0.4, 0))
				->AddChild(new UIText(0.6, 0, Tabs[i]->Name, UI::Text)));
			TabButtons.push_back(Button);
		}
	}


	void UpdateTaskWindow()
	{

	}

	void CreateTaskWindow()
	{
		TaskNameText = new UIText(0.4, 1, "No background task", UI::Text);
		TaskBackground->AddChild(new UIText(0.3, 0.8, "Background task", UI::Text));
		TaskBackground->AddChild(TaskNameText);
	}

	void CheckForUpdates()
	{
		Log::Print("Checking for updates...");
		BackgroundTask::SetStatus("Checking for updates");
		BackgroundTask::SetProgress(0.999);
		std::string Latest = Networking::GetLatestReleaseOf("R2Northstar/Northstar");
		
		if (Latest.empty())
		{
			Log::Print("Could not get latest game version",  Log::Error);
			BackgroundTask::SetStatus("Could not get latest game version");
			BackgroundTask::SetProgress(1);
			return;
		}

		if (Latest != Game::GetCurrentVersion())
		{
			Log::Print("Game needs to be updated",  Log::Warning);
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
		Log::Print("Checking for installer updates...");
		BackgroundTask::SetStatus("Checking for installer updates");
		BackgroundTask::SetProgress(0.999);

		std::string Ver = Networking::GetLatestReleaseOf("Klemmbaustein/NorthstarInstaller");
		if (Ver.empty())
		{
			return;
		}
		if (GithubInstallerVersion != Ver)
		{
			RequiresUpdate = true;
			Log::Print(std::format("Installer requires update. {} -> {}", GithubInstallerVersion, Ver) , Log::Warning);
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
		if (Window::ShowPopup("Update", "An update for the installer is avaliabe.\nWould you like to install it?") != Window::PopupReply::Yes)
		{
			return;
		}
		BackgroundTask::SetProgress(0.3);
		Networking::DownloadLatestReleaseOf("Klemmbaustein/NorthstarInstaller");
		Networking::ExtractZip("Data/temp/net/latest.zip", "Data/temp/install");

		system("start update.bat");
		exit(0);
	}
}

float BackgroundFade = 0;

int main(int argc, char** argv)
{
	if (std::filesystem::exists("update.bat"))
	{
		std::filesystem::remove("update.bat");
	}
	Application::Initialize("Installer", 0);
	Log::Print("Created app window");

	Networking::Init();

	UI::LoadFonts();

	Vector2f bgCenter = Vector2f(-0.3, 0);

	auto bg = new UIBackground(true, 0, 1, Vector2f(2.5 * (16.f/9.f), 2.5));
	bg->SetSizeMode(UIBox::E_PIXEL_RELATIVE);
	bg->SetUseTexture(true, Texture::LoadTexture("Data/Game.png"));

	Installer::TaskBackground = new UIBackground(false, Vector2f(0.5, 0.85), Vector3f32(0.15), Vector2f(0.5, 0.15));
	Installer::TaskProgressBar = new UIBackground(false, Vector2f(0.5, 0.985), Vector3f32(1, 0.5, 0), Vector2f(0.5, 0.05));
	Installer::TaskProgressBar->SetBorder(UIBox::E_ROUNDED, 0.5);

	Installer::CreateTaskWindow();

	Game::GamePath = Game::GetTitanfallLocation();

	Log::Print("Loading tabs...");
	Installer::Tabs =
	{
		new LaunchTab(),
		new ModsTab(),
		new SettingsTab(),
	};

	Installer::TabBackground = new UIBackground(true, Vector2f(-1, 0.85), 0, Vector2f(1.5, 0.15));
	Installer::TabBackground->SetOpacity(0.3);
	Log::Print("Loading tab bar...");
	Installer::GenerateTabs();


	Log::Print("Successfully started launcher");

	while (!Application::Quit)
	{
		for (auto i : Installer::Tabs)
		{
			i->Tick();
		}
		bg->SetPosition(Vector2f(0.0) - bg->GetUsedSize() / 2);
		BackgroundTask::UpdateTaskStatus();
		if (BackgroundTask::IsRunningTask)
		{
			BackgroundFade = 0;
			Installer::TaskProgressBar->SetOpacity(1);
			if (BackgroundTask::CurrentTaskProgress == 1)
			{
				Installer::UpdateTaskWindow();
			}
			else
			{
				Installer::TaskProgressBar->SetMinSize(Vector2f(0.5 * BackgroundTask::CurrentTaskProgress, 0.05));
				Installer::TaskProgressBar->IsVisible = true;
				Installer::TaskNameText->SetText(BackgroundTask::CurrentTaskStatus);
			}
		}
		else if (BackgroundFade < 1)
		{
			Installer::TaskProgressBar->SetMinSize(Vector2f(0.5, 0.05));
			BackgroundFade += Application::DeltaTime;
			Installer::TaskProgressBar->SetOpacity(1 - BackgroundFade);
			Installer::TaskProgressBar->IsVisible = true;
			Installer::TaskNameText->SetText(BackgroundTask::CurrentTaskStatus);
		}
		else
		{
			Installer::TaskProgressBar->IsVisible = false;
			Installer::TaskNameText->SetText("No background task");
		}

		if (!BackgroundTask::IsRunningTask && !Installer::LaunchTasks.empty())
		{
			new BackgroundTask(Installer::LaunchTasks.front());
			Installer::LaunchTasks.pop();
		}

		Application::UpdateWindow();
		if (Application::Quit && BackgroundTask::IsRunningTask)
		{
			Application::Quit = false;
		}
#if !DEBUG
		if (Installer::RequiresUpdate)
		{
			Installer::RequiresUpdate = false;
			new BackgroundTask(Installer::UpdateInstaller);
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