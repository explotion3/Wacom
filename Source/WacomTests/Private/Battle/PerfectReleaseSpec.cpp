// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"

#include "Cards/CardDefinition.h"

/**
 * 先机命中与抵抗顺序回归：
 *   - Cost == 部位出牌前先机时触发先机命中
 *   - 抵抗判定先于完美释放
 *
 * 验证：事件流里 InitiativeHit 出现在 ResistanceResolved 之前，
 *       而 PerfectReleaseResolved 不存在（本卡没有 PerfectReleaseEffects，只验证顺序）。
 *
 * 另：非迅捷卡命中时取双方最高单段伤害，卡牌严格大于敌方时应触发眩晕。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattlePerfectReleaseSpec,
	"Wacom.Battle.Play.InitiativeHitAndResistanceOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattlePerfectReleaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(1);
	UCardDefinition* RH = Fx.MakeNoopCard(1);

	// Cost = 5, Damage = 10。敌人单部位 Init=5 → 命中；CardPeak(10) > IntentPeak(1) → 眩晕。
	UCardDefinition* HitCard = Fx.MakeSimpleDamageCard(/*Cost*/5, /*Dmg*/10);
	TArray<UCardDefinition*> Deck = { HitCard };
	for (int32 i = 0; i < 4; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }

	UCharacterDefinition* Char  = Fx.MakeCharacter(LH, RH, Deck);
	UEnemyDefinition*     Enemy = Fx.MakeSinglePartEnemy(/*HP*/50, /*Init*/5);
	UBattleSession*       S     = Fx.CreateSession(Char, Enemy, 1);

	// 清掉 Initialize 的事件

	const FBattleSnapshot Snap = S->BuildSnapshot();
	const FGuid TargetPart = FWacomBattleFixture::FindPartInstanceId(Snap, 0);
	const FGuid CardId     = FWacomBattleFixture::FindHandInstanceByCardId(Snap, HitCard->CardId);
	TestTrue(TEXT("CardInHand"), CardId.IsValid());

	const FBattleResolution Resolution = S->ResolveCommand(
		FWacomBattleFixture::MakePlayCardOnPartInstance(Snap, CardId, TargetPart));
	TestTrue(TEXT("Play ok"), Resolution.IsOk());

	const TArray<FBattleEvent>& Events = Resolution.Events;

	int32 IdxHit = INDEX_NONE, IdxResist = INDEX_NONE, IdxStun = INDEX_NONE;
	for (int32 i = 0; i < Events.Num(); ++i)
	{
		if (Events[i].Type == EBattleEventType::InitiativeHit      && IdxHit    == INDEX_NONE) { IdxHit    = i; }
		if (Events[i].Type == EBattleEventType::ResistanceResolved && IdxResist == INDEX_NONE) { IdxResist = i; }
		if (Events[i].Type == EBattleEventType::StatusApplied      && IdxStun   == INDEX_NONE) { IdxStun   = i; }
	}
	TestTrue(TEXT("HitEmitted"),                    IdxHit    != INDEX_NONE);
	TestTrue(TEXT("ResistanceEmitted"),             IdxResist != INDEX_NONE);
	TestTrue(TEXT("Hit before Resistance"),         IdxHit < IdxResist);
	TestTrue(TEXT("StunStatusEmitted (CR>IR)"),     IdxStun   != INDEX_NONE);
	TestTrue(TEXT("Resistance before Stun event"),  IdxResist < IdxStun);
	return true;
}
