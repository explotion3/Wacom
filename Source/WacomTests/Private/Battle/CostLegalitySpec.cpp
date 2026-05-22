// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Commands/BattleCommand.h"

#include "Cards/CardDefinition.h"

/**
 * Cost > Enemy Initiative Sum 时卡牌不可用。
 *
 * 构造：敌人总先机 = 3（单部位）；卡 Cost = 5。
 * 断言：
 *   - Snapshot 里 bIsPlayable == false
 *   - SubmitCommand PlayCard 失败，状态码 = NotEnoughInitiative
 *   - 再建一个 Cost=3 的卡，bIsPlayable == true
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCostLegalitySpec,
	"Wacom.Battle.Play.CostExceedsInitiativeRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCostLegalitySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(1);
	UCardDefinition* RH = Fx.MakeNoopCard(1);
	UCardDefinition* ExpensiveCard = Fx.MakeSimpleDamageCard(/*Cost*/5, /*Dmg*/1);
	UCardDefinition* AffordableCard = Fx.MakeSimpleDamageCard(/*Cost*/3, /*Dmg*/1);

	TArray<UCardDefinition*> Deck = { ExpensiveCard, AffordableCard };
	// 补齐到 5 张避免抽空
	for (int32 i = 0; i < 3; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }

	UCharacterDefinition* Char  = Fx.MakeCharacter(LH, RH, Deck);
	UEnemyDefinition*     Enemy = Fx.MakeSinglePartEnemy(/*HP*/50, /*Init*/3, /*Resist*/0);
	UBattleSession*       S     = Fx.CreateSession(Char, Enemy, 1);

	const FBattleSnapshot Snap = S->BuildSnapshot();

	const FGuid TargetPart = FWacomBattleFixture::FindPartInstanceId(Snap, 0);
	const FGuid ExpId      = FWacomBattleFixture::FindHandInstanceByCardId(Snap, ExpensiveCard->CardId);
	const FGuid AffId      = FWacomBattleFixture::FindHandInstanceByCardId(Snap, AffordableCard->CardId);
	TestTrue(TEXT("ExpensiveInHand"),  ExpId.IsValid());
	TestTrue(TEXT("AffordableInHand"), AffId.IsValid());

	// Snapshot 里的 bIsPlayable
	for (const auto& HC : Snap.Hand.Cards)
	{
		if (HC.InstanceId == ExpId) { TestFalse(TEXT("Expensive not playable"), HC.bIsPlayable); }
		if (HC.InstanceId == AffId) { TestTrue (TEXT("Affordable playable"), HC.bIsPlayable); }
	}

	// 提交 PlayCard 应被拒绝
	const FWacomStatus StExp = S->SubmitCommand(FBattleCommand::MakePlayCard(ExpId, TargetPart));
	TestEqual(TEXT("Expensive rejected code"), (int32)StExp.Code, (int32)EWacomError::NotEnoughInitiative);

	// 提交可支付的
	const FWacomStatus StAff = S->SubmitCommand(FBattleCommand::MakePlayCard(AffId, TargetPart));
	TestTrue(TEXT("Affordable accepted"), StAff.IsOk());
	return true;
}
