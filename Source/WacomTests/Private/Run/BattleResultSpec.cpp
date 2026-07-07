// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "RunSession.h"
#include "RunStateTypes.h"
#include "Runtime/BattleEnemyKeys.h"
#include "Session/BattleResultPacket.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"

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
	URunSession* MakeRunWithCharacter(FWacomBattleFixture& Fx, TStrongObjectPtr<URunSession>& RunPtr)
	{
		UCharacterDefinition* Char = Fx.MakeCharacter(
			Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
			{ Fx.MakeNoopCard(0) });

		RunPtr = TStrongObjectPtr<URunSession>(NewObject<URunSession>());
		RunPtr->Initialize(Char);
		return RunPtr.Get();
	}

	void FinishBattleForTest(URunSession* Run, const FBattleResultPacket& Packet, const TCHAR* TriggerPersistentId)
	{
		Run->OnBattleFinishedFromTrigger(Packet, FName(TriggerPersistentId));
	}

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
	UFunction* Function = URunSession::StaticClass()->FindFunctionByName(
		GET_FUNCTION_NAME_CHECKED(URunSession, OnBattleFinished));
	if (!TestNotNull(TEXT("OnBattleFinished function exists"), Function))
	{
		return false;
	}

	TestTrue(TEXT("No-trigger wrapper is deprecated for Blueprint callers"),
		Function->HasMetaData(TEXT("DeprecatedFunction")));
	TestTrue(TEXT("Deprecation message points to trigger-aware entry"),
		Function->GetMetaData(TEXT("DeprecationMessage")).Contains(TEXT("OnBattleFinishedFromTrigger")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunResultFatigueOnEveryBattleSpec,
	"Wacom.Run.Result.FatigueOnEveryBattle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunResultFatigueOnEveryBattleSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	TStrongObjectPtr<URunSession> RunPtr;
	URunSession* Run = MakeRunWithCharacter(Fx, RunPtr);

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;

	TestEqual(TEXT("Initial Fatigue=0"),
		Run->GetPressureValue(EWacomPressureType::Fatigue), 0);

	FinishBattleForTest(Run, Packet, TEXT("Run.Result.Fatigue.Victory"));

	TestEqual(TEXT("Fatigue +1 after victory"),
		Run->GetPressureValue(EWacomPressureType::Fatigue), 1);
	TestEqual(TEXT("Wound unchanged"),
		Run->GetPressureValue(EWacomPressureType::Wound), 0);
	// 失败也加疲劳。
	FBattleResultPacket DefeatPacket;
	DefeatPacket.Outcome = EBattleOutcome::Defeat;
	FinishBattleForTest(Run, DefeatPacket, TEXT("Run.Result.Fatigue.Defeat"));
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
	TStrongObjectPtr<URunSession> RunPtr;
	URunSession* Run = MakeRunWithCharacter(Fx, RunPtr);

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	Packet.bCrossedHighHpThreshold = true;

	FinishBattleForTest(Run, Packet, TEXT("Run.Result.HighHpThreshold"));
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
	TStrongObjectPtr<URunSession> RunPtr;
	URunSession* Run = MakeRunWithCharacter(Fx, RunPtr);

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	Packet.bCrossedLowHpThreshold = true;

	FinishBattleForTest(Run, Packet, TEXT("Run.Result.LowHpThreshold"));
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
	TStrongObjectPtr<URunSession> RunPtr;
	URunSession* Run = MakeRunWithCharacter(Fx, RunPtr);

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	Packet.bMutualDestruction = true;

	TestTrue(TEXT("bRunActive=true initially"), Run->IsRunActive());

	FinishBattleForTest(Run, Packet, TEXT("Run.Result.MutualDestruction"));

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
	TStrongObjectPtr<URunSession> RunPtr;
	URunSession* Run = MakeRunWithCharacter(Fx, RunPtr);

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	Packet.bCrossedHighHpThreshold = true;
	Packet.bCrossedLowHpThreshold = true;
	Packet.bMutualDestruction = true;

	FinishBattleForTest(Run, Packet, TEXT("Run.Result.AllFlags"));

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
	TStrongObjectPtr<URunSession> RunPtr;
	URunSession* Run = MakeRunWithCharacter(Fx, RunPtr);

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Undetermined;
	// 即使 flag 都 true，Undetermined 也不应结算压力。
	Packet.bCrossedHighHpThreshold = true;
	Packet.bMutualDestruction = true;

	FinishBattleForTest(Run, Packet, TEXT("Run.Result.Undetermined"));

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
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(10, 1, 0);

	{
		TStrongObjectPtr<URunSession> RunPtr;
		URunSession* Run = MakeRunWithCharacter(Fx, RunPtr);
		UCardDefinition* RewardCard = Fx.MakeNoopCard(/*Cost*/0);

		FBattleResultPacket Packet;
		Packet.Outcome = EBattleOutcome::Victory;
		FBattleGainedCard GainedCard;
		GainedCard.Definition = RewardCard;
		GainedCard.SourcePartKey = FBattleEnemyPartKey::Make(TEXT("Encounter"), TEXT("Enemy"), TEXT("Test.Part.Solo"));
		GainedCard.SourceChoice = EKnockdownChoice::Aid;
		Packet.GainedCards.Add(GainedCard);

		const int32 Before = CountCardInOwnedZones(Run, RewardCard);
		FinishBattleForTest(Run, Packet, TEXT("Run.Result.Reward.Victory"));
		TestEqual(TEXT("Victory settles gained reward card to Run ownership"),
			CountCardInOwnedZones(Run, RewardCard),
			Before + 1);
	}

	{
		TStrongObjectPtr<URunSession> RunPtr;
		URunSession* Run = MakeRunWithCharacter(Fx, RunPtr);
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
		FinishBattleForTest(Run, Packet, TEXT("Run.Result.Reward.Withdraw"));
		TestEqual(TEXT("Withdraw victory still settles already gained reward card"),
			CountCardInOwnedZones(Run, RewardCard),
			Before + 1);
	}

	{
		TStrongObjectPtr<URunSession> RunPtr;
		URunSession* Run = MakeRunWithCharacter(Fx, RunPtr);
		UCardDefinition* RewardCard = Fx.MakeNoopCard(/*Cost*/0);

		FBattleResultPacket Packet;
		Packet.Outcome = EBattleOutcome::Defeat;
		FBattleGainedCard GainedCard;
		GainedCard.Definition = RewardCard;
		GainedCard.SourcePartKey = FBattleEnemyPartKey::Make(TEXT("Encounter"), TEXT("Enemy"), TEXT("Test.Part.Solo"));
		GainedCard.SourceChoice = EKnockdownChoice::Aid;
		Packet.GainedCards.Add(GainedCard);

		const int32 Before = CountCardInOwnedZones(Run, RewardCard);
		FinishBattleForTest(Run, Packet, TEXT("Run.Result.Reward.Defeat"));
		TestEqual(TEXT("Defeat does not settle gained reward card"),
			CountCardInOwnedZones(Run, RewardCard),
			Before);
	}

	return true;
}
