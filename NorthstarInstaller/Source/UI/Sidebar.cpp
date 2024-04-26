#include "Sidebar.h"
#include "Icon.h"

#include "../Tabs/LaunchTab.h"
#include "../Tabs/SettingsTab.h"
#include "../Tabs/ModsTab.h"
#include "../Tabs/ProfileTab.h"
#include "../Tabs/ServerBrowserTab.h"

#include "../Translation.h"
#include "../Installer.h"
#include "../Log.h"

using namespace KlemmUI;

UIBackground* Sidebar::SidebarBackground = nullptr;
std::vector<UITab*> Sidebar::Tabs;
std::vector<KlemmUI::UIButton*> Sidebar::TabButtons;
size_t Sidebar::SelectedTab = 0;
size_t Sidebar::HoveredTab = 0;

UIBackground* Sidebar::HoveredTabName = nullptr;

void Sidebar::GenerateTabName(size_t TabIndex)
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

void Sidebar::GenerateTabs()
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

		auto Button = new UIButton(true, 0, 1, [](int Index)
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
		TabButtons.push_back(Installer::SetButtonColorIfSelected(Button, i == SelectedTab));
	}
	SidebarBackground->UpdateElement();
	SidebarBackground->RedrawElement();
}

void Sidebar::Load()
{

	Log::Print("--- Loading tabs ---");
	Tabs =
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
}

void Sidebar::Update()
{
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
}

float Sidebar::GetSize()
{
	return SidebarBackground->GetUsedSize().X;
}
