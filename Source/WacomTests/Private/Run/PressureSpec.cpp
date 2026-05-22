// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "RunSession.h"
#include "RunState.h"
#include "RunStateTypes.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"

#include "UObject/StrongObjectPtr.h"

/**
 * Stage 2：压力系统 trigger 完整接入测试。
 *
 * 覆盖：
 *   - 时段定时触发（饥饿 / 疲劳 / 腐朽）
 *   - 战外行为触发（伤口 / 嗜血 / 劣迹）
 *   - 负重幂等重算
 *   - 压力减少 / 归零
 */

namespace
{
	URunSession* MakePressureRunWithCharacter(FWacomBattleFixture& Fx, TStrongObjectPtr<URunSession>& RunPtr)
	{
		UCharacterDefinition* Char = Fx.MakeCharacter(
			Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
			{ Fx.MakeNoopCard(0) });

		RunPtr = TStrongObjectPtr<URunSession>(NewObject<URunSession>());
		RunPtr->Initialize(Char);
		return RunPtr.Get();
	}
}

// ================ 时段定时触发 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPressureMorningAtInitDoesNotTriggerSpec,
	"Wacom.Run.Pressure.MorningAtInitDoesNotTrigger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPressureMorningAtInitDoesNotTriggerSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	TStrongObjectPtr<URunSession> RunPtr;
	URunSession* Run = MakePressureRunWithCharacter(Fx, RunPtr);

	// Initialize 后应在 Morning，但不应触发饥饿（Initialize 不算"进入清晨"）。
	TestEqual(TEXT("Hunger=0 at start"),
		Run->GetPressureValue(EWacomPressureType::Hunger), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPressureEnterDuskAddsHungerSpec,
	"Wacom.Run.Pressure.EnterDuskAddsHunger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPressureEnterDuskAddsHungerSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	TStrongObjectPtr<URunSession> RunPtr;
	URunSession* Run = MakePressureRunWithCharacter(Fx, RunPtr);

	// Morning → Day（无副作用）→ Dusk（饥饿 +5）
	Run->AdvanceToNextPhase();  // Day
	TestEqual(TEXT("Hunger=0 entering Day"),
		Run->GetPressureValue(EWacomPressureType::Hunger), 0);

	Run->AdvanceToNextPhase();  // Dusk
	TestEqual(TEXT("Hunger +5 entering Dusk"),
		Run->GetPressureValue(EWacomPressureType::Hunger), 5);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPressureEnterSunriseAddsFatigueSpec,
	"Wacom.Run.Pressure.EnterSunriseAddsFatigue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPressureEnterSunriseAddsFatigueSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	TStrongObjectPtr<URunSession> RunPtr;
	URunSession* Run = MakePressureRunWithCharacter(Fx, RunPtr);

	// Morning → Day → Dusk → Night → Sunrise
	Run->AdvanceToNextPhase();  // Day
	Run->AdvanceToNextPhase();  // Dusk
	Run->AdvanceToNextPhase();  // Night
	TestEqual(TEXT("Fatigue=0 before Sunrise"),
		Run->GetPressureValue(EWacomPressureType::Fatigue), 0);

	Run->AdvanceToNextPhase();  // Sunrise
	TestEqual(TEXT("Fatigue +10 entering Sunrise"),
		Run->GetPressureValue(EWacomPressureType::Fatigue), 10);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPressureCompletingDayAddsDecayAndHungerSpec,
	"Wacom.Run.Pressure.CompletingDayAddsDecayAndHunger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPressureCompletingDayAddsDecayAndHungerSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	TStrongObjectPtr<URunSession> RunPtr;
	URunSession* Run = MakePressureRunWithCharacter(Fx, RunPtr);

	// 完整循环到次日 Morning
	Run->AdvanceToNextPhase();  // Day
	Run->AdvanceToNextPhase();  // Dusk         (Hunger +5)
	Run->AdvanceToNextPhase();  // Night
	Run->AdvanceToNextPhase();  // Sunrise      (Fatigue +10)
	Run->AdvanceToNextPhase();  // Morning(Day=2) (Hunger +5, Decay +5)

	const FRunState& State = Run->GetRunState();
	TestEqual(TEXT("CurrentDayNumber=2"), State.CurrentDayNumber, 2);
	TestTrue(TEXT("Phase=Morning"),
		State.CurrentTimePhase == ETimePhase::Morning);

	TestEqual(TEXT("Hunger 5+5=10"),
		Run->GetPressureValue(EWacomPressureType::Hunger), 10);
	TestEqual(TEXT("Fatigue 10"),
		Run->GetPressureValue(EWacomPressureType::Fatigue), 10);
	TestEqual(TEXT("Decay 5 from completing day"),
		Run->GetPressureValue(EWacomPressureType::Decay), 5);

	return true;
}

