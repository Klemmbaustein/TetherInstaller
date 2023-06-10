#include "LogPanel.h"
#include "Log.h"
#include "UIDef.h"

namespace Log
{
	struct LogMsg
	{
		std::string msg;
		Severity sev;
	};
	
	std::vector<LogMsg> BufferedMessages;


	Vector3f32 SeverityColors[3] =
	{
		Vector3f32(0.4, 0.6, 1),
		Vector3f32(1, 1, 0),
		Vector3f32(1, 0.25, 0.25),
	};
}

UIBackground* LogPanel::LogBackground = nullptr;
UIScrollBox* LogPanel::LogScrollBox = nullptr;

LogPanel::LogPanel()
{
	LogBackground = new UIBackground(false, Vector2f(0.4, -0.8), 0.1, Vector2f(0.6, 1.8));
	LogBackground->Align = UIBox::E_REVERSE;

	auto Callbackfun = [](std::string Msg, Log::Severity sev) {
		auto NewText = new UIText(0.25, ColoredText({ TextSegment("[" + Log::SeverityStrings[sev] + "]: ", Log::SeverityColors[sev]), TextSegment(Msg, Vector3f32(1))}), UI::MonoText);
		NewText->Wrap = true;
		NewText->WrapDistance = 1.95;
		LogBackground->AddChild(NewText
			->SetPadding(0));
	};

	for (auto& i : Log::BufferedMessages)
	{
		Callbackfun(i.msg, i.sev);
	}

	Log::RegisterOnPrintCallback(Callbackfun);
}

void LogPanel::Update()
{
}

void LogPanel::PrebufferLogEvents()
{
	Log::RegisterOnPrintCallback([](std::string Msg, Log::Severity sev) {
		Log::LogMsg m;
		m.msg = Msg;
		m.sev = sev;
		Log::BufferedMessages.push_back(m);
		});
}
