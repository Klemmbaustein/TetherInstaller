#include "Networking.h"

#include <curl/curl.h>
#include "JSON/json.hpp"

#include "Log.h"

#include <fstream>
#include <filesystem>

namespace Networking
{
	const std::string NorthstarVersionFile = "Data/";
	const std::string NetTempFolder = "temp/net/";

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

		if (std::filesystem::exists("temp/invalidresponse.txt"))
		{
			std::filesystem::remove("temp/invalidresponse.txt");
		}
	}


	void Download(std::string url, std::string target, bool SendUsrAgent)
	{
		Log::Print("Downloading url " + url + " -> " + target);

		std::string target_cp = target;

		CURL* curl_handle;
		FILE* pagefile;

		curl_global_init(CURL_GLOBAL_ALL);

		/* init the curl session */
		curl_handle = curl_easy_init();

		if (SendUsrAgent)
		{
			struct curl_slist* headers = NULL; // init to NULL is important 
			headers = curl_slist_append(headers, "User-Agent: None");

			curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
		}

		/* set URL to get here */
		curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

		/* Switch on full protocol/debug output while testing */
		curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

		/* disable progress meter, set to 0L to enable and disable debug output */
		curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);

		/* send all data to this function  */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

		/* open the file */
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
			Log::Print("pagefile could not be created: " + std::string(target_cp), Log::Install, Log::Error);
			return;
		}

		/* cleanup curl stuff */
		curl_easy_cleanup(curl_handle);


		return;
	}


	std::string Networking::GetLatestReleaseOf(std::string RepoName)
	{
		using namespace nlohmann;

		CheckNetTempFolder();

		Download("https://api.github.com/repos/" + RepoName + "/releases/latest", NetTempFolder + "version.temp", true);

		try
		{
			std::ifstream in = std::ifstream(NetTempFolder + "version.temp");
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
			Log::Print("Github response has an invalid layout.", Log::Install, Log::Error);
			Log::Print(e.what(), Log::Install, Log::Error);

			Log::Print("Writing response to temp/invalidresponse.txt", Log::Install, Log::Error);
			std::filesystem::copy(NetTempFolder + "version.temp", "temp/invalidresponse.txt");
		}
		return "";
	}


	std::string DownloadLatestReleaseOf(std::string RepoName)
	{
		using namespace nlohmann;
		CheckNetTempFolder();
		//Download("https://github.com/R2Northstar/Northstar/releases/download/v1.14.2/Northstar.release.v1.14.2.zip", "test.txt", false);

		Download("https://api.github.com/repos/" + RepoName + "/releases/latest", NetTempFolder + "version.temp", true);

		try
		{
			std::ifstream in = std::ifstream(NetTempFolder + "version.temp");
			std::stringstream str; str << in.rdbuf();
			auto response = json::parse(str.str());

			for (auto& elem : response["assets"])
			{
				std::string url = elem.at("browser_download_url").dump();

				url = url.substr(1, url.size() - 2);

				Log::Print("Found latest release -> " + url);
				//Log::Print("Downloading new release of " + url + "...");
				Download(url, NetTempFolder + "latest.zip", false);
				return NetTempFolder + "latest.zip";
			}
		}
		catch (std::exception& e)
		{
			Log::Print("Github response has an invalid layout.", Log::Install, Log::Error);
			Log::Print(e.what(), Log::Install, Log::Error);

			Log::Print("Writing response to temp/invalidresponse.txt", Log::Install, Log::Error);
			std::filesystem::copy(NetTempFolder + "version.temp", "temp/invalidresponse.txt");
		}
		return "";
	}

	void Init()
	{
		Log::Print("Initialised cURL");
		curl_global_init(CURL_GLOBAL_ALL);
	}

	void Cleanup()
	{
		curl_global_cleanup();

	}
}