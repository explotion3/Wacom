// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"

/**
 * 回合开始抽 5 张普通卡牌。
 *
 * 条件：抽牌堆足够多普通卡 + 左右手锚点存在。
 * 断言：Initialize 后，Hand 的普通卡数量 == 5。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleDrawFiveSpec,
	"Wacom.Battle.TurnStart.DrawsFiveNormalCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleDrawFiveSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LeftHand  = Fx.MakeNoopCard(2);
	UCardDefinition* RightHand = Fx.MakeNoopCard(2);
	TArray<UCardDefinition*> Deck;
	for (int32 i = 0; i < 10; ++i)
	{
		Deck.Add(Fx.MakeNoopCard(1));
	}
	UCharacterDefinition* Char  = Fx.MakeCharacter(LeftHand, RightHand, Deck);
	UEnemyDefinition*     Enemy = Fx.MakeSinglePartEnemy(/*HP*/20, /*Init*/5, /*Resist*/0);
	UBattleSession*       S     = Fx.CreateSession(Char, Enemy, /*Seed*/12345);

	const FBattleSnapshot Snap = S->BuildSnapshot();

	TestEqual(TEXT("NormalCardCount"), Snap.Hand.NormalCardCount, 5);
	TestEqual(TEXT("NormalCardLimit"), Snap.Hand.NormalCardLimit, 10);
	TestTrue (TEXT("LeftHandPresent"), Snap.Hand.bLeftHandPresent);
	TestTrue (TEXT("RightHandPresent"), Snap.Hand.bRightHandPresent);
	TestEqual(TEXT("DrawPileAfter"),  Snap.PileCounts.DrawCount, 5);  // 10 - 5
	return true;
}
