#pragma once
#include "UITab.h"
#include <KlemmUI/UI/UIButton.h>
#include <KlemmUI/UI/UIBackground.h>
#include <KlemmUI/UI/UIText.h>
#include <KlemmUI/UI/UIScrollBox.h>
#include <KlemmUI/UI/UITextField.h>
#include <atomic>

#include "../Thunderstore.h"

class ModsTab : public UITab
{
	static std::vector<KlemmUI::UIButton*> PageButtons;
	static std::vector<KlemmUI::UIButton*> CategoryButtons;
	KlemmUI::UIScrollBox* ModsScrollBox = nullptr;
	std::vector<unsigned int> ModTextures;
	unsigned int ModPreviewTexture = 0;
	float PrevAspectRatio = 0;
	std::vector<KlemmUI::UIBackground*> ModImages;
	bool IsInModInfo = false;
	KlemmUI::UIBox* GenerateModInfoText(std::vector<std::string> Text, Vector3f Color, std::string Icon, float IconPadding = 0.01f);
	static void InstallMod();
public:

	static void InstallCurrentMod();
	void GenerateAvailabilityMessage();
	static ModsTab* CurrentModsTab;
	static std::atomic<unsigned int> ModsPerPage;
	KlemmUI::UITextField* SearchBar = nullptr;
	std::string Filter = "";
	bool DownloadingPage = false;
	bool LoadedModList = false;
	size_t SelectedPage = 0;
	void GenerateModInfo();
	void GenerateModPage();
	void GenerateModImages();
	void ShowLoadingText();
	static void Reload();
	void OnTranslationChanged() override;
	void UpdateClickedCategoryButton();

	static Vector2ui GetModsPerPage();
	void ClearLoadedTextures();
	static void CheckForModUpdates();

	ModsTab();
	void Tick() override;
	virtual ~ModsTab();
};