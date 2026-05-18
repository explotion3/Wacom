// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "RunSession.h"
#include "RunState.h"
#include "RunStateTypes.h"

#include "Characters/CharacterDefinition.h"
#include "Cards/CardDefinition.h"

#include "UObject/StrongObjectPtr.h"

/**
 * Stage 1.1：FRunState 战外状态容器测试。
 *
 * 覆盖：
 *   - Initialize 时 FingerCount / HpPerFinger 从 Character 读取
 *   - Initialize 时 StarterDeck 同步到 Backpack；非容器卡进 BattleDeck（Stage 4.1 a2 规则）
 *   - Initialize 时时段重置为 Morning + 初始节点
 *   - RemoveFinger 同步增加 Disability 压力
 *   - 手指掉光触发 IsRunFailed
 *   - 压力 8 条加和 ≥ 100% 触发 IsRunFailed
 *   - AddPressure clamp 到 [0, 100]
 *   - AddExperience 满 Capacity 入账技能并清零
 *   - ConsumeNode 自动推进时段
 *   - AdvanceToNextPhase 五时段循环 + 跨日 +1
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunStateInitializeSpec,
	"Wacom.Run.State.InitializePopulatesFromCharacter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunStateInitializeSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(1);
	UCardDefinition* RH = Fx.MakeNoopCard(1);
	// 加一张 Capacity=10 的容器卡，让 FluxCapacity 足够装下 Deck 不超容（避免初始负重压力）。
	UCardDefinition* BagCard = Fx.MakeNoopCard(0);
	BagCard->Physique.Capacity = 10;
	TArray<UCardDefinition*> Deck = { Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), BagCard };
	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Initialize succeeds"), Run->Initialize(Char));

	const FRunState& State = Run->GetRunState();
	TestEqual(TEXT("FingerCount from character"), State.FingerCount, Char->FingerCount);
	TestEqual(TEXT("HpPerFinger from character"), State.HpPerFinger, Char->HpPerFinger);

	TestEqual(TEXT("Backpack contains only container card"), State.Backpack.Num(), 1);
	// Stage 4.1 a2：非容器卡进 BattleDeck，容器卡只进 Backpack。一张卡同时只能在一个区。
	// 4 张 deck 中 3 张普通 + 1 张 BagCard 容器卡 → BattleDeck 应只含 3 张普通；Backpack 只含 1 张 Bag。
	TestEqual(TEXT("BattleDeck contains only non-container cards"),
		State.BattleDeck.Num(), Deck.Num() - 1);

	TestEqual(TEXT("Initial day = 1"), State.CurrentDayNumber, 1);
	TestTrue(TEXT("Initial phase = Morning"),
		State.CurrentTimePhase == ETimePhase::Morning);
	TestEqual(TEXT("Initial node count = Morning init"),
		State.RemainingNodeCount, State.InitialNodeCount_Morning);

	TestEqual(TEXT("Initial pressure total = 0"), State.Pressure.GetTotal(), 0);
	TestEqual(TEXT("Initial exp = 0"), State.ExperienceCurrent, 0);
	TestEqual(TEXT("Initial skill count = 0"), State.AcquiredSkills.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunStateRemoveFingerSpec,
	"Wacom.Run.State.RemoveFingerAddsDisability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunStateRemoveFingerSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Fx.MakeNoopCard(0) });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	const int32 InitialFingers = Run->GetFingerCount();
	TestEqual(TEXT("Initial Disability = 0"),
		Run->GetPressureValue(EWacomPressureType::Disability), 0);

	Run->RemoveFinger(1);
	TestEqual(TEXT("Finger -1"), Run->GetFingerCount(), InitialFingers - 1);
	TestEqual(TEXT("Disability +5"),
		Run->GetPressureValue(EWacomPressureType::Disability), 5);

	Run->RemoveFinger(2);
	TestEqual(TEXT("Finger -3 total"), Run->GetFingerCount(), InitialFingers - 3);
	TestEqual(TEXT("Disability +15 total"),
		Run->GetPressureValue(EWacomPressureType::Disability), 15);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunStateFingerDepletedFailsRunSpec,
	"Wacom.Run.State.FingerZeroFailsRun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunStateFingerDepletedFailsRunSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Fx.MakeNoopCard(0) });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestFalse(TEXT("Run not failed initially"), Run->IsRunFailed());
	TestFalse(TEXT("Fingers not depleted initially"), Run->IsFingerDepleted());

	// 全部失去（默认 10 指）。
	Run->RemoveFinger(Char->FingerCount);

	TestTrue(TEXT("Fingers depleted"), Run->IsFingerDepleted());
	TestTrue(TEXT("Run failed by finger zero"), Run->IsRunFailed());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunStatePressureCapFailsRunSpec,
	"Wacom.Run.State.PressureCapFailsRun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunStatePressureCapFailsRunSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	// 给一张 Capacity=10 容器卡，让初始 Burden=0（避免初始负重影响压力总和）。
	UCardDefinition* BagCard = Fx.MakeNoopCard(0);
	BagCard->Physique.Capacity = 10;
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Fx.MakeNoopCard(0), BagCard });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestFalse(TEXT("Run not failed initially"), Run->IsRunFailed());
	TestEqual(TEXT("Initial Burden=0"),
		Run->GetPressureValue(EWacomPressureType::Burden), 0);

	// 八条平均堆到 100 总和。
	Run->AddPressure(EWacomPressureType::Hunger,    20);
	Run->AddPressure(EWacomPressureType::Wound,     20);
	Run->AddPressure(EWacomPressureType::Fatigue,   20);
	Run->AddPressure(EWacomPressureType::Burden,    20);
	Run->AddPressure(EWacomPressureType::Decay,     10);
	Run->AddPressure(EWacomPressureType::Misdeed,   10);

	TestEqual(TEXT("Pressure total = 100"), Run->GetTotalPressure(), 100);
	TestTrue(TEXT("Pressure cap reached"), Run->IsPressureCapReached());
	TestTrue(TEXT("Run failed by pressure cap"), Run->IsRunFailed());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunStatePressureClampSpec,
	"Wacom.Run.State.PressureClampedToZeroAndHundred",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunStatePressureClampSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Fx.MakeNoopCard(0) });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	// 上限 clamp 到 100
	Run->AddPressure(EWacomPressureType::Wound, 200);
	TestEqual(TEXT("Wound clamped to 100"),
		Run->GetPressureValue(EWacomPressureType::Wound), 100);

	// 下限 clamp 到 0（负数减不到 0 以下）
	Run->AddPressure(EWacomPressureType::Wound, -500);
	TestEqual(TEXT("Wound clamped to 0"),
		Run->GetPressureValue(EWacomPressureType::Wound), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunStateExperienceGrantsSkillSpec,
	"Wacom.Run.State.ExperienceFullGrantsSkill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunStateExperienceGrantsSkillSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Fx.MakeNoopCard(0) });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	const int32 Cap = Run->GetExperienceCapacity();
	TestTrue(TEXT("Capacity > 0"), Cap > 0);

	// 一次满经验 → 1 个技能 + 经验清零。
	Run->AddExperience(Cap);
	TestEqual(TEXT("Skill count +1"), Run->GetAcquiredSkillCount(), 1);
	TestEqual(TEXT("Experience reset"), Run->GetExperienceCurrent(), 0);

	// 一次溢出（2 倍 cap + 余 3）→ 2 个技能 + 余 3。
	Run->AddExperience(Cap * 2 + 3);
	TestEqual(TEXT("Skill count +3 total"), Run->GetAcquiredSkillCount(), 3);
	TestEqual(TEXT("Experience remainder = 3"), Run->GetExperienceCurrent(), 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunStateConsumeNodeAdvancesPhaseSpec,
	"Wacom.Run.State.ConsumeNodeAdvancesPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunStateConsumeNodeAdvancesPhaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Fx.MakeNoopCard(0) });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	const FRunState& State = Run->GetRunState();
	TestTrue(TEXT("Start at Morning"),
		State.CurrentTimePhase == ETimePhase::Morning);
	const int32 MorningNodes = State.InitialNodeCount_Morning;
	TestEqual(TEXT("Initial Morning nodes"), State.RemainingNodeCount, MorningNodes);

	// 消耗一个节点：仍在 Morning。
	Run->ConsumeNode(1);
	TestTrue(TEXT("Still Morning after consuming 1"),
		State.CurrentTimePhase == ETimePhase::Morning);
	TestEqual(TEXT("Remaining nodes = MorningNodes - 1"),
		State.RemainingNodeCount, MorningNodes - 1);

	// 把剩下的节点全部消耗：自动推进到 Day。
	Run->ConsumeNode(MorningNodes - 1);
	TestTrue(TEXT("Advanced to Day"),
		State.CurrentTimePhase == ETimePhase::Day);
	TestEqual(TEXT("Day initial nodes"),
		State.RemainingNodeCount, State.InitialNodeCount_Day);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunStatePhaseCycleAdvancesDaySpec,
	"Wacom.Run.State.PhaseCycleAdvancesDay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunStatePhaseCycleAdvancesDaySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Fx.MakeNoopCard(0) });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	const FRunState& State = Run->GetRunState();
	TestEqual(TEXT("Start day = 1"), State.CurrentDayNumber, 1);

	// 五次推进：Morning → Day → Dusk → Night → Sunrise → Morning（次日）。
	Run->AdvanceToNextPhase();
	TestTrue(TEXT("Now Day"),     State.CurrentTimePhase == ETimePhase::Day);
	Run->AdvanceToNextPhase();
	TestTrue(TEXT("Now Dusk"),    State.CurrentTimePhase == ETimePhase::Dusk);
	Run->AdvanceToNextPhase();
	TestTrue(TEXT("Now Night"),   State.CurrentTimePhase == ETimePhase::Night);
	Run->AdvanceToNextPhase();
	TestTrue(TEXT("Now Sunrise"), State.CurrentTimePhase == ETimePhase::Sunrise);
	TestEqual(TEXT("Still day 1 at Sunrise"), State.CurrentDayNumber, 1);

	Run->AdvanceToNextPhase();
	TestTrue(TEXT("Back to Morning"),
		State.CurrentTimePhase == ETimePhase::Morning);
	TestEqual(TEXT("Day = 2"), State.CurrentDayNumber, 2);

	return true;
}
