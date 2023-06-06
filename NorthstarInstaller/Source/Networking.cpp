#include "Networking.h"

#include <curl/curl.h>
#include "JSON/json.hpp"

#include "Log.h"

#include <fstream>
#include <filesystem>
#include <zip.h>

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


	void Download(std::string url, std::string target, std::string Header)
	{
		Log::Print("Requesting url " + url);

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

		Download("https://api.github.com/repos/" + RepoName + "/releases/latest", NetTempFolder + "version.temp", "User-Agent: None");

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

		Download("https://api.github.com/repos/" + RepoName + "/releases/latest", NetTempFolder + "version.temp", "User-Agent: None");

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
				Download(url, NetTempFolder + "latest.zip", "");
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
	void ExtractZip(std::string File, std::string TargetFolder)
	{
		int err = 0;
		zip* z = zip_open(File.c_str(), 0, &err);
		std::string TargetDir = TargetFolder + "/";

		struct zip_stat* finfo = NULL;
		finfo = (struct zip_stat*)calloc(256, sizeof(int));
		zip_stat_init(finfo);
		zip_file_t* fd = NULL;
		char* txt = NULL;
		int count = 0;
		while ((zip_stat_index(z, count, 0, finfo)) == 0) {

			// allocate room for the entire file contents.
			txt = (char*)calloc(finfo->size + 1, sizeof(char));
			fd = zip_fopen_index(
				z, count, 0); // opens file at count index
			// reads from fd finfo->size
			// bytes into txt buffer.
			zip_fread(fd, txt, finfo->size);

			// Let's hope i'll never have to compile this on linux.
			size_t slash = std::string(finfo->name).find_last_of("/\\");
			if (slash != std::string::npos)
			{
				std::filesystem::create_directories(TargetDir + std::string(finfo->name).substr(0, slash));
			}
			std::ofstream(TargetDir + std::string(finfo->name), std::ios::out | std::ios::binary).write(txt, finfo->size);

			// frees allocated buffer, will
			// reallocate on next iteration of loop.
			free(txt);
			zip_fclose(fd);
			// increase index by 1 and the loop will
			// stop when files are not found.
			count++;
		}
		zip_close(z);
	}
}