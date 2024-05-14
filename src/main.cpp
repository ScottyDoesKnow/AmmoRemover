#include "common/IDebugLog.h"
#include "f4se_common/f4se_version.h" // RUNTIME_VERSION
#include "f4se/GameEvents.h"          // TESDeathEvent
#include "f4se/GameObjects.h"         // TESAmmo
#include "f4se/GameReferences.h"      // Actor
#include "f4se/GameRTTI.h"            // DYNAMIC_CAST
#include "f4se/GameTypes.h"           // SimpleLock
#include "f4se/PluginAPI.h"           // F4SEInterface, PluginInfo
#include "f4se/PluginManager.h"

#include <bitset>
#include <ShlObj.h> // CSIDL_MYDOCUMENTS

#include "version.h" // VERSION_VERSTRING, VERSION_MAJOR

// These are needed!
#include "f4se/GameAPI.cpp"   // Heap_Allocate
#include "f4se/GameRTTI.cpp"  // DYNAMIC_CAST
#include "f4se/GameTypes.cpp" // SimpleLock

#define DEBUG 0
#define TRUE_AMMO_REMOVER 0

boolean onInitRan = false;

// https://github.com/powerof3/SKSEPlugins/blob/master/po3_FEC/main.cpp
class TESDeathEventHandler : public BSTEventSink<TESDeathEvent> {
public:
	// https://stackoverflow.com/a/1008289
	TESDeathEventHandler(TESDeathEventHandler const&) = delete;
	void operator=(TESDeathEventHandler const&) = delete;

	static TESDeathEventHandler* GetSingleton()
	{
		static TESDeathEventHandler singleton;
		return &singleton;
	}

	EventResult ReceiveEvent(TESDeathEvent* evn, void* dispatcher) override
	{
		if (!evn || !evn->source)
			return kEvent_Continue;

		const auto actor = DYNAMIC_CAST(evn->source, TESObjectREFR, Actor);
		if (!actor)
			return kEvent_Continue;

		AmmoStack ammoData;
		if (!LoadUnloadAmmo(actor, ammoData, false))
			return kEvent_Continue;

#if TRUE_AMMO_REMOVER != 1
		if (!AddAmmo(actor, ammoData))
		{
#if DEBUG != 0
			_MESSAGE("[DEBUG] ReceiveEvent AddAmmo failed.");
#endif
			LoadUnloadAmmo(actor, ammoData, true);
		}
#endif

		return kEvent_Continue;
	}

private:
	TESDeathEventHandler() {}

	static constexpr UInt64 LOWER_32_MASK = 0x00000000FFFFFFFF;
	static constexpr UInt64 UPPER_32_MASK = 0xFFFFFFFF00000000;
	static constexpr size_t CANT_DROP_FLAG_POS = 2;

	struct AmmoStack
	{
		TESAmmo* ammo;
		UInt32	ammoCount;
	};

