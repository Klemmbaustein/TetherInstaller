#include "Log.h"

namespace Log
{
	std::string TypeStrings[2] =
	{
		"General",
		"Install"
	};

	std::string SeverityStrings[3] =
	{
		"Info",
		"Warning",
		"Error"
	};

	void(*PrintCallback)(std::string Msg, Severity MsgSeverity) = nullptr;
}

void Log::RegisterOnPrintCallback(void(*OnPrint)(std::string Msg, Severity MsgSeverity))
{
	PrintCallback = OnPrint;
}

void Log::Print(std::string Msg, Severity MsgSeverity)
{
	std::puts((SeverityStrings[MsgSeverity] + "]: " + Msg).c_str());
	if (PrintCallback)
	{
		PrintCallback(Msg, MsgSeverity);
	}
}
