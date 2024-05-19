#pragma once

#include "f4se/GameObjects.h"

// https://github.com/powerof3/SKSEPlugins/blob/master/po3_FEC/main.cpp
class TESDeathEventHandler final : public BSTEventSink<TESDeathEvent> {
public:
	// https://codereview.stackexchange.com/a/173935
	TESDeathEventHandler(const TESDeathEventHandler&) = delete;
	TESDeathEventHandler& operator=(const TESDeathEventHandler&) = delete;
	TESDeathEventHandler(TESDeathEventHandler&&) = delete;
	TESDeathEventHandler& operator=(TESDeathEventHandler&&) = delete;
	~TESDeathEventHandler() override = default;

	static SInt32 wepAmmoPercent;
	static SInt32 invAmmoPercent;

	static TESDeathEventHandler* GetSingleton()
	{
		static TESDeathEventHandler singleton;
		return &singleton;
	}

	EventResult ReceiveEvent(TESDeathEvent* event, void* dispatcher) override;

protected:
	struct AmmoStack
	{
		TESAmmo* ammo;
		SInt32 count;
	};

private:
	TESDeathEventHandler() {}

	static bool UnloadReloadAmmo(const Actor* actor, AmmoStack& ammoStack, bool unload);
	static bool SetAmmo(TESObjectREFR* source, const Actor* actor, const AmmoStack ammoStack);
};
