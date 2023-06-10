#include <Application.h>
#include <UI/UIBackground.h>
#include <Rendering/Texture.h>

#include "Game.h"
#include "Log.h"
#include "LogPanel.h"
#include "Installer.h"
#include "UIDef.h"
#include "Networking.h"

#include "Tabs/LaunchTab.h"
#include "Tabs/SettingsTab.h"
#include "Tabs/ModsTab.h"

/*
* 
* TODO:
* Unfuck markdown parser. (The fucky markdown parser is now part of the UISystem library)
* Auto update mods.
* Auto update installer.
* Make background thread system good.
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

	std::thread* CurrentBackgroundThread = nullptr;
	std::string BackgroundTask;
	std::string BackgroundName;
	const std::string InstallerVersion = "0.1.1";
	float ThreadProgress = 0;

	bool HasCheckedForModUpdates = false;

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
				->SetBorder(UIBox::E_ROUNDED, 1)
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
		TaskBackground->Align = UIBox::E_REVERSE;
		TaskNameText = new UIText(0.4, 1, "No background task", UI::Text);
		TaskBackground->AddChild(TaskNameText);
		TaskBackground->AddChild(new UIText(0.3, 0.8, "Background task", UI::Text));
	}

	void CheckForUpdates()
	{
		Log::Print("Checking for updates...");
		BackgroundTask = "Checking for updates...";
		BackgroundName = "Checking for updates";
		ThreadProgress = 0.999;
		std::string Latest = Networking::GetLatestReleaseOf("R2Northstar/Northstar");
		
		if (Latest.empty())
		{
			Log::Print("Could not get latest game version",  Log::Error);
			BackgroundTask = "Could not get latest game version";
			ThreadProgress = 1;
			return;
		}

		if (Latest != Game::GetCurrentVersion())
		{
			Log::Print("Game needs to be updated",  Log::Warning);
			Game::RequiresUpdate = true;
			BackgroundTask = "Update required";
			ThreadProgress = 1;
			return;
		}
		BackgroundTask = "No update required";
		Log::Print("No update required");
		ThreadProgress = 1;
	}
	
}

float BackgroundFade = 0;

int main(int argc, char** argv)
{
	LogPanel::PrebufferLogEvents();
	Application::Initialize("Installer", 0);
	Log::Print("Created app window");

	Networking::Init();

	UI::LoadFonts();

	Vector2f bgCenter = Vector2f(-0.3, 0);

	auto bg = new UIBackground(true, 0, 1, Vector2f(2 * (16.f/9.f), 2));
	bg->SetSizeMode(UIBox::E_PIXEL_RELATIVE);
	bg->SetUseTexture(true, Texture::LoadTexture("Data/Game.png"));

	new LogPanel();

	Installer::TaskBackground = new UIBackground(false, Vector2f(0.4, -1), Vector3f32(0.15), Vector2f(0.6, 0.2));
	Installer::TaskProgressBar = new UIBackground(false, Vector2f(0.4, -1.025), Vector3f32(1, 0.5, 0), Vector2f(0.6, 0.05));
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

	Installer::TabBackground = new UIBackground(true, Vector2f(-1, 0.85), 0, Vector2f(1.4, 0.15));
	Installer::TabBackground->SetOpacity(0.3);
	Log::Print("Loading tab bar...");
	Installer::GenerateTabs();


	Log::Print("Successfully started launcher");

	Installer::CurrentBackgroundThread = new std::thread(Installer::CheckForUpdates);

	while (!Application::Quit)
	{
		for (auto i : Installer::Tabs)
		{
			i->Tick();
		}
		bg->SetPosition(bgCenter - bg->GetUsedSize() / 2);
		if (Installer::CurrentBackgroundThread)
		{
			BackgroundFade = 0;
			Installer::TaskProgressBar->SetOpacity(1);
			if (Installer::ThreadProgress == 1)
			{
				Installer::ThreadProgress = 0;
				Installer::CurrentBackgroundThread->join();
				delete Installer::CurrentBackgroundThread;
				Installer::CurrentBackgroundThread = nullptr;
				Installer::UpdateTaskWindow();
			}
			else
			{
				Installer::TaskProgressBar->SetMinSize(Vector2f(0.6 * Installer::ThreadProgress, 0.05));
				Installer::TaskProgressBar->IsVisible = true;
				Installer::TaskNameText->SetText(Installer::BackgroundTask);
			}
		}
		else if (BackgroundFade < 1)
		{
			Installer::TaskProgressBar->SetMinSize(Vector2f(0.6, 0.05));
			BackgroundFade += Application::DeltaTime;
			Installer::TaskProgressBar->SetOpacity(1 - BackgroundFade);
			Installer::TaskProgressBar->IsVisible = true;
		}
		else
		{
			Installer::TaskProgressBar->IsVisible = false;
			Installer::TaskNameText->SetText("No background task");
		}

		if (!Installer::CurrentBackgroundThread && !Installer::HasCheckedForModUpdates)
		{
			Installer::HasCheckedForModUpdates = true;
			Installer::CurrentBackgroundThread = new std::thread(ModsTab::CheckForModUpdates);
		}

		Application::UpdateWindow();
		if (Application::Quit && Installer::CurrentBackgroundThread)
		{
			Application::Quit = false;
		}
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