#include "FullScreenNotify.h"

#include <KlemmUI/UI/UIButton.h>

#include "UIDef.h"
#include "Icon.h"

#include "../Translation.h"

using namespace Translation;

FullScreenNotify::FullScreenNotify(std::string Title)
{
	BlockingBackground = new UIBackground(true, -1, 0, 2);
	BlockingBackground
		->SetOpacity(0.75f)
		->SetHorizontalAlign(UIBox::Align::Centered)
		->SetVerticalAlign(UIBox::Align::Centered)
		->HasMouseCollision = true;

	ContentBox = new UIBox(false, 0);

	BlockingBackground->AddChild((new UIBackground(false, 0, 1, Vector2f(0.6f, 0)))
		->SetBorder(UIBox::BorderType::Rounded, 0.25f)
		->AddChild((new UIBox(true, 0))
			->SetPadding(0)
			->SetVerticalAlign(UIBox::Align::Centered)
			->AddChild((new UIBackground(true, 0, 0, 0.06f))
				->SetUseTexture(true, Icon("Settings/About").TextureID)
				->SetPadding(0.01, 0.01, 0.01, 0)
				->SetSizeMode(UIBox::SizeMode::AspectRelative))
			->AddChild(new UIText(0.4f, 0, GetTranslation(Title), UI::Text)))
		->AddChild((new UIBackground(true, 0, 0, Vector2f32(0.6f, 0.005f)))
			->SetPadding(0, 0, 0.01, 0.01))
		->AddChild(ContentBox
			->SetPadding(0)));
}

FullScreenNotify::~FullScreenNotify()
{
	delete BlockingBackground;
}

static FullScreenNotify* CurrentNotify;

void FullScreenNotify::AddOptions(std::vector<NotifyOption> Options)
{
	this->Options = Options;
	CurrentNotify = this;
	UIBox* OptionsBox = new UIBox(true, 0);
	ContentBox->AddChild(OptionsBox
		->SetPadding(0.04, 0, 0, 0));
	int Index = 0;
	for (const NotifyOption& i : Options)
	{
		OptionsBox->AddChild((new UIButton(true, 0, 0.0f, [](int Index)
			{
				CurrentNotify->Options[Index].OnClicked();
				delete CurrentNotify->BlockingBackground;
			}, Index++))
			->SetPadding(0.01, 0.01, 0.01, 0)
			->SetBorder(UIBox::BorderType::Rounded, 0.25f)
			->SetPaddingSizeMode(UIBox::SizeMode::AspectRelative)
			->SetVerticalAlign(UIBox::Align::Centered)
			->AddChild((new UIBackground(true, 0, 1, 0.05f))
				->SetUseTexture(true, Icon(i.Icon).TextureID)
				->SetSizeMode(UIBox::SizeMode::AspectRelative)
				->SetPaddingSizeMode(UIBox::SizeMode::AspectRelative))
			->AddChild((new UIText(0.3f, 1, GetTranslation(i.Name), UI::Text))
				->SetPaddingSizeMode(UIBox::SizeMode::AspectRelative)
				->SetPadding(0.01, 0.01, 0, 0.02)));
	}
}