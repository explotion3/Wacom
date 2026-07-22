// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Misc/AutomationTest.h"

#include "RunSession.h"
#include "RunStateTypes.h"
#include "Runtime/BattleEnemyKeys.h"
#include "Session/BattleResultPacket.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Map/WacomFloorMapDefinition.h"

#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

/**
 * Stage 1.2：BattleResultPacket → URunSession::OnBattleFinished 战外结算测试。
 *
 * 覆盖：
 *   - 任何包过一次 → 疲劳 +1%
 *   - bCrossedHighHpThreshold → 伤口 +1%
 *   - bCrossedLowHpThreshold  → 伤口 +5%
 *   - bMutualDestruction      → 伤口 +10%
 *   - 全 flag 一起 → 累加正确
 *   - Outcome=Defeat → bRunActive=false
 *   - Outcome=Victory + bMutualDestruction → bRunActive 仍 true
 *   - Outcome=Undetermined → 不结算压力
 */

namespace
{
	struct FRunResultFixture
	{
		FWacomRunExplorationFixture Exploration;
		URunSession* Run = nullptr;

		explicit FRunResultFixture(FWacomBattleFixture& Fx)
		{
			UCharacterDefinition* Char = Fx.MakeCharacter(
				Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
				{ Fx.MakeNoopCard(0) });
			UWacomFloorMapDefinition* Floor =
				Exploration.MakeLinearFloor(TEXT("BattleResult.Floor"), 1);
			Floor->Nodes[0].NodeType = EWacomMapNodeType::Encounter;
			Run = Exploration.CreateInitializedSession(
				Char,
				Exploration.MakeJourney({ Floor }, TEXT("BattleResult.Journey"))).Session;
		}

		FRunExplorationResolution Settle(const FBattleResultPacket& Packet) const
		{
			const FRunExplorationResolution Begin =
				Run->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
			if (!Begin.IsOk() || !Begin.NodeActivityTicket.IsSet())
			{
				return Begin;
			}
			return Run->SettleEncounterNodeActivity(
				Begin.NodeActivityTicket.GetValue(),
				Packet);
		}
	};

