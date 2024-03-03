#include "Icon.h"
#include <KlemmUI/Rendering/Texture.h>
#include "../Installer.h"

std::unordered_map<std::string, unsigned int> Icon::LoadedIcons;

Icon::Icon(std::string Name)
{
	auto ico = LoadedIcons.find(Name);
	if (ico == LoadedIcons.end())
	{
		TextureID = Texture::LoadTexture(Installer::CurrentPath + "Data/icons/" + Name + ".png");
		LoadedIcons.insert(std::pair(Name, TextureID));
	}
	else
	{
		TextureID = ico->second;
	}
}
