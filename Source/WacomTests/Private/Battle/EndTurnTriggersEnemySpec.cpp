// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"

/**
 * 结束阶段调用敌方部位行动子流程。
 *
 * 单部位敌人，初始意图 Damage(1) 打玩家。Player MaxHp=100，结束回合后玩家 Hp=99。
 * 事件流里应出现 EnemyPartActed。
 * 同时：新一回合的 CurrentWaitValue 重置为 2。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEndTurnTriggersEnemySpec,
	"Wacom.Battle.EndTurn.InvokesEnemyActions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEndTurnTriggersEnemySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(1);
	UCardDefinition* RH = Fx.MakeNoopCard(1);
	TArray<UCardDefinition*> Deck;
	for (int32 i = 0; i < 5; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }
	UCharacterDefinition* Char  = Fx.MakeCharacter(LH, RH, Deck);

	// Init=20 保证结束阶段敌人尚未行动过（纯由 EndTurn 触发）
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/50, /*Init*/20);
	UBattleSession*   S     = Fx.CreateSession(Char, Enemy, 1);

	FBattleSnapshot Before = S->BuildSnapshot();
	TestEqual(TEXT("Player initial HP"), Before.Player.CurrentHp, 100);

	const FBattleResolution Resolution = S->ResolveCommand(FBattleCommand::MakeEndTurn());
	TestTrue(TEXT("EndTurn ok"), Resolution.IsOk());

	const TArray<FBattleEvent>& Events = Resolution.Events;
	bool bActed = false;
	for (const FBattleEvent& E : Events)
	{
		if (E.Type == EBattleEventType::EnemyPartActed) { bActed = true; break; }
	}
	TestTrue(TEXT("EnemyPartActed emitted"), bActed);

	const FBattleSnapshot After = S->BuildSnapshot();
	TestEqual(TEXT("Player HP decreased by 1"), After.Player.CurrentHp, 99);
	TestEqual(TEXT("CurrentWaitValue reset to 2"), After.CurrentWaitValue, 2);
	TestEqual(TEXT("TurnNumber 2"), After.TurnNumber, 2);
	return true;
}
