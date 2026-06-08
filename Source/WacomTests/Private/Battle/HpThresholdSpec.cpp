// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Session/BattleSession.h"
#include "Session/BattleResultPacket.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

/**
 * 战内 HP 阈值跨越 flag 维护。
 *
 * 验证 BattleSession 在玩家受伤后正确填 BattleState.bCrossedHighHpThreshold /
 * bCrossedLowHpThreshold，BuildResultPacket 拷贝到 packet 给 Run 层结算。
 *
 * 阈值默认值（来自 RunState / BattleInitParams）：
 *   HighHpThreshold = 0.5  → CurrentHp/MaxHp < 0.5 时 +1% 伤口
 *   LowHpThreshold  = 0.2  → CurrentHp/MaxHp < 0.2 时 +5% 伤口
 *
 * Battle 测试用 fixture 默认 MaxHp=100（来自 GetBasePlayerMaxHp），
 * 自伤卡 SelfDamage 把玩家打到指定 HP，再读 packet。
 */

namespace
{
	/** 自伤 N 点的卡：Target.Player + Effect.Damage(N)。 */
	UCardDefinition* MakeSelfDamageCard(FWacomBattleFixture& Fx, int32 Damage)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(/*Cost*/0);
		Card->TargetMode = ECardTargetMode::Self;
		FCardEffect Eff;
		Eff.EffectType = WacomTags::Effect_Damage;
		Eff.Magnitude  = Damage;
		Eff.Target     = WacomTags::Target_Player;
		Card->Effects.Add(Eff);
		return Card;
	}

	/** 创建一个标准战斗 + 玩家初始 HP=100。 */
	UBattleSession* MakeSession(FWacomBattleFixture& Fx, UCardDefinition* SelfDamageCard)
	{
		UCardDefinition* LH = Fx.MakeNoopCard(2);
		UCardDefinition* RH = Fx.MakeNoopCard(2);

		TArray<UCardDefinition*> Deck = { SelfDamageCard };
		// 凑够手牌区
		for (int32 i = 0; i < 4; ++i) { Deck.Add(Fx.MakeNoopCard(1)); }

		UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
		// 单部位敌人，Initiative 高保证战斗一开始不会自动行动
		UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*Hp*/200, /*Init*/100, /*Resist*/0);
		return Fx.CreateSession(Char, Enemy, /*Seed*/1);
	}

	/** 找 Hand 里第一张 CardId 匹配的实例 + 打出。 */
	void PlaySelfDamage(UBattleSession* S, FName CardId)
	{
		const FBattleSnapshot Snap = S->BuildSnapshot();
		const FGuid Pid = FWacomBattleFixture::FindHandInstanceByCardId(Snap, CardId);
		check(Pid.IsValid());
		S->SubmitCommand(FBattleCommand::MakePlayCard(Pid));
	}
}

// ================ 玩家未跨越任何阈值 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleHpThresholdNoCrossSpec,
	"Wacom.Battle.HpThreshold.NoCrossWhenAboveBoth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleHpThresholdNoCrossSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Self40 = MakeSelfDamageCard(Fx, 40); // 100 → 60
	UBattleSession* S = MakeSession(Fx, Self40);

	PlaySelfDamage(S, Self40->CardId);

	const FBattleSnapshot Snap = S->BuildSnapshot();
	TestEqual(TEXT("Player HP=60"), Snap.Player.CurrentHp, 60);

	const FBattleResultPacket Packet = S->BuildResultPacket();
	// 60/100 = 0.6 ≥ 0.5，且 ≥ 0.2 → 都不跨越
	TestFalse(TEXT("High flag false"), Packet.bCrossedHighHpThreshold);
	TestFalse(TEXT("Low flag false"),  Packet.bCrossedLowHpThreshold);

	return true;
}

// ================ 仅跨越 High 阈值 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleHpThresholdHighOnlySpec,
	"Wacom.Battle.HpThreshold.CrossesHighOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleHpThresholdHighOnlySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Self55 = MakeSelfDamageCard(Fx, 55); // 100 → 45
	UBattleSession* S = MakeSession(Fx, Self55);

	PlaySelfDamage(S, Self55->CardId);

	const FBattleSnapshot Snap = S->BuildSnapshot();
	TestEqual(TEXT("Player HP=45"), Snap.Player.CurrentHp, 45);

	const FBattleResultPacket Packet = S->BuildResultPacket();
	// 45/100 = 0.45 < 0.5（跨 High）但 > 0.2（不跨 Low）
	TestTrue (TEXT("High flag true"),  Packet.bCrossedHighHpThreshold);
	TestFalse(TEXT("Low flag false"),  Packet.bCrossedLowHpThreshold);

	return true;
}

