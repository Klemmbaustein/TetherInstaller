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
#include "BackgroundTask.h"

#define DEV_NET_DEBUGGING 0

#if _WIN32
#include <wtsapi32.h>
#include <iostream>
#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "crypt32.lib")
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
}
#endif



namespace Networking
{

	const std::string NetTempFolder = "Data/temp/net/";

	static size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream)
	{
		size_t written = fwrite(ptr, size, nmemb, (FILE*)stream);
		return written;
	}

	int progress_func(void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded)
	{
		if (TotalToDownload == 0)
		{ 
			return 0;
		}
		BackgroundTask::SetProgress(NowDownloaded / TotalToDownload * 0.9f);
		return 0;
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


	void Download(std::string url, std::string target, std::string Header, bool IsDownload)
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

		if (IsDownload)
		{
			// Internal CURL progressmeter must be disabled if we provide our own callback
			curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
			// Install the callback function
			curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, progress_func);
		}

		/* open the file */
		if (target_cp.find_last_of("/\\") != std::string::npos)
		{
			std::string Path = target_cp.substr(0, target_cp.find_last_of("/\\"));
			try
			{
				std::filesystem::create_directories(Path);
			}
			catch (std::exception& e)
			{
				Window::ShowPopupError(e.what());
				return;
			}
		}

		pagefile = fopen(target_cp.c_str(), "wb");
		if (pagefile) 
		{

			/* write the page body to this file handle */
			curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);

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

	bool IsProcessRunning(std::string Name)
	{
#if _WIN32
		DWORD pCount = 0;
		PWTS_PROCESS_INFOA ppProcessInfo, pProcess;
		WTSEnumerateProcessesA(WTS_CURRENT_SERVER_HANDLE, 0, 1, &ppProcessInfo, &pCount);
		pProcess = ppProcessInfo;

		for (int i = 0; i < pCount; i++)
		{
			std::string pName = pProcess->pProcessName;
			if (ToLowerCase(Name) == ToLowerCase(pName))
			{
				return true;
			}
			pProcess++;
		}
#endif
		return false;
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

	std::string DownloadLatestReleaseOf(std::string RepoName, std::string NecessaryAssetName)
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
				if (!NecessaryAssetName.empty() && elem.at("name").get<std::string>().find(NecessaryAssetName) == std::string::npos)
				{
					continue;
				}

				std::string url = elem.at("browser_download_url").dump();

				url = url.substr(1, url.size() - 2);

				Log::Print("Found latest release -> " + url);
				Download(url, NetTempFolder + "latest.zip", "User-Agent: " + Installer::UserAgent, true);
				return NetTempFolder + "latest.zip";
			}
		}
		catch (std::exception& e)
		{
			Log::Print("Github response has an invalid layout. - https://api.github.com/repos/" + RepoName + "/releases/latest", Log::Error);
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

#if _WIN32
		if (IsProcessRunning("EADesktop.exe"))
		{
			return;
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

				STARTUPINFOA Startup;
				PROCESS_INFORMATION pi;
				ZeroMemory(&Startup, sizeof(Startup));
				Startup.cb = sizeof(Startup);
				ZeroMemory(&pi, sizeof(pi));

				CreateProcessA((UTF8GameDir + "\\EA Desktop\\EALauncher.exe").c_str(),
					NULL,
					NULL,
					NULL,
					FALSE,
					0,
					NULL,
					NULL,
					&Startup,
					&pi);

				// Wait until child process exits.
				WaitForSingleObject(pi.hProcess, INFINITE);

				// Close process and thread handles. 
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
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