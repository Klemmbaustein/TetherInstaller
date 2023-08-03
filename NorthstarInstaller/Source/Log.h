#pragma once
#include <string>
// #include <format>

namespace Log
{
	enum Severity
	{
		Info = 0,
		Warning = 1,
		Error = 2
	};

	void SetIsVerbose(bool NewIsVerbose);
	bool GetIsVerbose();

	void Print(std::string Message, Severity Type = Info);
}
#define LOG_PRINTF(msg, ...) Log::Print(msg)