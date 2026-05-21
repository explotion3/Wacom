// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Validation/RunEventDefinitionValidation.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UCardDefinition* MakeRunEventValidationCard(UObject* Outer)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = TEXT("Card.Validation");
		Card->DisplayName = FText::FromString(TEXT("校验卡"));
		return Card;
	}

	UWacomRunEventDefinition* MakeValidRunEventForValidation(UObject* Outer)
	{
		UWacomRunEventDefinition* Event = NewObject<UWacomRunEventDefinition>(Outer);
		Event->EventId = TEXT("Event.Validation");
		Event->DisplayName = FText::FromString(TEXT("校验事件"));
		Event->StartNodeId = TEXT("Start");

		UCardDefinition* Card = MakeRunEventValidationCard(Event);

		FWacomRunEventChoiceDefinition Jump;
		Jump.ChoiceId = TEXT("Jump");
		Jump.NextNodeId = TEXT("End");

		FWacomRunEventConditionDefinition HasCard;
		HasCard.Type = EWacomRunEventConditionType::HasCard;
		HasCard.CardDefinition = Card;

		FWacomRunEventConditionDefinition EventNotDone;
		EventNotDone.Type = EWacomRunEventConditionType::EventNotCompleted;
		EventNotDone.TargetPersistentId = TEXT("Event.Dependency");

		FWacomRunEventEffectDefinition RemoveCard;
		RemoveCard.Type = EWacomRunEventEffectType::RemoveCard;
		RemoveCard.CardDefinition = Card;

		FWacomRunEventEffectDefinition MarkEvent;
		MarkEvent.Type = EWacomRunEventEffectType::MarkEventCompleted;
		MarkEvent.TargetPersistentId = TEXT("Event.Dependency");

		FWacomRunEventChoiceDefinition Resolve;
		Resolve.ChoiceId = TEXT("Resolve");
		Resolve.Conditions = { HasCard, EventNotDone };
		Resolve.Effects = { RemoveCard, MarkEvent };
		Resolve.bCloseEventAfterResolve = true;

		FWacomRunEventNodeDefinition Start;
		Start.NodeId = TEXT("Start");
		Start.Choices = { Jump };

		FWacomRunEventNodeDefinition End;
		End.NodeId = TEXT("End");
		End.Choices = { Resolve };

		Event->Nodes = { Start, End };
		return Event;
	}

	bool ValidateRunEventForTest(const UWacomRunEventDefinition* Event, TArray<FText>& OutErrors)
	{
		return FWacomRunEventDefinitionValidation::Validate(Event, OutErrors);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunEventValidationValidSpec,
	"Wacom.Data.RunEvent.Validation.ValidEvent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunEventValidationValidSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeValidRunEventForValidation(GetTransientPackage()));
	TArray<FText> Errors;
	TestTrue(TEXT("Valid RunEvent passes validation"), ValidateRunEventForTest(Event.Get(), Errors));
	TestEqual(TEXT("No validation errors"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunEventValidationGraphStructureSpec,
	"Wacom.Data.RunEvent.Validation.GraphStructure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunEventValidationGraphStructureSpec::RunTest(const FString& /*Parameters*/)
{
	TArray<FText> Errors;

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeValidRunEventForValidation(GetTransientPackage()));
		Event->StartNodeId = TEXT("MissingStart");
		TestFalse(TEXT("Missing start node fails"), ValidateRunEventForTest(Event.Get(), Errors));
		TestTrue(TEXT("Missing start node has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeValidRunEventForValidation(GetTransientPackage()));
		Event->Nodes[1].NodeId = Event->Nodes[0].NodeId;
		TestFalse(TEXT("Duplicate NodeId fails"), ValidateRunEventForTest(Event.Get(), Errors));
		TestTrue(TEXT("Duplicate NodeId has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeValidRunEventForValidation(GetTransientPackage()));
		const FWacomRunEventChoiceDefinition DuplicateChoice = Event->Nodes[0].Choices[0];
		Event->Nodes[0].Choices.Add(DuplicateChoice);
		TestFalse(TEXT("Duplicate ChoiceId fails"), ValidateRunEventForTest(Event.Get(), Errors));
		TestTrue(TEXT("Duplicate ChoiceId has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeValidRunEventForValidation(GetTransientPackage()));
		Event->Nodes[0].Choices[0].NextNodeId = TEXT("MissingNext");
		TestFalse(TEXT("Invalid NextNodeId fails"), ValidateRunEventForTest(Event.Get(), Errors));
		TestTrue(TEXT("Invalid NextNodeId has error"), Errors.Num() > 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunEventValidationRequiredFieldsSpec,
	"Wacom.Data.RunEvent.Validation.RequiredFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunEventValidationRequiredFieldsSpec::RunTest(const FString& /*Parameters*/)
{
	TArray<FText> Errors;

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeValidRunEventForValidation(GetTransientPackage()));
		Event->Nodes[1].Choices[0].Conditions[0].CardDefinition = nullptr;
		TestFalse(TEXT("HasCard missing CardDefinition fails"), ValidateRunEventForTest(Event.Get(), Errors));
		TestTrue(TEXT("HasCard missing CardDefinition has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeValidRunEventForValidation(GetTransientPackage()));
		Event->Nodes[1].Choices[0].Effects[0].CardDefinition = nullptr;
		TestFalse(TEXT("RemoveCard missing CardDefinition fails"), ValidateRunEventForTest(Event.Get(), Errors));
		TestTrue(TEXT("RemoveCard missing CardDefinition has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeValidRunEventForValidation(GetTransientPackage()));
		Event->Nodes[1].Choices[0].Conditions[1].TargetPersistentId = NAME_None;
		TestFalse(TEXT("Event state condition missing TargetPersistentId fails"), ValidateRunEventForTest(Event.Get(), Errors));
		TestTrue(TEXT("Event state condition missing TargetPersistentId has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeValidRunEventForValidation(GetTransientPackage()));
		Event->Nodes[1].Choices[0].Effects[1].TargetPersistentId = NAME_None;
		TestFalse(TEXT("MarkEventCompleted missing TargetPersistentId fails"), ValidateRunEventForTest(Event.Get(), Errors));
		TestTrue(TEXT("MarkEventCompleted missing TargetPersistentId has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeValidRunEventForValidation(GetTransientPackage()));
		FWacomRunEventConditionDefinition PressureCondition;
		PressureCondition.Type = EWacomRunEventConditionType::MaxPressure;
		PressureCondition.PressureType = NAME_None;
		Event->Nodes[1].Choices[0].Conditions = { PressureCondition };
		TestFalse(TEXT("MaxPressure missing PressureType fails"), ValidateRunEventForTest(Event.Get(), Errors));
		TestTrue(TEXT("MaxPressure missing PressureType has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeValidRunEventForValidation(GetTransientPackage()));
		FWacomRunEventEffectDefinition PressureEffect;
		PressureEffect.Type = EWacomRunEventEffectType::AddPressure;
		PressureEffect.PressureType = NAME_None;
		Event->Nodes[1].Choices[0].Effects = { PressureEffect };
		TestFalse(TEXT("AddPressure missing PressureType fails"), ValidateRunEventForTest(Event.Get(), Errors));
		TestTrue(TEXT("AddPressure missing PressureType has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeValidRunEventForValidation(GetTransientPackage()));
		FWacomRunEventEffectDefinition ConsumeNode;
		ConsumeNode.Type = EWacomRunEventEffectType::ConsumeNode;
		ConsumeNode.Value = -1;
		Event->Nodes[1].Choices[0].Effects = { ConsumeNode };
		TestFalse(TEXT("Negative ConsumeNode fails"), ValidateRunEventForTest(Event.Get(), Errors));
		TestTrue(TEXT("Negative ConsumeNode has error"), Errors.Num() > 0);
	}

	return true;
}
