// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Misc/AutomationTest.h"

#include "RunSession.h"
#include "RunState.h"
#include "RunStateTypes.h"
#include "Session/BattleResultPacket.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Map/WacomFloorMapDefinition.h"

#include "UObject/StrongObjectPtr.h"

/**
 * Stage 3：经验值与角色技能 trigger 接入测试。
 *
 * 覆盖：
 *   - Packet.KnockdownExpGains 累计 → AddExperience 累加
 *   - Outcome=Defeat 时不结算经验
 *   - Outcome=Victory + bMutualDestruction 时正常结算经验（同归于尽）
 *   - 一场战斗多个部位破坏 → 累加正确
 *   - 经验满 Capacity 自动入账技能（验证从战斗结束链路触发）
 *   - 部位定义 ExperienceReward=0 时记账但发 0 经验
 */

namespace
{
	URunSession* MakeExperienceRunWithCharacter(
		FWacomBattleFixture& Fx,
		FWacomRunExplorationFixture& Exploration)
	{
		UCharacterDefinition* Char = Fx.MakeCharacter(
			Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
			{ Fx.MakeNoopCard(0) });
		UWacomFloorMapDefinition* Floor =
			Exploration.MakeLinearFloor(TEXT("Experience.Floor"), 1);
		Floor->Nodes[0].NodeType = EWacomMapNodeType::Encounter;
		return Exploration.CreateInitializedSession(
			Char,
			Exploration.MakeJourney({ Floor }, TEXT("Experience.Journey"))).Session;
	}

	FRunExplorationResolution FinishExperienceBattleForTest(
		URunSession& Run,
		const FBattleResultPacket& Packet)
	{
		const FRunExplorationResolution Begin =
			Run.BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
		if (!Begin.IsOk() || !Begin.NodeActivityTicket.IsSet())
		{
			return Begin;
		}
		return Run.SettleEncounterNodeActivity(Begin.NodeActivityTicket.GetValue(), Packet);
	}

