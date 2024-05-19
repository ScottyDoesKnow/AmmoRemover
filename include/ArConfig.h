#pragma once

#include "ArLogger.h"

class ArConfig
{
public:
	enum InitializeResult
	{
		Failed,
		ConfigFile,
		Defaults
	};

	static InitializeResult Initialize(SInt32& wepAmmoPercent, SInt32& invAmmoPercent, ArLogger::LogLevel& logLevel);

private:
	static bool GetDocumentsPath(char* path, rsize_t pathSize, const char* relativePath);
	static bool Exists(const char* path);
	static bool Load(const char* path, SInt32& wepAmmoPercent, SInt32& invAmmoPercent, ArLogger::LogLevel& logLevel);
	static bool GetLocalPath(char* path, rsize_t pathSize, const char* relativePath);
	static bool Copy(const char* sourcePath, const char* destPath);
};
