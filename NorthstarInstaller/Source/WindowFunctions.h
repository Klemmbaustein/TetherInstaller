#pragma once
#include <string>

namespace Window
{
	std::string ShowSelectFolderDialog();

	enum class PopupReply
	{
		No = 0,
		Yes = 1,
	};

	PopupReply ShowPopupQuestion(std::string Title, std::string Message);
	void ShowPopup(std::string Title, std::string Message);
}