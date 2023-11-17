#include "WindowFunctions.h"
#include "Log.h"
#include <tinyfiledialogs.h>

#if _WIN32
#include <Windows.h>
#include <ShlObj.h>
#include <iostream>
std::string wstrtostr(const std::wstring& wstr)
{
	std::string strTo;
	char* szTo = new char[wstr.length() + 1];
	szTo[wstr.size()] = '\0';
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, szTo, (int)wstr.length(), NULL, NULL);
	strTo = szTo;
	delete[] szTo;
	return strTo;
}
std::string Window::ShowSelectFileDialog(bool PickFolders)
{
	std::string FilePath = "";
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
		COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog* pFileOpen;

		// Create the FileOpenDialog object.
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
			IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr))
		{
			// Show the Open dialog box.
			if (PickFolders)
			{
				pFileOpen->SetOptions(FOS_PICKFOLDERS);
			}
			hr = pFileOpen->Show(NULL);
			// Get the file name from the dialog box.
			if (SUCCEEDED(hr))
			{
				IShellItem* pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr))
				{
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
					// Display the file name to the user.
					if (SUCCEEDED(hr))
					{
						FilePath = wstrtostr(pszFilePath);
						return FilePath;
					}
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
		CoUninitialize();
	}
	return FilePath;
}

#else
// tinyfd does *not* use the regular open file dialog on windows.
std::string Window::ShowSelectFileDialog(bool PickFolders)
{
	if (PickFolders)
	{
		char* ret = tinyfd_selectFolderDialog("Tether Installer", nullptr);
		if (ret)
		{
			return ret;
		}
		return "";
	}

	char* ret = tinyfd_openFileDialog("Tether Installer",
		nullptr,
		0,
		nullptr,
		nullptr,
		0);
	if (ret)
	{
		return ret;
	}
	return "";
}
#endif

Window::PopupReply Window::ShowPopupQuestion(std::string Title, std::string Message)
{
	int a = tinyfd_messageBox(Title.c_str(), Message.c_str(), "yesno", "question", 1);
	if (a == 1)
	{
		return Window::PopupReply::Yes;
	}
	return Window::PopupReply::No;
}
void Window::ShowPopup(std::string Title, std::string Message)
{
	tinyfd_messageBox(Title.c_str(), Message.c_str(), "ok", "info", 1);
}
void Window::ShowPopupError(std::string Message)
{
	Log::Print(Message, Log::Error);
	tinyfd_messageBox("Tether Installer", Message.c_str(), "ok", "error", 1);
}
