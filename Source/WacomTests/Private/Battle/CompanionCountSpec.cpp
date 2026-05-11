// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Commands/BattleCommand.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardPassive.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

/**
 * P3.4 OnCompanionCount 被动（拂晓飞蛾）。对齐 Phase2_P3_Plan §6。
 *
 * 两条测试覆盖：
 *   - Wacom.Battle.Passive.CompanionCountTriggersReturn   打 3 张 Companion 后 Fuxiao 回手
 *   - Wacom.Battle.Passive.CompanionCountResetsAfterTrigger 触发后计数清零
 */

namespace
{
	/**
	 * 构造一张拥有 OnCompanionCount 被动的卡（模拟拂晓飞蛾）。
	 * 本身也是 Companion。Cost=0，无主动效果。
	 */
	UCardDefinition* MakeFuxiaoLikeCard(FWacomBattleFixture& Fx, int32 Threshold)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->Keywords.AddTag(WacomTags::Card_Keyword_Companion);

		FCardPassive Passive;
		Passive.Trigger          = WacomTags::Passive_Trigger_OnCompanionCount;
		Passive.TriggerThreshold = Threshold;
		Card->Passives.Add(Passive);
		return Card;
	}

	/** 构造一张 Companion 关键字的 Noop 卡。 */
	UCardDefinition* MakeCompanionNoop(FWacomBattleFixture& Fx, int32 Cost)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(Cost);
		Card->Keywords.AddTag(WacomTags::Card_Keyword_Companion);
		return Card;
	}

	bool HandHasCard(const FBattleSnapshot& Snap, const FGuid& Id)
	{
		for (const FHandCardSnapshot& C : Snap.Hand.Cards)
		{
			if (C.InstanceId == Id) { return true; }
		}
		return false;
	}
}

// ================================================================
// Test 1: CompanionCountTriggersReturn
// Deck: 3 Companion noop + Fuxiao-like (threshold=3) + 6 fillers.
// Seed loop until Fuxiao stays in DrawPile (not drawn in first 5).
// Play 3 Companion cards → Fuxiao returns to hand.
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCompanionCountTriggersReturnSpec,
	"Wacom.Battle.Passive.CompanionCountTriggersReturn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCompanionCountTriggersReturnSpec::RunTest(const FString& /*Parameters*/)
{
	bool bCovered = false;

	for (int32 Seed = 1; Seed <= 200 && !bCovered; ++Seed)
	{
		FWacomBattleFixture Fx;

		UCardDefinition* LH = Fx.MakeNoopCard(5);
		UCardDefinition* RH = Fx.MakeNoopCard(5);

		// ALL normal cards are Companion. Fuxiao is also Companion.
		// 14 Companion noop + Fuxiao = 15 cards. Draw 5 from 15.
		// Fuxiao has 10/15 = 67% chance of staying in Draw.
		// All 5 drawn cards are Companion (guaranteed since all are Companion).
		// So we always have >= 5 Companion in hand when Fuxiao is not drawn.
		UCardDefinition* Comp[14];
		for (int32 i = 0; i < 14; ++i) { Comp[i] = MakeCompanionNoop(Fx, 0); }
		UCardDefinition* Fuxiao = MakeFuxiaoLikeCard(Fx, 3);

		TArray<UCardDefinition*> Deck;
Deck.Add(Fuxiao);  // First in array → bottom of DrawPile → drawn last → stays in Draw.
for (int32 i = 0; i < 14; ++i) { Deck.Add(Comp[i]); }

		UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
		UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(500, 50, 0);
		UBattleSession*   S     = Fx.CreateSession(Char, Enemy, Seed);

		FBattleSnapshot Snap = S->BuildSnapshot();

		// Check Fuxiao is NOT in hand (stayed in DrawPile).
		const FGuid FuxiaoId = FWacomBattleFixture::FindHandInstanceByCardId(Snap, Fuxiao->CardId);
		if (FuxiaoId.IsValid())
		{
			continue;  // Fuxiao was drawn into hand this seed, skip.
		}

		// Find 3 Companion cards in hand to play.
		TArray<FGuid> CompIds;
		for (const FHandCardSnapshot& C : Snap.Hand.Cards)
		{
			if (C.bIsHandAnchor) { continue; }
			if (C.Definition && C.Definition->Keywords.HasTag(WacomTags::Card_Keyword_Companion))
			{
				CompIds.Add(C.InstanceId);
			}
		}
		if (CompIds.Num() < 3) { continue; }  // Not enough companions drawn

		// Play 3 Companion cards.
		for (int32 i = 0; i < 3; ++i)
		{
			TestTrue(FString::Printf(TEXT("Seed=%d PlayComp%d"), Seed, i),
				S->SubmitCommand(FBattleCommand::MakePlayCard(CompIds[i], FGuid())).IsOk());
		}

		// After 3rd play, Fuxiao should be in hand.
		Snap = S->BuildSnapshot();
		const FGuid FuxiaoAfter = FWacomBattleFixture::FindHandInstanceByCardId(Snap, Fuxiao->CardId);
		TestTrue(FString::Printf(TEXT("Seed=%d Fuxiao returned to hand"), Seed),
			FuxiaoAfter.IsValid());

		// CompanionPlayedCount should be reset to 0.
		TestEqual(FString::Printf(TEXT("Seed=%d CompanionPlayedCount reset"), Seed),
			Snap.CompanionPlayedCount, 0);

		bCovered = true;
	}

	TestTrue(TEXT("At least one seed covers Fuxiao in DrawPile"), bCovered);
	return true;
}

