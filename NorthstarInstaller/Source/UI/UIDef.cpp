#include "UIDef.h"
#include "../Log.h"
#include "../Installer.h"

namespace UI
{
	TextRenderer* Text;
	TextRenderer* MonoText;
	void LoadFonts()
	{
		Log::Print("Loading UI fonts...");
		MonoText = new TextRenderer(Installer::CurrentPath + "Data/Monospace.ttf");
		Text = new TextRenderer(Installer::CurrentPath + "Data/Font.ttf");
	}
}