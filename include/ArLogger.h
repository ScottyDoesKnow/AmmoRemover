#pragma once

class ArLogger
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
	static void LogP(LogLevel level, const char* prefix, const char* format, ...);

protected:
	static IDebugLog::LogLevel Convert(LogLevel level)
	{
		return static_cast<IDebugLog::LogLevel>(level);
	}

private:
	static void LogInternal(LogLevel level, const char* prefix, const char* format, va_list args);
};
