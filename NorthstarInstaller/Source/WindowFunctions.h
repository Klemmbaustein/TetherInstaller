#pragma once
#include <string>

namespace WindowFunc
{
	std::string ShowSelectFileDialog(bool PickFolders);

	enum class PopupReply
	{
		No = 0,
		Yes = 1,
	};

	PopupReply ShowPopupQuestion(std::string Title, std::string Message);
	void ShowPopup(std::string Title, std::string Message);
	void ShowPopupError(std::string Message);
	
	std::string GetAppPath();
}