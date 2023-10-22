#pragma once
#include <string>
#include <unordered_map>

struct Icon
{
	Icon(std::string Name);
	Icon() {};

	unsigned int TextureID = 0;
private:
	static std::unordered_map<std::string, unsigned int> LoadedIcons;
};