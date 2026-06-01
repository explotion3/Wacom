// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "KeyChests/RunKeyChestDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "Validation/RunKeyChestDefinitionValidation.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UCardDefinition* MakeRunKeyChestValidationCard(UObject* Outer)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = TEXT("DebugKey");
		Card->DisplayName = FText::FromString(TEXT("钥匙"));
		Card->Keywords.AddTag(WacomTags::Card_Keyword_Tool);
		return Card;
	}

	UWacomRunKeyChestDefinition* MakeValidRunKeyChestDefinition(UObject* Outer)
	{
		UWacomRunKeyChestDefinition* Definition =
			NewObject<UWacomRunKeyChestDefinition>(Outer);
		Definition->ChestId = TEXT("Chest.Definition.DebugKey");
		Definition->AllowedCardDefinitions.Add(
			MakeRunKeyChestValidationCard(Definition));
		Definition->AllowedCardIds = { TEXT("DebugKey") };
		Definition->GoldReward = 3;
		Definition->bConsumeCardOnSuccess = true;
		return Definition;
	}

	bool ValidateRunKeyChestDefinitionForTest(
		const UWacomRunKeyChestDefinition* Definition,
		TArray<FText>& OutErrors)
	{
		return FWacomRunKeyChestDefinitionValidation::Validate(
			Definition,
			OutErrors);
	}

	UWacomRunKeyChestDefinition* LoadDebugKeyGoldChestDefinition()
	{
		return LoadObject<UWacomRunKeyChestDefinition>(
			nullptr,
			TEXT("/Game/Wacom/Data/KeyChests/DA_KeyChest_DebugKeyGold3.DA_KeyChest_DebugKeyGold3"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunKeyChestDefinitionValidationValidSpec,
	"Wacom.Data.RunKeyChestDefinition.Validation.ValidDebugKeyChestDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunKeyChestDefinitionValidationValidSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunKeyChestDefinition> Definition(
		MakeValidRunKeyChestDefinition(GetTransientPackage()));
	TArray<FText> Errors;

	TestTrue(TEXT("Valid key chest definition passes"),
		ValidateRunKeyChestDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("No validation errors"), Errors.Num(), 0);
	TestTrue(TEXT("Definition helper reports valid"), Definition->IsConfigValid());
	TestEqual(TEXT("Definition helper reason is None"),
		Definition->GetConfigWarningReason(), NAME_None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunKeyChestDefinitionValidationMissingChestIdSpec,
	"Wacom.Data.RunKeyChestDefinition.Validation.MissingChestIdReportsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunKeyChestDefinitionValidationMissingChestIdSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunKeyChestDefinition> Definition(
		MakeValidRunKeyChestDefinition(GetTransientPackage()));
	Definition->ChestId = NAME_None;

	TArray<FText> Errors;
	TestFalse(TEXT("Missing ChestId fails"),
		ValidateRunKeyChestDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("Helper reports missing id"),
		Definition->GetConfigWarningReason(), FName(TEXT("MissingChestId")));
	TestTrue(TEXT("Missing ChestId has error"), Errors.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunKeyChestDefinitionValidationMissingPositiveFilterSpec,
	"Wacom.Data.RunKeyChestDefinition.Validation.MissingPositiveCardFilterReportsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunKeyChestDefinitionValidationMissingPositiveFilterSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunKeyChestDefinition> Definition(
		MakeValidRunKeyChestDefinition(GetTransientPackage()));
	Definition->AllowedCardDefinitions.Reset();
	Definition->AllowedCardIds.Reset();
	Definition->RequiredKeywords.Reset();
	Definition->BlockedKeywords.Reset();

	TArray<FText> Errors;
	TestFalse(TEXT("Missing positive filter fails"),
		ValidateRunKeyChestDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("Helper reports missing positive filter"),
		Definition->GetConfigWarningReason(),
		FName(TEXT("MissingPositiveCardFilter")));
	TestTrue(TEXT("Missing positive filter has error"), Errors.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunKeyChestDefinitionValidationBlockedOnlySpec,
	"Wacom.Data.RunKeyChestDefinition.Validation.BlockedOnlyFilterReportsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunKeyChestDefinitionValidationBlockedOnlySpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunKeyChestDefinition> Definition(
		MakeValidRunKeyChestDefinition(GetTransientPackage()));
	Definition->AllowedCardDefinitions.Reset();
	Definition->AllowedCardIds.Reset();
	Definition->RequiredKeywords.Reset();
	Definition->BlockedKeywords.Reset();
	Definition->BlockedKeywords.AddTag(WacomTags::Card_Keyword_Weapon);

	TArray<FText> Errors;
	TestFalse(TEXT("Blocked-only filter fails"),
		ValidateRunKeyChestDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("Helper reports missing positive filter"),
		Definition->GetConfigWarningReason(),
		FName(TEXT("MissingPositiveCardFilter")));
	TestTrue(TEXT("Blocked-only filter has error"), Errors.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunKeyChestDefinitionValidationInvalidGoldSpec,
	"Wacom.Data.RunKeyChestDefinition.Validation.InvalidGoldRewardReportsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunKeyChestDefinitionValidationInvalidGoldSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunKeyChestDefinition> Definition(
		MakeValidRunKeyChestDefinition(GetTransientPackage()));
	Definition->GoldReward = 0;

	TArray<FText> Errors;
	TestFalse(TEXT("Invalid gold reward fails"),
		ValidateRunKeyChestDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("Helper reports invalid gold"),
		Definition->GetConfigWarningReason(),
		FName(TEXT("InvalidGoldReward")));
	TestTrue(TEXT("Invalid gold has error"), Errors.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunKeyChestDefinitionDebugKeyGoldAssetSpec,
	"Wacom.Data.RunKeyChestDefinition.DebugKeyGoldChestDefinitionAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunKeyChestDefinitionDebugKeyGoldAssetSpec::RunTest(
	const FString& /*Parameters*/)
{
	UWacomRunKeyChestDefinition* Definition =
		LoadDebugKeyGoldChestDefinition();
	if (!TestNotNull(TEXT("Debug key gold chest definition asset loads"), Definition))
	{
		return false;
	}

	TestEqual(TEXT("ChestId"),
		Definition->ChestId, FName(TEXT("KeyChest.Debug.KeyGold3")));
	TestEqual(TEXT("Allowed card definitions"),
		Definition->AllowedCardDefinitions.Num(), 1);
	if (const UCardDefinition* DebugKey =
		Definition->AllowedCardDefinitions.Num() > 0
			? Definition->AllowedCardDefinitions[0].Get()
			: nullptr)
	{
		TestEqual(TEXT("Allowed definition card id"),
			DebugKey->CardId, FName(TEXT("DebugKey")));
		TestTrue(TEXT("Allowed definition has Tool keyword"),
			DebugKey->Keywords.HasTagExact(WacomTags::Card_Keyword_Tool));
	}
	TestTrue(TEXT("Allowed ids contain DebugKey"),
		Definition->AllowedCardIds.Contains(FName(TEXT("DebugKey"))));
	TestEqual(TEXT("Gold reward"), Definition->GoldReward, 3);
	TestTrue(TEXT("Consumes source card"), Definition->bConsumeCardOnSuccess);
	TestEqual(TEXT("Interact prompt"),
		Definition->InteractPromptText.ToString(), FString(TEXT("需要钥匙")));
	TestEqual(TEXT("Hover prompt"),
		Definition->HoverPromptText.ToString(), FString(TEXT("拖入钥匙")));
	TestEqual(TEXT("Completed prompt"),
		Definition->CompletedPromptText.ToString(), FString(TEXT("宝箱已打开")));
	TestEqual(TEXT("Preview prompt"),
		Definition->PreviewPromptText.ToString(), FString(TEXT("使用钥匙打开宝箱")));

	TArray<FText> Errors;
	TestTrue(TEXT("Debug key gold chest definition passes validation"),
		ValidateRunKeyChestDefinitionForTest(Definition, Errors));
	TestEqual(TEXT("Debug key gold validation errors"), Errors.Num(), 0);
	return true;
}
