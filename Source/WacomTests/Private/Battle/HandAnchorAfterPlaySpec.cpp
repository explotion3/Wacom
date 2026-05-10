// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Commands/BattleCommand.h"

#include "Cards/CardDefinition.h"

/**
 * 对齐 Architecture.md §12 测试项 #11：
 *   左右手牌打出后不进入任何区域（Discard / Exhaust 都不在）。
 *
 * 构造：把左手牌做成 Cost=1、无效果的可打出牌。打出后：
 *   - 不在 Hand
 *   - DiscardPile 张数不变
 *   - ExhaustPile 张数不变
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleHandAnchorAfterPlaySpec,
	"Wacom.Battle.Play.AnchorNotEnteringAnyPile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleHandAnchorAfterPlaySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	// 左手：Cost=1 无效果（可被 PlayCard 成功）
	UCardDefinition* LH = Fx.MakeNoopCard(1);
	UCardDefinition* RH = Fx.MakeNoopCard(1);
	TArray<UCardDefinition*> Deck;
	for (int32 i = 0; i < 5; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }

	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
	UEnemyDefinition*     Enemy = Fx.MakeSinglePartEnemy(50, 10, 0);
	UBattleSession*       S     = Fx.CreateSession(Char, Enemy, 1);

	FBattleSnapshot Snap = S->BuildSnapshot();
	const int32 DiscardBefore = Snap.PileCounts.DiscardCount;
	const int32 ExhaustBefore = Snap.PileCounts.ExhaustCount;

	const FGuid LHId = FWacomBattleFixture::FindHandInstanceByCardId(Snap, LH->CardId);
	TestTrue(TEXT("LeftHand in hand"), LHId.IsValid());

	TestTrue(TEXT("Play LeftHand"), S->SubmitCommand(FBattleCommand::MakePlayCard(LHId, FGuid())).IsOk());

	Snap = S->BuildSnapshot();
	TestEqual(TEXT("LeftHand gone from hand"),
		FWacomBattleFixture::FindHandIndex(Snap, LHId), INDEX_NONE);
	TestEqual(TEXT("DiscardCount unchanged"), Snap.PileCounts.DiscardCount, DiscardBefore);
	TestEqual(TEXT("ExhaustCount unchanged"), Snap.PileCounts.ExhaustCount, ExhaustBefore);
	TestFalse(TEXT("bLeftHandPresent false"), Snap.Hand.bLeftHandPresent);
	return true;
}
