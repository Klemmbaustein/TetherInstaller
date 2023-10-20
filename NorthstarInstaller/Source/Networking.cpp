#include "Networking.h"

#define CURL_STATICLIB 1
#include <curl/curl.h>
#include "nlohmann/json.hpp"
#include "miniz/miniz.h"

#include "Log.h"
#include "Installer.h"
#include "WindowFunctions.h"

#include <fstream>
#include <filesystem>
#include <regex>

#define DEV_NET_DEBUGGING 0

#if _WIN32
#include <wtsapi32.h>
#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "wldap32.lib" )
#pragma comment(lib, "crypt32.lib" )
#pragma comment(lib, "Ws2_32.lib")

#endif


std::string ToLowerCase(std::string str);
#if _WIN32
LONG GetStringRegKey(HKEY hKey, const std::wstring& strValueName, std::wstring& strValue, const std::wstring& strDefaultValue);
bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
	CURL_STATICLIB;
}
#endif



namespace Networking
{

	const std::string NorthstarVersionFile = "Data/";
	const std::string NetTempFolder = "Data/temp/net/";

	static size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream)
	{
		size_t written = fwrite(ptr, size, nmemb, (FILE*)stream);
		return written;
	}


	void CheckNetTempFolder()
	{
		if (!std::filesystem::exists(NetTempFolder))
		{
			std::filesystem::create_directories(NetTempFolder);
		}

		if (std::filesystem::exists("Data/temp/invalidresponse.txt"))
		{
			std::filesystem::remove("Data/temp/invalidresponse.txt");
		}
	}


	void Download(std::string url, std::string target, std::string Header)
	{
#if DEV_NET_DEBUGGING
		Log::Print("Requesting url " + url);
#endif

		std::string target_cp = target;

		CURL* curl_handle;
		FILE* pagefile;

		/* init the curl session */
		curl_handle = curl_easy_init();

		if (!Header.empty())
		{
			struct curl_slist* headers = NULL; // init to NULL is important 
			headers = curl_slist_append(headers, Header.c_str());

			curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
		}

		/* set URL to get here */
		curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

		/* Switch on full protocol/debug output while testing */
		// curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

		/* disable progress meter, set to 0L to enable and disable debug output */
		curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);

		/* send all data to this function  */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

		/* open the file */
		if (target_cp.find_last_of("/\\") != std::string::npos)
		{
			std::string Path = target_cp.substr(0, target_cp.find_last_of("/\\"));
			std::filesystem::create_directories(Path);
		}

		pagefile = fopen(target_cp.c_str(), "wb");
		if (pagefile) 
		{

			/* write the page body to this file handle */
			curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);

			/* get it! */
			CURLcode res;
			res = curl_easy_perform(curl_handle);

			/* close the header file */
			fclose(pagefile);
		}
		else
		{
			Log::Print("pagefile could not be created: " + std::string(target_cp), Log::Error);
			return;
		}

		/* cleanup curl stuff */
		curl_easy_cleanup(curl_handle);

		return;
	}


	std::string GetLatestReleaseOf(std::string RepoName)
	{
		using namespace nlohmann;

		CheckNetTempFolder();

		Download("https://api.github.com/repos/" + RepoName + "/releases/latest", NetTempFolder + "version.txt", "User-Agent: " + Installer::UserAgent);

		try
		{
			std::ifstream in = std::ifstream(NetTempFolder + "version.txt");
			in.exceptions(std::ios::failbit | std::ios::badbit);
			std::stringstream str; str << in.rdbuf();
			auto response = json::parse(str.str());

			std::string latestname = response.at("name").dump();

			latestname = latestname.substr(1, latestname.size() - 2);

			Log::Print("Found latest release -> " + latestname);
			return latestname;
		}
		catch (std::exception& e)
		{
			Log::Print("Github response has an invalid layout.", Log::Error);
			Log::Print(e.what(), Log::Error);

			Log::Print("Writing response to Data/temp/invalidresponse.txt", Log::Error);
			std::filesystem::copy(NetTempFolder + "version.txt", "Data/temp/invalidresponse.txt");
		}
		return "";
	}

	std::string DownloadLatestReleaseOf(std::string RepoName)
	{
		using namespace nlohmann;
		CheckNetTempFolder();

		Download("https://api.github.com/repos/" + RepoName + "/releases/latest", NetTempFolder + "version.txt", "User-Agent: " + Installer::UserAgent);

		try
		{
			std::ifstream in = std::ifstream(NetTempFolder + "version.txt");
			std::stringstream str; str << in.rdbuf();
			auto response = json::parse(str.str());

			for (auto& elem : response["assets"])
			{
				std::string url = elem.at("browser_download_url").dump();

				url = url.substr(1, url.size() - 2);

				Log::Print("Found latest release -> " + url);
				Download(url, NetTempFolder + "latest.zip", "");
				return NetTempFolder + "latest.zip";
			}
		}
		catch (std::exception& e)
		{
			Log::Print("Github response has an invalid layout.", Log::Error);
			Log::Print(e.what(), Log::Error);

			Log::Print("Writing response to Data/temp/invalidresponse.txt", Log::Error);
			std::filesystem::copy(NetTempFolder + "version.txt", "Data/temp/invalidresponse.txt");
		}
		return "";
	}

	void Init()
	{
		Log::Print("Checking internet connection");
		Log::Print("Initialised cURL");
		std::filesystem::create_directories("Data/temp/net/");
		curl_global_init(CURL_GLOBAL_ALL);
		Networking::Download("https://northstar.tf", "Data/temp/net/ntf.html", "");
		if (std::filesystem::is_empty("Data/temp/net/ntf.html"))
		{
			std::filesystem::remove("Data/temp/net/ntf.html");
			Window::ShowPopup("Error", "Failed to ping https://northstar.tf\n\n\
What this could mean:\n\
- No internet connection\n\
- The northstar master server is currently offline\n\n\
What you can do:\n\
- Check your internet connection\n\
- Wait (Up to a few hours)");
			exit(0);
		}
		std::filesystem::remove("Data/temp/net/ntf.html");

#if _WIN32
		DWORD pCount = 0;
		PWTS_PROCESS_INFO ppProcessInfo, pProcess;
		WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &ppProcessInfo, &pCount);
		pProcess = ppProcessInfo;
		const char* pgm = "EADesktop.exe";

		for (int i = 0; i < pCount; i++)
		{
			std::wstring pName = pProcess->pProcessName;
			if (ToLowerCase(pgm) == ToLowerCase(std::string(pName.begin(), pName.end())))
			{
				Log::Print("EA app is running.");
				return;
			}
			pProcess++;
		}
		HKEY RegKey;
		LONG lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Electronic Arts\\EA Desktop", 0, KEY_READ, &RegKey);
		if (lRes == ERROR_SUCCESS)
		{
			std::wstring GameDir, GameDirDefault;
			LONG KeyResult = GetStringRegKey(RegKey, L"InstallLocation", GameDir, GameDirDefault);
			if (KeyResult == ERROR_SUCCESS)
			{
				std::string UTF8GameDir = std::string(GameDir.begin(), GameDir.end());
				Log::Print("Found EA app through Windows registry: " + UTF8GameDir);

				UTF8GameDir = std::regex_replace(UTF8GameDir, std::regex(" "), "^ ");


				system((UTF8GameDir + "\\EA^ Desktop\\EALauncher.exe").c_str());
			}
		}
