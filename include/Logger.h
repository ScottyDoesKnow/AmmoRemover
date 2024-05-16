#pragma once

class Logger
{
public:
	enum LogLevel
	{
		Fatal = 0,
		Error,
		Warning,
		Message,
		Verbose,
		Debug
	};

	static LogLevel logLevel;

	static void Initialize();
	static void Log(LogLevel level, const char* format, ...);
	static void Log(LogLevel level, const char* method, const char* format, ...);

protected:
	static IDebugLog::LogLevel Convert(LogLevel level)
	{
		return static_cast<IDebugLog::LogLevel>(level);
	}
};
