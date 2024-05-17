#include "Config.h"

#include <fstream>
#include <ShlObj_core.h>
#include <Shlwapi.h>

#include "INIReader.h"

static constexpr char DEFAULT_CONFIG_PATH[] = R"(\AmmoRemoverDefaults.ini)";
static constexpr char CONFIG_PATH[] = R"(\My Games\Fallout4\F4SE\AmmoRemover.ini)";
static constexpr char CONFIG_PATH_TRUE[] = R"(\My Games\Fallout4\F4SE\TrueAmmoRemover.ini)";

static constexpr SInt32 AMMO_PERCENT_MAX = 1000;

Config::InitializeResult Config::Initialize(SInt32& ammoPercent, Logger::LogLevel& logLevel)
{
	char defaultConfigPath[MAX_PATH];
	if (!GetLocalPath(defaultConfigPath, MAX_PATH, DEFAULT_CONFIG_PATH) || !Load(defaultConfigPath, ammoPercent, logLevel))
		return Failed;

	char configPath[MAX_PATH];
	if (!GetDocumentsPath(configPath, MAX_PATH, ammoPercent == 0 ? CONFIG_PATH_TRUE : CONFIG_PATH))
		return Defaults;

	if (Exists(configPath) && Load(configPath, ammoPercent, logLevel))
		return ConfigFile;

	if (Copy(defaultConfigPath, configPath) && Load(configPath, ammoPercent, logLevel))
		return ConfigFile;

	if (Load(defaultConfigPath, ammoPercent, logLevel))
		return Defaults;

	return Failed;
}

bool Config::GetDocumentsPath(char* path, const rsize_t pathSize, const char* relativePath)
{
	const HRESULT error = SHGetFolderPath(nullptr, CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE,
		nullptr, SHGFP_TYPE_CURRENT, path);
	if (FAILED(error))
		return false;

	if (strlen(path) + strlen(relativePath) > pathSize)
		return false;

	strcat_s(path, pathSize, relativePath);
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

	const auto ammoPercentVal = iniReader.GetInteger("Config", "Ammo Percent", -1);
	if (ammoPercentVal < 0 || ammoPercentVal > AMMO_PERCENT_MAX)
		return false;

	auto logLevelVal = iniReader.GetInteger("Config", "Log Level", -1);
	if (logLevelVal < Logger::Fatal || logLevelVal > Logger::Debug)
		return false;

	ammoPercent = ammoPercentVal;
	logLevel = static_cast<Logger::LogLevel>(logLevelVal);
	return true;
}

bool Config::GetLocalPath(char* path, const rsize_t pathSize, const char* relativePath)
{
	HMODULE hModule;
	if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		reinterpret_cast<LPCSTR>(&GetLocalPath), &hModule)
		|| !GetModuleFileName(hModule, path, MAX_PATH)
		|| !PathRemoveFileSpec(path))
	{
		return false;
	}

	if (strlen(path) + strlen(relativePath) > pathSize)
		return false;

	strcat_s(path, pathSize, relativePath);
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
