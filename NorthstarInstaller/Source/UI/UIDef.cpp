#include "UIDef.h"
#include "../Log.h"
#include "../Installer.h"

namespace UI
{
	using namespace KlemmUI;

	Font* Text;
	Font* MonoText;
	void LoadFonts()
	{
		Log::Print("Loading UI fonts...");
		MonoText = new Font(Installer::CurrentPath + "Data/Monospace.ttf");
		Text = new Font(Installer::CurrentPath + "Data/Font.ttf");
	}
}