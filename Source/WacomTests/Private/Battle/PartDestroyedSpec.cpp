// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Commands/BattleCommand.h"

#include "Cards/CardDefinition.h"

/**
 * HP 归零部位立刻失去意图和先机，不参与后续先机扣减。
 *
 * 构造：三部位敌人，Head HP=5，其余 50；卡 Damage=10。打尾巴（非 Head）不会打破。
 *   然后打 Head，Head HP 归零即破坏，其 CurrentInitiative 应立即变 0，
 *   之后再打一张非迅捷低伤害卡，验证 Head 的先机不再被推进。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattlePartDestroyedSpec,
	"Wacom.Battle.Play.DestroyedPartLosesInitiative",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattlePartDestroyedSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(0);
	UCardDefinition* RH = Fx.MakeNoopCard(0);

	UCardDefinition* HeavyCard = Fx.MakeSimpleDamageCard(/*Cost*/1, /*Dmg*/10);
	UCardDefinition* LightCard = Fx.MakeSimpleDamageCard(/*Cost*/1, /*Dmg*/1);
	TArray<UCardDefinition*> Deck = { HeavyCard, LightCard };
	for (int32 i = 0; i < 3; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }

	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);

	// Head HP = 5（脆），Body/Tail HP = 50
	// 初始先机 Head=7, Body=7, Tail=7（Sum = 21 保证 Cost=1 可打）
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(/*HP*/5, 50, 50, /*Init*/7, 7, 7);
	UBattleSession*   S     = Fx.CreateSession(Char, Enemy, 1);

	FBattleSnapshot Snap = S->BuildSnapshot();
	const FGuid Head  = FWacomBattleFixture::FindPartInstanceId(Snap, 0);
	const FGuid Body  = FWacomBattleFixture::FindPartInstanceId(Snap, 1);
	const FGuid Tail  = FWacomBattleFixture::FindPartInstanceId(Snap, 2);
	const FGuid HeavyId = FWacomBattleFixture::FindHandInstanceByCardId(Snap, HeavyCard->CardId);
	const FGuid LightId = FWacomBattleFixture::FindHandInstanceByCardId(Snap, LightCard->CardId);
	TestTrue(TEXT("HeavyInHand"), HeavyId.IsValid());
	TestTrue(TEXT("LightInHand"), LightId.IsValid());

	// 打 Heavy：Head 被破坏；Body/Tail 先机 - 1 = 6；Head 先机 = 0（因为破坏）。
	TestTrue(TEXT("PlayHeavy"),
		S->SubmitCommand(FWacomBattleFixture::MakePlayCardOnPartInstance(Snap, HeavyId, Head)).IsOk());
	Snap = S->BuildSnapshot();

	TestEqual(TEXT("HeadHp == 0"),       FWacomBattleFixture::FindPartHp(Snap, 0), 0);
	TestTrue (TEXT("HeadDestroyed"),     FWacomBattleFixture::GetEnemyPartSnapshot(Snap, 0)->bDestroyed);
	TestEqual(TEXT("HeadInit == 0"),     FWacomBattleFixture::FindPartInitiative(Snap, 0), 0);
	TestEqual(TEXT("BodyInit == 6"),     FWacomBattleFixture::FindPartInitiative(Snap, 1), 6);
	TestEqual(TEXT("TailInit == 6"),     FWacomBattleFixture::FindPartInitiative(Snap, 2), 6);

	// 部位破坏后弹击倒事件，需要玩家选一个非撤离选项才能继续战斗。
	// 测试场景里左右手都在手牌（LH/RH 是 NoopCard 且未打出），选援助即可。
	TestTrue(TEXT("Phase pending knockdown"),
		S->GetPhase() == EBattlePhase::PendingKnockdownChoice);
	TestTrue(TEXT("Aid OK"),
		S->SubmitCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid)).IsOk());
	TestTrue(TEXT("Phase back to PlayerAction"),
		S->GetPhase() == EBattlePhase::PlayerAction);

	// 再打 Light（Cost=1）：Head 已破坏，先机不变；Body/Tail -1 = 5。
	TestTrue(TEXT("PlayLight"),
		S->SubmitCommand(FWacomBattleFixture::MakePlayCardOnPartInstance(Snap, LightId, Body)).IsOk());
	Snap = S->BuildSnapshot();

	TestEqual(TEXT("HeadInit still 0"), FWacomBattleFixture::FindPartInitiative(Snap, 0), 0);
	TestEqual(TEXT("BodyInit 5"),       FWacomBattleFixture::FindPartInitiative(Snap, 1), 5);
	TestEqual(TEXT("TailInit 5"),       FWacomBattleFixture::FindPartInitiative(Snap, 2), 5);

	// 敌人总先机应排除破坏部位
	const FEnemySnapshot* EnemySnapshot = FWacomBattleFixture::GetEnemySnapshot(Snap, 0);
	TestNotNull(TEXT("Enemy snapshot exists"), EnemySnapshot);
	TestEqual(TEXT("InitiativeSum excludes destroyed"), EnemySnapshot ? EnemySnapshot->InitiativeSum : -1, 10);
	return true;
}
