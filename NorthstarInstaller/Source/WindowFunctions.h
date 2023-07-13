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

	PopupReply ShowPopup(std::string Title, std::string Message);
}