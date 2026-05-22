// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Commands/BattleCommand.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Cards/CardZoneHook.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

/**
 * ZoneHook 消费回归测试。
 *
 * 五条测试覆盖：
 *   - Wacom.Battle.ZoneHook.LeftHitSkipsInitiativePush   左手区 + 先机命中 → 不推进先机
 *   - Wacom.Battle.ZoneHook.RightPlayTransfersCost       右手区 OnPlay → 被腾挪卡 -1 Cost / 本卡 +1 Cost
 *   - Wacom.Battle.ZoneHook.RightPlayCostAccumulates     连续两次右手区 → 本卡累计 +2 Cost
 *   - Wacom.Battle.Effect.AddCostWorksOnSelf             通用 AddCost 对 Target.Self 作用
 *   - Wacom.Battle.Effect.ReduceCostClampsAtZero         ReduceCost 过度时 RuntimeCost 不低于 0
 */

namespace
{
	// ---- 卡 builder ----

	/** Damage(N) + Target.SingleEnemyPart。自带一个 ZoneHook（Zone × Trigger × ExtraEffects）。 */
	UCardDefinition* MakeZoneHookDamageCard(
		FWacomBattleFixture& Fx,
		int32 Cost,
		int32 Damage,
		const FGameplayTag& HookZone,
		const FGameplayTag& HookTrigger,
		const TArray<FCardEffect>& ExtraEffects)
	{
		UCardDefinition* Card = Fx.MakeSimpleDamageCard(Cost, Damage);

		FCardZoneHook Hook;
		Hook.Zone         = HookZone;
		Hook.Trigger      = HookTrigger;
		Hook.ExtraEffects = ExtraEffects;
		Card->ZoneHooks.Add(Hook);
		return Card;
	}

	/** 空效果卡（Cost，无 Target 要求）+ 一个 ZoneHook。 */
	UCardDefinition* MakeZoneHookNoopCard(
		FWacomBattleFixture& Fx,
		int32 Cost,
		const FGameplayTag& HookZone,
		const FGameplayTag& HookTrigger,
		const TArray<FCardEffect>& ExtraEffects)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(Cost);

		FCardZoneHook Hook;
		Hook.Zone         = HookZone;
		Hook.Trigger      = HookTrigger;
		Hook.ExtraEffects = ExtraEffects;
		Card->ZoneHooks.Add(Hook);
		return Card;
	}

	/** 找到第一张拥有 Zone == 目标区域 + 非锚点 + 指定 CardId 的手牌实例。 */
	FGuid FindCardInstanceInZone(const FBattleSnapshot& Snap, FName CardId, EHandZone Zone)
	{
		for (const FHandCardSnapshot& C : Snap.Hand.Cards)
		{
			if (C.bIsHandAnchor) { continue; }
			if (C.Zone != Zone) { continue; }
			if (C.Definition && C.Definition->CardId == CardId) { return C.InstanceId; }
		}
		return FGuid();
	}

	/** 取手牌中指定实例的 RuntimeCost（未找到返回 -1）。 */
	int32 GetRuntimeCostInHand(const FBattleSnapshot& Snap, const FGuid& Id)
	{
		for (const FHandCardSnapshot& C : Snap.Hand.Cards)
		{
			if (C.InstanceId == Id) { return C.RuntimeCost; }
		}
		return -1;
	}
}

