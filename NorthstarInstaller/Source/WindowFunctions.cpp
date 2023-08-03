#include "WindowFunctions.h"
#include "Log.h"
#if _WIN32
#include <Shlobj.h>
#include <shobjidl.h> 


//https://www.cplusplus.com/forum/windows/74644/
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


std::string Window::ShowSelectFolderDialog()
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
			pFileOpen->SetOptions(FOS_PICKFOLDERS);
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

Window::PopupReply Window::ShowPopupQuestion(std::string Title, std::string Message)
{
	int a = MessageBoxA(NULL, Message.c_str(), Title.c_str(), MB_YESNO | MB_ICONQUESTION);
	if (a == IDYES)
	{
		return Window::PopupReply::Yes;
	}
	return Window::PopupReply::No;
}
void Window::ShowPopup(std::string Title, std::string Message)
{
	MessageBoxA(NULL, Message.c_str(), Title.c_str(), MB_OK);
}
#else
std::string Window::ShowSelectFolderDialog()
{
	return "";
}

Window::PopupReply Window::ShowPopupQuestion(std::string Title, std::string Message)
{
	return Window::PopupReply::No;
}
void Window::ShowPopup(std::string Title, std::string Message)
{
	Log::Print(Title + ": " + Message);
}
#endif