	int32 CountCardInOwnedZones(const URunSession* Run, const UCardDefinition* Card)
	{
		int32 Count = 0;
		const FRunState& State = Run->GetRunState();

		auto CountInPile = [Card, &Count](const TArray<FCardInstance>& Pile)
		{
			for (const FCardInstance& Instance : Pile)
			{
				if (Instance.Definition == Card)
				{
					++Count;
				}
			}
		};

		CountInPile(State.Backpack);
		CountInPile(State.BattleDeck);
		CountInPile(State.BurdenZone);
		for (const FSpecialZone& SpecialZone : State.SpecialZones)
		{
			CountInPile(SpecialZone.Cards);
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunResultNoTriggerWrapperDeprecatedSpec,
	"Wacom.Run.Result.NoTriggerWrapperDeprecated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunResultNoTriggerWrapperDeprecatedSpec::RunTest(const FString& /*Parameters*/)
{
	TestNull(TEXT("No-trigger settlement wrapper is removed"),
		URunSession::StaticClass()->FindFunctionByName(TEXT("OnBattleFinished")));
	TestNull(TEXT("Trigger-id settlement wrapper is removed"),
		URunSession::StaticClass()->FindFunctionByName(TEXT("OnBattleFinishedFromTrigger")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunResultFatigueOnEveryBattleSpec,
	"Wacom.Run.Result.FatigueOnEveryBattle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunResultFatigueOnEveryBattleSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	FRunResultFixture Fixture(Fx);
	URunSession* Run = Fixture.Run;

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	Packet.bWithdrawn = true;

	TestEqual(TEXT("Initial Fatigue=0"),
		Run->GetPressureValue(EWacomPressureType::Fatigue), 0);

	TestTrue(TEXT("Withdraw settlement succeeds"), Fixture.Settle(Packet).IsOk());

	TestEqual(TEXT("Fatigue +1 after victory"),
		Run->GetPressureValue(EWacomPressureType::Fatigue), 1);
	TestEqual(TEXT("Wound unchanged"),
		Run->GetPressureValue(EWacomPressureType::Wound), 0);
	// 失败也加疲劳。
	FBattleResultPacket DefeatPacket;
	DefeatPacket.Outcome = EBattleOutcome::Defeat;
	TestTrue(TEXT("Defeat settlement succeeds"), Fixture.Settle(DefeatPacket).IsOk());
	TestEqual(TEXT("Fatigue +1 after defeat"),
		Run->GetPressureValue(EWacomPressureType::Fatigue), 2);
	TestFalse(TEXT("bRunActive false after defeat"), Run->IsRunActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunResultHighHpThresholdAddsWoundSpec,
	"Wacom.Run.Result.HighHpThresholdAddsWound1",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunResultHighHpThresholdAddsWoundSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	FRunResultFixture Fixture(Fx);
	URunSession* Run = Fixture.Run;

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	Packet.bCrossedHighHpThreshold = true;

	TestTrue(TEXT("Battle settlement succeeds"), Fixture.Settle(Packet).IsOk());
	TestEqual(TEXT("Wound +1 from HighHpThreshold"),
		Run->GetPressureValue(EWacomPressureType::Wound), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunResultLowHpThresholdAddsWoundSpec,
	"Wacom.Run.Result.LowHpThresholdAddsWound5",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunResultLowHpThresholdAddsWoundSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	FRunResultFixture Fixture(Fx);
	URunSession* Run = Fixture.Run;

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	Packet.bCrossedLowHpThreshold = true;

	TestTrue(TEXT("Battle settlement succeeds"), Fixture.Settle(Packet).IsOk());
	TestEqual(TEXT("Wound +5 from LowHpThreshold"),
		Run->GetPressureValue(EWacomPressureType::Wound), 5);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunResultMutualDestructionAddsWoundSpec,
	"Wacom.Run.Result.MutualDestructionAddsWound10",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunResultMutualDestructionAddsWoundSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	FRunResultFixture Fixture(Fx);
	URunSession* Run = Fixture.Run;

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	Packet.bMutualDestruction = true;

	TestTrue(TEXT("bRunActive=true initially"), Run->IsRunActive());

	TestTrue(TEXT("Battle settlement succeeds"), Fixture.Settle(Packet).IsOk());

	TestEqual(TEXT("Wound +10 from MutualDestruction"),
		Run->GetPressureValue(EWacomPressureType::Wound), 10);
	// GDD §9.2：同归于尽不触发战外失败。
	TestTrue(TEXT("bRunActive remains true after mutual destruction"),
		Run->IsRunActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunResultAllFlagsAccumulateSpec,
	"Wacom.Run.Result.AllFlagsAccumulate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunResultAllFlagsAccumulateSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	FRunResultFixture Fixture(Fx);
	URunSession* Run = Fixture.Run;

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	Packet.bCrossedHighHpThreshold = true;
	Packet.bCrossedLowHpThreshold = true;
	Packet.bMutualDestruction = true;

	TestTrue(TEXT("Battle settlement succeeds"), Fixture.Settle(Packet).IsOk());

	TestEqual(TEXT("Wound +1+5+10=16"),
		Run->GetPressureValue(EWacomPressureType::Wound), 16);
	TestEqual(TEXT("Fatigue +1"),
		Run->GetPressureValue(EWacomPressureType::Fatigue), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunResultUndeterminedSkipsAccumulationSpec,
	"Wacom.Run.Result.UndeterminedSkipsAccumulation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunResultUndeterminedSkipsAccumulationSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	FRunResultFixture Fixture(Fx);
	URunSession* Run = Fixture.Run;

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Undetermined;
	// 即使 flag 都 true，Undetermined 也不应结算压力。
	Packet.bCrossedHighHpThreshold = true;
	Packet.bMutualDestruction = true;

	TestFalse(TEXT("Undetermined settlement is rejected"), Fixture.Settle(Packet).IsOk());

	TestEqual(TEXT("Fatigue unchanged on Undetermined"),
		Run->GetPressureValue(EWacomPressureType::Fatigue), 0);
	TestEqual(TEXT("Wound unchanged on Undetermined"),
		Run->GetPressureValue(EWacomPressureType::Wound), 0);
	TestTrue(TEXT("bRunActive unchanged on Undetermined"), Run->IsRunActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunBattleRewardCardsAddedToBackpackSpec,
	"Wacom.Run.BattleRewardCardsAddedToBackpack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunBattleRewardCardsAddedToBackpackSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(10, 1);

	{
		FRunResultFixture Fixture(Fx);
		URunSession* Run = Fixture.Run;
		UCardDefinition* RewardCard = Fx.MakeNoopCard(/*Cost*/0);

		FBattleResultPacket Packet;
		Packet.Outcome = EBattleOutcome::Victory;
		FBattleGainedCard GainedCard;
		GainedCard.Definition = RewardCard;
		GainedCard.SourcePartKey = FBattleEnemyPartKey::Make(TEXT("Encounter"), TEXT("Enemy"), TEXT("Test.Part.Solo"));
		GainedCard.SourceChoice = EKnockdownChoice::Aid;
		Packet.GainedCards.Add(GainedCard);

		const int32 Before = CountCardInOwnedZones(Run, RewardCard);
		TestTrue(TEXT("Victory settlement succeeds"), Fixture.Settle(Packet).IsOk());
		TestEqual(TEXT("Victory settles gained reward card to Run ownership"),
			CountCardInOwnedZones(Run, RewardCard),
			Before + 1);
	}

	{
		FRunResultFixture Fixture(Fx);
		URunSession* Run = Fixture.Run;
		UCardDefinition* RewardCard = Fx.MakeNoopCard(/*Cost*/0);

		FBattleResultPacket Packet;
		Packet.Outcome = EBattleOutcome::Victory;
		Packet.bWithdrawn = true;
		FBattleGainedCard GainedCard;
		GainedCard.Definition = RewardCard;
		GainedCard.SourcePartKey = FBattleEnemyPartKey::Make(TEXT("Encounter"), TEXT("Enemy"), TEXT("Test.Part.Solo"));
		GainedCard.SourceChoice = EKnockdownChoice::Destroy;
		Packet.GainedCards.Add(GainedCard);

		const int32 Before = CountCardInOwnedZones(Run, RewardCard);
		TestTrue(TEXT("Withdraw settlement succeeds"), Fixture.Settle(Packet).IsOk());
		TestEqual(TEXT("Withdraw victory still settles already gained reward card"),
			CountCardInOwnedZones(Run, RewardCard),
			Before + 1);
	}

	{
		FRunResultFixture Fixture(Fx);
		URunSession* Run = Fixture.Run;
		UCardDefinition* RewardCard = Fx.MakeNoopCard(/*Cost*/0);

		FBattleResultPacket Packet;
		Packet.Outcome = EBattleOutcome::Defeat;
		FBattleGainedCard GainedCard;
		GainedCard.Definition = RewardCard;
		GainedCard.SourcePartKey = FBattleEnemyPartKey::Make(TEXT("Encounter"), TEXT("Enemy"), TEXT("Test.Part.Solo"));
		GainedCard.SourceChoice = EKnockdownChoice::Aid;
		Packet.GainedCards.Add(GainedCard);

		const int32 Before = CountCardInOwnedZones(Run, RewardCard);
		TestTrue(TEXT("Defeat settlement succeeds"), Fixture.Settle(Packet).IsOk());
		TestEqual(TEXT("Defeat does not settle gained reward card"),
			CountCardInOwnedZones(Run, RewardCard),
			Before);
	}

	return true;
}
