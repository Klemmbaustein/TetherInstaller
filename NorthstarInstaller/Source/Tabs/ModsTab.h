#pragma once
#include "UITab.h"
#include <UI/UIButton.h>
#include <UI/UIBackground.h>
#include <UI/UIText.h>
#include <UI/UIScrollBox.h>
#include <UI/UITextField.h>

class ModsTab : public UITab
{
	static ModsTab* CurrentModsTab;
	static std::vector<UIButton*> ModButtons;
	static std::vector<UIButton*> PageButtons;
	static std::vector<UIButton*> CategoryButtons;
	UIBackground* ModsBackground = nullptr;
	UIScrollBox* ModsScrollBox = nullptr;
	std::vector<unsigned int> ModTextures;
	unsigned int ModPreviewTexture = 0;
	float AspectRatio = 0;
public:
	UITextField* SearchBar = nullptr;
	std::string Filter = "";
	bool DownloadingPage = false;
	bool LoadedModList = false;
	size_t SelectedPage = 1;
	void GenerateModInfo();
	void GenerateModPage();
	void ShowLoadingText();
	void UpdateClickedCategoryButton();
	ModsTab();
	void Tick() override;
	virtual ~ModsTab();
};