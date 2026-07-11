// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Commands/BattleCommand.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

/**
 * 中毒结算回归测试。
 *
 * 四条测试覆盖：
 *   - Wacom.Battle.Poison.TickOnCardPlay     玩家打牌后敌方中毒部位扣血
 *   - Wacom.Battle.Poison.TickOnEnemyAct     敌方部位行动后玩家中毒扣血
 *   - Wacom.Battle.Poison.PenetratesShield   Shield > 0 时中毒仍扣 HP
 *   - Wacom.Battle.Poison.StacksUnchanged    结算后层数不减
 */

namespace
{
	/** 构造一张对目标部位施加中毒 Stacks 的卡牌（Cost，TargetMode = SingleEnemyPart）。 */
	UCardDefinition* MakePoisonCardOnPart(FWacomBattleFixture& Fx, int32 Cost, int32 Stacks)
	{
		// 复用 NoopCard 创建 + 注册 Roots 的逻辑，然后改写目标与效果。
		UCardDefinition* Card = Fx.MakeNoopCard(Cost);
		Card->TargetMode = ECardTargetMode::SingleEnemyPart;

		FCardEffect Eff;
		Eff.EffectType = WacomTags::Effect_ApplyStatus_Poison;
		Eff.Magnitude  = Stacks;
		Eff.Target     = WacomTags::Target_SingleEnemyPart;
		Card->Effects.Add(Eff);
		return Card;
	}

	/** 构造一张对玩家施加中毒 Stacks 的卡牌（Cost，TargetMode = Self）。 */
	UCardDefinition* MakePoisonCardOnPlayer(FWacomBattleFixture& Fx, int32 Cost, int32 Stacks)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(Cost);
		Card->TargetMode = ECardTargetMode::Self;

		FCardEffect Eff;
		Eff.EffectType = WacomTags::Effect_ApplyStatus_Poison;
		Eff.Magnitude  = Stacks;
		Eff.Target     = WacomTags::Target_Player;
		Card->Effects.Add(Eff);
		return Card;
	}

	/** 构造一张：对玩家自身加 Shield，再施加中毒到玩家的组合卡（Cost = 1）。 */
	UCardDefinition* MakeShieldThenPoisonPlayerCard(FWacomBattleFixture& Fx, int32 Cost, int32 ShieldAmount, int32 PoisonStacks)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(Cost);
		Card->TargetMode = ECardTargetMode::Self;

		// 效果[0]：Shield 加到玩家
		FCardEffect ShieldEff;
		ShieldEff.EffectType = WacomTags::Status_Shield;
		ShieldEff.Magnitude  = ShieldAmount;
		ShieldEff.Target     = WacomTags::Target_Self;   // Self 在 Card 语境下默认映射 Player（非 Shuffle）
		Card->Effects.Add(ShieldEff);

		// 效果[1]：Poison 施加玩家
		FCardEffect PoisonEff;
		PoisonEff.EffectType = WacomTags::Effect_ApplyStatus_Poison;
		PoisonEff.Magnitude  = PoisonStacks;
		PoisonEff.Target     = WacomTags::Target_Player;
		Card->Effects.Add(PoisonEff);
		return Card;
	}
}

// ================================================================
// Test 1: TickOnCardPlay
// 打一张施加 3 层毒的卡到敌方部位 → 中毒结算触发 → 部位 HP -3。
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattlePoisonTickOnCardPlaySpec,
	"Wacom.Battle.Poison.TickOnCardPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattlePoisonTickOnCardPlaySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(2);
	UCardDefinition* RH = Fx.MakeNoopCard(2);

	UCardDefinition* PoisonCard = MakePoisonCardOnPart(Fx, /*Cost*/1, /*Stacks*/3);
	TArray<UCardDefinition*> Deck = { PoisonCard };
	for (int32 i = 0; i < 4; ++i) { Deck.Add(Fx.MakeNoopCard(1)); }

	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);

	// 先机 20 足以让打牌之后敌方不行动（20 - 1 = 19 > 0）。
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/30, /*Init*/20, /*Resist*/0);
	UBattleSession*   S     = Fx.CreateSession(Char, Enemy, 1);

	FBattleSnapshot Snap = S->BuildSnapshot();
	const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snap, 0);
	const FGuid Pid    = FWacomBattleFixture::FindHandInstanceByCardId(Snap, PoisonCard->CardId);
	TestTrue(TEXT("PoisonCardInHand"), Pid.IsValid());

	TestEqual(TEXT("PartHp initial"), FWacomBattleFixture::FindPartHp(Snap, 0), 30);

	// 打出中毒卡：
	// 1) 施加 3 层毒到部位
	// 2) PlayCardResolver 末尾 Status Semantics 结算 Poison → 部位 -3 HP
	// 敌方先机由 20 扣到 19，不触发行动。
	TestTrue(TEXT("PlayPoison"),
		S->ResolveCommand(FWacomBattleFixture::MakePlayCardOnPartInstance(Snap, Pid, PartId)).IsOk());

	Snap = S->BuildSnapshot();
	TestEqual(TEXT("PartHp after poison tick"), FWacomBattleFixture::FindPartHp(Snap, 0), 27);
	TestEqual(TEXT("PartInitiative untouched by poison"), FWacomBattleFixture::FindPartInitiative(Snap, 0), 19);

	// 层数不减
	const FEnemyPartSnapshot& PartSnap = *FWacomBattleFixture::GetEnemyPartSnapshot(Snap, 0);
	const int32* PoisonStacks = PartSnap.StatusStacks.Find(WacomTags::Status_Poison);
	TestTrue (TEXT("PoisonStacks present"), PoisonStacks != nullptr);
	TestEqual(TEXT("PoisonStacks == 3"),    PoisonStacks ? *PoisonStacks : -1, 3);
	TestTrue (TEXT("Statuses contains Poison"), PartSnap.Statuses.HasTag(WacomTags::Status_Poison));

	return true;
}

