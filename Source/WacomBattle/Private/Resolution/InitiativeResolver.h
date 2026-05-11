// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FBattleEventBus;
class UCardDefinition;

/**
 * 先机命中 / 抵抗 / 完美释放。
 *
 * 对齐 Battle_Rules §9 + Data_Schema_Draft §4。
 *
 * 典型调用顺序（PlayCardResolver::Resolve 中）：
 *   1. SnapshotInitiativeBeforePlay  — 在主效果之前记录各部位先机
 *   2. CollectInitiativeHits         — 按 RuntimeCost 找命中部位
 *   3. ResolveResistance             — 先于完美释放判定晕厥
 *   4. （主效果在这两步之间调用）
 *   5. ResolvePerfectRelease         — 主效果之后、先机推进之前
 */
class FInitiativeResolver
{
public:
	struct FPreCastEntry
	{
		FGuid PartInstanceId;
		int32 InitiativeBeforePlay = 0;
	};

	/** 记录所有存活部位的出牌前当前先机。 */
	static void SnapshotInitiativeBeforePlay(const FBattleState& State, TArray<FPreCastEntry>& Out);

	/**
	 * 按 Battle_Rules §9 判断先机命中：
	 * RuntimeCost 正好等于某部位出牌前当前先机即命中。多个部位可同时命中。
	 */
	static void CollectInitiativeHits(
		const TArray<FPreCastEntry>& PreCast,
		int32 RuntimeCost,
		TArray<FGuid>& OutHitPartIds);

	/**
	 * 对命中部位执行抵抗判定。CardResistance > IntentResistance → 部位晕厥。
	 * 晕厥用 Status.Stunned 层数模型记录，第一阶段每次施加 1 层。
	 */
	static void ResolveResistance(
		FBattleState& State,
		FBattleEventBus& Events,
		const UCardDefinition& Def,
		int32 RuntimeCost,
		const TArray<FGuid>& HitPartIds,
		const FGuid& CardId);

	/**
	 * 对命中部位执行完美释放效果。
	 * Battle_Rules §9：迅捷卡不触发；主效果致死的部位不参与。
	 */
	static void ResolvePerfectRelease(
		FBattleState& State,
		FBattleEventBus& Events,
		const UCardDefinition& Def,
		int32 RuntimeCost,
		const TArray<FGuid>& HitPartIds,
		const FGuid& CardId,
		bool bSwift);
};
