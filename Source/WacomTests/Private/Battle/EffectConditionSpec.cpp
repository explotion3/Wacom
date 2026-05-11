// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Commands/BattleCommand.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Cards/EffectCondition.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

/**
 * WacomData 优先级 2：FEffectCondition 条件过滤。
 *
 * 三条测试覆盖：
 *   - Wacom.Battle.Effect.Condition.NoConditionAlwaysExecutes   未设置条件 → 效果永远执行
 *   - Wacom.Battle.Effect.Condition.TargetHasStatusBlocksWhenAbsent
 *       目标不含指定 Status → 效果跳过
 *   - Wacom.Battle.Effect.Condition.TargetHasStatusAllowsWhenPresent
 *       目标含指定 Status → 效果执行
 */

namespace
{
	/** 构造一张 Cost=1 + Damage + 可选条件的卡。 */
	UCardDefinition* MakeConditionalDamageCard(
		FWacomBattleFixture& Fx,
		int32 Damage,
		const FEffectCondition& Condition)
	{
		UCardDefinition* Card = Fx.MakeSimpleDamageCard(/*Cost*/1, Damage);
		if (!Card->Effects.IsEmpty())
		{
			Card->Effects[0].Condition = Condition;
		}
		return Card;
	}
}

// ================================================================
// Test 1: NoConditionAlwaysExecutes
// 验证：未设置条件的普通伤害卡行为不变（回归测试 / 基线）。
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEffectConditionNoneAlwaysExecutes,
	"Wacom.Battle.Effect.Condition.NoConditionAlwaysExecutes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEffectConditionNoneAlwaysExecutes::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(5);
	UCardDefinition* RH = Fx.MakeNoopCard(5);

	// Condition.IsSet() == false → 条件永真。
	UCardDefinition* Card = Fx.MakeSimpleDamageCard(/*Cost*/1, /*Dmg*/5);

	TArray<UCardDefinition*> Deck = { Card };
	for (int32 i = 0; i < 4; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }

	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/50, /*Init*/50, 0);
	UBattleSession*   S     = Fx.CreateSession(Char, Enemy, 1);

	FBattleSnapshot Snap = S->BuildSnapshot();
	const FGuid Id     = FWacomBattleFixture::FindHandInstanceByCardId(Snap, Card->CardId);
	const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snap, 0);
	TestTrue(TEXT("CardInHand"), Id.IsValid());
	TestEqual(TEXT("PartHp initial"), FWacomBattleFixture::FindPartHp(Snap, 0), 50);

	TestTrue(TEXT("Play"), S->SubmitCommand(FBattleCommand::MakePlayCard(Id, PartId)).IsOk());

	Snap = S->BuildSnapshot();
	TestEqual(TEXT("PartHp dropped"), FWacomBattleFixture::FindPartHp(Snap, 0), 45);
	return true;
}

// ================================================================
// Test 2: TargetHasStatusBlocksWhenAbsent
// 卡带条件 Condition.Target.HasStatus(Status.Poison)。
// 目标没有中毒 → 伤害不造成。
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEffectConditionTargetHasStatusBlocksWhenAbsent,
	"Wacom.Battle.Effect.Condition.TargetHasStatusBlocksWhenAbsent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEffectConditionTargetHasStatusBlocksWhenAbsent::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(5);
	UCardDefinition* RH = Fx.MakeNoopCard(5);

	FEffectCondition Cond;
	Cond.ConditionType = WacomTags::Condition_Target_HasStatus;
	Cond.ParamTag      = WacomTags::Status_Poison;

	UCardDefinition* Card = MakeConditionalDamageCard(Fx, /*Dmg*/5, Cond);

	TArray<UCardDefinition*> Deck = { Card };
	for (int32 i = 0; i < 4; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }

	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/50, /*Init*/50, 0);
	UBattleSession*   S     = Fx.CreateSession(Char, Enemy, 1);

	FBattleSnapshot Snap = S->BuildSnapshot();
	const FGuid Id     = FWacomBattleFixture::FindHandInstanceByCardId(Snap, Card->CardId);
	const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snap, 0);
	TestEqual(TEXT("PartHp initial"), FWacomBattleFixture::FindPartHp(Snap, 0), 50);

	// 目标部位不含中毒 → 条件失败 → 伤害跳过。
	TestTrue(TEXT("Play"), S->SubmitCommand(FBattleCommand::MakePlayCard(Id, PartId)).IsOk());

	Snap = S->BuildSnapshot();
	TestEqual(TEXT("PartHp unchanged (condition failed)"),
		FWacomBattleFixture::FindPartHp(Snap, 0), 50);
	return true;
}