// ================================================================
// Test 1: LeftHitSkipsInitiativePush
// 构造：卡 Cost=3，ZoneHook[Left, OnPerfectReleaseHit]。
// 敌人单部位先机 = 3 → cost 命中先机 → 命中时 Hook 触发 → 先机不推进。
// 需要卡在左手区。用 seed 扫描。
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleZoneHookLeftHitSkipsInitiativePushSpec,
	"Wacom.Battle.ZoneHook.LeftHitSkipsInitiativePush",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleZoneHookLeftHitSkipsInitiativePushSpec::RunTest(const FString& /*Parameters*/)
{
	// 至少一个 seed 命中左手区即可。
	bool bAnyCoveredSeed = false;

	for (int32 Seed = 1; Seed <= 50 && !bAnyCoveredSeed; ++Seed)
	{
		FWacomBattleFixture Fx;

		UCardDefinition* LH = Fx.MakeNoopCard(5);
		UCardDefinition* RH = Fx.MakeNoopCard(5);

		UCardDefinition* LeftHook = MakeZoneHookDamageCard(
			Fx, /*Cost*/3, /*Damage*/1,
			WacomTags::HandZone_Left,
			WacomTags::ZoneHook_Trigger_OnPerfectReleaseHit,
			/*ExtraEffects*/ {});

		TArray<UCardDefinition*> Deck = { LeftHook };
		for (int32 i = 0; i < 4; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }

		UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
		UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/100, /*Init*/3, /*Resist*/0);
		UBattleSession*   S     = Fx.CreateSession(Char, Enemy, Seed);

		FBattleSnapshot Snap = S->BuildSnapshot();
		const FGuid LeftId = FindCardInstanceInZone(Snap, LeftHook->CardId, EHandZone::Left);
		if (!LeftId.IsValid()) { continue; }  // 非左手区 → 跳过这个 seed

		const int32 InitBefore = FWacomBattleFixture::FindPartInitiative(Snap, 0);
		TestEqual(TEXT("Enemy init pre-cast = 3"), InitBefore, 3);

		// 打出：先机命中（cost 3 == init 3），Damage=1 → 部位 HP 99。
		// Left + OnPerfectReleaseHit → 跳过先机推进。
		// 期望：部位先机仍为 3（未被 -3）。
		const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snap, 0);
		TestTrue(TEXT("Play Left"),
			S->SubmitCommand(FBattleCommand::MakePlayCard(LeftId, PartId)).IsOk());

		Snap = S->BuildSnapshot();
		TestEqual(FString::Printf(TEXT("Seed=%d init unchanged after left-hook hit"), Seed),
			FWacomBattleFixture::FindPartInitiative(Snap, 0), 3);
		TestEqual(TEXT("HP dropped by main effect"),
			FWacomBattleFixture::FindPartHp(Snap, 0), 99);

		bAnyCoveredSeed = true;
	}

	TestTrue(TEXT("At least one seed has hook card in Left zone"), bAnyCoveredSeed);
	return true;
}

// ================================================================
// Test 2: RightPlayTransfersCost
// 构造：卡 Cost=1，ZoneHook[Right, OnPlay]。
// ExtraEffects = Shuffle.Random + ReduceCost(LastShuffledCard, 1) + AddCost(Self, 1)。
// 当卡在右手区打出时，一张手牌被腾挪 -1 cost，本卡 +1 cost（本次仍用打出时 RuntimeCost=1）。
//
// 实际验证：打出后本卡已经不在手里（会进弃牌）。只能验证"被腾挪卡 -1 Cost"。
// 为验证"本卡 +1 Cost"需要连续两次进入手牌——见 Test 3 CostAccumulates。
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleZoneHookRightPlayTransfersCostSpec,
	"Wacom.Battle.ZoneHook.RightPlayTransfersCost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleZoneHookRightPlayTransfersCostSpec::RunTest(const FString& /*Parameters*/)
{
	bool bCovered = false;

	for (int32 Seed = 1; Seed <= 50 && !bCovered; ++Seed)
	{
		FWacomBattleFixture Fx;

		UCardDefinition* LH = Fx.MakeNoopCard(5);
		UCardDefinition* RH = Fx.MakeNoopCard(5);

		FCardEffect E0; E0.EffectType = WacomTags::Effect_Shuffle_Random;
		E0.Target                      = WacomTags::Target_RandomHandCard;

		FCardEffect E1; E1.EffectType = WacomTags::Effect_Card_ReduceCost;
		E1.Magnitude = 1;
		E1.Target    = WacomTags::Target_LastShuffledCard;

		FCardEffect E2; E2.EffectType = WacomTags::Effect_Card_AddCost;
		E2.Magnitude = 1;
		E2.Target    = WacomTags::Target_Self;

		UCardDefinition* RightHook = MakeZoneHookNoopCard(
			Fx, /*Cost*/1,
			WacomTags::HandZone_Right,
			WacomTags::ZoneHook_Trigger_OnPlay,
			{ E0, E1, E2 });

		// 其他普通卡 Cost=5，方便区分被腾挪卡的 RuntimeCost 变化。
		UCardDefinition* Filler = Fx.MakeNoopCard(5);
		TArray<UCardDefinition*> Deck = { RightHook };
		for (int32 i = 0; i < 4; ++i) { Deck.Add(Filler); }

		UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
		UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/500, /*Init*/50, 0);
		UBattleSession*   S     = Fx.CreateSession(Char, Enemy, Seed);

		FBattleSnapshot Snap = S->BuildSnapshot();
		const FGuid RightId = FindCardInstanceInZone(Snap, RightHook->CardId, EHandZone::Right);
		if (!RightId.IsValid()) { continue; }

		TestTrue(TEXT("PlayRight"),
			S->SubmitCommand(FBattleCommand::MakePlayCard(RightId, FGuid())).IsOk());

		// 断言：手牌里某张 Filler 的 RuntimeCost == 4（5 - 1）。
		Snap = S->BuildSnapshot();
		int32 FoundReducedCount = 0;
		for (const FHandCardSnapshot& C : Snap.Hand.Cards)
		{
			if (C.bIsHandAnchor) { continue; }
			if (C.Definition && C.Definition->CardId == Filler->CardId)
			{
				if (C.RuntimeCost == 4) { ++FoundReducedCount; }
			}
		}
		TestTrue(FString::Printf(TEXT("Seed=%d one filler has RuntimeCost 4"), Seed),
			FoundReducedCount >= 1);

		bCovered = true;
	}

	TestTrue(TEXT("At least one seed has Right-zone card"), bCovered);
	return true;
}

