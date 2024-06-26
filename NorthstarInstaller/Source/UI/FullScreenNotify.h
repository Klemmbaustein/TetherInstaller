#pragma once
#include <KlemmUI/UI/UIBox.h>
#include <KlemmUI/UI/UIBackground.h>

class FullScreenNotify
{
	KlemmUI::UIBackground* BlockingBackground = nullptr;
public:
	FullScreenNotify(std::string Title);
	~FullScreenNotify();

	KlemmUI::UIBox* ContentBox = nullptr;

	struct NotifyOption
	{
		std::string Name;
		std::string Icon;
		void (*OnClicked)();
	};

	void AddOptions(std::vector<NotifyOption> Options);
private:
	std::vector<NotifyOption> Options;
};