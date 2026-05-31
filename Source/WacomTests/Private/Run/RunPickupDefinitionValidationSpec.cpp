// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Pickups/RunPickupDefinition.h"
#include "Validation/RunPickupDefinitionValidation.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UCardDefinition* MakeRunPickupValidationCard(UObject* Outer)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = TEXT("Card.Pickup.Validation");
		Card->DisplayName = FText::FromString(TEXT("拾取校验卡"));
		return Card;
	}

	UWacomRunPickupDefinition* MakeValidGoldPickupDefinition(UObject* Outer)
	{
		UWacomRunPickupDefinition* Definition = NewObject<UWacomRunPickupDefinition>(Outer);
		Definition->PickupId = TEXT("Pickup.Definition.Gold");
		Definition->RewardType = EWacomRunPickupRewardType::Gold;
		Definition->GoldAmount = 3;
		return Definition;
	}

	UWacomRunPickupDefinition* MakeValidCardPickupDefinition(UObject* Outer)
	{
		UWacomRunPickupDefinition* Definition = NewObject<UWacomRunPickupDefinition>(Outer);
		Definition->PickupId = TEXT("Pickup.Definition.Card");
		Definition->RewardType = EWacomRunPickupRewardType::Card;
		Definition->CardDefinition = MakeRunPickupValidationCard(Definition);
		return Definition;
	}

	bool ValidateRunPickupDefinitionForTest(
		const UWacomRunPickupDefinition* Definition,
		TArray<FText>& OutErrors)
	{
		return FWacomRunPickupDefinitionValidation::Validate(Definition, OutErrors);
	}

	UWacomRunPickupDefinition* LoadDebugGoldPickupDefinition()
	{
		return LoadObject<UWacomRunPickupDefinition>(
			nullptr,
			TEXT("/Game/Wacom/Data/Pickups/DA_Pickup_DebugGold3.DA_Pickup_DebugGold3"));
	}

	UWacomRunPickupDefinition* LoadDebugPoisonFangPickupDefinition()
	{
		return LoadObject<UWacomRunPickupDefinition>(
			nullptr,
			TEXT("/Game/Wacom/Data/Pickups/DA_Pickup_DebugPoisonFang.DA_Pickup_DebugPoisonFang"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunPickupValidationGoldSpec,
	"Wacom.Data.RunPickup.Validation.ValidGoldPickupDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunPickupValidationGoldSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunPickupDefinition> Definition(
		MakeValidGoldPickupDefinition(GetTransientPackage()));
	TArray<FText> Errors;

	TestTrue(TEXT("Valid gold pickup definition passes"),
		ValidateRunPickupDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("No validation errors"), Errors.Num(), 0);
	TestTrue(TEXT("Definition helper reports valid"), Definition->IsRewardConfigValid());
	TestEqual(TEXT("Definition helper reason is None"),
		Definition->GetRewardConfigWarningReason(), NAME_None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunPickupValidationCardSpec,
	"Wacom.Data.RunPickup.Validation.ValidCardPickupDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunPickupValidationCardSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunPickupDefinition> Definition(
		MakeValidCardPickupDefinition(GetTransientPackage()));
	TArray<FText> Errors;

	TestTrue(TEXT("Valid card pickup definition passes"),
		ValidateRunPickupDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("No validation errors"), Errors.Num(), 0);
	TestTrue(TEXT("Definition helper reports valid"), Definition->IsRewardConfigValid());
	TestEqual(TEXT("Definition helper reason is None"),
		Definition->GetRewardConfigWarningReason(), NAME_None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunPickupValidationMissingPickupIdSpec,
	"Wacom.Data.RunPickup.Validation.MissingPickupIdReportsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunPickupValidationMissingPickupIdSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunPickupDefinition> Definition(
		MakeValidGoldPickupDefinition(GetTransientPackage()));
	Definition->PickupId = NAME_None;

	TArray<FText> Errors;
	TestFalse(TEXT("Missing PickupId fails"),
		ValidateRunPickupDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("Helper reports missing id"),
		Definition->GetRewardConfigWarningReason(), FName(TEXT("MissingPickupId")));
	TestTrue(TEXT("Missing PickupId has error"), Errors.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunPickupValidationMissingRewardTypeSpec,
	"Wacom.Data.RunPickup.Validation.MissingRewardTypeReportsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunPickupValidationMissingRewardTypeSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunPickupDefinition> Definition(
		MakeValidGoldPickupDefinition(GetTransientPackage()));
	Definition->RewardType = EWacomRunPickupRewardType::None;

	TArray<FText> Errors;
	TestFalse(TEXT("Missing reward type fails"),
		ValidateRunPickupDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("Helper reports missing reward type"),
		Definition->GetRewardConfigWarningReason(), FName(TEXT("MissingRewardType")));
	TestTrue(TEXT("Missing reward type has error"), Errors.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunPickupValidationInvalidGoldSpec,
	"Wacom.Data.RunPickup.Validation.InvalidGoldAmountReportsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunPickupValidationInvalidGoldSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunPickupDefinition> Definition(
		MakeValidGoldPickupDefinition(GetTransientPackage()));
	Definition->GoldAmount = 0;

	TArray<FText> Errors;
	TestFalse(TEXT("Invalid gold amount fails"),
		ValidateRunPickupDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("Helper reports invalid gold"),
		Definition->GetRewardConfigWarningReason(), FName(TEXT("InvalidGoldAmount")));
	TestTrue(TEXT("Invalid gold has error"), Errors.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunPickupValidationMissingCardSpec,
	"Wacom.Data.RunPickup.Validation.MissingCardDefinitionReportsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunPickupValidationMissingCardSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunPickupDefinition> Definition(
		MakeValidCardPickupDefinition(GetTransientPackage()));
	Definition->CardDefinition = nullptr;

	TArray<FText> Errors;
	TestFalse(TEXT("Missing card definition fails"),
		ValidateRunPickupDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("Helper reports missing card"),
		Definition->GetRewardConfigWarningReason(), FName(TEXT("MissingCardDefinition")));
	TestTrue(TEXT("Missing card has error"), Errors.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunPickupDebugGoldAssetSpec,
	"Wacom.Data.RunPickup.DebugGoldPickupDefinitionAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunPickupDebugGoldAssetSpec::RunTest(const FString& /*Parameters*/)
{
	UWacomRunPickupDefinition* Definition = LoadDebugGoldPickupDefinition();
	if (!TestNotNull(TEXT("Debug gold pickup definition asset loads"), Definition))
	{
		return false;
	}

	TestEqual(TEXT("Debug gold pickup id"),
		Definition->PickupId, FName(TEXT("Pickup.Debug.Gold3")));
	TestEqual(TEXT("Debug gold reward type"),
		Definition->RewardType, EWacomRunPickupRewardType::Gold);
	TestEqual(TEXT("Debug gold amount"), Definition->GoldAmount, 3);
	TestNull(TEXT("Debug gold has no card definition"), Definition->CardDefinition.Get());

	TArray<FText> Errors;
	TestTrue(TEXT("Debug gold pickup definition passes validation"),
		ValidateRunPickupDefinitionForTest(Definition, Errors));
	TestEqual(TEXT("Debug gold validation errors"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunPickupDebugPoisonFangAssetSpec,
	"Wacom.Data.RunPickup.DebugPoisonFangPickupDefinitionAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunPickupDebugPoisonFangAssetSpec::RunTest(const FString& /*Parameters*/)
{
	UWacomRunPickupDefinition* Definition = LoadDebugPoisonFangPickupDefinition();
	if (!TestNotNull(TEXT("Debug poison fang pickup definition asset loads"), Definition))
	{
		return false;
	}

	TestEqual(TEXT("Debug poison fang pickup id"),
		Definition->PickupId, FName(TEXT("Pickup.Debug.PoisonFang")));
	TestEqual(TEXT("Debug poison fang reward type"),
		Definition->RewardType, EWacomRunPickupRewardType::Card);
	TestNotNull(TEXT("Debug poison fang card definition"),
		Definition->CardDefinition.Get());
	if (const UCardDefinition* Card = Definition->CardDefinition.Get())
	{
		TestEqual(TEXT("Debug poison fang card id"),
			Card->CardId, FName(TEXT("PoisonFang")));
	}

	TArray<FText> Errors;
	TestTrue(TEXT("Debug poison fang pickup definition passes validation"),
		ValidateRunPickupDefinitionForTest(Definition, Errors));
	TestEqual(TEXT("Debug poison fang validation errors"), Errors.Num(), 0);
	return true;
}
