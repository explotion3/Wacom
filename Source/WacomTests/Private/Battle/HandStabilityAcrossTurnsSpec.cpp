// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Commands/BattleCommand.h"

#include "Cards/CardDefinition.h"

/**
 * 对齐 Architecture.md §12 测试项 #3：
 *   左右手都在手牌时，新抽卡不会移动已有手牌位置。
 *
 * 策略：
 *   1. 第 1 回合：抽到 5 张 + 左右手，共 7 张（都在手牌）。
 *   2. 结束回合（敌人先机足够大不死玩家）：普通卡全进弃牌；左右手保留。
 *   3. 第 2 回合开始：抽 5 张普通卡；左右手锚点在 Hand 已有。
 *
 * 断言：第 2 回合开始时，左右手锚点在 Hand 的"相对顺序"没变（以 index 相对左边 0 为锚）。
 * 即：若第 1 回合结束时 Left 在 Right 左边，第 2 回合抽完 5 张后 Left 仍在 Right 左边。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleHandStabilityAcrossTurnsSpec,
	"Wacom.Battle.TurnStart.KeepsAnchorsOrderIfBothPresent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleHandStabilityAcrossTurnsSpec::RunTest(const FString& /*Parameters*/)
{
	// 验证多个 seed 下的稳定性
	for (int32 Seed = 1; Seed <= 10; ++Seed)
	{
		FWacomBattleFixture Fx;

		UCardDefinition* LH = Fx.MakeNoopCard(1);
		UCardDefinition* RH = Fx.MakeNoopCard(1);
		TArray<UCardDefinition*> Deck;
		for (int32 i = 0; i < 10; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }
		UCharacterDefinition* Char  = Fx.MakeCharacter(LH, RH, Deck);
		UEnemyDefinition*     Enemy = Fx.MakeSinglePartEnemy(1000, 100, 0);  // 无限耐打
		UBattleSession*       S     = Fx.CreateSession(Char, Enemy, Seed);

		// 第 1 回合 Hand 里左右手的相对顺序
		FBattleSnapshot Snap1 = S->BuildSnapshot();
		int32 L1 = INDEX_NONE, R1 = INDEX_NONE;
		for (int32 i = 0; i < Snap1.Hand.Cards.Num(); ++i)
		{
			if (Snap1.Hand.Cards[i].InstanceId == FWacomBattleFixture::FindHandInstanceByCardId(Snap1, LH->CardId)) { L1 = i; }
			if (Snap1.Hand.Cards[i].InstanceId == FWacomBattleFixture::FindHandInstanceByCardId(Snap1, RH->CardId)) { R1 = i; }
		}
		TestTrue(FString::Printf(TEXT("Seed=%d anchors in hand turn1"), Seed), L1 != INDEX_NONE && R1 != INDEX_NONE);
		const bool LhLeftOfRhBefore = L1 < R1;

		// 结束回合（敌人打不死玩家因 MaxHp=100 而 intent damage = 1）
		TestTrue(TEXT("EndTurn ok"), S->SubmitCommand(FBattleCommand::MakeEndTurn()).IsOk());

		// 第 2 回合 Hand
		FBattleSnapshot Snap2 = S->BuildSnapshot();
		int32 L2 = INDEX_NONE, R2 = INDEX_NONE;
		for (int32 i = 0; i < Snap2.Hand.Cards.Num(); ++i)
		{
			if (Snap2.Hand.Cards[i].InstanceId == FWacomBattleFixture::FindHandInstanceByCardId(Snap2, LH->CardId)) { L2 = i; }
			if (Snap2.Hand.Cards[i].InstanceId == FWacomBattleFixture::FindHandInstanceByCardId(Snap2, RH->CardId)) { R2 = i; }
		}
		TestTrue(FString::Printf(TEXT("Seed=%d anchors in hand turn2"), Seed), L2 != INDEX_NONE && R2 != INDEX_NONE);

		const bool LhLeftOfRhAfter = L2 < R2;
		TestEqual(FString::Printf(TEXT("Seed=%d anchor order preserved"), Seed),
		          LhLeftOfRhAfter, LhLeftOfRhBefore);
	}
	return true;
}
