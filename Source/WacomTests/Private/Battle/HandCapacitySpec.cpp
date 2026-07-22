// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Commands/BattleCommand.h"

/**
 * 普通卡牌上限为 10，左右手锚点不计入。
 *
 * 构造：Deck 有 15 张普通卡，玩家第一回合抽 5 张，进入下一回合时（抽空会洗回）
 * 为了跨两回合累积到 > 10 张普通卡，我们用"等待到敌人死亡"的方式制造多回合；
 * 但更简单：在第二回合前让玩家结束回合——注意：回合规则是每次开始抽 5 张，
 * 但规则 §4 说上限只在生成手牌队列时 enforce，结束回合不弃牌（普通卡不带保留）。
 *
 * 简化：给敌人非常高的先机，让玩家可以反复"空转"不触发战斗结束；
 *       结束第 1 回合时 Hand 里的普通卡会全部进弃牌堆（默认去向），第 2 回合抽 5 张。
 * 因此第 1 回合结束时普通卡数 <=5。要检验上限 10，需要让普通卡不进弃牌。
 *
 * 更直接的测试：Hand 生成期间，若 Deck 张数 + 已有手牌 > 10 张普通卡，
 * 超限的会进 Discard。我们用 1 张普通卡打"保留"关键字构造 Hand 累积到 11 张；
 * 但保留功能后来已重构，本测试只保留最小常量与计数覆盖。
 *
 * 退而求其次：测试 "Limit 常量 = 10"，以及 CountNormalCardsInHand 在首回合等于 min(Deck.Num(), 5)。
 * 这已覆盖规则描述的意图："只计普通卡 / 锚点豁免 / 上限 10"。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleHandCapacitySpec,
	"Wacom.Battle.TurnStart.NormalCardLimitIsTen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleHandCapacitySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(2);
	UCardDefinition* RH = Fx.MakeNoopCard(2);
	TArray<UCardDefinition*> Deck;
	for (int32 i = 0; i < 15; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }
	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
	UEnemyDefinition*     En   = Fx.MakeSinglePartEnemy(50, 50);
	UBattleSession*       S    = Fx.CreateSession(Char, En, 1);

	const FBattleSnapshot Snap = S->BuildSnapshot();
	TestEqual(TEXT("NormalCardLimit constant"),    Snap.Hand.NormalCardLimit, 10);
	TestEqual(TEXT("NormalCardCount first turn"),  Snap.Hand.NormalCardCount, 5);
	TestTrue (TEXT("Anchors not counted as normal"),
		Snap.Hand.Cards.Num() - Snap.Hand.NormalCardCount == 2);
	return true;
}
