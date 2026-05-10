// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Commands/BattleCommand.h"

/**
 * 对齐 Architecture.md §12 测试项 #5/#6：
 *   - 等待先扣当前等待值，再 +1
 *   - 每回合等待值重置为 2
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleWaitMechanicsSpec,
	"Wacom.Battle.Wait.AppliesValueThenIncrements",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleWaitMechanicsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(2);
	UCardDefinition* RH = Fx.MakeNoopCard(2);
	TArray<UCardDefinition*> Deck;
	for (int32 i = 0; i < 5; ++i) { Deck.Add(Fx.MakeNoopCard(1)); }
	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);

	// 敌人先机够高：20，避免等待触发行动导致判断混乱。
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/50, /*Init*/20, 0);
	UBattleSession*   S     = Fx.CreateSession(Char, Enemy, 1);

	const FBattleSnapshot Before = S->BuildSnapshot();
	TestEqual(TEXT("CurrentWaitValue initial"), Before.CurrentWaitValue, 2);
	TestEqual(TEXT("PartInitiative initial"),   FWacomBattleFixture::FindPartInitiative(Before, 0), 20);

	// 第一次等待：扣 2、值变 3
	TestTrue(TEXT("Wait1 ok"), S->SubmitCommand(FBattleCommand::MakeWait()).IsOk());
	{
		const FBattleSnapshot After = S->BuildSnapshot();
		TestEqual(TEXT("Init after wait1"),     FWacomBattleFixture::FindPartInitiative(After, 0), 18);
		TestEqual(TEXT("WaitValue after wait1"), After.CurrentWaitValue, 3);
	}

	// 第二次等待：扣 3、值变 4
	TestTrue(TEXT("Wait2 ok"), S->SubmitCommand(FBattleCommand::MakeWait()).IsOk());
	{
		const FBattleSnapshot After = S->BuildSnapshot();
		TestEqual(TEXT("Init after wait2"),     FWacomBattleFixture::FindPartInitiative(After, 0), 15);
		TestEqual(TEXT("WaitValue after wait2"), After.CurrentWaitValue, 4);
	}

	// 结束回合 → 新回合开始应把等待值重置为 2
	TestTrue(TEXT("EndTurn ok"), S->SubmitCommand(FBattleCommand::MakeEndTurn()).IsOk());
	{
		const FBattleSnapshot After = S->BuildSnapshot();
		TestEqual(TEXT("WaitValue reset on new turn"), After.CurrentWaitValue, 2);
	}
	return true;
}
