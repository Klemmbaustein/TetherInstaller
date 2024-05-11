#include "Sidebar.h"

#include "../Tabs/LaunchTab.h"
#include "../Tabs/SettingsTab.h"
#include "../Tabs/ModsTab.h"
#include "../Tabs/ProfileTab.h"
#include "../Tabs/ServerBrowserTab.h"

#include "../Translation.h"
#include "../Installer.h"
#include "../Log.h"

#include "../Markup/Sidebar.hpp"
#include "../Markup/SideBarButton.hpp"
#include "../Markup/SideBarTooltip.hpp"

using namespace KlemmUI;

SideBar* SidebarClass::SidebarBackground = nullptr;
std::vector<UITab*> SidebarClass::Tabs;
std::vector<KlemmUI::UIBox*> SidebarClass::TabButtons;
size_t SidebarClass::SelectedTab = 0;
size_t SidebarClass::HoveredTab = 0;

UIBox* SidebarClass::HoveredTabName = nullptr;

void SidebarClass::GenerateTabName(size_t TabIndex)
{
	if (HoveredTabName)
	{
		delete HoveredTabName;
		HoveredTabName = nullptr;
	}

	if (TabIndex > Tabs.size())
	{
		return;
	}
	auto Tooltip = new SideBarTooltip();

	Tooltip->SetTitle(Translation::GetTranslation("tab_" + Tabs[TabIndex]->Name));
	Tooltip->SetDescription(Translation::GetTranslation("tab_" + Tabs[TabIndex]->Name + "_description"));
	Tooltip->SetPosition(TabButtons[TabIndex]->GetPosition() + Vector2f(TabButtons[TabIndex]->GetUsedSize().X, 0));

	HoveredTabName = Tooltip;
}

void SidebarClass::GenerateTabs()
{
	if (!SidebarBackground)
	{
		return;
	}

	TabButtons.clear();

	SidebarBackground->background->DeleteChildren();
	for (size_t i = 0; i < Tabs.size(); i++)
	{
		Tabs[i]->Background->IsVisible = SelectedTab == i;

		auto Button = new SideBarButton();
		
		Button->button->OnClickedFunctionIndex = [](int Index)
			{
				Tabs.at(Index)->OnClicked();
				SelectedTab = Index;
				GenerateTabs();
			};
		Button->button->ButtonIndex = (int)i;
		Button->buttonIcon->SetUseTexture(true, Installer::MainWindow->UI.LoadReferenceTexture("itab_" + Tabs[i]->Name + ".png"));


		SidebarBackground->background->AddChild(Button);
		Installer::SetButtonColorIfSelected(Button->button, i == SelectedTab);
		TabButtons.push_back(Button);
	}
}

void SidebarClass::Load()
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

	SidebarBackground = new SideBar();
	GenerateTabs();
}

void SidebarClass::Update()
{
	bool AnyButtonHovered = false;
	for (size_t i = 0; i < TabButtons.size(); i++)
	{
		if (TabButtons[i]->IsBeingHovered())
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

float SidebarClass::GetSize()
{
	return SidebarBackground->GetUsedSize().X;
}
