#include "Config.h"

#include <fstream>
#include <ShlObj_core.h>
#include <Shlwapi.h>

#include "INIReader.h"

static constexpr char DEFAULT_CONFIG_PATH[] = R"(\AmmoRemoverDefaults.ini)";
static constexpr char CONFIG_PATH[] = R"(\My Games\Fallout4\F4SE\AmmoRemover.ini)";

static constexpr SInt32 AMMO_PERCENT_MAX = 1000;

bool Config::Initialize(SInt32& ammoPercent, Logger::LogLevel& logLevel)
{
	char configPath[MAX_PATH];
	if (!GetPath(configPath, MAX_PATH))
		return false;

	if (!Exists(configPath) || !Load(configPath, ammoPercent, logLevel))
	{
		char defaultConfigPath[MAX_PATH];
		if (!GetDefaultPath(defaultConfigPath, MAX_PATH)
			|| !Copy(defaultConfigPath, configPath)
			|| !Load(configPath, ammoPercent, logLevel))
		{
			return false;
		}
	}

	return true;
}

bool Config::GetPath(char* path, const rsize_t pathSize)
{
	const HRESULT error = SHGetFolderPath(nullptr, CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE,
		nullptr, SHGFP_TYPE_CURRENT, path);
	if (FAILED(error))
		return false;

	if (strlen(path) + strlen(CONFIG_PATH) > pathSize)
		return false;

	strcat_s(path, pathSize, CONFIG_PATH);
	return true;
}

bool Config::Exists(const char* path)
{
	if (FILE* file = fopen(path, "r"))
	{
		fclose(file);
		return true;
	}

	return false;
}

bool Config::Load(const char* path, SInt32& ammoPercent, Logger::LogLevel& logLevel)
{
	const auto iniReader = INIReader(path);
	if (FAILED(iniReader.ParseError()))
		return false;

	const auto ammoPercentTemp = iniReader.GetInteger("Config", "Ammo Percent", -1);
	if (ammoPercentTemp < 0 || ammoPercentTemp > AMMO_PERCENT_MAX)
		return false;

	auto logLevelVal = iniReader.GetInteger("Config", "Log Level", -1);
	if (logLevelVal < Logger::Fatal || logLevelVal > Logger::Debug)
		return false;

	ammoPercent = ammoPercentTemp;
	logLevel = static_cast<Logger::LogLevel>(logLevelVal);
	return true;
}

bool Config::GetDefaultPath(char* path, const rsize_t pathSize)
{
	HMODULE hModule;
	if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		reinterpret_cast<LPCSTR>(&GetDefaultPath), &hModule)
		|| !GetModuleFileName(hModule, path, MAX_PATH)
		|| !PathRemoveFileSpec(path))
	{
		return false;
	}

	if (strlen(path) + strlen(DEFAULT_CONFIG_PATH) > pathSize)
		return false;

	strcat_s(path, pathSize, DEFAULT_CONFIG_PATH);
	return true;
}

bool Config::Copy(const char* sourcePath, const char* destPath)
{
	const std::ifstream sourceFile(sourcePath, std::ios::binary);
	if (!sourceFile)
		return false;

	std::ofstream destFile(destPath, std::ios::binary | std::ios::trunc);
	if (!destFile)
		return false;

	destFile << sourceFile.rdbuf();

	if (!destFile)
		return false;

	return true;
}
