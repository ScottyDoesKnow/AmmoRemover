#include "TESDeathEventHandler.h"

#include <bitset>

#include "f4se/GameReferences.h"
#include "f4se/GameRTTI.h"
#include "f4se/PapyrusEvents.h"
#include "f4se/PapyrusUtilities.h"

#include "ArLogger.h"

static constexpr UInt64 LOWER_32_MASK = 0x00000000FFFFFFFF;
static constexpr UInt64 UPPER_32_MASK = 0xFFFFFFFF00000000;
static constexpr size_t CANT_DROP_FLAG_POS = 2;

static constexpr char PREFIX_RE[] = R"(Event   | )";
static constexpr char PREFIX_UA[] = R"(Unload  | )";
static constexpr char PREFIX_RA[] = R"(Reload  | )";
static constexpr char PREFIX_SA[] = R"(SetAmmo | )";

SInt32 TESDeathEventHandler::wepAmmoPercent = 100;
SInt32 TESDeathEventHandler::invAmmoPercent = 100;

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

	if ((wepAmmoPercent > 0 || invAmmoPercent != 100) && !SetAmmo(event->source, actor, ammoData))
	{
		ArLogger::LogP(ArLogger::Message, PREFIX_RE, "SetAmmo failed, attempting to reload...");
		UnloadReloadAmmo(actor, ammoData, false);
	}

	return kEvent_Continue;
}

bool TESDeathEventHandler::UnloadReloadAmmo(const Actor* actor, AmmoStack& ammoStack, const bool unload)
{
	const auto prefix = unload ? PREFIX_UA : PREFIX_RA;
	const auto action = unload ? "unload" : "reload";

	ArLogger::LogP(ArLogger::Verbose, prefix, "Attempting to %s ammo...", action);

	const auto middleProcess = actor->middleProcess;
	if (!middleProcess)
	{
		ArLogger::LogP(ArLogger::Verbose, prefix, "Failed to get actor data.");
		return false;
	}

	const auto unk08 = middleProcess->unk08;
	if (!unk08)
	{
		ArLogger::LogP(ArLogger::Verbose, prefix, "Failed to get EquipData array.");
		return false;
	}

	SimpleLocker locker(&unk08->lock);

	const auto equipDatas = unk08->equipData;
	for (UInt32 i = 0; i < equipDatas.count; ++i)
	{
		Actor::AIProcess::Data08::EquipData equipData;
		if (!equipDatas.GetNthItem(i, equipData))
		{
			ArLogger::LogP(ArLogger::Verbose, prefix, "Failed to get EquipData.");
			return false; // If one fails, all the rest will fail
		}

		const auto item = equipData.item;
		if (!item)
		{
			ArLogger::LogP(ArLogger::Debug, prefix, "Failed to get item.");
			continue;
		}

		if (std::bitset<sizeof(UInt32) * CHAR_BIT>(item->flags).test(CANT_DROP_FLAG_POS))
		{
			ArLogger::LogP(ArLogger::Verbose, prefix, "%s (%lu) ignored due to 'Can't Drop' flag.",
				item->GetFullName(), item->formID);
			continue;
		}

		const auto equippedData = equipData.equippedData;
		if (!equippedData)
		{
			ArLogger::LogP(ArLogger::Debug, prefix, "%s (%lu) has no equippedData.",
				item->GetFullName(), item->formID);
			continue;
		}

		const auto ammo = equippedData->ammo;
		if (!ammo)
		{
			ArLogger::LogP(ArLogger::Debug, prefix, "%s (%lu) has no ammo.",
				item->GetFullName(), item->formID);
			continue;
		}

		// To test the gatling laser: tgm, player.placeatme 769FA, additem 75FE4 1, additem E27BC
		const auto ammoCharge = ammo->data.health;
		if (ammoCharge > 0)
		{
			ArLogger::LogP(ArLogger::Verbose, prefix, "%s (%lu) ignored due to having a charge (%hu).",
				ammo->GetFullName(), ammo->formID, ammoCharge);
			continue;
		}

		const auto ammoCount = static_cast<SInt32>(equippedData->unk18 & LOWER_32_MASK); // Ammo count is lower 32 bits
		if (ammoCount < 0)
		{
			ArLogger::LogP(ArLogger::Verbose, prefix, "%s (%lu) ignored due to having negative ammo count (%li).",
				ammo->GetFullName(), ammo->formID, ammoCount);
			continue;
		}

		if (unload)
		{
			if (ammoCount == 0)
			{
				ArLogger::LogP(ArLogger::Verbose, prefix, "%s (%lu) ignored due to having ammo count of 0.");
				continue; // Sometimes this method gets called twice, this will skip it on subsequent calls
			}

			ArLogger::LogP(ArLogger::Verbose, prefix, "Unloading %lu %s (%lu).",
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
				ArLogger::LogP(ArLogger::Debug, prefix, "%s (%lu) skipped due to not matching desired %s (%lu).",
					ammo->GetFullName(), ammo->formID, ammoStack.ammo->GetFullName(), ammoStack.ammo->formID);
				continue;
			}

			if (ammoCount != 0)
			{
				ArLogger::LogP(ArLogger::Verbose, prefix, "%s (%lu) ignored due to having ammo count other than 0 (%li).",
					ammo->GetFullName(), ammo->formID, ammoCount);
				continue;
			}

			ArLogger::LogP(ArLogger::Verbose, prefix, "Reloading %li %s (%lu).",
				ammoStack.count, ammo->GetFullName(), ammo->formID);

			// Reload ammo
			equippedData->unk18 += ammoStack.count;
			return true;
		}
	}

	ArLogger::LogP(ArLogger::Verbose, prefix, "Failed to find equippedData to %s.", action);
	return false;
}