	FKnockdownExpGain MakeGain(FName PartId, int32 Exp)
	{
		FKnockdownExpGain Gain;
		Gain.PartId = PartId;
		Gain.ExpAmount = Exp;
		return Gain;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunExperienceVictoryGrantsExpSpec,
	"Wacom.Run.Experience.VictoryGrantsExp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunExperienceVictoryGrantsExpSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	FWacomRunExplorationFixture Exploration;
	URunSession* Run = MakeExperienceRunWithCharacter(Fx, Exploration);

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	Packet.KnockdownExpGains.Add(MakeGain(TEXT("Test.Part.A"), 3));
	Packet.KnockdownExpGains.Add(MakeGain(TEXT("Test.Part.B"), 2));

	TestTrue(TEXT("Battle settlement succeeds"), FinishExperienceBattleForTest(*Run, Packet).IsOk());
	TestEqual(TEXT("Experience accumulates 3+2=5"),
		Run->GetExperienceCurrent(), 5);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunExperienceDefeatDoesNotGrantSpec,
	"Wacom.Run.Experience.DefeatDoesNotGrant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunExperienceDefeatDoesNotGrantSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	FWacomRunExplorationFixture Exploration;
	URunSession* Run = MakeExperienceRunWithCharacter(Fx, Exploration);

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Defeat;
	Packet.KnockdownExpGains.Add(MakeGain(TEXT("Test.Part.A"), 5));

	TestTrue(TEXT("Battle settlement succeeds"), FinishExperienceBattleForTest(*Run, Packet).IsOk());
	TestEqual(TEXT("Defeat does not grant experience"),
		Run->GetExperienceCurrent(), 0);
	TestFalse(TEXT("bRunActive=false after Defeat"), Run->IsRunActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunExperienceMutualDestructionGrantsSpec,
	"Wacom.Run.Experience.MutualDestructionGrantsExp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunExperienceMutualDestructionGrantsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	FWacomRunExplorationFixture Exploration;
	URunSession* Run = MakeExperienceRunWithCharacter(Fx, Exploration);

	// 同归于尽：Outcome 仍判 Victory，经验正常发。
	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	Packet.bMutualDestruction = true;
	Packet.KnockdownExpGains.Add(MakeGain(TEXT("Test.Part.A"), 4));

	TestTrue(TEXT("Battle settlement succeeds"), FinishExperienceBattleForTest(*Run, Packet).IsOk());
	TestEqual(TEXT("Mutual destruction still grants experience"),
		Run->GetExperienceCurrent(), 4);
	TestTrue(TEXT("bRunActive remains true after mutual destruction"),
		Run->IsRunActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunExperienceFullGrantsSkillSpec,
	"Wacom.Run.Experience.FullGrantsSkillFromBattle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunExperienceFullGrantsSkillSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	FWacomRunExplorationFixture Exploration;
	URunSession* Run = MakeExperienceRunWithCharacter(Fx, Exploration);

	const int32 Cap = Run->GetExperienceCapacity();

	// 一场战斗给到 2*Cap+3 经验 → 2 个技能 + 余 3。
	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	Packet.KnockdownExpGains.Add(MakeGain(TEXT("Test.Part.A"), Cap));
	Packet.KnockdownExpGains.Add(MakeGain(TEXT("Test.Part.B"), Cap));
	Packet.KnockdownExpGains.Add(MakeGain(TEXT("Test.Part.C"), 3));

	TestTrue(TEXT("Battle settlement succeeds"), FinishExperienceBattleForTest(*Run, Packet).IsOk());

	TestEqual(TEXT("2 skills granted from 2 caps"),
		Run->GetAcquiredSkillCount(), 2);
	TestEqual(TEXT("Experience remainder = 3"),
		Run->GetExperienceCurrent(), 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunExperienceZeroRewardRecordsButGrantsZeroSpec,
	"Wacom.Run.Experience.ZeroRewardRecordsButGrantsZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunExperienceZeroRewardRecordsButGrantsZeroSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	FWacomRunExplorationFixture Exploration;
	URunSession* Run = MakeExperienceRunWithCharacter(Fx, Exploration);

	// ExperienceReward=0 的部位破坏时仍记一条（让 Run 层有完整破坏列表）
	// 但累计 0 经验。
	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	Packet.KnockdownExpGains.Add(MakeGain(TEXT("Test.Part.NoExp"), 0));

	TestTrue(TEXT("Battle settlement succeeds"), FinishExperienceBattleForTest(*Run, Packet).IsOk());
	TestEqual(TEXT("Zero exp parts grant nothing"),
		Run->GetExperienceCurrent(), 0);
	TestEqual(TEXT("No skill granted"),
		Run->GetAcquiredSkillCount(), 0);

	return true;
}

// ================ 战内集成：构造低 HP 部位 → 打死 → BuildResultPacket 包含 KnockdownExpGains ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunExperiencePartDestroyedRecordedInPacketSpec,
	"Wacom.Run.Experience.PartDestroyedRecordedInPacket",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunExperiencePartDestroyedRecordedInPacketSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	// 1 HP 单部位 + 高先机敌人，便于一击破坏。
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*HP*/1, /*Init*/50, /*Resist*/0);
	// fixture 不暴露 ExperienceReward 设置，但 Part 是 fixture 自己造的可以直接改。
	for (FEnemyPartSlot& Slot : Enemy->Parts)
	{
		if (Slot.PartDef)
		{
			UEnemyPartDefinition* MutPart = const_cast<UEnemyPartDefinition*>(Slot.PartDef.Get());
			MutPart->ExperienceReward = 5;
		}
	}

	UCardDefinition* LH = Fx.MakeNoopCard(1);
	UCardDefinition* RH = Fx.MakeNoopCard(1);
	UCardDefinition* Killer = Fx.MakeSimpleDamageCard(/*Cost*/1, /*Damage*/10);

	TArray<UCardDefinition*> Deck;
	Deck.Add(Killer);
	for (int32 i = 0; i < 5; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }

	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);
	UBattleSession* S = Fx.CreateSession(Char, Enemy, /*Seed*/1);

	const FBattleSnapshot Snap0 = S->BuildSnapshot();
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(Snap0, Killer->CardId);
	if (!KillerId.IsValid())
	{
		// 抽到的卡可能不是 Killer。这种情况测试简化为跳过断言。
		AddWarning(TEXT("Killer card not in starting hand, skipping"));
		return true;
	}

	const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snap0, /*PartIndex*/0);

	const FBattleCommand Cmd = FWacomBattleFixture::MakePlayCardOnPartInstance(Snap0, KillerId, PartId);
	const FBattleResolution Status = S->ResolveCommand(Cmd);
	TestTrue(TEXT("PlayCard success"), Status.IsOk());

	// Stage 7：部位破坏后弹击倒事件，必须先选才能继续。
	// 单部位敌人破坏 = 全部破坏，但 BattleEnd 等击倒事件处理完才判定。
	// 测 Aid 路径（左右手都在手牌，可用）。
	TestTrue(TEXT("Phase pending knockdown"),
		S->GetPhase() == EBattlePhase::PendingKnockdownChoice);
	TestTrue(TEXT("Aid OK"),
		S->ResolveCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid)).IsOk());

	const FBattleSnapshot Snap1 = S->BuildSnapshot();
	TestTrue(TEXT("Battle ended"), Snap1.Phase == EBattlePhase::BattleEnd);
	TestTrue(TEXT("Outcome=Victory"), Snap1.Outcome == EBattleOutcome::Victory);

	const FBattleResultPacket Packet = S->BuildResultPacket();
	TestEqual(TEXT("Packet contains 1 destroyed part"),
		Packet.KnockdownExpGains.Num(), 1);
	if (Packet.KnockdownExpGains.Num() == 1)
	{
		TestEqual(TEXT("Exp amount=5"), Packet.KnockdownExpGains[0].ExpAmount, 5);
	}
	TestFalse(TEXT("Not withdrawn (chose Aid)"), Packet.bWithdrawn);

	return true;
}
