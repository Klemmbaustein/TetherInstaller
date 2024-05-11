#include "Translation.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <fstream>
#include <cstdarg>
#include "Log.h"
#include "Installer.h"
#include "UI/Sidebar.h"
using namespace nlohmann;

namespace Translation
{
	json TranslationFile;
	std::string LoadedTranslation = "English";
}

std::string Translation::GetTranslation(std::string Name)
{
	try
	{
		return TranslationFile.at("translations").at(Name);
	}
	catch (json::exception& e)
	{
		//Log::Print(e.what(), Log::Error);
		return Name;
	}
}

std::string Translation::GetMapName(std::string Map)
{
	try
	{
		return TranslationFile.at("translations").at("map_names").at(Map);
	}
	catch (json::exception& e)
	{
		//Log::Print(e.what(), Log::Error);
		return Map;
	}
}

std::string Translation::GetGameModeName(std::string Mode)
{
	try
	{
		return TranslationFile.at("translations").at("game_modes").at(Mode);
	}
	catch (json::exception& e)
	{
		//Log::Print(e.what(), Log::Error);
		return Mode;
	}
}

std::string Translation::GetTSOrderingName(std::string Ordering)
{
	try
	{
		return TranslationFile.at("translations").at("mod_categories").at(Ordering);
	}
	catch (json::exception& e)
	{
		//Log::Print(e.what(), Log::Error);
		return Ordering;
	}
}

void Translation::LoadTranslation(std::string Name)
{
	LoadedTranslation = Name;
	try
	{
		TranslationFile = json::parse(GetTranslationFile(Name), nullptr, true, true);
	}
	catch (json::exception& e)
	{
		Log::Print(e.what(), Log::Error);
	}
	for (const auto& i : SidebarClass::Tabs)
	{
		if (i->TabTitle)
		{
			i->TabTitle->SetText(GetTranslation("tab_" + i->Name));
			i->OnTranslationChanged();
		}
	}

	std::ofstream out = std::ofstream(Installer::CurrentPath + "Data/var/language.txt");
	out << Name;
	out.close();
	Log::Print(Name + " -> " + Installer::CurrentPath + "Data/var/language.txt");
	Log::Print("Changed language to " + Name);
}

std::string Translation::Format(std::string Format, ...)
{
	char* buf = new char[Format.size() + 250ull]();
	va_list va;
	va_start(va, Format);
#if _WIN32 && 0
	vsprintf_s(buf, Format.size() + 250, Format.c_str(), va);
#else
	vsprintf(buf, Format.c_str(), va);
#endif
	va_end(va);

	std::string StrBuffer = buf;
	delete[] buf;
	return StrBuffer;
}

std::string Translation::GetLastTranslation()
{
	if (!std::filesystem::exists(Installer::CurrentPath + "/Data/var/language.txt"))
	{
		return "English";
	}
	std::ifstream i = std::ifstream(Installer::CurrentPath + "/Data/var/language.txt");
	std::stringstream istream;
	istream << i.rdbuf();
	return istream.str();
}

std::vector<std::string> Translation::GetAvailableTranslations()
{
	std::vector<std::string> Found;
	for (const auto& i : std::filesystem::directory_iterator(Installer::CurrentPath + "Data/translation"))
	{
		Found.push_back(ReadLanguageProperty(i.path().u8string()));
	}
	return Found;
}

std::vector<std::string> Translation::NewLineStringToStringArray(std::string str)
{
	std::vector<std::string> Array = { "" };

	for (char c : str)
	{
		if (c == '\n')
		{
			Array.push_back("");
		}
		else
		{
			Array[Array.size() - 1].append({ c });
		}
	}

	return Array;
}

std::string Translation::ReadLanguageProperty(std::string File)
{
	std::ifstream in = std::ifstream(File);
	std::stringstream inStrS;
	inStrS << in.rdbuf();
	in.close();
	try
	{
		return json::parse(inStrS.str(), nullptr, true, true).at("name");
	}
	catch (json::exception& e)
	{
		Log::Print(e.what(), Log::Error);
	}
	return "";
}

std::string Translation::GetTranslationFile(std::string TranslationName)
{
	std::vector<std::string> Found;
	for (const auto& i : std::filesystem::directory_iterator(Installer::CurrentPath + "Data/translation"))
	{
		std::ifstream in = std::ifstream(i.path());
		std::stringstream inStrS;
		inStrS << in.rdbuf();
		in.close();
		try
		{
			if (json::parse(inStrS.str(), nullptr, true, true).at("name") == TranslationName)
			{
				return inStrS.str();
			}
		}
		catch (json::exception& e)
		{
			Log::Print(e.what(), Log::Error);
		}
	}
	return "";
}