bool TESDeathEventHandler::SetAmmo(TESObjectREFR* source, const Actor* actor, const AmmoStack ammoStack)
{
	ArLogger::LogP(ArLogger::Verbose, PREFIX_SA, "Attempting to set ammo...");

	const auto inventory = actor->inventoryList;
	if (!inventory)
	{
		ArLogger::LogP(ArLogger::Verbose, PREFIX_SA, "Failed to get inventory.");
		return false;
	}

	BSWriteLocker locker(&inventory->inventoryLock);

	auto items = inventory->items;
	for (UInt32 i = 0; i < items.count; ++i)
	{
		BGSInventoryItem item;
		if (!items.GetNthItem(i, item))
		{
			ArLogger::LogP(ArLogger::Verbose, PREFIX_SA, "Failed to get inventory item.");
			return false; // If one fails, all the rest will fail
		}

		if (item.form == ammoStack.ammo)
		{
			const SInt32 wepAmmo = ammoStack.count * wepAmmoPercent / 100;
			const SInt32 invAmmo = item.stack->count * invAmmoPercent / 100;
			const SInt32 total = wepAmmo + invAmmo;

			ArLogger::LogP(ArLogger::Verbose, PREFIX_SA, "Setting %s (%lu) to %li. %li (%li%% of %li) from weapon and %li (%li%% of %li) from inventory.",
				ammoStack.ammo->GetFullName(), ammoStack.ammo->formID, total,
				wepAmmo, wepAmmoPercent, ammoStack.count,
				invAmmo, invAmmoPercent, item.stack->count);

			if (total > 0)
				item.stack->count = total;
			else
			{
				const auto handle = PapyrusVM::GetHandleFromObject(static_cast<void*>(source), TESObjectREFR::kTypeID);
				bool silent = true;
				TESObjectREFR* target = nullptr;
				SendPapyrusEvent4<TESForm*, SInt32, bool, TESObjectREFR*>(handle, "ObjectReference", "RemoveItem", item.form, item.stack->count, silent, target);
			}

			return true;
		}
	}

	ArLogger::LogP(ArLogger::Verbose, PREFIX_SA, "Failed to find ammo.");
	return false;
}