// ================================================================
// Test 2: CompanionCountResetsAfterTrigger
// After trigger (count reset to 0), play 2 more Companion → count = 2, no re-trigger.
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCompanionCountResetsAfterTriggerSpec,
	"Wacom.Battle.Passive.CompanionCountResetsAfterTrigger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCompanionCountResetsAfterTriggerSpec::RunTest(const FString& /*Parameters*/)
{
	bool bCovered = false;

	for (int32 Seed = 1; Seed <= 200 && !bCovered; ++Seed)
	{
		FWacomBattleFixture Fx;

		UCardDefinition* LH = Fx.MakeNoopCard(5);
		UCardDefinition* RH = Fx.MakeNoopCard(5);

		// ALL normal cards are Companion. 14 Companion + Fuxiao = 15 cards.
		// Draw 5 → all drawn are Companion (guaranteed). Fuxiao 67% stays in Draw.
		UCardDefinition* Comp[14];
		for (int32 i = 0; i < 14; ++i) { Comp[i] = MakeCompanionNoop(Fx, 0); }

		UCardDefinition* Fuxiao = MakeFuxiaoLikeCard(Fx, 3);

		TArray<UCardDefinition*> Deck;
Deck.Add(Fuxiao);  // First in array → bottom of DrawPile → drawn last → stays in Draw.
for (int32 i = 0; i < 14; ++i) { Deck.Add(Comp[i]); }

		UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
		UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(500, 50, 0);
		UBattleSession*   S     = Fx.CreateSession(Char, Enemy, Seed);

		FBattleSnapshot Snap = S->BuildSnapshot();

		// Fuxiao must NOT be in hand.
		if (FWacomBattleFixture::FindHandInstanceByCardId(Snap, Fuxiao->CardId).IsValid())
		{
			continue;
		}

		// Need at least 5 Companion cards in hand.
		TArray<FGuid> CompIds;
		for (const FHandCardSnapshot& C : Snap.Hand.Cards)
		{
			if (C.bIsHandAnchor) { continue; }
			if (C.Definition && C.Definition->Keywords.HasTag(WacomTags::Card_Keyword_Companion))
			{
				CompIds.Add(C.InstanceId);
			}
		}
		if (CompIds.Num() < 5) { continue; }

		// Play 3 → trigger.
		for (int32 i = 0; i < 3; ++i)
		{
			TestTrue(TEXT("PlayComp"),
				S->SubmitCommand(FBattleCommand::MakePlayCard(CompIds[i], FGuid())).IsOk());
		}

		Snap = S->BuildSnapshot();
		TestEqual(TEXT("Count reset after trigger"), Snap.CompanionPlayedCount, 0);

		// Play 2 more Companion cards (CompIds[3], CompIds[4]).
		// After trigger, Fuxiao is now in hand. Playing it would also count as Companion.
		// But we play the remaining non-Fuxiao companions.
		for (int32 i = 3; i < 5; ++i)
		{
			TestTrue(TEXT("PlayComp post-trigger"),
				S->SubmitCommand(FBattleCommand::MakePlayCard(CompIds[i], FGuid())).IsOk());
		}

		Snap = S->BuildSnapshot();
		TestEqual(TEXT("Count == 2 after 2 more plays"), Snap.CompanionPlayedCount, 2);

		// Fuxiao should still be in hand (not re-triggered because count < 3).
		TestTrue(TEXT("Fuxiao still in hand"),
			HandHasCard(Snap, FWacomBattleFixture::FindHandInstanceByCardId(Snap, Fuxiao->CardId)));

		bCovered = true;
	}

	TestTrue(TEXT("At least one seed covers reset scenario"), bCovered);
	return true;
}
