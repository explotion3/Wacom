// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "RunSession.h"
#include "RunState.h"
#include "Tags/WacomGameplayTags.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	UCardDefinition* MakeRunWorldInteractionCard(
		UObject* Outer,
		FName CardId,
		const FGameplayTagContainer& Keywords = FGameplayTagContainer())
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = CardId;
		Card->DisplayName = FText::FromName(CardId);
		Card->Rarity = WacomTags::Card_Rarity_White;
		Card->Keywords = Keywords;
		return Card;
	}

	UCharacterDefinition* MakeRunWorldInteractionCharacter(
		UObject* Outer,
		UCardDefinition* Card)
	{
		UCardDefinition* CapacityProvider = NewObject<UCardDefinition>(Outer);
		CapacityProvider->CardId = TEXT("RunWorldCardInteraction.Bag");
		CapacityProvider->DisplayName = FText::FromString(TEXT("Run World Card Interaction Bag"));
		CapacityProvider->Rarity = WacomTags::Card_Rarity_White;
		CapacityProvider->Physique.Capacity = 4;

		UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
		Character->CharacterId = TEXT("RunWorldCardInteraction.Character");
		Character->DisplayName = FText::FromString(TEXT("Run World Card Interaction Character"));
		Character->FingerCount = 10;
		Character->HpPerFinger = 2;
		Character->StarterDeck = { Card, CapacityProvider };
		return Character;
	}

	FGuid GetFirstBattleDeckInstanceId(const URunSession& Run)
	{
		const FRunState& State = Run.GetRunState();
		return State.BattleDeck.Num() > 0 ? State.BattleDeck[0].InstanceId : FGuid();
	}

	FRunWorldCardInteractionRequest MakeChestRequest(
		const URunSession& Run,
		UCardDefinition* Card,
		FName PersistentId = TEXT("Chest.Debug.Test"),
		int32 GoldReward = 3)
	{
		FRunWorldCardInteractionRequest Request;
		Request.PersistentId = PersistentId;
		Request.SourceCardInstanceId = GetFirstBattleDeckInstanceId(Run);
		Request.AllowedCardDefinitions = { Card };
		Request.AllowedCardIds = { Card ? Card->CardId : NAME_None };
		Request.bConsumeCardOnSuccess = true;
		Request.GoldReward = GoldReward;
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunWorldCardInteractionSuccessSpec,
	"Wacom.Run.WorldCardInteraction.ChestKeyInteractionConsumesExactCardAddsGoldAndMarksCompleted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunWorldCardInteractionSuccessSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Key(
		MakeRunWorldInteractionCard(GetTransientPackage(), TEXT("DebugKey")));
	TStrongObjectPtr<UCharacterDefinition> Character(
		MakeRunWorldInteractionCharacter(GetTransientPackage(), Key.Get()));
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character.Get()));

	const FGuid KeyInstanceId = GetFirstBattleDeckInstanceId(*Run);
	FRunWorldCardInteractionRequest Request = MakeChestRequest(*Run, Key.Get());
	const FRunWorldCardInteractionValidation Validation =
		Run->ValidateRunWorldCardInteraction(Request);
	TestTrue(TEXT("Validation can submit"), Validation.bCanSubmit);

	TestTrue(TEXT("Submit succeeds"), Run->SubmitRunWorldCardInteraction(Request));
	TestEqual(TEXT("Gold +3"), Run->GetGold(), 3);
	TestTrue(TEXT("Completed marked"),
		Run->IsRunWorldInteractionCompleted(TEXT("Chest.Debug.Test")));
	TestFalse(TEXT("Exact card consumed"),
		Run->ValidateDestroyCardByInstance(KeyInstanceId).bCanExecute);
	TestEqual(TEXT("BattleDeck empty"), Run->GetRunState().BattleDeck.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunWorldCardInteractionWrongCardSpec,
	"Wacom.Run.WorldCardInteraction.ChestKeyInteractionRejectsWrongCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunWorldCardInteractionWrongCardSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Key(
		MakeRunWorldInteractionCard(GetTransientPackage(), TEXT("DebugKey")));
	TStrongObjectPtr<UCardDefinition> Wrong(
		MakeRunWorldInteractionCard(GetTransientPackage(), TEXT("WrongCard")));
	TStrongObjectPtr<UCharacterDefinition> Character(
		MakeRunWorldInteractionCharacter(GetTransientPackage(), Wrong.Get()));
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character.Get()));

	FRunWorldCardInteractionRequest Request = MakeChestRequest(*Run, Key.Get());
	Request.AllowedCardDefinitions = { Key.Get() };
	Request.AllowedCardIds = { TEXT("DebugKey") };
	const FRunWorldCardInteractionValidation Validation =
		Run->ValidateRunWorldCardInteraction(Request);
	TestFalse(TEXT("Wrong card rejected"), Validation.bCanSubmit);
	TestEqual(TEXT("Wrong card reason"), Validation.DisabledReason, FName(TEXT("CardNotAccepted")));
	TestFalse(TEXT("Submit rejected"), Run->SubmitRunWorldCardInteraction(Request));
	TestEqual(TEXT("Gold unchanged"), Run->GetGold(), 0);
	TestFalse(TEXT("Not completed"), Run->IsRunWorldInteractionCompleted(Request.PersistentId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunWorldCardInteractionRepeatSpec,
	"Wacom.Run.WorldCardInteraction.ChestKeyInteractionRejectsRepeatCompletedId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunWorldCardInteractionRepeatSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Key(
		MakeRunWorldInteractionCard(GetTransientPackage(), TEXT("DebugKey")));
	TStrongObjectPtr<UCharacterDefinition> Character(
		MakeRunWorldInteractionCharacter(GetTransientPackage(), Key.Get()));
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character.Get()));

	FRunWorldCardInteractionRequest Request = MakeChestRequest(*Run, Key.Get());
	Request.bConsumeCardOnSuccess = false;
	TestTrue(TEXT("First submit"), Run->SubmitRunWorldCardInteraction(Request));
	const FRunWorldCardInteractionValidation RepeatValidation =
		Run->ValidateRunWorldCardInteraction(Request);
	TestFalse(TEXT("Repeat rejected"), RepeatValidation.bCanSubmit);
	TestEqual(TEXT("Repeat reason"), RepeatValidation.DisabledReason, FName(TEXT("AlreadyCompleted")));
	TestFalse(TEXT("Second submit rejected"), Run->SubmitRunWorldCardInteraction(Request));
	TestEqual(TEXT("Gold only once"), Run->GetGold(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunWorldCardInteractionMissingIdSpec,
	"Wacom.Run.WorldCardInteraction.ChestKeyInteractionRejectsMissingPersistentId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunWorldCardInteractionMissingIdSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Key(
		MakeRunWorldInteractionCard(GetTransientPackage(), TEXT("DebugKey")));
	TStrongObjectPtr<UCharacterDefinition> Character(
		MakeRunWorldInteractionCharacter(GetTransientPackage(), Key.Get()));
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character.Get()));

	FRunWorldCardInteractionRequest Request = MakeChestRequest(*Run, Key.Get(), NAME_None);
	const FRunWorldCardInteractionValidation Validation =
		Run->ValidateRunWorldCardInteraction(Request);
	TestFalse(TEXT("Missing id rejected"), Validation.bCanSubmit);
	TestEqual(TEXT("Missing id reason"),
		Validation.DisabledReason,
		FName(TEXT("MissingPersistentId")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunWorldCardInteractionMissingSourceSpec,
	"Wacom.Run.WorldCardInteraction.ChestKeyInteractionRejectsMissingSourceCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunWorldCardInteractionMissingSourceSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Key(
		MakeRunWorldInteractionCard(GetTransientPackage(), TEXT("DebugKey")));
	TStrongObjectPtr<UCharacterDefinition> Character(
		MakeRunWorldInteractionCharacter(GetTransientPackage(), Key.Get()));
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character.Get()));

	FRunWorldCardInteractionRequest Request = MakeChestRequest(*Run, Key.Get());
	Request.SourceCardInstanceId = FGuid();
	const FRunWorldCardInteractionValidation Validation =
		Run->ValidateRunWorldCardInteraction(Request);
	TestFalse(TEXT("Missing source rejected"), Validation.bCanSubmit);
	TestEqual(TEXT("Missing source reason"),
		Validation.DisabledReason,
		FName(TEXT("MissingSourceCard")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunWorldCardInteractionInvalidGoldSpec,
	"Wacom.Run.WorldCardInteraction.ChestKeyInteractionRejectsInvalidGoldReward",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunWorldCardInteractionInvalidGoldSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Key(
		MakeRunWorldInteractionCard(GetTransientPackage(), TEXT("DebugKey")));
	TStrongObjectPtr<UCharacterDefinition> Character(
		MakeRunWorldInteractionCharacter(GetTransientPackage(), Key.Get()));
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character.Get()));

	FRunWorldCardInteractionRequest Request = MakeChestRequest(*Run, Key.Get(), TEXT("Chest.BadGold"), 0);
	const FRunWorldCardInteractionValidation Validation =
		Run->ValidateRunWorldCardInteraction(Request);
	TestFalse(TEXT("Invalid gold rejected"), Validation.bCanSubmit);
	TestEqual(TEXT("Invalid gold reason"),
		Validation.DisabledReason,
		FName(TEXT("InvalidGoldReward")));
	return true;
}
