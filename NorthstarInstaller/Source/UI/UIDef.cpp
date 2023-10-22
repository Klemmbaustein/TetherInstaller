#include "UIDef.h"
#include "../Log.h"

namespace UI
{
	TextRenderer* Text;
	TextRenderer* MonoText;
	void LoadFonts()
	{
		Log::Print("Loading UI fonts...");
		MonoText = new TextRenderer("Data/Monospace.ttf");
		Text = new TextRenderer("Data/Font.ttf");
	}
}