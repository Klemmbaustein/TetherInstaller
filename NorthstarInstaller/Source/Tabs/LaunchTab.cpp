#include "LaunchTab.h"
#include <UI/UIText.h>
#include <UI/UIBackground.h>

#include "../UIDef.h"
#include "../Log.h"
#include "../Game.h"
#include "../Installer.h"

#include <thread>
#include <atomic>

void LaunchNorthstar()
{
	if (Installer::CurrentBackgroundThread)
	{
		return;
	}
	if (!Game::GamePath.empty() && !Game::RequiresUpdate)
	{
		Log::Print("Starting game...");

		Installer::CurrentBackgroundThread = new std::thread([]()
			{		
				Installer::BackgroundName = "Northstar is running";
				Installer::BackgroundTask = "Northstar";
				Log::Print("Game has started");
				system((Game::GamePath + "/NorthstarLauncher.exe").c_str());
				Log::Print("Game has finished running");
				Installer::ThreadProgress = 1;
			});
	}
	if (Game::RequiresUpdate && !Game::GamePath.empty())
	{
		Game::UpdateGameAsync();
	}
}

LaunchTab::LaunchTab()
{
	Name = "Play";
	Log::Print("Loading launch tab...");
	Background->Align = UIBox::E_DEFAULT;

	auto TextBox = (new UIBackground(true, 0, 0, 0))->SetOpacity(0.3);
	TextBox->Align = UIBox::E_CENTERED;

	LaunchButton = new UIButton(true, 0, 1, LaunchNorthstar);
	LaunchText = new UIText(0.7, 0, "Launch", UI::Text);

	Background->AddChild(TextBox
		->SetMinSize(Vector2f(1.4, 0.2))
		->SetPadding(0)
		->AddChild(LaunchButton
			->SetPadding(0.03)
			->SetBorder(UIBox::E_ROUNDED, 1)
			->AddChild(LaunchText)));


}

void LaunchTab::Tick()
{
	if (Game::GamePath.empty())
	{
		LaunchText->SetText("Titanfall 2 not found");
	}
	else if (Installer::CurrentBackgroundThread)
	{
		LaunchText->SetText(Installer::BackgroundName);
	}
	else if (Game::RequiresUpdate)
	{
		LaunchText->SetText("Update northstar");
	}
	else
	{
		LaunchText->SetText("Launch Northstar");
	}
}

LaunchTab::~LaunchTab()
{
}
