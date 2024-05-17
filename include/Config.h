#pragma once

#include "Logger.h"

class Config
{
public:
	enum InitializeResult
	{
		Failed,
		ConfigFile,
		Defaults
	};

	static InitializeResult Initialize(SInt32& ammoPercent, Logger::LogLevel& logLevel);

private:
	static bool GetPath(char* path, rsize_t pathSize);
	static bool Exists(const char* path);
	static bool Load(const char* path, SInt32& ammoPercent, Logger::LogLevel& logLevel);
	static bool GetDefaultPath(char* path, rsize_t pathSize);
	static bool Copy(const char* sourcePath, const char* destPath);
};
