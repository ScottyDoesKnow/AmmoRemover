#include "ArLogger.h"

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

ArLogger::LogLevel ArLogger::logLevel = Message;

void ArLogger::Initialize()
{
	IDebugLog::OpenRelative(CSIDL_MYDOCUMENTS, LOG_PATH);
	IDebugLog::SetPrintLevel(Convert(logLevel));
	IDebugLog::SetLogLevel(Convert(logLevel));
}


void ArLogger::Log(const LogLevel level, const char* format, ...)
{
	if (level > logLevel)
		return;

	va_list args;
	va_start(args, format);
	LogInternal(level, nullptr, format, args);
	va_end(args);
}


void ArLogger::LogP(const LogLevel level, const char* prefix, const char* format, ...)
{
	if (level > logLevel)
		return;

	va_list args;
	va_start(args, format);
	LogInternal(level, prefix, format, args);
	va_end(args);
}

void ArLogger::LogInternal(const LogLevel level, const char* prefix, const char* format, const va_list args)
{
	const UInt8 levelVal = static_cast<UInt8>(level);

	const size_t length = strlen(PREFIX[levelVal]) + (prefix != nullptr ? strlen(prefix) : 0) + strlen(format) + 1;
	const auto output = new char[length];
	strcpy_s(output, length, PREFIX[levelVal]);
	if (prefix != nullptr)
		strcat_s(output, length, prefix);
	strcat_s(output, length, format);

	va_list argsCopy;
	va_copy(argsCopy, args);
	IDebugLog::Log(Convert(level), output, argsCopy);
	va_end(argsCopy);

	delete[] output;
}