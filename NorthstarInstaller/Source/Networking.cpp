#include "Networking.h"

#include "Log.h"
#include "Installer.h"
#include "WindowFunctions.h"

#define CURL_STATICLIB 1
#include <curl/curl.h>
#include "nlohmann/json.hpp"
#include "miniz.h"

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

	static size_t WriteData(void* ptr, size_t size, size_t nmemb, void* stream)
	{
		size_t written = fwrite(ptr, size, nmemb, (FILE*)stream);
		return written;
	}

	static size_t StringWrite(uint8_t* ptr, size_t size, size_t nmemb, std::string* str)
	{
		size_t NumBytes = size * nmemb;
		str->reserve(str->size() + NumBytes);
		for (size_t i = 0; i < NumBytes; i++)
		{
			str->push_back(ptr[i]);
		}
		return size * NumBytes;
	}

	int ProgressFunction(void* ptr, curl_off_t TotalToDownload, curl_off_t NowDownloaded, curl_off_t TotalToUpload, curl_off_t NowUploaded)
	{
		if (TotalToDownload == 0)
		{ 
			return 0;
		}
		BackgroundTask::SetProgress((float)NowDownloaded / (float)TotalToDownload * 0.9f);
		return 0;
	}

	static void DownloadInternal(std::string Url, std::string UserAgent, void* WriteFunction, void* WriteData, bool Download)
	{
		CURL* CurlHandle = curl_easy_init();
		curl_slist* headers = NULL;
		headers = curl_slist_append(headers, ("User-Agent: " + UserAgent).c_str());
		curl_easy_setopt(CurlHandle, CURLOPT_HTTPHEADER, headers);

		curl_easy_setopt(CurlHandle, CURLOPT_WRITEDATA, WriteData);
		curl_easy_setopt(CurlHandle, CURLOPT_FOLLOWLOCATION, true);

		curl_easy_setopt(CurlHandle, CURLOPT_URL, Url.c_str());
		curl_easy_setopt(CurlHandle, CURLOPT_WRITEFUNCTION, WriteFunction);
		curl_easy_setopt(CurlHandle, CURLOPT_NOPROGRESS, !Download);

		if (Download)
		{
			curl_easy_setopt(CurlHandle, CURLOPT_XFERINFOFUNCTION, ProgressFunction);
		}

		CURLcode res = curl_easy_perform(CurlHandle);

		curl_easy_cleanup(CurlHandle);
	}

	static size_t FileWrite(uint8_t* ptr, size_t size, size_t nmemb, std::ofstream* stream)
	{
		stream->write((char*)ptr, size * nmemb);
		return size * nmemb;
	}

	void Download(std::string Url, std::string To, std::string UserAgent, bool IsDownload)
	{
		std::ofstream OutFile = std::ofstream(To, std::ios::binary | std::ios::out);
		DownloadInternal(Url, UserAgent, FileWrite, &OutFile, IsDownload);
		return;
	}

	std::string DownloadString(std::string Url, std::string UserAgent)
	{
		std::string OutString;

		DownloadInternal(Url, UserAgent, StringWrite, &OutString, false);

		return OutString;
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

		try
		{
			auto response = json::parse(DownloadString("https://api.github.com/repos/" + RepoName + "/releases/latest", Installer::UserAgent));

			std::string latestname = response.at("name").dump();

			latestname = latestname.substr(1, latestname.size() - 2);

			Log::Print("Found latest release -> " + latestname);
			return latestname;
		}
		catch (std::exception& e)
		{
			Log::Print("Github response has an invalid layout.", Log::Error);
			Log::Print(e.what(), Log::Error);
		}
		return "";
	}

	std::string DownloadLatestReleaseOf(std::string RepoName, std::string NecessaryAssetName)
	{
		using namespace nlohmann;
		try
		{
			auto response = json::parse(DownloadString("https://api.github.com/repos/" + RepoName + "/releases/latest", Installer::UserAgent));

			for (auto& elem : response["assets"])
			{
				if (!NecessaryAssetName.empty() && elem.at("name").get<std::string>().find(NecessaryAssetName) == std::string::npos)
				{
					continue;
				}

				std::string url = elem.at("browser_download_url").dump();

				url = url.substr(1, url.size() - 2);

				Log::Print("Found latest release -> " + url);
				Download(url, Installer::CurrentPath + NetTempFolder + "latest.zip", "User-Agent: " + Installer::UserAgent, true);
				return Installer::CurrentPath + NetTempFolder + "latest.zip";
			}
		}
		catch (std::exception& e)
		{
			Log::Print("Github response has an invalid layout. - https://api.github.com/repos/" + RepoName + "/releases/latest", Log::Error);
			Log::Print(e.what(), Log::Error);
		}
		return "";
	}

	void Init()
	{
		Log::Print("Checking internet connection");
		Log::Print("Initialised cURL");
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
				char* MultiByte = new char[GameDir.size()];
				WideCharToMultiByte(CP_UTF8, 0, GameDir.c_str(), -1, MultiByte, GameDir.size(), NULL, NULL);
				std::string UTF8GameDir = MultiByte;
				delete[] MultiByte;

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
