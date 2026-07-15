// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "RunSession.h"
#include "RunState.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UWacomRunEventDefinition* MakeChoiceEvaluationEvent(
		UObject* Outer,
		const FWacomRunEventChoiceDefinition& Choice)
	{
		UWacomRunEventDefinition* Event = NewObject<UWacomRunEventDefinition>(Outer);
		Event->EventId = TEXT("Event.ChoiceEvaluation");
		Event->DisplayName = FText::FromString(TEXT("选项求值测试"));
		Event->StartNodeId = TEXT("Start");

		FWacomRunEventNodeDefinition StartNode;
		StartNode.NodeId = Event->StartNodeId;
		StartNode.Choices.Add(Choice);
		Event->Nodes.Add(MoveTemp(StartNode));
		return Event;
	}

	const FRunEventChoiceSnapshot* FindChoiceSnapshot(
		const FRunEventSnapshot& Snapshot,
		FName ChoiceId)
	{
		return Snapshot.Choices.FindByPredicate(
			[ChoiceId](const FRunEventChoiceSnapshot& Choice)
			{
				return Choice.ChoiceId == ChoiceId;
			});
	}

	void AddOwnedBackpackCard(FRunState& State, UCardDefinition* CardDefinition, FGuid& OutInstanceId)
	{
		FCardInstance Instance;
		Instance.InstanceId = FGuid::NewGuid();
		Instance.Definition = CardDefinition;
		OutInstanceId = Instance.InstanceId;
		State.Backpack.Add(MoveTemp(Instance));
	}

	struct FUnsatisfiedConditionCase
	{
		FString Label;
		FWacomRunEventConditionDefinition Condition;
		ERunEventChoiceRequirementKind ExpectedKind = ERunEventChoiceRequirementKind::None;
		FName ExpectedReason = NAME_None;
		TFunction<void(FRunState&)> Arrange;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEventChoiceConditionFactsMatchSubmissionSpec,
	"Wacom.Run.Event.ChoiceEvaluation.ConditionFactsMatchSubmission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEventChoiceConditionFactsMatchSubmissionSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* ProbeCard = Fx.MakeNoopCard(0);
	const FName DependencyEventId = TEXT("Event.ChoiceEvaluation.Dependency");
	const FName ProbeFlagId = TEXT("Run.Flag.ChoiceEvaluation");

	TArray<FUnsatisfiedConditionCase> Cases;
	auto AddCase = [&Cases](
		const TCHAR* Label,
		const FWacomRunEventConditionDefinition& Condition,
		ERunEventChoiceRequirementKind ExpectedKind,
		FName ExpectedReason,
		TFunction<void(FRunState&)> Arrange = nullptr)
	{
		FUnsatisfiedConditionCase& Entry = Cases.AddDefaulted_GetRef();
		Entry.Label = Label;
		Entry.Condition = Condition;
		Entry.ExpectedKind = ExpectedKind;
		Entry.ExpectedReason = ExpectedReason;
		Entry.Arrange = MoveTemp(Arrange);
	};

	{
		FWacomRunEventConditionDefinition Condition;
		Condition.Type = EWacomRunEventConditionType::MinGold;
		Condition.Value = 2;
		AddCase(TEXT("MinGold"), Condition, ERunEventChoiceRequirementKind::MinGold,
			TEXT("InsufficientGold"));
	}
	{
		FWacomRunEventConditionDefinition Condition;
		Condition.Type = EWacomRunEventConditionType::MinActionPoints;
		Condition.Value = 2;
		AddCase(TEXT("MinActionPoints"), Condition, ERunEventChoiceRequirementKind::MinActionPoints,
			TEXT("InsufficientActionPoints"),
			[](FRunState& State) { State.TimeState.RemainingActionPoints = 0; });
	}
	{
		FWacomRunEventConditionDefinition Condition;
		Condition.Type = EWacomRunEventConditionType::MaxPressure;
		Condition.PressureType = TEXT("Misdeed");
		Condition.Value = 0;
		AddCase(TEXT("MaxPressure"), Condition, ERunEventChoiceRequirementKind::MaxPressure,
			TEXT("PressureTooHigh"),
			[](FRunState& State) { State.Pressure.Add(EWacomPressureType::Misdeed, 1); });
	}
	{
		FWacomRunEventConditionDefinition Condition;
		Condition.Type = EWacomRunEventConditionType::HasCard;
		Condition.CardDefinition = ProbeCard;
		AddCase(TEXT("HasCard"), Condition, ERunEventChoiceRequirementKind::HasCard,
			TEXT("MissingRequiredCard"));
	}
	{
		FWacomRunEventConditionDefinition Condition;
		Condition.Type = EWacomRunEventConditionType::MissingCard;
		Condition.CardDefinition = ProbeCard;
		AddCase(TEXT("MissingCard"), Condition, ERunEventChoiceRequirementKind::MissingCard,
			TEXT("AlreadyHasCard"),
			[ProbeCard](FRunState& State)
			{
				FGuid IgnoredId;
				AddOwnedBackpackCard(State, ProbeCard, IgnoredId);
			});
	}
	{
		FWacomRunEventConditionDefinition Condition;
		Condition.Type = EWacomRunEventConditionType::EventCompleted;
		Condition.TargetPersistentId = DependencyEventId;
		AddCase(TEXT("EventCompleted"), Condition, ERunEventChoiceRequirementKind::EventCompleted,
			TEXT("RequiredEventNotCompleted"));
	}
	{
		FWacomRunEventConditionDefinition Condition;
		Condition.Type = EWacomRunEventConditionType::EventNotCompleted;
		Condition.TargetPersistentId = DependencyEventId;
		AddCase(TEXT("EventNotCompleted"), Condition, ERunEventChoiceRequirementKind::EventNotCompleted,
			TEXT("RequiredEventAlreadyCompleted"),
			[DependencyEventId](FRunState& State)
			{
				State.RunEventStates.FindOrAdd(DependencyEventId).bCompleted = true;
			});
	}
	{
		FWacomRunEventConditionDefinition Condition;
		Condition.Type = EWacomRunEventConditionType::RunFlagSet;
		Condition.FlagId = ProbeFlagId;
		AddCase(TEXT("RunFlagSet"), Condition, ERunEventChoiceRequirementKind::RunFlagSet,
			TEXT("RequiredRunFlagMissing"));
	}
	{
		FWacomRunEventConditionDefinition Condition;
		Condition.Type = EWacomRunEventConditionType::RunFlagNotSet;
		Condition.FlagId = ProbeFlagId;
		AddCase(TEXT("RunFlagNotSet"), Condition, ERunEventChoiceRequirementKind::RunFlagNotSet,
			TEXT("BlockedRunFlagSet"),
			[ProbeFlagId](FRunState& State) { State.RunFlags.Add(ProbeFlagId); });
	}

	for (int32 CaseIndex = 0; CaseIndex < Cases.Num(); ++CaseIndex)
	{
		const FUnsatisfiedConditionCase& Case = Cases[CaseIndex];
		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run.Get());
		if (Case.Arrange)
		{
			Case.Arrange(State);
		}

		FWacomRunEventChoiceDefinition Choice;
		Choice.ChoiceId = TEXT("Probe");
		Choice.Conditions.Add(Case.Condition);
		TStrongObjectPtr<UWacomRunEventDefinition> Event(
			MakeChoiceEvaluationEvent(Run.Get(), Choice));
		const FName PersistentId(*FString::Printf(
			TEXT("Event.ChoiceEvaluation.%d"),
			CaseIndex));

		TestTrue(
			*FString::Printf(TEXT("%s event begins"), *Case.Label),
			Run->BeginRunEvent(PersistentId, Event.Get()));
		const FRunEventSnapshot Snapshot = Run->BuildCurrentRunEventSnapshot();
		const FRunEventChoiceSnapshot* ChoiceSnapshot =
			FindChoiceSnapshot(Snapshot, Choice.ChoiceId);
		if (!TestNotNull(
			*FString::Printf(TEXT("%s snapshot choice exists"), *Case.Label),
			ChoiceSnapshot))
		{
			continue;
		}

		TestFalse(
			*FString::Printf(TEXT("%s snapshot is unavailable"), *Case.Label),
			ChoiceSnapshot->bAvailable);
		TestEqual(
			*FString::Printf(TEXT("%s snapshot reason"), *Case.Label),
			ChoiceSnapshot->DisabledReason,
			Case.ExpectedReason);
		TestEqual(
			*FString::Printf(TEXT("%s has one requirement"), *Case.Label),
			ChoiceSnapshot->Requirements.Num(),
			1);
		if (ChoiceSnapshot->Requirements.Num() == 1)
		{
			const FRunEventChoiceRequirementSnapshot& Requirement =
				ChoiceSnapshot->Requirements[0];
			TestEqual(
				*FString::Printf(TEXT("%s requirement kind"), *Case.Label),
				Requirement.Kind,
				Case.ExpectedKind);
			TestFalse(
				*FString::Printf(TEXT("%s requirement is unsatisfied"), *Case.Label),
				Requirement.bSatisfied);
			TestEqual(
				*FString::Printf(TEXT("%s requirement reason"), *Case.Label),
				Requirement.DisabledReason,
				Case.ExpectedReason);
		}

		const FRunEventChoiceResult SubmitResult =
			Run->ChooseRunEventOptionWithResult(Choice.ChoiceId);
		TestFalse(
			*FString::Printf(TEXT("%s submit fails"), *Case.Label),
			SubmitResult.bSucceeded);
		TestEqual(
			*FString::Printf(TEXT("%s submit reason matches snapshot"), *Case.Label),
			SubmitResult.DisabledReason,
			Case.ExpectedReason);
		Run->EndRunEvent();
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEventChoiceEvaluationOrderAndLiveStateSpec,
	"Wacom.Run.Event.ChoiceEvaluation.FirstFailureAndLiveState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEventChoiceEvaluationOrderAndLiveStateSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	const FName FlagId = TEXT("Run.Flag.ChoiceEvaluation.Live");

	FWacomRunEventChoiceDefinition Choice;
	Choice.ChoiceId = TEXT("Probe");
	FWacomRunEventConditionDefinition GoldCondition;
	GoldCondition.Type = EWacomRunEventConditionType::MinGold;
	GoldCondition.Value = 5;
	Choice.Conditions.Add(GoldCondition);
	FWacomRunEventConditionDefinition FlagCondition;
	FlagCondition.Type = EWacomRunEventConditionType::RunFlagSet;
	FlagCondition.FlagId = FlagId;
	Choice.Conditions.Add(FlagCondition);

	TStrongObjectPtr<UWacomRunEventDefinition> Event(
		MakeChoiceEvaluationEvent(Run.Get(), Choice));
	TestTrue(TEXT("Ordered condition event begins"),
		Run->BeginRunEvent(TEXT("Event.ChoiceEvaluation.Order"), Event.Get()));

	FRunEventSnapshot Snapshot = Run->BuildCurrentRunEventSnapshot();
	const FRunEventChoiceSnapshot* ChoiceSnapshot = FindChoiceSnapshot(Snapshot, Choice.ChoiceId);
	if (!TestNotNull(TEXT("Ordered condition snapshot exists"), ChoiceSnapshot))
	{
		return false;
	}
	TestEqual(TEXT("First failed condition owns snapshot reason"),
		ChoiceSnapshot->DisabledReason, FName(TEXT("InsufficientGold")));
	TestEqual(TEXT("All condition facts remain visible"), ChoiceSnapshot->Requirements.Num(), 2);
	if (ChoiceSnapshot->Requirements.Num() == 2)
	{
		TestEqual(TEXT("Second failure fact is still generated"),
			ChoiceSnapshot->Requirements[1].DisabledReason,
			FName(TEXT("RequiredRunFlagMissing")));
	}

	Run->AddGold(5);
	FWacomRunSessionTestAccess::GetMutableRunState(*Run.Get()).RunFlags.Add(FlagId);
	Snapshot = Run->BuildCurrentRunEventSnapshot();
	ChoiceSnapshot = FindChoiceSnapshot(Snapshot, Choice.ChoiceId);
	TestTrue(TEXT("Fresh snapshot becomes available"), ChoiceSnapshot && ChoiceSnapshot->bAvailable);

	TestTrue(TEXT("Gold can change after snapshot"), Run->RemoveGold(5));
	const FRunEventChoiceResult StaleSnapshotSubmit =
		Run->ChooseRunEventOptionWithResult(Choice.ChoiceId);
	TestFalse(TEXT("Submit does not trust stale available snapshot"), StaleSnapshotSubmit.bSucceeded);
	TestEqual(TEXT("Submit recomputes current failure reason"),
		StaleSnapshotSubmit.DisabledReason,
		FName(TEXT("InsufficientGold")));

	Run->AddGold(5);
	const FRunEventChoiceResult FreshSubmit =
		Run->ChooseRunEventOptionWithResult(Choice.ChoiceId);
	TestTrue(TEXT("Submit succeeds after current state satisfies conditions"), FreshSubmit.bSucceeded);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEventChoiceEvaluationPaymentConflictParitySpec,
	"Wacom.Run.Event.ChoiceEvaluation.PaymentConflictMatchesSubmission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEventChoiceEvaluationPaymentConflictParitySpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* PaidCard = Fx.MakeNoopCard(0);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run.Get());
	FGuid PaidCardInstanceId;
	AddOwnedBackpackCard(State, PaidCard, PaidCardInstanceId);

	FWacomRunEventChoiceDefinition Choice;
	Choice.ChoiceId = TEXT("Pay");
	Choice.CardPayment.bRequiresOwnedCardPayment = true;
	Choice.CardPayment.AllowedCardDefinitions.Add(PaidCard);
	FWacomRunEventEffectDefinition RemoveCardEffect;
	RemoveCardEffect.Type = EWacomRunEventEffectType::RemoveCard;
	RemoveCardEffect.CardDefinition = PaidCard;
	Choice.Effects.Add(RemoveCardEffect);

	TStrongObjectPtr<UWacomRunEventDefinition> Event(
		MakeChoiceEvaluationEvent(Run.Get(), Choice));
	TestTrue(TEXT("Conflicting payment event begins"),
		Run->BeginRunEvent(TEXT("Event.ChoiceEvaluation.PaymentConflict"), Event.Get()));

	const FRunEventSnapshot Snapshot = Run->BuildCurrentRunEventSnapshot();
	const FRunEventChoiceSnapshot* ChoiceSnapshot = FindChoiceSnapshot(Snapshot, Choice.ChoiceId);
	if (!TestNotNull(TEXT("Conflicting payment snapshot exists"), ChoiceSnapshot))
	{
		return false;
	}
	const FName ExpectedReason = TEXT("PaymentChoiceHasRemoveCardEffect");
	TestFalse(TEXT("Conflicting payment snapshot is unavailable"), ChoiceSnapshot->bAvailable);
	TestEqual(TEXT("Conflicting payment snapshot reason"),
		ChoiceSnapshot->DisabledReason, ExpectedReason);
	TestEqual(TEXT("Conflicting payment has one requirement"),
		ChoiceSnapshot->Requirements.Num(), 1);
	if (ChoiceSnapshot->Requirements.Num() == 1)
	{
		TestEqual(TEXT("Conflicting payment requirement kind"),
			ChoiceSnapshot->Requirements[0].Kind,
			ERunEventChoiceRequirementKind::CardPayment);
		TestFalse(TEXT("Conflicting payment requirement is unsatisfied"),
			ChoiceSnapshot->Requirements[0].bSatisfied);
		TestEqual(TEXT("Conflicting payment requirement reason"),
			ChoiceSnapshot->Requirements[0].DisabledReason,
			ExpectedReason);
	}

	const FRunDeckOperationValidation Validation =
		Run->ValidateRunEventOptionCardPayment(Choice.ChoiceId, PaidCardInstanceId);
	TestFalse(TEXT("Conflicting payment validation fails"), Validation.bCanExecute);
	TestEqual(TEXT("Conflicting payment validation reason matches snapshot"),
		Validation.DisabledReason, ExpectedReason);

	const FRunEventChoiceResult SubmitResult =
		Run->ChooseRunEventOptionWithPaidCardResult(Choice.ChoiceId, PaidCardInstanceId);
	TestFalse(TEXT("Conflicting payment submit fails"), SubmitResult.bSucceeded);
	TestEqual(TEXT("Conflicting payment submit reason matches snapshot"),
		SubmitResult.DisabledReason, ExpectedReason);
	TestEqual(TEXT("Rejected payment keeps owned card"), State.Backpack.Num(), 1);
	return true;
}