// ================================================================
// Test 3: RightPlayCostAccumulates
// 同一张 ZoneHook[Right, OnPlay] 卡连续两次打出 → 本卡 RuntimeCostModifier += 2。
// 实现方式：卡配成 Combo（打出后留在原位置），这样第 2 次还能打出来。
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleZoneHookRightPlayCostAccumulatesSpec,
	"Wacom.Battle.ZoneHook.RightPlayCostAccumulates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleZoneHookRightPlayCostAccumulatesSpec::RunTest(const FString& /*Parameters*/)
{
	bool bCovered = false;

	for (int32 Seed = 1; Seed <= 50 && !bCovered; ++Seed)
	{
		FWacomBattleFixture Fx;

		UCardDefinition* LH = Fx.MakeNoopCard(5);
		UCardDefinition* RH = Fx.MakeNoopCard(5);

		FCardEffect E0; E0.EffectType = WacomTags::Effect_Shuffle_Random;
		E0.Target                      = WacomTags::Target_RandomHandCard;

		FCardEffect E1; E1.EffectType = WacomTags::Effect_Card_ReduceCost;
		E1.Magnitude = 1;
		E1.Target    = WacomTags::Target_LastShuffledCard;

		FCardEffect E2; E2.EffectType = WacomTags::Effect_Card_AddCost;
		E2.Magnitude = 1;
		E2.Target    = WacomTags::Target_Self;

		// Combo → 本卡打出后留在原位置（不进弃牌）。
		UCardDefinition* RightHook = MakeZoneHookNoopCard(
			Fx, /*Cost*/1,
			WacomTags::HandZone_Right,
			WacomTags::ZoneHook_Trigger_OnPlay,
			{ E0, E1, E2 });
		RightHook->Keywords.AddTag(WacomTags::Card_Keyword_Combo);

		UCardDefinition* Filler = Fx.MakeNoopCard(5);
		TArray<UCardDefinition*> Deck = { RightHook };
		for (int32 i = 0; i < 4; ++i) { Deck.Add(Filler); }

		UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
		UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/500, /*Init*/50, 0);
		UBattleSession*   S     = Fx.CreateSession(Char, Enemy, Seed);

		FBattleSnapshot Snap = S->BuildSnapshot();
		FGuid RightId = FindCardInstanceInZone(Snap, RightHook->CardId, EHandZone::Right);
		if (!RightId.IsValid()) { continue; }

		// 第 1 次打出
		TestTrue(TEXT("Play1"),
			S->SubmitCommand(FBattleCommand::MakePlayCard(RightId, FGuid())).IsOk());

		Snap = S->BuildSnapshot();
		// Combo 留在原位置，Cost = 1 + 1 = 2
		const int32 CostAfter1 = GetRuntimeCostInHand(Snap, RightId);
		TestEqual(FString::Printf(TEXT("Seed=%d CostAfter1 == 2"), Seed), CostAfter1, 2);

		// 打出后本卡仍在右手区才能触发 Hook。腾挪可能把它移到别的区域。
		// 我们只要求本卡的 RuntimeCostModifier 累加；第 2 次若不再在 Right 区，Hook 不触发，
		// 测试仍算通过但不验证"再 +1"。保险起见找另一个 seed 来验证累积。
		const EHandZone ZoneAfter1 = [&Snap, &RightId]()
		{
			for (const FHandCardSnapshot& C : Snap.Hand.Cards)
			{
				if (C.InstanceId == RightId) { return C.Zone; }
			}
			return EHandZone::None;
		}();

		if (ZoneAfter1 != EHandZone::Right) { continue; }  // 换一个 seed

		// 第 2 次打出 → 验证 Cost 累计到 3
		TestTrue(TEXT("Play2"),
			S->SubmitCommand(FBattleCommand::MakePlayCard(RightId, FGuid())).IsOk());

		Snap = S->BuildSnapshot();
		const int32 CostAfter2 = GetRuntimeCostInHand(Snap, RightId);
		TestEqual(FString::Printf(TEXT("Seed=%d CostAfter2 == 3"), Seed), CostAfter2, 3);

		bCovered = true;
	}

	TestTrue(TEXT("At least one seed reaches double-play in Right zone"), bCovered);
	return true;
}

