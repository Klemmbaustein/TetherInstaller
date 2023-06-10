#pragma once
#include <string>

namespace Log
{

	enum Severity
	{
		Info,
		Warning,
		Error
	};

	extern std::string TypeStrings[2];
	extern std::string SeverityStrings[3];

	void RegisterOnPrintCallback(void (*OnPrint)(std::string Msg, Severity MsgSeverity));
	void Print(std::string Msg, Severity MsgSeverity = Info);
}