// ================================================================
// Test 3: TargetHasStatusAllowsWhenPresent
// 先施加中毒再打条件卡，条件成立 → 伤害造成。
// ================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEffectConditionTargetHasStatusAllowsWhenPresent,
	"Wacom.Battle.Effect.Condition.TargetHasStatusAllowsWhenPresent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEffectConditionTargetHasStatusAllowsWhenPresent::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(5);
	UCardDefinition* RH = Fx.MakeNoopCard(5);

	// 施毒卡（无条件）：Cost=1, ApplyPoison 3 层。
	UCardDefinition* PoisonCard = Fx.MakeNoopCard(/*Cost*/1);
	PoisonCard->TargetMode = ECardTargetMode::SingleEnemyPart;
	{
		FCardEffect E;
		E.EffectType = WacomTags::Effect_ApplyStatus_Poison;
		E.Magnitude  = 3;
		E.Target     = WacomTags::Target_SingleEnemyPart;
		PoisonCard->Effects.Add(E);
	}

	// 条件伤害卡：Damage 5，条件 Target.HasStatus(Poison)。
	FEffectCondition Cond;
	Cond.ConditionType = WacomTags::Condition_Target_HasStatus;
	Cond.ParamTag      = WacomTags::Status_Poison;
	UCardDefinition* DmgCard = MakeConditionalDamageCard(Fx, /*Dmg*/5, Cond);

	TArray<UCardDefinition*> Deck = { PoisonCard, DmgCard };
	for (int32 i = 0; i < 3; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }

	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
	// HP 足够承受多次中毒结算 + 条件伤害。
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/100, /*Init*/50, 0);
	UBattleSession*   S     = Fx.CreateSession(Char, Enemy, 1);

	FBattleSnapshot Snap = S->BuildSnapshot();
	const FGuid PartId    = FWacomBattleFixture::FindPartInstanceId(Snap, 0);
	const FGuid PoisonId  = FWacomBattleFixture::FindHandInstanceByCardId(Snap, PoisonCard->CardId);
	const FGuid DmgId     = FWacomBattleFixture::FindHandInstanceByCardId(Snap, DmgCard->CardId);
	TestTrue(TEXT("PoisonInHand"), PoisonId.IsValid());
	TestTrue(TEXT("DmgInHand"),    DmgId.IsValid());

	// 打施毒卡：施加 3 层中毒，立即结算 -3 HP。部位 HP 100 → 97。
	TestTrue(TEXT("PlayPoison"), S->SubmitCommand(FBattleCommand::MakePlayCard(PoisonId, PartId)).IsOk());
	Snap = S->BuildSnapshot();
	TestEqual(TEXT("PartHp after poison tick"), FWacomBattleFixture::FindPartHp(Snap, 0), 97);

	// 打条件伤害卡：条件成立（Poison 在）→ -5 HP。P3.1 中毒会再结算 -3。
	// 打牌后总 HP = 97 - 5 (伤害) - 3 (中毒) = 89。
	TestTrue(TEXT("PlayDmg"), S->SubmitCommand(FBattleCommand::MakePlayCard(DmgId, PartId)).IsOk());
	Snap = S->BuildSnapshot();
	TestEqual(TEXT("PartHp after conditional damage"), FWacomBattleFixture::FindPartHp(Snap, 0), 89);
	return true;
}
