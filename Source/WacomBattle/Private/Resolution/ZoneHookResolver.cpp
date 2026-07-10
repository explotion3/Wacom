// Copyright Wacom. All Rights Reserved.

#include "Resolution/ZoneHookResolver.h"
#include "Effects/CardEffectDispatcher.h"

#include "Core/BattleState.h"
#include "Hand/HandZoneService.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Cards/CardZoneHook.h"

namespace
{
	/**
	 * 把 ZoneHook.Zone tag 转成 EHandZone。不支持的返回 EHandZone::None。
	 */
	EHandZone HookZoneTagToEnum(const FGameplayTag& Tag)
	{
		if (Tag == WacomTags::HandZone_Left)  { return EHandZone::Left;  }
		if (Tag == WacomTags::HandZone_Both)  { return EHandZone::Both;  }
		if (Tag == WacomTags::HandZone_Right) { return EHandZone::Right; }
		return EHandZone::None;
	}
}

void FZoneHookResolver::RunOnPlayHooks(
	FBattleState& State,
	FBattleEventBus& Events,
	const UCardDefinition& Def,
	int32 RuntimeCost,
	const FGuid& SelectedPartId,
	const FGuid& CardId,
	IBattleOperationAdapter* OperationAdapter)
{
	const EHandZone CurrentZone = FHandZoneService::GetZoneOf(State, CardId);
	if (CurrentZone == EHandZone::None) { return; }

	// 所有 OnPlay Hook 共享一条效果链的 LastShuffledCardId：
	// 让 ReduceCost/AddCost 能引用紧邻上一条 Shuffle 的产物。
	FGuid LastShuffledCardId;
	for (const FCardZoneHook& Hook : Def.ZoneHooks)
	{
		if (Hook.Trigger != WacomTags::ZoneHook_Trigger_OnPlay) { continue; }
		if (HookZoneTagToEnum(Hook.Zone) != CurrentZone)            { continue; }

		for (const FCardEffect& Eff : Hook.ExtraEffects)
		{
			FCardEffectDispatcher::Execute(State, Events, Eff, RuntimeCost,
				SelectedPartId, CardId, LastShuffledCardId, FGuid(), OperationAdapter);
		}
	}
}

bool FZoneHookResolver::ShouldSkipInitiativePush(
	const FBattleState& State,
	const UCardDefinition& Def,
	const FGuid& CardId,
	bool bHasInitiativeHit)
{
	if (!bHasInitiativeHit) { return false; }

	const EHandZone CurrentZone = FHandZoneService::GetZoneOf(State, CardId);
	if (CurrentZone == EHandZone::None) { return false; }

	for (const FCardZoneHook& Hook : Def.ZoneHooks)
	{
		if (Hook.Trigger != WacomTags::ZoneHook_Trigger_OnPerfectReleaseHit) { continue; }
		if (HookZoneTagToEnum(Hook.Zone) != CurrentZone)                         { continue; }
		return true;
	}
	return false;
}


