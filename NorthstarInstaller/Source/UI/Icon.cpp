#include "Icon.h"
#include <KlemmUI/Rendering/Texture.h>

std::unordered_map<std::string, unsigned int> Icon::LoadedIcons;

Icon::Icon(std::string Name)
{
	auto ico = LoadedIcons.find(Name);
	if (ico == LoadedIcons.end())
	{
		TextureID = Texture::LoadTexture("Data/icons/" + Name + ".png");
		LoadedIcons.insert(std::pair(Name, TextureID));
	}
	else
	{
		TextureID = ico->second;
	}
}