// ================ 战外行为触发 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPressureRightHandActionAddsWoundSpec,
	"Wacom.Run.Pressure.RightHandActionAddsWound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPressureRightHandActionAddsWoundSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	TStrongObjectPtr<URunSession> RunPtr;
	URunSession* Run = MakePressureRunWithCharacter(Fx, RunPtr);

	Run->OnRightHandDestructiveAction();
	TestEqual(TEXT("Wound +1"),
		Run->GetPressureValue(EWacomPressureType::Wound), 1);

	Run->OnRightHandDestructiveAction();
	Run->OnRightHandDestructiveAction();
	TestEqual(TEXT("Wound 3 after 3 actions"),
		Run->GetPressureValue(EWacomPressureType::Wound), 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPressureCompanionDestroyedAddsBloodlustSpec,
	"Wacom.Run.Pressure.CompanionDestroyedAddsBloodlust",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPressureCompanionDestroyedAddsBloodlustSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	TStrongObjectPtr<URunSession> RunPtr;
	URunSession* Run = MakePressureRunWithCharacter(Fx, RunPtr);

	Run->OnCompanionCardPermanentlyDestroyed();
	TestEqual(TEXT("Bloodlust +1"),
		Run->GetPressureValue(EWacomPressureType::Bloodlust), 1);

	Run->OnCompanionCardPermanentlyDestroyed();
	TestEqual(TEXT("Bloodlust 2"),
		Run->GetPressureValue(EWacomPressureType::Bloodlust), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPressureTheftAccumulatesSpec,
	"Wacom.Run.Pressure.TheftAccumulatesByFormula",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPressureTheftAccumulatesSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	TStrongObjectPtr<URunSession> RunPtr;
	URunSession* Run = MakePressureRunWithCharacter(Fx, RunPtr);

	// b 增量语义：第 n 次 +(n*(n+1)/2 + 1)
	// n=1 → +2     total 2
	// n=2 → +4     total 6
	// n=3 → +7     total 13
	// n=4 → +11    total 24

	Run->OnTheftCommitted();
	TestEqual(TEXT("After theft #1 → 2"),
		Run->GetPressureValue(EWacomPressureType::Misdeed), 2);

	Run->OnTheftCommitted();
	TestEqual(TEXT("After theft #2 → 6"),
		Run->GetPressureValue(EWacomPressureType::Misdeed), 6);

	Run->OnTheftCommitted();
	TestEqual(TEXT("After theft #3 → 13"),
		Run->GetPressureValue(EWacomPressureType::Misdeed), 13);

	Run->OnTheftCommitted();
	TestEqual(TEXT("After theft #4 → 24"),
		Run->GetPressureValue(EWacomPressureType::Misdeed), 24);

	TestEqual(TEXT("TheftCount=4"), Run->GetRunState().TheftCount, 4);

	return true;
}

// ================ 负重重算 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPressureBurdenZeroWhenWithinCapacitySpec,
	"Wacom.Run.Pressure.BurdenZeroWhenWithinCapacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPressureBurdenZeroWhenWithinCapacitySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	// 显式给一张 Capacity 大于 fixture 默认 deck 大小的容器卡，让初始 Burden=0。
	UCardDefinition* BagCard = Fx.MakeNoopCard(0);
	BagCard->Physique.Capacity = 12;
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Fx.MakeNoopCard(0), BagCard });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	// Initialize 自动 RecomputeBurden，应仍是 0（容器 Capacity=12 > 背包卡数 2）。
	TestEqual(TEXT("Burden=0 within capacity"),
		Run->GetPressureValue(EWacomPressureType::Burden), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPressureBurdenSetByOverCountSpec,
	"Wacom.Run.Pressure.BurdenSetByOverCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPressureBurdenSetByOverCountSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	// 构造一个 Capacity=12 的容器卡，加进 StarterDeck → Initialize 后进 Backpack
	// （非容器卡进 BattleDeck，容器卡只进 Backpack）。
	UCardDefinition* BagCard = Fx.MakeNoopCard(/*Cost*/0);
	BagCard->Physique.Capacity = 12;

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ BagCard });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	FRunState& State = Run->GetMutableRunStateForAutomationTest();
	const int32 Capacity = Run->GetFluxCapacity();
	TestEqual(TEXT("FluxCapacity=12 from container card capacity"), Capacity, 12);

	// Stage 4.5.0：zone 元素是 FCardInstance；测试中按 Definition 包成 instance 追加。
	auto MakeInst = [](UCardDefinition* Def) -> FCardInstance
	{
		FCardInstance Inst;
		Inst.Definition = Def;
		return Inst;
	};

	// 加普通卡到刚好通量内容 Capacity。Backpack 里已有 1 张 A 容器，它也占内容格。
	while (State.Backpack.Num() < Capacity)
	{
		State.Backpack.Add(MakeInst(Fx.MakeNoopCard(0)));
	}
	while (State.BattleDeck.Num() < Run->GetBattleDeckCapacity())
	{
		State.BattleDeck.Add(MakeInst(Fx.MakeNoopCard(0)));
	}
	Run->RecomputeBurden();
	TestEqual(TEXT("Burden=0 at capacity"),
		Run->GetPressureValue(EWacomPressureType::Burden), 0);

	// 超 3 张 → n=3 → 3*4/2 = 6
	for (int32 i = 0; i < 3; ++i) { State.Backpack.Add(MakeInst(Fx.MakeNoopCard(0))); }
	Run->RecomputeBurden();
	TestEqual(TEXT("Burden=6 over 3"),
		Run->GetPressureValue(EWacomPressureType::Burden), 6);

	// 再加到超 5 张 → n=5 → 5*6/2 = 15（覆盖语义，不是 6+15）
	for (int32 i = 0; i < 2; ++i) { State.Backpack.Add(MakeInst(Fx.MakeNoopCard(0))); }
	Run->RecomputeBurden();
	TestEqual(TEXT("Burden=15 over 5 (覆盖)"),
		Run->GetPressureValue(EWacomPressureType::Burden), 15);

	// 腾出 5 个通量空位，BurdenZone 头部应全部回填 → 0
	for (int32 i = 0; i < 5 && State.Backpack.Num() > 0; ++i)
	{
		State.Backpack.Pop();
	}
	Run->RecomputeBurden();
	TestEqual(TEXT("Burden=0 after reducing"),
		Run->GetPressureValue(EWacomPressureType::Burden), 0);

	return true;
}

