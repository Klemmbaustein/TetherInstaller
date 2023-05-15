#pragma once
#include <UI/UIText.h>
#include <UI/UIBackground.h>
#include <UI/UIScrollBox.h>

class LogPanel
{
public:
	LogPanel();
	void Update();

	static void PrebufferLogEvents();

	static UIBackground* LogBackground;
	static UIScrollBox* LogScrollBox;
};