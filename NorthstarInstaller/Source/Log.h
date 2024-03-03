#pragma once
#include <string>
#include <version>

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

	typedef void(*LogFn)(const char* msg, int sev);
	void OverrideLogFunction(LogFn Function);
}

#if __cpp_lib_format >= 202207L
#include <format>
#define LOG_PRINTF(msg, ...) Log::Print(std::format(msg, __VA_ARGS__))
#else
#define LOG_PRINTF(msg, ...) Log::Print(msg)
#endif