// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Commands/BattleCommand.h"

#include "Cards/CardDefinition.h"

/**
 * 连击牌打出后回到原位置（仍在手牌）。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleComboReturnSpec,
	"Wacom.Battle.Play.ComboCardStaysInHand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleComboReturnSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(0);
	UCardDefinition* RH = Fx.MakeNoopCard(0);
	UCardDefinition* Combo = Fx.MakeComboDamageCard(/*Cost*/1, /*Dmg*/1);

	TArray<UCardDefinition*> Deck = { Combo };
	for (int32 i = 0; i < 4; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }

	UCharacterDefinition* Char  = Fx.MakeCharacter(LH, RH, Deck);
	UEnemyDefinition*     Enemy = Fx.MakeSinglePartEnemy(50, 10, 0);
	UBattleSession*       S     = Fx.CreateSession(Char, Enemy, 1);

	FBattleSnapshot Snap = S->BuildSnapshot();
	const FGuid ComboId = FWacomBattleFixture::FindHandInstanceByCardId(Snap, Combo->CardId);
	TestTrue(TEXT("ComboInHand"), ComboId.IsValid());

	const int32 IdxBefore     = FWacomBattleFixture::FindHandIndex(Snap, ComboId);
	const int32 DiscardBefore = Snap.PileCounts.DiscardCount;
	const FGuid TargetPart    = FWacomBattleFixture::FindPartInstanceId(Snap, 0);

	TestTrue(TEXT("Play Combo"),
		S->ResolveCommand(FWacomBattleFixture::MakePlayCardOnPartInstance(Snap, ComboId, TargetPart)).IsOk());
	Snap = S->BuildSnapshot();

	const int32 IdxAfter = FWacomBattleFixture::FindHandIndex(Snap, ComboId);
	TestTrue(TEXT("Combo still in hand"), IdxAfter != INDEX_NONE);
	TestEqual(TEXT("Combo at same index"), IdxAfter, IdxBefore);
	TestEqual(TEXT("DiscardCount unchanged"), Snap.PileCounts.DiscardCount, DiscardBefore);
	return true;
}