// ================ 减少 / 归零 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPressureRemovePressureClampsSpec,
	"Wacom.Run.Pressure.RemovePressureClampsAtZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPressureRemovePressureClampsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	TStrongObjectPtr<URunSession> RunPtr;
	URunSession* Run = MakePressureRunWithCharacter(Fx, RunPtr);

	Run->AddPressure(EWacomPressureType::Wound, 5);
	TestEqual(TEXT("Wound=5"),
		Run->GetPressureValue(EWacomPressureType::Wound), 5);

	Run->RemovePressure(EWacomPressureType::Wound, 3);
	TestEqual(TEXT("Wound=2 after -3"),
		Run->GetPressureValue(EWacomPressureType::Wound), 2);

	// 减 100 应被 clamp 到 0
	Run->RemovePressure(EWacomPressureType::Wound, 100);
	TestEqual(TEXT("Wound=0 clamped"),
		Run->GetPressureValue(EWacomPressureType::Wound), 0);

	// 负 / 0 amount 不影响
	Run->AddPressure(EWacomPressureType::Wound, 5);
	Run->RemovePressure(EWacomPressureType::Wound, -10);
	TestEqual(TEXT("Wound=5 unchanged on negative amount"),
		Run->GetPressureValue(EWacomPressureType::Wound), 5);
	Run->RemovePressure(EWacomPressureType::Wound, 0);
	TestEqual(TEXT("Wound=5 unchanged on zero amount"),
		Run->GetPressureValue(EWacomPressureType::Wound), 5);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPressureClearPressureSpec,
	"Wacom.Run.Pressure.ClearPressureSetsZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPressureClearPressureSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	TStrongObjectPtr<URunSession> RunPtr;
	URunSession* Run = MakePressureRunWithCharacter(Fx, RunPtr);

	Run->AddPressure(EWacomPressureType::Wound, 50);
	Run->AddPressure(EWacomPressureType::Hunger, 50);

	Run->ClearPressure(EWacomPressureType::Wound);
	TestEqual(TEXT("Wound cleared"),
		Run->GetPressureValue(EWacomPressureType::Wound), 0);
	// 其他不受影响
	TestEqual(TEXT("Hunger unchanged"),
		Run->GetPressureValue(EWacomPressureType::Hunger), 50);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPressureSetPressureClampsSpec,
	"Wacom.Run.Pressure.SetPressureClamps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPressureSetPressureClampsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	TStrongObjectPtr<URunSession> RunPtr;
	URunSession* Run = MakePressureRunWithCharacter(Fx, RunPtr);

	Run->SetPressure(EWacomPressureType::Wound, 200);
	TestEqual(TEXT("Wound clamped to 100"),
		Run->GetPressureValue(EWacomPressureType::Wound), 100);

	Run->SetPressure(EWacomPressureType::Wound, -50);
	TestEqual(TEXT("Wound clamped to 0 from negative"),
		Run->GetPressureValue(EWacomPressureType::Wound), 0);

	return true;
}
