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

	void(*PrintCallback)(std::string Msg, PrintType Type, Severity MsgSeverity) = nullptr;
}

void Log::RegisterOnPrintCallback(void(*OnPrint)(std::string Msg, PrintType Type, Severity MsgSeverity))
{
	PrintCallback = OnPrint;
}

void Log::Print(std::string Msg, PrintType Type, Severity MsgSeverity)
{
	std::puts(("[" + TypeStrings[Type] + "] [" + SeverityStrings[MsgSeverity] + "]: " + Msg).c_str());
	if (PrintCallback)
	{
		PrintCallback(Msg, Type, MsgSeverity);
	}
}
