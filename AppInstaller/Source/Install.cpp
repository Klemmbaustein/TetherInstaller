#include "Install.h"
#include "resource.h"
#include "miniz.h"
#include <Windows.h>
#include <fstream>
#include <filesystem>
#include <thread>
#include <ShlObj_core.h>

std::thread* InstallThread = nullptr;
bool IsFinished = false;

HRESULT CreateLink(LPCWSTR lpszPathObj, LPCWSTR lpszPathLink, LPCWSTR lpszDesc)
{
	HRESULT hres;
	IShellLink* psl;

	// Get a pointer to the IShellLink interface. It is assumed that CoInitialize
	// has already been called.
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
	if (SUCCEEDED(hres))
	{
		IPersistFile* ppf;

		// Set the path to the shortcut target and add the description. 
		psl->SetPath(lpszPathObj);
		psl->SetDescription(lpszDesc);

		// Query IShellLink for the IPersistFile interface, used for saving the 
		// shortcut in persistent storage. 
		hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

		if (SUCCEEDED(hres))
		{
			// Add code here to check return value from MultiByteWideChar 
			// for success.

			// Save the link by calling IPersistFile::Save. 
			hres = ppf->Save(lpszPathLink, TRUE);
			ppf->Release();
		}
		psl->Release();
	}
	return hres;
}

std::string ws2s(const std::wstring& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
	char* buf = new char[len];
	WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, buf, len, 0, 0);
	std::string r(buf);
	delete[] buf;
	return r;
}

static void ExtractZip(std::string File, std::string TargetFolder)
{
	std::string TargetDir = TargetFolder + "/";

	mz_zip_archive archive;

	memset(&archive, 0, sizeof(mz_zip_archive));

	mz_zip_reader_init_file(&archive, File.c_str(), 0);

	const int FileCount = mz_zip_reader_get_num_files(&archive);

	if (FileCount <= 0)
		return;


	mz_zip_archive_file_stat stats;

	for (int i = 0; i < FileCount; i++)
	{
		memset(&stats, 0, sizeof(mz_zip_archive_file_stat));
		mz_zip_reader_file_stat(&archive, i, &stats);

		const bool IsDirectory = mz_zip_reader_is_file_a_directory(&archive, i);

		std::string TargetFileDir = TargetDir + stats.m_filename;
		TargetFileDir = TargetFileDir.substr(0, TargetFileDir.find_last_of("\\/"));
		std::filesystem::create_directories(TargetFileDir);

		mz_zip_reader_extract_to_file(&archive, i, (TargetDir + stats.m_filename).c_str(), 0);
	}

	mz_zip_reader_end(&archive);
}

static void InstallAppThread(bool CreateStartShortcut, bool CreateDesktopShortcut)
{
	// Load the .zip resource.
	HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(IDR_INSTALLER_ZIP), L"CONTENT");
	if (!hResource)
	{
		MessageBoxA(NULL, "Could not find embedded app data.", "Error", 0);
		return;
	}
	HGLOBAL hGlobal = LoadResource(NULL, hResource);
	if (!hGlobal)
	{
		MessageBoxA(NULL, "Could not find embedded app data.", "Error", 0);
		return;
	}
	BYTE* pData = (BYTE*)LockResource(hGlobal);
	int size = SizeofResource(NULL, hResource);

	WCHAR* Buffer = new WCHAR[MAX_PATH]();

	// Write .zip resource to appdata.
	SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &Buffer);
	std::wstring AppDir = std::wstring(Buffer) + L"\\TetherInstaller";
	std::filesystem::create_directories(AppDir);
	std::ofstream Out = std::ofstream(AppDir + L"\\App.zip", std::ios::binary);

	Out.write((char*)pData, size);
	Out.close();

	FreeResource(hGlobal);

	ExtractZip(ws2s(AppDir + L"\\App.zip"), ws2s(AppDir));

	if (CreateDesktopShortcut)
	{
		SHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &Buffer);
		CreateLink((AppDir + L"\\TetherInstaller.exe").c_str(), (std::wstring(Buffer) + L"\\Tether Installer.lnk").c_str(), L"");
	}

	if (CreateStartShortcut)
	{
		SHGetKnownFolderPath(FOLDERID_StartMenu, 0, NULL, &Buffer);
		CreateLink((AppDir + L"\\TetherInstaller.exe").c_str(), (std::wstring(Buffer) + L"\\Tether Installer.lnk").c_str(), L"");
	}

	delete[] Buffer;

	IsFinished = true;
}


void Install::InstallApp(bool CreateStartShortcut, bool CreateDesktopShortcut)
{
	InstallThread = new std::thread(InstallAppThread, CreateStartShortcut, CreateDesktopShortcut);
}

bool Install::IsInstalling()
{
	if (IsFinished)
	{
		if (InstallThread)
		{
			InstallThread->join();
			delete InstallThread;
			InstallThread = nullptr;
		}
		return false;
	}
	return true;
}

void Install::LaunchApp()
{
	WCHAR* Buffer = new WCHAR[MAX_PATH]();
	
	SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &Buffer);
	ShellExecute(NULL, NULL, (std::wstring(Buffer) + L"\\TetherInstaller\\TetherInstaller.exe").c_str(), NULL, NULL, 1);
	delete[] Buffer;
}
