#include "TitleBar.h"
#include "UIDef.h"
#include "Sidebar.h"

#include <KlemmUI/Rendering/Texture.h>

#include "../Installer.h"
#include "../Translation.h"

#include "../Tabs/ProfileTab.h"

using namespace Translation;
using namespace KlemmUI;

UIBackground* TitleBar::WindowButtonBox = nullptr;
KlemmUI::UIText* TitleBar::AppTitle = nullptr;
bool TitleBar::UseSystemTitleBar = false;
std::vector<unsigned int> TitleBar::WindowButtonsIcons;
std::string TitleBar::TitleText;

void TitleBar::Load()
{
	WindowButtonBox = new UIBackground(true, 0, 0.1);
	
	WindowButtonBox
		->SetPadding(0)
		->SetHorizontalAlign(UIBox::Align::Reverse)
		->SetMinSize(Vector2f(2, 0.0));
	(new UIBox(false, Vector2f(-1, 0.7)))
		->SetMinSize(Vector2f(2, 0.3))
		->AddChild(WindowButtonBox);


	WindowButtonsIcons =
	{
		Texture::LoadTexture(Installer::CurrentPath + "/Data/WindowX.png"),
		Texture::LoadTexture(Installer::CurrentPath + "/Data/WindowResize.png"),
		Texture::LoadTexture(Installer::CurrentPath + "/Data/WindowResize2.png"),
		Texture::LoadTexture(Installer::CurrentPath + "/Data/WindowMin.png"),
	};

	GenerateWindowButtons();

	AppTitle = new UIText(14, 1, "", UI::Text);
	AppTitle->SetTextSizeMode(UIBox::SizeMode::PixelRelative);
	AppTitle->SetPosition(2);
}

float TitleBar::GetSize()
{
	return WindowButtonBox->GetUsedSize().Y;
}

void TitleBar::SetTitle(std::string NewTitle)
{
	AppTitle->SetText(NewTitle);
}

void TitleBar::Update()
{
	std::string AppNameString = GetTranslation("app_name");

#if TF_PLUGIN
	AppNameString.append(" (Northstar Plugin)");
#endif

	std::string ProfileName = ProfileTab::CurrentProfile.DisplayName;
	if (ProfileName.empty())
	{
		ProfileName = "None";
	}

	std::string Title = Format(GetTranslation("title_bar"),
		AppNameString.c_str(),
		GetTranslation("tab_" + Sidebar::Tabs[Sidebar::SelectedTab]->Name).c_str(),
		ProfileName.c_str());

	AppTitle->IsVisible = !UseSystemTitleBar;
	
	if (UseSystemTitleBar && TitleText != Title)
	{
		TitleText = Title;
		Installer::MainWindow->SetTitle(Title);
	}
	else if (!UseSystemTitleBar)
	{
		SetTitle(Title);
	}

	AppTitle->SetPosition(WindowButtonBox->GetPosition() + Vector2f(Sidebar::GetSize(), 0) + Vector2f(UIBox::PixelSizeToScreenSize(3, Window::GetActiveWindow())));
}

bool TitleBar::SetUseSystemTitleBar(bool NewVal)
{
	return UseSystemTitleBar = NewVal;
}

bool TitleBar::GetUseSystemTitleBar()
{
	return UseSystemTitleBar;
}

bool TitleBar::IsHovered()
{
	return WindowButtonBox->IsBeingHovered();
}

void TitleBar::GenerateWindowButtons()
{
	WindowButtonBox->DeleteChildren();
	if (UseSystemTitleBar)
	{
		return;
	}

	std::vector<int> Buttons;
	if (Installer::MainWindow->GetWindowFullScreen())
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
				Installer::MainWindow->Close();
				break;
			case 1:
			case 2:
				Installer::MainWindow->SetWindowFullScreen(!Installer::MainWindow->GetWindowFullScreen());
				GenerateWindowButtons();
				break;
			case 3:
				Installer::MainWindow->SetMinimized(true);
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
				->SetPadding(Installer::MainWindow->GetWindowFullScreen() ? 18 : 10,
					10,
					16,
					(Installer::MainWindow->GetWindowFullScreen() && i == 0) ? 24 : 16)));
	}
}

