#pragma once
#include <string>

namespace Log
{
	enum PrintType
	{
		General,
		Install
	};

	enum Severity
	{
		Info,
		Warning,
		Error
	};

	extern std::string TypeStrings[2];
	extern std::string SeverityStrings[3];

	void RegisterOnPrintCallback(void (*OnPrint)(std::string Msg, PrintType Type, Severity MsgSeverity));
	void Print(std::string Msg, PrintType Type = General, Severity MsgSeverity = Info);
}