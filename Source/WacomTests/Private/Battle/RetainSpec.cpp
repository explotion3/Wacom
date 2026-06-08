// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Commands/BattleCommand.h"

#include "Cards/CardDefinition.h"
#include "Events/BattleEvent.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

/**
 * 保留关键字 + 双手区保留回归测试。
 *
 * 四条测试覆盖：
 *   - Wacom.Battle.Retain.NormalCardRetainKeeps           带 Retain 的普通卡回合结束留在手牌
 *   - Wacom.Battle.Retain.NormalCardNoRetainDiscards      普通卡无 Retain → 进弃牌
 *   - Wacom.Battle.Retain.BothZoneKeepsWhenAnchorsPresent 左右手都在，双手区普通卡保留
 *   - Wacom.Battle.Retain.BothZoneDiscardsWhenAnchorMissing 只剩一张锚点时，原双手区普通卡按单锚判定 → 无 Retain 即弃
 */

namespace
{
	/** 在给定的 Hand 快照里找到第一张位于目标 Zone 且非锚点的卡 InstanceId。 */
	FGuid FindFirstCardInZone(const FBattleSnapshot& Snap, EHandZone Zone)
	{
		for (const FHandCardSnapshot& C : Snap.Hand.Cards)
		{
			if (C.bIsHandAnchor) { continue; }
			if (C.Zone == Zone) { return C.InstanceId; }
		}
		return FGuid();
	}

	/** Hand 中是否存在指定 InstanceId 的卡。 */
	bool HandHas(const FBattleSnapshot& Snap, const FGuid& Id)
	{
		for (const FHandCardSnapshot& C : Snap.Hand.Cards)
		{
			if (C.InstanceId == Id) { return true; }
		}
		return false;
	}

	int32 FindHandIndex(const FBattleSnapshot& Snap, const FGuid& Id)
	{
		for (int32 Index = 0; Index < Snap.Hand.Cards.Num(); ++Index)
		{
			if (Snap.Hand.Cards[Index].InstanceId == Id)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}
}

// ================================================================
// Test 1: NormalCardRetainKeeps
// 普通卡带 Retain 关键字 → 回合结束不进弃牌。
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleRetainNormalCardKeepsSpec,
	"Wacom.Battle.Retain.NormalCardRetainKeeps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleRetainNormalCardKeepsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(2);
	UCardDefinition* RH = Fx.MakeNoopCard(2);

	// RetainCard：Noop + Retain 关键字。DamageCardWithKeywords 返回的是伤害卡，
	// 我们要"无效果 + Retain"，直接创建后写 Keywords。
	UCardDefinition* RetainCard = Fx.MakeNoopCard(0);
	RetainCard->Keywords.AddTag(WacomTags::Card_Keyword_Retain);

	TArray<UCardDefinition*> Deck = { RetainCard };
	for (int32 i = 0; i < 4; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }

	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);

	// 敌人先机高 + MaxHp 足够大，EndTurn 后玩家不死。
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/100, /*Init*/50, /*Resist*/0);
	UBattleSession*   S     = Fx.CreateSession(Char, Enemy, 1);

	FBattleSnapshot Snap = S->BuildSnapshot();
	const FGuid RetainId = FWacomBattleFixture::FindHandInstanceByCardId(Snap, RetainCard->CardId);
	TestTrue(TEXT("RetainCardInHand"), RetainId.IsValid());

	// 结束回合：新回合开始后 Hand 应该仍包含 RetainCard。
	TestTrue(TEXT("EndTurn ok"), S->SubmitCommand(FBattleCommand::MakeEndTurn()).IsOk());

	Snap = S->BuildSnapshot();
	TestTrue (TEXT("RetainCard still in hand next turn"), HandHas(Snap, RetainId));
	// 并且没被误判为进弃牌堆
	// （不直接访问 DiscardPile，但 PileCounts.DiscardCount 应 < 5）
	TestTrue (TEXT("Not all normals went to discard"), Snap.PileCounts.DiscardCount < 5);
	return true;
}