	static bool LoadUnloadAmmo(const Actor* actor, AmmoStack& ammoStack, const bool load)
	{
#if DEBUG != 0
		_MESSAGE("[DEBUG] LoadUnloadAmmo --------------------------------");
#endif

		const auto middleProcess = actor->middleProcess;
		if (!middleProcess)
			return false;

		const auto unk08 = middleProcess->unk08;
		if (!unk08)
			return false;

		SimpleLocker locker(&unk08->lock);

		const auto equipDataArray = unk08->equipData;
		for (UInt32 i = 0; i < equipDataArray.count; ++i)
		{
			Actor::MiddleProcess::Data08::EquipData equipData;
			if (!equipDataArray.GetNthItem(i, equipData))
				return false; // If one fails, all the rest will fail

			const auto item = equipData.item;
			if (!item)
				continue;

			if (std::bitset<sizeof(UInt32) * CHAR_BIT>(item->flags).test(CANT_DROP_FLAG_POS))
			{
#if DEBUG != 0
				_MESSAGE("[DEBUG] LoadUnloadAmmo ignored due to Can't Drop flag.");
#endif
				continue; // Ignore weapons marked as Can't Drop
			}

			const auto equippedData = equipData.equippedData;
			if (!equippedData)
				continue;

			const auto ammo = equippedData->ammo;
			if (!ammo)
				continue;

			// To test the gatling laser: tgm, player.placeatme 769FA, additem 75FE4 1, additem E27BC
			const auto ammoCharge = ammo->data.health;
			if (ammoCharge > 0)
			{
#if DEBUG != 0
				_MESSAGE("[DEBUG] LoadUnloadAmmo ammoCharge: %hu", ammoCharge);
				_MESSAGE("[DEBUG] LoadUnloadAmmo ignored ammo with charge.");
#endif
				continue; // Ignore ammo with charge
			}

			const auto ammoCount = static_cast<UInt32>(equippedData->unk18 & LOWER_32_MASK); // Ammo count is lower 32 bits
			if (load)
			{
				if (ammoCount != 0)
					continue; // Don't load ammo if ammo isn't empty

				if (ammo != ammoStack.ammo)
					continue;

				// Load ammo
				equippedData->unk18 += ammoStack.ammoCount;
			}
			else
			{
				if (ammoCount == 0)
					continue; // Sometimes this method gets called twice, this will skip it on subsequent calls

#if DEBUG != 0
				_MESSAGE("[DEBUG] LoadUnloadAmmo ammo: %s", ammo->GetFullName());
				_MESSAGE("[DEBUG] LoadUnloadAmmo ammoCount: %lu", ammoCount);
				_MESSAGE("[DEBUG] LoadUnloadAmmo formID: %lu", ammo->formID);
#endif

				// Unload ammo
				equippedData->unk18 &= UPPER_32_MASK; // Clear lower 32 bits

				// Store data
				ammoStack.ammo = ammo;
				ammoStack.ammoCount = ammoCount;
			}

			// Stop searching
			return true;
		}

		return false;
	}

	static bool AddAmmo(const Actor* actor, const AmmoStack ammoData)
	{
		const auto inventoryList = actor->inventoryList;
		if (!inventoryList)
			return false;

		BSWriteLocker locker(&inventoryList->inventoryLock);

		const auto inventoryItems = inventoryList->items;
		for (UInt32 j = 0; j < inventoryItems.count; ++j)
		{

			BGSInventoryItem item;
			if (!inventoryItems.GetNthItem(j, item))
				return false; // If one fails, all the rest will fail

			if (item.form == ammoData.ammo)
			{
				// Add to count
				item.stack->count += ammoData.ammoCount;

				// Stop searching
				return true;
			}
		}

		return false;
	}
};

void OnInit(F4SEMessagingInterface::Message* message)
{
	if (onInitRan)
		return;

	// https://github.com/Neanka/f4se/blob/master/f4se/f4seee/main.cpp
	if (message->type != F4SEMessagingInterface::kMessage_GameDataReady)
	{
		_MESSAGE("[MESSAGE] OnInit not GameDataReady.");
		return;
	}

	onInitRan = true;

	_MESSAGE("[MESSAGE] OnInit started.");
	GetEventDispatcher<TESDeathEvent>()->AddEventSink(TESDeathEventHandler::GetSingleton());
	_MESSAGE("[MESSAGE] OnInit finished.");
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

	bool F4SEPlugin_Load(const F4SEInterface* f4se)
	{
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, R"(\My Games\Fallout4\F4SE\AmmoRemover.log)");
		gLog.SetPrintLevel(IDebugLog::kLevel_DebugMessage);
		gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

		_MESSAGE("[MESSAGE] v%s", AMMO_REMOVER_VERSION);

		if (f4se->isEditor)
		{
			_FATALERROR("[FATAL ERROR] Loaded in editor, marking as incompatible!\n");
			return false;
		}

		// https://github.com/Neanka/f4se/blob/master/f4se/f4seee/main.cpp
		const PluginHandle handle = f4se->GetPluginHandle();
		const auto messagingInterface = static_cast<F4SEMessagingInterface*>(f4se->QueryInterface(kInterface_Messaging));
		if (!messagingInterface->RegisterListener(handle, "F4SE", OnInit))
			return false;

		_MESSAGE("[MESSAGE] Loaded.");

		return true;
	}
};