#endif
	}

	void Cleanup()
	{
		curl_global_cleanup();

	}
	void ExtractZip(std::string File, std::string TargetFolder)
	{
		std::string TargetDir = TargetFolder + "/";

		mz_zip_archive archive;

		memset(&archive, 0, sizeof(mz_zip_archive));

		mz_zip_reader_init_file(&archive, File.c_str(), 0);

		const int FileCount = mz_zip_reader_get_num_files(&archive);

		if (FileCount <= 0)
			return;


		mz_zip_archive_file_stat stats;

		Log::Print("Extracting zip...");

		for (int i = 0; i < FileCount; i++)
		{
			memset(&stats, 0, sizeof(mz_zip_archive_file_stat));
			mz_zip_reader_file_stat(&archive, i, &stats);

			const bool IsDirectory = mz_zip_reader_is_file_a_directory(&archive, i);

			Log::Print("Found: " + std::string(stats.m_filename));

			std::string TargetFileDir = TargetDir + stats.m_filename;
			TargetFileDir = TargetFileDir.substr(0, TargetFileDir.find_last_of("\\/"));
			std::filesystem::create_directories(TargetFileDir);

			mz_zip_reader_extract_to_file(&archive, i, (TargetDir + stats.m_filename).c_str(), 0);
		}

		mz_zip_reader_end(&archive);

	}
}