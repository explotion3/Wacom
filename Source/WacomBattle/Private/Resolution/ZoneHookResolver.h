// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FBattleEventBus;
class UCardDefinition;

/**
 * ZoneHook 消费。
 *
 * 当前支持两种 Trigger：
 * - ZoneHook.Trigger.OnPlay              — 卡在指定区域打出时执行 ExtraEffects
 * - ZoneHook.Trigger.OnPerfectReleaseHit — 卡在指定区域 + 存在先机命中时，跳过先机推进
 *
 * 新增 Trigger 时在此类扩展。
 */
class FZoneHookResolver
{
public:
	/**
	 * 执行本卡 OnPlay 的 ZoneHook ExtraEffects。
	 *
	 * 触发时机：CardPlayed 事件发射之后、"记录出牌前先机"之前。
	 * LastShuffledCardId 在所有 ExtraEffects 之间共享。
	 */
	static void RunOnPlayHooks(
		FBattleState& State,
		FBattleEventBus& Events,
		const UCardDefinition& Def,
		int32 RuntimeCost,
		const FGuid& SelectedPartId,
		const FGuid& CardId);

	/**
	 * 判断本卡在当前区域 + 存在先机命中时，是否需要"跳过先机推进"。
	 *
	 * 当前规则（朝光暮蝶左手区）：命中即跳过，不执行 ExtraEffects。
	 */
	static bool ShouldSkipInitiativePush(
		const FBattleState& State,
		const UCardDefinition& Def,
		const FGuid& CardId,
		bool bHasInitiativeHit);
};