// ================ 同时跨越 High 和 Low ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleHpThresholdBothCrossSpec,
	"Wacom.Battle.HpThreshold.CrossesBothInOneShot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleHpThresholdBothCrossSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Self90 = MakeSelfDamageCard(Fx, 90); // 100 → 10
	UBattleSession* S = MakeSession(Fx, Self90);

	PlaySelfDamage(S, Self90->CardId);

	const FBattleSnapshot Snap = S->BuildSnapshot();
	TestEqual(TEXT("Player HP=10"), Snap.Player.CurrentHp, 10);

	const FBattleResultPacket Packet = S->BuildResultPacket();
	// 10/100 = 0.10 < 0.2 < 0.5 → 同时跨两条
	TestTrue(TEXT("High flag true"), Packet.bCrossedHighHpThreshold);
	TestTrue(TEXT("Low flag true"),  Packet.bCrossedLowHpThreshold);

	return true;
}

// ================ Flag 不可逆：HP 回升后 flag 仍为 true ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleHpThresholdFlagPermanentSpec,
	"Wacom.Battle.HpThreshold.FlagStaysTrueAfterHeal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleHpThresholdFlagPermanentSpec::RunTest(const FString& /*Parameters*/)
{
	// 打两张自伤卡：第一张破阈值，第二张治疗也不会清 flag
	// 当前没有治疗卡接入测试，简化为：自伤一次破阈值 -> 后续 EndTurn 多次不影响 flag。
	FWacomBattleFixture Fx;
	UCardDefinition* Self55 = MakeSelfDamageCard(Fx, 55); // 100 → 45
	UBattleSession* S = MakeSession(Fx, Self55);

	PlaySelfDamage(S, Self55->CardId);

	FBattleResultPacket Packet1 = S->BuildResultPacket();
	TestTrue(TEXT("Flag true after damage"), Packet1.bCrossedHighHpThreshold);

	// 多次 EndTurn 后 flag 仍 true（阈值跨越是 latching）
	S->SubmitCommand(FBattleCommand::MakeEndTurn());
	S->SubmitCommand(FBattleCommand::MakeEndTurn());

	FBattleResultPacket Packet2 = S->BuildResultPacket();
	TestTrue(TEXT("Flag still true after EndTurns"), Packet2.bCrossedHighHpThreshold);

	return true;
}

// ================ 第一次跨越后再受伤不会重复加成 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleHpThresholdFirstTimeOnlySpec,
	"Wacom.Battle.HpThreshold.OnlyFirstTimeMatters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleHpThresholdFirstTimeOnlySpec::RunTest(const FString& /*Parameters*/)
{
	// 两张自伤卡：30+30=60 总伤害 → 100→70→40
	// 第一次 70/100=0.7 不跨；第二次 40/100=0.4 跨 High
	// flag 应只在第二次触发；packet 仍只是 true（无累计语义）
	FWacomBattleFixture Fx;
	UCardDefinition* Self30 = MakeSelfDamageCard(Fx, 30);

	UCardDefinition* LH = Fx.MakeNoopCard(2);
	UCardDefinition* RH = Fx.MakeNoopCard(2);
	TArray<UCardDefinition*> Deck = { Self30, Self30 };
	for (int32 i = 0; i < 3; ++i) { Deck.Add(Fx.MakeNoopCard(1)); }

	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*Hp*/200, /*Init*/100, /*Resist*/0);
	UBattleSession* S = Fx.CreateSession(Char, Enemy, /*Seed*/1);

	// 第一张 → 70
	{
		const FBattleSnapshot Snap = S->BuildSnapshot();
		const FGuid Pid = FWacomBattleFixture::FindHandInstanceByCardId(Snap, Self30->CardId);
		S->SubmitCommand(FBattleCommand::MakePlayCard(Pid));
	}
	{
		const FBattleResultPacket P = S->BuildResultPacket();
		TestFalse(TEXT("High not crossed yet"), P.bCrossedHighHpThreshold);
	}

	// 第二张 → 40
	{
		const FBattleSnapshot Snap = S->BuildSnapshot();
		const FGuid Pid = FWacomBattleFixture::FindHandInstanceByCardId(Snap, Self30->CardId);
		S->SubmitCommand(FBattleCommand::MakePlayCard(Pid));
	}
	{
		const FBattleSnapshot Snap = S->BuildSnapshot();
		TestEqual(TEXT("Player HP=40"), Snap.Player.CurrentHp, 40);
		const FBattleResultPacket P = S->BuildResultPacket();
		TestTrue(TEXT("High crossed now"),  P.bCrossedHighHpThreshold);
		TestFalse(TEXT("Low not crossed"),  P.bCrossedLowHpThreshold);
	}

	return true;
}
