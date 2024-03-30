#pragma once
#include <string>
#include <vector>

namespace Translation
{
	std::string GetTranslation(std::string Name);
	std::string GetMapName(std::string Map);
	std::string GetGameModeName(std::string Mode);
	std::string GetTSOrderingName(std::string Ordering);
	extern std::string LoadedTranslation;

	void LoadTranslation(std::string Name);

	std::string Format(std::string Format, ...);

	std::string GetLastTranslation();

	std::vector<std::string> GetAvailableTranslations();
	
	std::vector<std::string> NewLineStringToStringArray(std::string str);

	std::string ReadLanguageProperty(std::string File);

	std::string GetTranslationFile(std::string TranslationName);
}