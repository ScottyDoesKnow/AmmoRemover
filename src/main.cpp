#include "f4se/PluginAPI.h"
#include "f4se_common/f4se_version.h"

#include "ArConfig.h"
#include "ArLogger.h"
#include "TESDeathEventHandler.h"
#include "version.h"

bool onInitRan = false;

void OnInit(F4SEMessagingInterface::Message* message)
{
	if (onInitRan)
		return;

	ArLogger::Log(ArLogger::Message, "Attempting to initialize plugin...");

	// https://github.com/Neanka/f4se/blob/master/f4se/f4seee/main.cpp
	if (message->type != F4SEMessagingInterface::kMessage_GameDataReady)
	{
		ArLogger::Log(ArLogger::Message, "Game data is not ready yet.\n");
		return;
	}

	onInitRan = true;

	ArLogger::Log(ArLogger::Message, "Initializing plugin...");
	GetEventDispatcher<TESDeathEvent>()->AddEventSink(TESDeathEventHandler::GetSingleton());
	ArLogger::Log(ArLogger::Message, "Plugin initialized successfully.\n");
}

extern "C"
{
	__declspec(dllexport) F4SEPluginVersionData F4SEPlugin_Version =
	{
		F4SEPluginVersionData::kVersion,
		AR_VERSION_INT,
		"AmmoRemover",
		"ScottyDoesKnow",
		0,                            // Not version independent (addresses)
		0,                            // Not version independent (structs)
		{ RUNTIME_COMPATIBILITY, 0 }, // Compatibility
		0,                            // Works with any version of the script extender. You probably do not need to put anything here.
	};

	__declspec(dllexport) bool F4SEPlugin_Load(const F4SEInterface* f4se)
	{
		SInt32 wepAmmoPercent, invAmmoPercent;
		ArLogger::LogLevel logLevel;
		const ArConfig::InitializeResult configResult = ArConfig::Initialize(wepAmmoPercent, invAmmoPercent, logLevel);

		if (configResult != ArConfig::Failed)
		{
			TESDeathEventHandler::wepAmmoPercent = wepAmmoPercent;
			TESDeathEventHandler::invAmmoPercent = invAmmoPercent;
			ArLogger::logLevel = logLevel;
		}

		ArLogger::Initialize();
		ArLogger::Log(ArLogger::Message, "Attempting to load v%s...", AR_VERSION_STR);

		switch (configResult)
		{
		case ArConfig::Failed:
			ArLogger::Log(ArLogger::Warning, "Failed to load any configuration, defaulting to %li%% Weapon and %li%% Inventory Ammo.",
				TESDeathEventHandler::wepAmmoPercent, TESDeathEventHandler::invAmmoPercent);
			break;
		case ArConfig::ConfigFile:
			ArLogger::Log(ArLogger::Message, "Loaded %li%% Weapon and %li%% Inventory Ammo.",
				TESDeathEventHandler::wepAmmoPercent, TESDeathEventHandler::invAmmoPercent);
			break;
		case ArConfig::Defaults:
			ArLogger::Log(ArLogger::Warning, "Failed to load %sAmmoRemover.ini, loaded %li%% Weapon and %li%% Inventory Ammo from AmmoRemoverDefaults.ini",
				TESDeathEventHandler::wepAmmoPercent == 0 ? (invAmmoPercent == 0 ? "TrueTrue" : "True") : "",
				TESDeathEventHandler::wepAmmoPercent, TESDeathEventHandler::invAmmoPercent);
			break;
		}

		if (f4se->isEditor)
		{
			ArLogger::Log(ArLogger::Fatal, "Unable to load in editor.\n");
			return false;
		}

		// https://github.com/Neanka/f4se/blob/master/f4se/f4seee/main.cpp
		const PluginHandle handle = f4se->GetPluginHandle();
		const auto messagingInterface = static_cast<F4SEMessagingInterface*>(f4se->QueryInterface(kInterface_Messaging));
		if (!messagingInterface->RegisterListener(handle, "F4SE", OnInit))
		{
			ArLogger::Log(ArLogger::Fatal, "Failed to register initialization listener.\n");
			return false;
		}

		ArLogger::Log(ArLogger::Message, "Plugin loaded successfully.\n");
		return true;
	}
};
