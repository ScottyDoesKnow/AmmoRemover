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
	static bool GetDocumentsPath(char* path, rsize_t pathSize, const char* relativePath);
	static bool Exists(const char* path);
	static bool Load(const char* path, SInt32& ammoPercent, Logger::LogLevel& logLevel);
	static bool GetLocalPath(char* path, rsize_t pathSize, const char* relativePath);
	static bool Copy(const char* sourcePath, const char* destPath);
};