// ================================================================
// Test 4: AddCostWorksOnSelf
// 直接在卡的 Effects 上配 AddCost + Target.Self，验证通用效果对本卡起作用。
// 用 Combo 保证打出后卡仍在手。
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEffectAddCostWorksOnSelfSpec,
	"Wacom.Battle.Effect.AddCostWorksOnSelf",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEffectAddCostWorksOnSelfSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(5);
	UCardDefinition* RH = Fx.MakeNoopCard(5);

	// Card Cost=0 + Combo + 主效果 AddCost(Self, 2)
	UCardDefinition* Card = Fx.MakeNoopCard(0);
	Card->Keywords.AddTag(WacomTags::Card_Keyword_Combo);
	FCardEffect E; E.EffectType = WacomTags::Effect_Card_AddCost;
	E.Magnitude = 2;
	E.Target    = WacomTags::Target_Self;
	Card->Effects.Add(E);

	TArray<UCardDefinition*> Deck = { Card };
	for (int32 i = 0; i < 4; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }

	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(500, 50, 0);
	UBattleSession*   S     = Fx.CreateSession(Char, Enemy, 1);

	FBattleSnapshot Snap = S->BuildSnapshot();
	const FGuid Id = FWacomBattleFixture::FindHandInstanceByCardId(Snap, Card->CardId);
	TestTrue(TEXT("InHand"), Id.IsValid());
	TestEqual(TEXT("InitialCost"), GetRuntimeCostInHand(Snap, Id), 0);

	TestTrue(TEXT("Play"), S->SubmitCommand(FBattleCommand::MakePlayCard(Id, FGuid())).IsOk());
	Snap = S->BuildSnapshot();
	TestEqual(TEXT("CostAfterPlay"), GetRuntimeCostInHand(Snap, Id), 2);

	return true;
}

// ================================================================
// Test 5: ReduceCostClampsAtZero
// BaseCost=1, 主效果 ReduceCost(Self, 5) + Combo → Modifier = -5 → RuntimeCost = max(0, 1-5) = 0。
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEffectReduceCostClampsAtZeroSpec,
	"Wacom.Battle.Effect.ReduceCostClampsAtZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEffectReduceCostClampsAtZeroSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(5);
	UCardDefinition* RH = Fx.MakeNoopCard(5);

	UCardDefinition* Card = Fx.MakeNoopCard(1);
	Card->Keywords.AddTag(WacomTags::Card_Keyword_Combo);
	FCardEffect E; E.EffectType = WacomTags::Effect_Card_ReduceCost;
	E.Magnitude = 5;
	E.Target    = WacomTags::Target_Self;
	Card->Effects.Add(E);

	TArray<UCardDefinition*> Deck = { Card };
	for (int32 i = 0; i < 4; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }

	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(500, 50, 0);
	UBattleSession*   S     = Fx.CreateSession(Char, Enemy, 1);

	FBattleSnapshot Snap = S->BuildSnapshot();
	const FGuid Id = FWacomBattleFixture::FindHandInstanceByCardId(Snap, Card->CardId);
	TestTrue(TEXT("InHand"), Id.IsValid());
	TestEqual(TEXT("InitialCost"), GetRuntimeCostInHand(Snap, Id), 1);

	TestTrue(TEXT("Play"), S->SubmitCommand(FBattleCommand::MakePlayCard(Id, FGuid())).IsOk());
	Snap = S->BuildSnapshot();
	// Modifier = -5, BaseCost = 1 → ComputeRuntimeCost clamps to 0
	TestEqual(TEXT("CostAfterPlay clamps to 0"), GetRuntimeCostInHand(Snap, Id), 0);

	return true;
}
