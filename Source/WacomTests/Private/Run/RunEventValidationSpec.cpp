// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Validation/RunEventDefinitionValidation.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	bool ErrorsContain(const TArray<FText>& Errors, const TCHAR* Needle)
	{
		for (const FText& Error : Errors)
		{
			if (Error.ToString().Contains(Needle))
			{
				return true;
			}
		}
		return false;
	}

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

	UWacomRunEventDefinition* MakeValidPaymentRunEventForValidation(UObject* Outer)
	{
		UWacomRunEventDefinition* Event = NewObject<UWacomRunEventDefinition>(Outer);
		Event->EventId = TEXT("Event.Validation.Payment");
		Event->DisplayName = FText::FromString(TEXT("卡牌支付校验事件"));
		Event->StartNodeId = TEXT("Start");

		UCardDefinition* Card = MakeRunEventValidationCard(Event);

		FWacomRunEventChoiceDefinition Pay;
		Pay.ChoiceId = TEXT("HandOverFang");
		Pay.LabelText = FText::FromString(TEXT("交出毒牙"));
		Pay.CardPayment.bRequiresOwnedCardPayment = true;
		Pay.CardPayment.PaymentZoneId = TEXT("RunEvent.Pay.Fang");
		Pay.CardPayment.AllowedCardDefinitions.Add(Card);
		Pay.NextNodeId = TEXT("End");

		FWacomRunEventNodeDefinition Start;
		Start.NodeId = TEXT("Start");
		Start.Choices = { Pay };

		FWacomRunEventChoiceDefinition Close;
		Close.ChoiceId = TEXT("Close");
		Close.bCloseEventAfterResolve = true;

		FWacomRunEventNodeDefinition End;
		End.NodeId = TEXT("End");
		End.Choices = { Close };

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunEventValidationPaymentAuthoringSpec,
	"Wacom.Data.RunEvent.Validation.PaymentAuthoring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunEventValidationPaymentAuthoringSpec::RunTest(const FString& /*Parameters*/)
{
	TArray<FText> Errors;

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(
			MakeValidPaymentRunEventForValidation(GetTransientPackage()));
		Event->Nodes[0].Choices[0].CardPayment.AllowedCardDefinitions.Reset();
		TestFalse(TEXT("Payment choice empty filter fails"), ValidateRunEventForTest(Event.Get(), Errors));
		TestTrue(TEXT("Empty filter error names node"),
			ErrorsContain(Errors, TEXT("Node Start")));
		TestTrue(TEXT("Empty filter error names choice"),
			ErrorsContain(Errors, TEXT("Choice HandOverFang")));
		TestTrue(TEXT("Empty filter error names payment zone"),
			ErrorsContain(Errors, TEXT("PaymentZoneId RunEvent.Pay.Fang")));
	}

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(
			MakeValidPaymentRunEventForValidation(GetTransientPackage()));
		FWacomRunEventEffectDefinition RemoveCard;
		RemoveCard.Type = EWacomRunEventEffectType::RemoveCard;
		RemoveCard.CardDefinition = Event->Nodes[0].Choices[0].CardPayment.AllowedCardDefinitions[0];
		Event->Nodes[0].Choices[0].Effects = { RemoveCard };
		TestFalse(TEXT("Payment choice with RemoveCard fails"), ValidateRunEventForTest(Event.Get(), Errors));
		TestTrue(TEXT("RemoveCard payment error names choice and zone"),
			ErrorsContain(Errors, TEXT("Choice HandOverFang"))
			&& ErrorsContain(Errors, TEXT("PaymentZoneId RunEvent.Pay.Fang")));
		TestTrue(TEXT("RemoveCard payment error explains exact-instance removal"),
			ErrorsContain(Errors, TEXT("移除精确实例")));
	}

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(
			MakeValidPaymentRunEventForValidation(GetTransientPackage()));
		FWacomRunEventChoiceDefinition Duplicate = Event->Nodes[0].Choices[0];
		Duplicate.ChoiceId = TEXT("SecondPay");
		Event->Nodes[0].Choices.Add(Duplicate);
		TestFalse(TEXT("Duplicate payment zone fails"), ValidateRunEventForTest(Event.Get(), Errors));
		TestTrue(TEXT("Duplicate zone error names node"),
			ErrorsContain(Errors, TEXT("Node Start")));
		TestTrue(TEXT("Duplicate zone error names zone"),
			ErrorsContain(Errors, TEXT("PaymentZoneId 重复：RunEvent.Pay.Fang")));
		TestTrue(TEXT("Duplicate zone error names both choices"),
			ErrorsContain(Errors, TEXT("HandOverFang")) && ErrorsContain(Errors, TEXT("SecondPay")));
	}

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(
			MakeValidPaymentRunEventForValidation(GetTransientPackage()));
		Event->Nodes[0].Choices[0].NextNodeId = TEXT("MissingEnd");
		TestFalse(TEXT("Payment choice invalid NextNodeId fails"), ValidateRunEventForTest(Event.Get(), Errors));
		TestTrue(TEXT("Invalid next node error names node choice and next"),
			ErrorsContain(Errors, TEXT("Node Start"))
			&& ErrorsContain(Errors, TEXT("Choice HandOverFang"))
			&& ErrorsContain(Errors, TEXT("NextNodeId 无效：MissingEnd")));
	}

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(
			MakeValidPaymentRunEventForValidation(GetTransientPackage()));
		Event->Nodes[0].Choices[0].ChoiceId = NAME_None;
		Event->Nodes[0].Choices[0].CardPayment.PaymentZoneId = NAME_None;
		TestFalse(TEXT("Payment choice missing ChoiceId fails"), ValidateRunEventForTest(Event.Get(), Errors));
		TestTrue(TEXT("Missing ChoiceId payment error names fallback zone failure"),
			ErrorsContain(Errors, TEXT("ChoiceId 不能为空"))
			&& ErrorsContain(Errors, TEXT("无法生成默认 PaymentZoneId")));
	}

	{
		TStrongObjectPtr<UWacomRunEventDefinition> Event(
			MakeValidPaymentRunEventForValidation(GetTransientPackage()));
		TestTrue(TEXT("Valid payment RunEvent passes validation"), ValidateRunEventForTest(Event.Get(), Errors));
		TestEqual(TEXT("No payment validation errors"), Errors.Num(), 0);
	}

	{
		UWacomRunEventDefinition* SnakeGift = LoadObject<UWacomRunEventDefinition>(
			nullptr,
			TEXT("/Game/Wacom/Data/Events/DA_Event_DebugSnakeGift.DA_Event_DebugSnakeGift"));
		TestNotNull(TEXT("Debug Snake Gift sample asset exists"), SnakeGift);
		if (SnakeGift)
		{
			TestTrue(TEXT("Debug Snake Gift payment sample passes validation"),
				ValidateRunEventForTest(SnakeGift, Errors));
			TestEqual(TEXT("Debug Snake Gift has no validation errors"), Errors.Num(), 0);
		}
	}

	return true;
}