// ================================================================
// Test 2: TickOnEnemyAct
// 玩家中毒，EndTurn → 敌方部位行动 → ActOnce 结束的 Status Semantics 扣玩家 HP。
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattlePoisonTickOnEnemyActSpec,
	"Wacom.Battle.Poison.TickOnEnemyAct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattlePoisonTickOnEnemyActSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(2);
	UCardDefinition* RH = Fx.MakeNoopCard(2);

	UCardDefinition* PoisonPlayer = MakePoisonCardOnPlayer(Fx, /*Cost*/1, /*Stacks*/3);
	TArray<UCardDefinition*> Deck = { PoisonPlayer };
	for (int32 i = 0; i < 4; ++i) { Deck.Add(Fx.MakeNoopCard(1)); }

	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);

	// 高先机避免打牌后触发敌方行动。
	// 首意图 Damage=1 打玩家（fixture 硬编码）。
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/100, /*Init*/20, /*Resist*/0);
	UBattleSession*   S     = Fx.CreateSession(Char, Enemy, 1);

	FBattleSnapshot Snap = S->BuildSnapshot();
	const FGuid Pid = FWacomBattleFixture::FindHandInstanceByCardId(Snap, PoisonPlayer->CardId);
	TestTrue(TEXT("PoisonPlayerCardInHand"), Pid.IsValid());
	TestEqual(TEXT("PlayerHp initial"), Snap.Player.CurrentHp, 100);

	// 打出 → 玩家获得 3 层毒 + 立即触发一次中毒结算（玩家 -3）。
	// 先机 20→19，敌方不行动。
	TestTrue(TEXT("PlayPoisonOnPlayer"), S->ResolveCommand(FBattleCommand::MakePlayCard(Pid)).IsOk());

	Snap = S->BuildSnapshot();
	TestEqual(TEXT("PlayerHp after card-play tick"), Snap.Player.CurrentHp, 97);
	const int32* PoisonStacksAfterPlay = Snap.Player.StatusStacks.Find(WacomTags::Status_Poison);
	TestTrue (TEXT("Player Poison present"), PoisonStacksAfterPlay != nullptr);
	TestEqual(TEXT("Player Poison stacks"),  PoisonStacksAfterPlay ? *PoisonStacksAfterPlay : -1, 3);

	// 结束回合 → 所有敌方部位按序行动（ResolveEndTurnActions）
	//   - 部位意图：Damage 1 打玩家 → 玩家 -1
	//   - ActOnce 末尾由 Status Semantics 结算 Poison → 玩家 -3（因为还有 3 层毒）
	// 最终玩家 HP = 97 - 1 - 3 = 93。
	TestTrue(TEXT("EndTurn"), S->ResolveCommand(FBattleCommand::MakeEndTurn()).IsOk());

	Snap = S->BuildSnapshot();
	TestEqual(TEXT("PlayerHp after enemy-act tick"), Snap.Player.CurrentHp, 93);

	// 层数仍然 3（未因结算消耗）。
	const int32* PoisonStacksAfterAct = Snap.Player.StatusStacks.Find(WacomTags::Status_Poison);
	TestTrue (TEXT("Player Poison still present"), PoisonStacksAfterAct != nullptr);
	TestEqual(TEXT("Player Poison stacks unchanged"), PoisonStacksAfterAct ? *PoisonStacksAfterAct : -1, 3);

	return true;
}

