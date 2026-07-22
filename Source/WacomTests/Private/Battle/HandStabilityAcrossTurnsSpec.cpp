// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Commands/BattleCommand.h"

#include "Cards/CardDefinition.h"

/**
 * 回合开始统一重建手牌队列：
 *   保留只保留卡仍在手牌池，不保留卡牌 index / 相对顺序 / 区域。
 *
 * 策略：
 *   1. 第 1 回合：抽到 5 张 + 左右手，共 7 张（都在手牌）。
 *   2. 结束回合：普通卡按保留规则处理，左右手保留。
 *   3. 第 2 回合开始：保留卡 + 新抽卡统一重排，再插入左右手锚点。
 *
 * 断言：第 2 回合左右手仍在 Hand，且两锚点之间仍至少有一张普通卡。
 * 不再要求跨回合锚点顺序或普通卡位置保持稳定。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleHandQueueRebuildAcrossTurnsSpec,
	"Wacom.Battle.TurnStart.RebuildsHandQueueEveryTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleHandQueueRebuildAcrossTurnsSpec::RunTest(const FString& /*Parameters*/)
{
	for (int32 Seed = 1; Seed <= 10; ++Seed)
	{
		FWacomBattleFixture Fx;

		UCardDefinition* LH = Fx.MakeNoopCard(1);
		UCardDefinition* RH = Fx.MakeNoopCard(1);
		TArray<UCardDefinition*> Deck;
		for (int32 i = 0; i < 10; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }
		UCharacterDefinition* Char  = Fx.MakeCharacter(LH, RH, Deck);
		UEnemyDefinition*     Enemy = Fx.MakeSinglePartEnemy(1000, 100);  // 无限耐打
		UBattleSession*       S     = Fx.CreateSession(Char, Enemy, Seed);

		FBattleSnapshot Snap1 = S->BuildSnapshot();
		int32 L1 = INDEX_NONE, R1 = INDEX_NONE;
		for (int32 i = 0; i < Snap1.Hand.Cards.Num(); ++i)
		{
			if (Snap1.Hand.Cards[i].InstanceId == FWacomBattleFixture::FindHandInstanceByCardId(Snap1, LH->CardId)) { L1 = i; }
			if (Snap1.Hand.Cards[i].InstanceId == FWacomBattleFixture::FindHandInstanceByCardId(Snap1, RH->CardId)) { R1 = i; }
		}
		TestTrue(FString::Printf(TEXT("Seed=%d anchors in hand turn1"), Seed), L1 != INDEX_NONE && R1 != INDEX_NONE);
		TestTrue(FString::Printf(TEXT("Seed=%d anchor spacing turn1"), Seed), FMath::Abs(L1 - R1) >= 2);

		TestTrue(TEXT("EndTurn ok"), S->ResolveCommand(FBattleCommand::MakeEndTurn()).IsOk());

		FBattleSnapshot Snap2 = S->BuildSnapshot();
		int32 L2 = INDEX_NONE, R2 = INDEX_NONE;
		for (int32 i = 0; i < Snap2.Hand.Cards.Num(); ++i)
		{
			if (Snap2.Hand.Cards[i].InstanceId == FWacomBattleFixture::FindHandInstanceByCardId(Snap2, LH->CardId)) { L2 = i; }
			if (Snap2.Hand.Cards[i].InstanceId == FWacomBattleFixture::FindHandInstanceByCardId(Snap2, RH->CardId)) { R2 = i; }
		}
		TestTrue(FString::Printf(TEXT("Seed=%d anchors in hand turn2"), Seed), L2 != INDEX_NONE && R2 != INDEX_NONE);
		TestTrue(FString::Printf(TEXT("Seed=%d anchor spacing turn2"), Seed), FMath::Abs(L2 - R2) >= 2);
		TestEqual(FString::Printf(TEXT("Seed=%d normal count limit still enforced"), Seed),
			Snap2.Hand.NormalCardLimit, 10);
		TestTrue(FString::Printf(TEXT("Seed=%d normal count not over limit"), Seed),
			Snap2.Hand.NormalCardCount <= Snap2.Hand.NormalCardLimit);
	}
	return true;
}