// ================================================================
// Test 2: NormalCardNoRetainDiscards
// 普通卡没有 Retain 且不在双手区 → 回合结束进弃牌。
// 通过手动操纵"只剩一张锚点"场景来保证待测卡不会因双手区规则而保留。
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleRetainNormalCardDiscardsSpec,
	"Wacom.Battle.Retain.NormalCardNoRetainDiscards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleRetainNormalCardDiscardsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	// 左右手 Cost = 1，这样可以打出锚点创造"只剩一张锚点"的场景。
	UCardDefinition* LH = Fx.MakeNoopCard(1);
	UCardDefinition* RH = Fx.MakeNoopCard(1);

	// Deck 做大一些避免新回合抽牌触发 Reshuffle 把 Discard 清空，
	// 导致"通过 PileCount 判断是否进了弃牌"的断言失效。
	TArray<UCardDefinition*> Deck;
	for (int32 i = 0; i < 15; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }

	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/500, /*Init*/50, /*Resist*/0);
	UBattleSession*   S     = Fx.CreateSession(Char, Enemy, 1);

	FBattleSnapshot Snap = S->BuildSnapshot();
	const FGuid LHId = FWacomBattleFixture::FindHandInstanceByCardId(Snap, LH->CardId);
	TestTrue(TEXT("LH anchor in hand"), LHId.IsValid());

	// 打出左手锚点（进 Limbo，本回合离开手牌）。这样结束回合时只剩右手。
	TestTrue(TEXT("PlayLH"), S->SubmitCommand(FBattleCommand::MakePlayCard(LHId)).IsOk());

	Snap = S->BuildSnapshot();
	TestTrue (TEXT("LH gone from hand"),  !HandHas(Snap, LHId));
	TestFalse(TEXT("bLeftHandPresent off"), Snap.Hand.bLeftHandPresent);

	// 记录当前所有非锚点普通卡的 ID（将要在 EndTurn 时被判定是否保留）。
	TArray<FGuid> NormalsBeforeEnd;
	for (const FHandCardSnapshot& C : Snap.Hand.Cards)
	{
		if (!C.bIsHandAnchor) { NormalsBeforeEnd.Add(C.InstanceId); }
	}
	TestTrue(TEXT("HasNormalsBefore"), NormalsBeforeEnd.Num() > 0);

	// 结束回合。只有一张锚点在手 → 无双手区 → 非保留普通卡全进弃牌。
	TestTrue(TEXT("EndTurn ok"), S->SubmitCommand(FBattleCommand::MakeEndTurn()).IsOk());

	// 下一回合 Hand 不应再含有上一回合的任何普通卡（它们应该都进了弃牌区）。
	// 由于 DrawPile 足够大不会触发 Reshuffle，所以新抽到的 5 张都是原先没在手的卡。
	Snap = S->BuildSnapshot();
	for (const FGuid& Id : NormalsBeforeEnd)
	{
		TestFalse(FString::Printf(TEXT("prev normal %s not kept"), *Id.ToString(EGuidFormats::Short)),
			HandHas(Snap, Id));
	}
	return true;
}

// ================================================================
// Test 3: BothZoneKeepsWhenAnchorsPresent
// 双手区普通卡（即使不带 Retain）在左右手都在手牌时应保留到下一回合手牌池。
// 保留不保证下回合继续处于双手区，也不保证 index 稳定。
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleRetainBothZoneKeepsSpec,
	"Wacom.Battle.Retain.BothZoneKeepsWhenAnchorsPresent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleRetainBothZoneKeepsSpec::RunTest(const FString& /*Parameters*/)
{
	// 多 Seed 循环：要求抽到的 Hand 双手区至少有 1 张普通卡。
	// AnchorSpacing 保证双手区至少 1 张普通卡，所以每个 seed 都能命中。
	for (int32 Seed = 1; Seed <= 10; ++Seed)
	{
		FWacomBattleFixture Fx;

		UCardDefinition* LH = Fx.MakeNoopCard(2);
		UCardDefinition* RH = Fx.MakeNoopCard(2);

		TArray<UCardDefinition*> Deck;
		for (int32 i = 0; i < 10; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }

		UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
		UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/500, /*Init*/50, 0);
		UBattleSession*   S     = Fx.CreateSession(Char, Enemy, Seed);

		FBattleSnapshot Snap = S->BuildSnapshot();
		TestTrue(TEXT("Both anchors in hand"),
			Snap.Hand.bLeftHandPresent && Snap.Hand.bRightHandPresent);

		const FGuid BothCardId = FindFirstCardInZone(Snap, EHandZone::Both);
		TestTrue(FString::Printf(TEXT("Seed=%d has Both-zone card"), Seed),
			BothCardId.IsValid());
		const int32 IndexBefore = FindHandIndex(Snap, BothCardId);
		TestTrue(FString::Printf(TEXT("Seed=%d both card index before valid"), Seed), IndexBefore != INDEX_NONE);

		// 结束回合。左右手都未被打出，仍在手牌 → 双手区普通卡保留。
		TestTrue(TEXT("EndTurn ok"),
			S->SubmitCommand(FBattleCommand::MakeEndTurn()).IsOk());

		Snap = S->BuildSnapshot();
		TestTrue(FString::Printf(TEXT("Seed=%d Both-zone card kept"), Seed),
			HandHas(Snap, BothCardId));
		TestTrue(FString::Printf(TEXT("Seed=%d kept card re-enters hand pool with valid index"), Seed),
			FindHandIndex(Snap, BothCardId) != INDEX_NONE);
	}
	return true;
}