// ================================================================
// Test 3: PenetratesShield
// 玩家同时加 Shield + 中毒 → 中毒结算直接扣 HP，不经 Shield。
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattlePoisonPenetratesShieldSpec,
	"Wacom.Battle.Poison.PenetratesShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattlePoisonPenetratesShieldSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(2);
	UCardDefinition* RH = Fx.MakeNoopCard(2);

	UCardDefinition* ComboCard = MakeShieldThenPoisonPlayerCard(Fx, /*Cost*/1, /*Shield*/100, /*Poison*/3);
	TArray<UCardDefinition*> Deck = { ComboCard };
	for (int32 i = 0; i < 4; ++i) { Deck.Add(Fx.MakeNoopCard(1)); }

	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/100, /*Init*/20, /*Resist*/0);
	UBattleSession*   S     = Fx.CreateSession(Char, Enemy, 1);

	FBattleSnapshot Snap = S->BuildSnapshot();
	const FGuid Pid = FWacomBattleFixture::FindHandInstanceByCardId(Snap, ComboCard->CardId);
	TestTrue(TEXT("ComboCardInHand"), Pid.IsValid());
	TestEqual(TEXT("PlayerHp initial"),     Snap.Player.CurrentHp, 100);
	TestEqual(TEXT("PlayerShield initial"), Snap.Player.Shield,    0);

	// 打出：
	// 1) Shield 100 加到玩家
	// 2) 施加 Poison 3 到玩家
	// 3) Status Semantics 结算 Poison → 玩家直接 -3 HP（Shield 不吸收）
	TestTrue(TEXT("PlayCombo"), S->ResolveCommand(FBattleCommand::MakePlayCard(Pid)).IsOk());

	Snap = S->BuildSnapshot();
	TestEqual(TEXT("PlayerShield unchanged by poison"), Snap.Player.Shield,    100);
	TestEqual(TEXT("PlayerHp dropped by poison direct"), Snap.Player.CurrentHp, 97);

	return true;
}

// ================================================================
// Test 4: StacksUnchanged
// 多次结算（两次打牌两次结算），层数始终等于首次施加的值。
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattlePoisonStacksUnchangedSpec,
	"Wacom.Battle.Poison.StacksUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattlePoisonStacksUnchangedSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(2);
	UCardDefinition* RH = Fx.MakeNoopCard(2);

	UCardDefinition* PoisonCard = MakePoisonCardOnPart(Fx, /*Cost*/1, /*Stacks*/3);
	UCardDefinition* NoopCost1  = Fx.MakeNoopCard(1);
	TArray<UCardDefinition*> Deck = { PoisonCard, NoopCost1 };
	for (int32 i = 0; i < 3; ++i) { Deck.Add(Fx.MakeNoopCard(1)); }

	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);

	// 足够高的先机避免敌方行动干扰。血量 50 足以承受多次 -3。
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/50, /*Init*/30, /*Resist*/0);
	UBattleSession*   S     = Fx.CreateSession(Char, Enemy, 1);

	FBattleSnapshot Snap = S->BuildSnapshot();
	const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snap, 0);
	const FGuid PoisonId = FWacomBattleFixture::FindHandInstanceByCardId(Snap, PoisonCard->CardId);
	const FGuid NoopId   = FWacomBattleFixture::FindHandInstanceByCardId(Snap, NoopCost1->CardId);
	TestTrue(TEXT("PoisonCardInHand"), PoisonId.IsValid());
	TestTrue(TEXT("NoopCostInHand"),   NoopId.IsValid());

	// 第 1 张：施加 3 层毒 + 结算一次（部位 HP 50→47）
	TestTrue(TEXT("Play1"),
		S->ResolveCommand(FWacomBattleFixture::MakePlayCardOnPartInstance(Snap, PoisonId, PartId)).IsOk());
	Snap = S->BuildSnapshot();
	TestEqual(TEXT("PartHp after tick1"), FWacomBattleFixture::FindPartHp(Snap, 0), 47);

	// 第 2 张：无效果卡，Status Semantics 仍应结算 Poison（部位 HP 47→44）
	TestTrue(TEXT("Play2"), S->ResolveCommand(FBattleCommand::MakePlayCard(NoopId)).IsOk());
	Snap = S->BuildSnapshot();
	TestEqual(TEXT("PartHp after tick2"), FWacomBattleFixture::FindPartHp(Snap, 0), 44);

	// 两次结算后层数仍 = 3。
	const FEnemyPartSnapshot& PartSnap = *FWacomBattleFixture::GetEnemyPartSnapshot(Snap, 0);
	const int32* PoisonStacks = PartSnap.StatusStacks.Find(WacomTags::Status_Poison);
	TestTrue (TEXT("Stacks present"),      PoisonStacks != nullptr);
	TestEqual(TEXT("Stacks still 3"),      PoisonStacks ? *PoisonStacks : -1, 3);

	return true;
}
