#include "TESDeathEventHandler.h"

#include <bitset>

#include "f4se/GameReferences.h"
#include "f4se/GameRTTI.h"

#include "Logger.h"

static constexpr UInt64 LOWER_32_MASK = 0x00000000FFFFFFFF;
static constexpr UInt64 UPPER_32_MASK = 0xFFFFFFFF00000000;
static constexpr size_t CANT_DROP_FLAG_POS = 2;

static constexpr char PREFIX_RE[] = R"(Event   | )";
static constexpr char PREFIX_UA[] = R"(Unload  | )";
static constexpr char PREFIX_RA[] = R"(Reload  | )";
static constexpr char PREFIX_AA[] = R"(AddAmmo | )";

SInt32 TESDeathEventHandler::ammoPercent = 100;

EventResult TESDeathEventHandler::ReceiveEvent(TESDeathEvent* event, void* dispatcher)
{
	if (!event || !event->source)
		return kEvent_Continue;

	const auto actor = DYNAMIC_CAST(event->source, TESObjectREFR, Actor);
	if (!actor)
		return kEvent_Continue;

	AmmoStack ammoData;
	if (!UnloadReloadAmmo(actor, ammoData, true))
		return kEvent_Continue;

	if (ammoPercent > 0 && !AddAmmo(actor, ammoData))
	{
		Logger::Log(Logger::Message, PREFIX_RE, "AddAmmo failed, attempting to reload...");
		UnloadReloadAmmo(actor, ammoData, false);
	}

	return kEvent_Continue;
}

bool TESDeathEventHandler::UnloadReloadAmmo(const Actor* actor, AmmoStack& ammoStack, const bool unload)
{
	const auto prefix = unload ? PREFIX_UA : PREFIX_RA;
	const auto action = unload ? "unload" : "reload";

	Logger::Log(Logger::Verbose, prefix, "Attempting to %s ammo...", action);

	const auto middleProcess = actor->middleProcess;
	if (!middleProcess)
	{
		Logger::Log(Logger::Verbose, prefix, "Failed to get actor data.");
		return false;
	}

	const auto unk08 = middleProcess->unk08;
	if (!unk08)
	{
		Logger::Log(Logger::Verbose, prefix, "Failed to get EquipData array.");
		return false;
	}

	SimpleLocker locker(&unk08->lock);

	const auto equipDataArray = unk08->equipData;
	for (UInt32 i = 0; i < equipDataArray.count; ++i)
	{
		Actor::MiddleProcess::Data08::EquipData equipData;
		if (!equipDataArray.GetNthItem(i, equipData))
		{
			Logger::Log(Logger::Verbose, prefix, "Failed to get EquipData.");
			return false; // If one fails, all the rest will fail
		}

		const auto item = equipData.item;
		if (!item)
		{
			Logger::Log(Logger::Debug, prefix, "Failed to get item.");
			continue;
		}

		if (std::bitset<sizeof(UInt32) * CHAR_BIT>(item->flags).test(CANT_DROP_FLAG_POS))
		{
			Logger::Log(Logger::Verbose, prefix, "%s (%lu) ignored due to 'Can't Drop' flag.",
				item->GetFullName(), item->formID);
			continue;
		}

		const auto equippedData = equipData.equippedData;
		if (!equippedData)
		{
			Logger::Log(Logger::Debug, prefix, "%s (%lu) has no equippedData.",
				item->GetFullName(), item->formID);
			continue;
		}

		const auto ammo = equippedData->ammo;
		if (!ammo)
		{
			Logger::Log(Logger::Debug, prefix, "%s (%lu) has no ammo.",
				item->GetFullName(), item->formID);
			continue;
		}

		// To test the gatling laser: tgm, player.placeatme 769FA, additem 75FE4 1, additem E27BC
		const auto ammoCharge = ammo->data.health;
		if (ammoCharge > 0)
		{
			Logger::Log(Logger::Verbose, prefix, "%s (%lu) ignored due to having a charge (%hu).",
				ammo->GetFullName(), ammo->formID, ammoCharge);
			continue;
		}

		const auto ammoCount = static_cast<SInt32>(equippedData->unk18 & LOWER_32_MASK); // Ammo count is lower 32 bits
		if (ammoCount < 0)
		{
			Logger::Log(Logger::Verbose, prefix, "%s (%lu) ignored due to having negative ammo count (%li).",
				ammo->GetFullName(), ammo->formID, ammoCount);
			continue;
		}

		if (unload)
		{
			if (ammoCount == 0)
			{
				Logger::Log(Logger::Verbose, prefix, "%s (%lu) ignored due to having ammo count of 0.");
				continue; // Sometimes this method gets called twice, this will skip it on subsequent calls
			}

			Logger::Log(Logger::Verbose, prefix, "Unloading %lu %s (%lu).",
				ammoCount, ammo->GetFullName(), ammo->formID);

			// Unload ammo
			equippedData->unk18 &= UPPER_32_MASK; // Clear lower 32 bits

			// Store data
			ammoStack.ammo = ammo;
			ammoStack.count = ammoCount;
			return true;
		}
		else
		{
			if (ammo != ammoStack.ammo)
			{
				Logger::Log(Logger::Debug, prefix, "%s (%lu) skipped due to not matching desired %s (%lu).",
					ammo->GetFullName(), ammo->formID, ammoStack.ammo->GetFullName(), ammoStack.ammo->formID);
				continue;
			}

			if (ammoCount != 0)
			{
				Logger::Log(Logger::Verbose, prefix, "%s (%lu) ignored due to having ammo count other than 0 (%li).",
					ammo->GetFullName(), ammo->formID, ammoCount);
				continue;
			}

			Logger::Log(Logger::Verbose, prefix, "Reloading %li %s (%lu).",
				ammoStack.count, ammo->GetFullName(), ammo->formID);

			// Reload ammo
			equippedData->unk18 += ammoStack.count;
			return true;
		}
	}

	Logger::Log(Logger::Verbose, prefix, "Failed to find equippedData to %s.", action);
	return false;
}

bool TESDeathEventHandler::AddAmmo(const Actor* actor, const AmmoStack ammoStack)
{
	Logger::Log(Logger::Verbose, PREFIX_AA, "Attempting to add ammo...");

	const auto inventoryList = actor->inventoryList;
	if (!inventoryList)
	{
		Logger::Log(Logger::Verbose, PREFIX_AA, "Failed to get inventory.");
		return false;
	}

	BSWriteLocker locker(&inventoryList->inventoryLock);

	const auto inventoryItems = inventoryList->items;
	for (UInt32 j = 0; j < inventoryItems.count; ++j)
	{
		BGSInventoryItem item;
		if (!inventoryItems.GetNthItem(j, item))
		{
			Logger::Log(Logger::Verbose, PREFIX_AA, "Failed to get inventory item.");
			return false; // If one fails, all the rest will fail
		}

		if (item.form == ammoStack.ammo)
		{
			const SInt32 toAdd = ammoStack.count * ammoPercent / 100;

			Logger::Log(Logger::Verbose, PREFIX_AA, "Adding %li (%li%% of %li) %s (%lu) to existing %li.",
				toAdd, ammoPercent, ammoStack.count, ammoStack.ammo->GetFullName(), ammoStack.ammo->formID, item.stack->count);

			item.stack->count += toAdd;
			return true;
		}
	}

	Logger::Log(Logger::Verbose, PREFIX_AA, "Failed to find ammo.");
	return false;
}