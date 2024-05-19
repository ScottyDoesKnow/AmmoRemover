#include "ArConfig.h"

#include <fstream>
#include <ShlObj_core.h>
#include <Shlwapi.h>

#include "INIReader.h"

static constexpr char DEFAULT_CONFIG_PATH[] = R"(\AmmoRemoverDefaults.ini)";
static constexpr char CONFIG_PATH[] = R"(\My Games\Fallout4\F4SE\AmmoRemover.ini)";
static constexpr char CONFIG_PATH_TRUE[] = R"(\My Games\Fallout4\F4SE\TrueAmmoRemover.ini)";
static constexpr char CONFIG_PATH_TRUE2[] = R"(\My Games\Fallout4\F4SE\TrueTrueAmmoRemover.ini)";

static constexpr SInt32 AMMO_PERCENT_MAX = 1000;

ArConfig::InitializeResult ArConfig::Initialize(SInt32& wepAmmoPercent, SInt32& invAmmoPercent, ArLogger::LogLevel& logLevel)
{
	char defaultConfigPath[MAX_PATH];
	if (!GetLocalPath(defaultConfigPath, MAX_PATH, DEFAULT_CONFIG_PATH)
		|| !Load(defaultConfigPath, wepAmmoPercent, invAmmoPercent, logLevel))
	{
		return Failed;
	}

	const auto relativePath = wepAmmoPercent == 0 ? (invAmmoPercent == 0 ? CONFIG_PATH_TRUE2 : CONFIG_PATH_TRUE) : CONFIG_PATH;

	char configPath[MAX_PATH];
	if (!GetDocumentsPath(configPath, MAX_PATH, relativePath))
		return Defaults;

	if (Exists(configPath) && Load(configPath, wepAmmoPercent, invAmmoPercent, logLevel))
		return ConfigFile;

	if (Copy(defaultConfigPath, configPath) && Load(configPath, wepAmmoPercent, invAmmoPercent, logLevel))
		return ConfigFile;

	if (Load(defaultConfigPath, wepAmmoPercent, invAmmoPercent, logLevel))
		return Defaults;

	return Failed;
}

bool ArConfig::GetDocumentsPath(char* path, const rsize_t pathSize, const char* relativePath)
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

bool ArConfig::Exists(const char* path)
{
	if (FILE* file = fopen(path, "r"))
	{
		fclose(file);
		return true;
	}

	return false;
}

bool ArConfig::Load(const char* path, SInt32& wepAmmoPercent, SInt32& invAmmoPercent, ArLogger::LogLevel& logLevel)
{
	const auto iniReader = INIReader(path);
	if (FAILED(iniReader.ParseError()))
		return false;

	const auto wepVal = iniReader.GetInteger("Config", "Weapon Ammo %", -1);
	if (wepVal < 0 || wepVal > AMMO_PERCENT_MAX)
		return false;

	const auto invVal = iniReader.GetInteger("Config", "Inventory Ammo %", -1);
	if (invVal < 0 || invVal > AMMO_PERCENT_MAX)
		return false;

	auto logVal = iniReader.GetInteger("Config", "Log Level", -1);
	if (logVal < ArLogger::Fatal || logVal > ArLogger::Debug)
		return false;

	wepAmmoPercent = wepVal;
	invAmmoPercent = invVal;
	logLevel = static_cast<ArLogger::LogLevel>(logVal);
	return true;
}

bool ArConfig::GetLocalPath(char* path, const rsize_t pathSize, const char* relativePath)
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

bool ArConfig::Copy(const char* sourcePath, const char* destPath)
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
