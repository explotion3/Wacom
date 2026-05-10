// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"

/**
 * 对齐 Architecture.md §12 测试项 #2：左右手牌插入后两者之间至少有一张普通卡牌。
 *
 * 策略：跑 10 个随机 seed，每次验证左右锚点在 Hand 中的 |index| 差 >= 2。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleHandAnchorSpacingSpec,
	"Wacom.Battle.TurnStart.AnchorsHaveAtLeastOneNormalBetween",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleHandAnchorSpacingSpec::RunTest(const FString& /*Parameters*/)
{
	for (int32 Seed = 1; Seed <= 10; ++Seed)
	{
		FWacomBattleFixture Fx;

		UCardDefinition* LH = Fx.MakeNoopCard(2);
		UCardDefinition* RH = Fx.MakeNoopCard(2);
		TArray<UCardDefinition*> Deck;
		for (int32 i = 0; i < 8; ++i)
		{
			Deck.Add(Fx.MakeNoopCard(1));
		}
		UCharacterDefinition* Char  = Fx.MakeCharacter(LH, RH, Deck);
		UEnemyDefinition*     Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
		UBattleSession*       S     = Fx.CreateSession(Char, Enemy, Seed);

		const FBattleSnapshot Snap = S->BuildSnapshot();

		int32 LeftIdx  = INDEX_NONE;
		int32 RightIdx = INDEX_NONE;
		for (int32 i = 0; i < Snap.Hand.Cards.Num(); ++i)
		{
			const auto& C = Snap.Hand.Cards[i];
			if (!C.bIsHandAnchor) { continue; }
			if (LeftIdx  == INDEX_NONE) { LeftIdx  = i; continue; }
			RightIdx = i;
		}
		TestTrue(TEXT("BothAnchorsInHand"), LeftIdx != INDEX_NONE && RightIdx != INDEX_NONE);
		const int32 Diff = FMath::Abs(LeftIdx - RightIdx);
		TestTrue(FString::Printf(TEXT("Seed=%d Diff>=2"), Seed), Diff >= 2);
	}
	return true;
}
