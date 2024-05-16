#include "Logger.h"

#include <ShlObj_core.h>

static constexpr char LOG_PATH[] = R"(\My Games\Fallout4\F4SE\AmmoRemover.log)";

static constexpr const char* PREFIX[] = {
	R"(FATAL   | )",
	R"(ERROR   | )",
	R"(WARNING | )",
	R"(MESSAGE | )",
	R"(VERBOSE | )",
	R"(DEBUG   | )"
};

Logger::LogLevel Logger::logLevel = Message;

void Logger::Initialize()
{
	IDebugLog::OpenRelative(CSIDL_MYDOCUMENTS, LOG_PATH);
	IDebugLog::SetPrintLevel(Convert(logLevel));
	IDebugLog::SetLogLevel(Convert(logLevel));
}


void Logger::Log(const LogLevel level, const char* format, ...)
{
	if (level > logLevel)
		return;

	va_list args;
	va_start(args, format);
	Log(level, nullptr, format, args);
	va_end(args);
}


void Logger::Log(const LogLevel level, const char* method, const char* format, ...)
{
	if (level > logLevel)
		return;

	const UInt8 levelVal = static_cast<UInt8>(level);

	const size_t length = strlen(PREFIX[levelVal]) + (method != nullptr ? strlen(method) : 0) + strlen(format) + 1;
	const auto output = new char[length];
	strcpy_s(output, length, PREFIX[levelVal]);
	if (method != nullptr)
		strcat_s(output, length, method);
	strcat_s(output, length, format);

	va_list args;
	va_start(args, format);
	IDebugLog::Log(Convert(level), output, args);
	va_end(args);

	delete[] output;
}