// ================================================================
// Test 4: BothZoneDiscardsWhenAnchorMissing
// 打出一张锚点 → 双手区消失 → 原"双手区"的普通卡按单锚规则判定，无 Retain 则进弃牌。
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleRetainBothZoneDiscardsIfAnchorMissingSpec,
	"Wacom.Battle.Retain.BothZoneDiscardsWhenAnchorMissing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleRetainBothZoneDiscardsIfAnchorMissingSpec::RunTest(const FString& /*Parameters*/)
{
	for (int32 Seed = 1; Seed <= 10; ++Seed)
	{
		FWacomBattleFixture Fx;

		// 左手 Cost=1 以便打出；右手 Cost=2 不打。
		UCardDefinition* LH = Fx.MakeNoopCard(1);
		UCardDefinition* RH = Fx.MakeNoopCard(2);

		TArray<UCardDefinition*> Deck;
		for (int32 i = 0; i < 10; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }

		UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
		UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/500, /*Init*/50, 0);
		UBattleSession*   S     = Fx.CreateSession(Char, Enemy, Seed);

		FBattleSnapshot Snap = S->BuildSnapshot();
		const FGuid LHId = FWacomBattleFixture::FindHandInstanceByCardId(Snap, LH->CardId);
		TestTrue(TEXT("LH in hand"), LHId.IsValid());

		// 打出左手：进 Limbo，LH 离开手牌。
		TestTrue(TEXT("PlayLH"), S->SubmitCommand(FBattleCommand::MakePlayCard(LHId)).IsOk());

		Snap = S->BuildSnapshot();
		TestFalse(FString::Printf(TEXT("Seed=%d LH present = false"), Seed), Snap.Hand.bLeftHandPresent);
		TestTrue (FString::Printf(TEXT("Seed=%d RH present = true"),  Seed), Snap.Hand.bRightHandPresent);

		// 记录当前手中所有非锚点卡的 ID（打出左手后，原双手区的牌仍在手里，但 Zone 已被重新判定）。
		TArray<FGuid> NormalsBeforeEnd;
		for (const FHandCardSnapshot& C : Snap.Hand.Cards)
		{
			if (!C.bIsHandAnchor) { NormalsBeforeEnd.Add(C.InstanceId); }
		}
		TestTrue(FString::Printf(TEXT("Seed=%d has normals before EndTurn"), Seed),
			NormalsBeforeEnd.Num() > 0);

		// EndTurn：单锚点在手 → 没有双手区 → 所有普通卡（无 Retain）全进弃牌区。
		TestTrue(TEXT("EndTurn ok"), S->SubmitCommand(FBattleCommand::MakeEndTurn()).IsOk());

		// 下一回合开始时，上一回合的普通卡应全部不在 Hand 中（进了弃牌）。
		// 注意：新回合又抽 5 张，所以必须按 ID 精确比对，而不是"Hand 是否非空"。
		Snap = S->BuildSnapshot();
		for (const FGuid& Id : NormalsBeforeEnd)
		{
			TestFalse(FString::Printf(TEXT("Seed=%d prev normal %s not kept"), Seed, *Id.ToString(EGuidFormats::Short)),
				HandHas(Snap, Id));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleTurnStartHandLimitDiscardEventSpec,
	"Wacom.Battle.TurnStart.HandLimitDiscardEvent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleTurnStartHandLimitDiscardEventSpec::RunTest(const FString& /*Parameters*/)
{
	for (int32 Seed = 1; Seed <= 10; ++Seed)
	{
		FWacomBattleFixture Fx;

		UCardDefinition* LH = Fx.MakeNoopCard(2);
		UCardDefinition* RH = Fx.MakeNoopCard(2);

		TArray<UCardDefinition*> Deck;
		for (int32 i = 0; i < 12; ++i)
		{
			UCardDefinition* RetainCard = Fx.MakeNoopCard(0);
			RetainCard->Keywords.AddTag(WacomTags::Card_Keyword_Retain);
			Deck.Add(RetainCard);
		}

		UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
		UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/500, /*Init*/50, 0);
		UBattleSession* S = Fx.CreateSession(Char, Enemy, Seed);

		// 清掉初始化事件。第一轮结束后：保留 5 张 + 新抽 5 张 = 10，刚好不超限。
		// 第二轮结束后：保留 10 张 + 新抽剩余 2 张 = 12，稳定触发 2 张上限弃牌。
		S->ConsumeEvents();
		TestTrue(TEXT("EndTurn ok"), S->SubmitCommand(FBattleCommand::MakeEndTurn()).IsOk());
		S->ConsumeEvents();
		TestTrue(TEXT("EndTurn second turn ok"), S->SubmitCommand(FBattleCommand::MakeEndTurn()).IsOk());

		const FBattleSnapshot Snap = S->BuildSnapshot();
		TestEqual(TEXT("Turn start enforces normal hand limit"),
			Snap.Hand.NormalCardCount, Snap.Hand.NormalCardLimit);

		const TArray<FBattleEvent> Events = S->ConsumeEvents();
		int32 LimitDiscardEvents = 0;
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type != EBattleEventType::HandLimitDiscarded)
			{
				continue;
			}

			++LimitDiscardEvents;
			TestTrue(TEXT("Turn start limit discard card id valid"), Event.CardInstanceId.IsValid());
			TestFalse(TEXT("Turn start limit discard actor empty"), Event.ActorInstanceId.IsValid());
			TestEqual(TEXT("Turn start limit discard source"),
				Event.HandLimitDiscardSource, EHandLimitDiscardSource::TurnStart);
		}
		TestEqual(TEXT("Turn start emits one event per limit discard"),
			LimitDiscardEvents, 2);
	}

	return true;
}
