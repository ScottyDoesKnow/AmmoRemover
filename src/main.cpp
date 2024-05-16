#include "f4se/PluginAPI.h"
#include "f4se_common/f4se_version.h"

#include "Logger.h"
#include "Config.h"
#include "TESDeathEventHandler.h"
#include "version.h"

bool onInitRan = false;

void OnInit(F4SEMessagingInterface::Message* message)
{
	if (onInitRan)
		return;

	Logger::Log(Logger::Message, "Attempting to initialize plugin...");

	// https://github.com/Neanka/f4se/blob/master/f4se/f4seee/main.cpp
	if (message->type != F4SEMessagingInterface::kMessage_GameDataReady)
	{
		Logger::Log(Logger::Message, "Game data is not ready yet.\n");
		return;
	}

	onInitRan = true;

	Logger::Log(Logger::Message, "Initializing plugin...");
	GetEventDispatcher<TESDeathEvent>()->AddEventSink(TESDeathEventHandler::GetSingleton());
	Logger::Log(Logger::Message, "Plugin initialized successfully.\n");
}

extern "C"
{
	__declspec(dllexport) F4SEPluginVersionData F4SEPlugin_Version =
	{
		F4SEPluginVersionData::kVersion,
		AMMO_REMOVER_VERSION_INT,
		"AmmoRemover",
		"ScottyDoesKnow",
		0,                            // Not version independent (addresses)
		0,                            // Not version independent (structs)
		{ RUNTIME_COMPATIBILITY, 0 }, // Compatibility
		0,                            // Works with any version of the script extender. You probably do not need to put anything here.
	};

	__declspec(dllexport) bool F4SEPlugin_Load(const F4SEInterface* f4se)
	{
		SInt32 ammoPercent;
		Logger::LogLevel logLevel;
		const bool configInitialized = Config::Initialize(ammoPercent, logLevel);

		if (configInitialized)
		{
			TESDeathEventHandler::ammoPercent = ammoPercent;
			Logger::logLevel = logLevel;
		}

		Logger::Initialize();
		Logger::Log(Logger::Message, "Attempting to load v%s...", AMMO_REMOVER_VERSION);

		if (configInitialized)
			Logger::Log(Logger::Message, "Loaded %li%% ammo setting.", ammoPercent);
		else
			Logger::Log(Logger::Message, "Failed to load configuration, defaulting to %li%% ammo.", TESDeathEventHandler::ammoPercent);

		if (f4se->isEditor)
		{
			Logger::Log(Logger::Fatal, "Unable to load in editor.\n");
			return false;
		}

		// https://github.com/Neanka/f4se/blob/master/f4se/f4seee/main.cpp
		const PluginHandle handle = f4se->GetPluginHandle();
		const auto messagingInterface = static_cast<F4SEMessagingInterface*>(f4se->QueryInterface(kInterface_Messaging));
		if (!messagingInterface->RegisterListener(handle, "F4SE", OnInit))
		{
			Logger::Log(Logger::Fatal, "Failed to register initialization listener.\n");
			return false;
		}

		Logger::Log(Logger::Message, "Plugin loaded successfully.\n");
		return true;
	}
};
