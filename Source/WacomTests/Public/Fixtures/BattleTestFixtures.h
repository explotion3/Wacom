// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/StrongObjectPtr.h"

class UBattleSession;
class UCardDefinition;
class UCharacterDefinition;
class UEnemyDefinition;
class UEnemyPartDefinition;
struct FBattleCommand;
struct FBattleSnapshot;
struct FHandCardSnapshot;
enum class EHandZone : uint8;

/**
 * WacomTests 的测试工厂 + 工具集合。
 *
 * 所有对象挂在 transient package。外部测试类通过 FWacomBattleFixture 持有，
 * 保证测试期间不被 GC。
 *
 * 不依赖 Content/ 下的 uasset，测试运行不需要 Content 状态。
 */
class WACOMTESTS_API FWacomBattleFixture
{
public:
	FWacomBattleFixture();
	~FWacomBattleFixture();

	// ---- DataAsset 工厂 ----

	/** 基础伤害卡。Cost + 目标 SingleEnemyPart + Effect.Damage(damage)。 */
	UCardDefinition* MakeSimpleDamageCard(int32 Cost, int32 Damage);

	/** 连击伤害卡。Combo + Damage。 */
	UCardDefinition* MakeComboDamageCard(int32 Cost, int32 Damage);

	/** 无效果空卡（仅占位）。用于纯测试手牌队列。 */
	UCardDefinition* MakeNoopCard(int32 Cost);

	/** 指定另一张手牌为目标的 Cost 修正卡。 */
	UCardDefinition* MakeHandCardCostModifierCard(int32 Cost, int32 Magnitude, bool bReduceCost);

	/** 自定义 Keywords 的伤害卡。 */
	UCardDefinition* MakeDamageCardWithKeywords(int32 Cost, int32 Damage, const TArray<FGameplayTag>& Keywords);

	/**
	 * 构造一个简单"单部位敌人"。
	 * Initiative 用作首意图先机；ResistanceValue 填 IntentResist。
	 * 第一条意图是 Effect.Damage(1) 打玩家，作为占位。
	 */
	UEnemyDefinition* MakeSinglePartEnemy(int32 Hp, int32 Initiative, int32 IntentResist);

	/**
	 * 三部位敌人，先机分别为 [H, B, T]，HP 分别为 [HH, HB, HT]。
	 * 意图都是 Damage(1) 打玩家，抵抗值 0。
	 */
	UEnemyDefinition* MakeThreePartEnemy(int32 HH, int32 HB, int32 HT, int32 IH, int32 IB, int32 IT);

	/**
	 * 构造一个角色。左手/右手/StarterDeck 全传入。
	 * StarterDeck 顺序决定 DrawPile 初始顺序（Initialize 后 DrawPile 尾部 = 抽牌堆顶）。
	 */
	UCharacterDefinition* MakeCharacter(UCardDefinition* LeftHand, UCardDefinition* RightHand,
	                                     const TArray<UCardDefinition*>& StarterDeck);

	// ---- Session 构造 ----

	/** 创建并 Initialize。失败 check-fail。返回的指针由 fixture 持有。 */
	UBattleSession* CreateSession(UCharacterDefinition* Character, UEnemyDefinition* Enemy, int32 Seed);

	// ---- Snapshot 查询 ----

	/** 找 Hand 中所属 CardId == CardDef->CardId 的第一张卡。失败返回无效 FGuid。 */
	static FGuid FindHandInstanceByCardId(const FBattleSnapshot& Snap, FName CardId);

	/** Hand 中某张卡的 index，找不到返回 INDEX_NONE。 */
	static int32 FindHandIndex(const FBattleSnapshot& Snap, const FGuid& InstanceId);

	/** 某 Part 的 CurrentInitiative。找不到返回 INT32_MIN。 */
	static int32 FindPartInitiative(const FBattleSnapshot& Snap, int32 PartIndex);

	/** 某 Part 的 CurrentHp。找不到返回 INT32_MIN。 */
	static int32 FindPartHp(const FBattleSnapshot& Snap, int32 PartIndex);

	/** 某 Part 的 InstanceId，按 EnemyDefinition.Parts 的顺序。 */
	static FGuid FindPartInstanceId(const FBattleSnapshot& Snap, int32 PartIndex);

private:
	// fixture 的每个 UObject 都挂 strong ptr 防 GC。
	TArray<TStrongObjectPtr<UObject>> Roots;

	// session 单独持有，便于测试直接访问
	TStrongObjectPtr<UBattleSession> SessionPtr;
};
