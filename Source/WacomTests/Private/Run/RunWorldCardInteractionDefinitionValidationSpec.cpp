// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "Validation/RunWorldCardInteractionDefinitionValidation.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UCardDefinition* MakeRunWorldCardInteractionValidationCard(UObject* Outer)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = TEXT("DebugKey");
		Card->DisplayName = FText::FromString(TEXT("钥匙"));
		Card->Keywords.AddTag(WacomTags::Card_Keyword_Tool);
		return Card;
	}

	UWacomRunWorldCardInteractionDefinition* MakeValidRunWorldCardInteractionDefinition(
		UObject* Outer)
	{
		UWacomRunWorldCardInteractionDefinition* Definition =
			NewObject<UWacomRunWorldCardInteractionDefinition>(Outer);
		Definition->InteractionId = TEXT("WorldCardInteraction.DebugKey");
		Definition->AllowedCardDefinitions.Add(
			MakeRunWorldCardInteractionValidationCard(Definition));
		Definition->AllowedCardIds = { TEXT("DebugKey") };
		FWacomRunWorldCardInteractionReward GoldReward;
		GoldReward.Type = EWacomRunWorldCardInteractionRewardType::Gold;
		GoldReward.GoldAmount = 3;
		Definition->Rewards = { GoldReward };
		Definition->bConsumeCardOnSuccess = true;
		return Definition;
	}

	bool ValidateRunWorldCardInteractionDefinitionForTest(
		const UWacomRunWorldCardInteractionDefinition* Definition,
		TArray<FText>& OutErrors)
	{
		return FWacomRunWorldCardInteractionDefinitionValidation::Validate(
			Definition,
			OutErrors);
	}

	UWacomRunWorldCardInteractionDefinition* LoadDebugKeyGoldInteractionDefinition()
	{
		return LoadObject<UWacomRunWorldCardInteractionDefinition>(
			nullptr,
			TEXT("/Game/Wacom/Data/Interactions/DA_RunWorldCardInteraction_DebugKeyGold3.DA_RunWorldCardInteraction_DebugKeyGold3"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunWorldCardInteractionDefinitionValidationValidSpec,
	"Wacom.Data.RunWorldCardInteractionDefinition.Validation.ValidDebugKeyInteractionDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunWorldCardInteractionDefinitionValidationValidSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunWorldCardInteractionDefinition> Definition(
		MakeValidRunWorldCardInteractionDefinition(GetTransientPackage()));
	TArray<FText> Errors;

	TestTrue(TEXT("Valid world card interaction definition passes"),
		ValidateRunWorldCardInteractionDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("No validation errors"), Errors.Num(), 0);
	TestTrue(TEXT("Definition helper reports valid"), Definition->IsConfigValid());
	TestEqual(TEXT("Definition helper reason is None"),
		Definition->GetConfigWarningReason(), NAME_None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunWorldCardInteractionDefinitionValidationMissingIdSpec,
	"Wacom.Data.RunWorldCardInteractionDefinition.Validation.MissingInteractionIdReportsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunWorldCardInteractionDefinitionValidationMissingIdSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunWorldCardInteractionDefinition> Definition(
		MakeValidRunWorldCardInteractionDefinition(GetTransientPackage()));
	Definition->InteractionId = NAME_None;

	TArray<FText> Errors;
	TestFalse(TEXT("Missing InteractionId fails"),
		ValidateRunWorldCardInteractionDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("Helper reports missing id"),
		Definition->GetConfigWarningReason(), FName(TEXT("MissingInteractionId")));
	TestTrue(TEXT("Missing InteractionId has error"), Errors.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunWorldCardInteractionDefinitionValidationMissingPositiveFilterSpec,
	"Wacom.Data.RunWorldCardInteractionDefinition.Validation.MissingPositiveCardFilterReportsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunWorldCardInteractionDefinitionValidationMissingPositiveFilterSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunWorldCardInteractionDefinition> Definition(
		MakeValidRunWorldCardInteractionDefinition(GetTransientPackage()));
	Definition->AllowedCardDefinitions.Reset();
	Definition->AllowedCardIds.Reset();
	Definition->RequiredKeywords.Reset();
	Definition->BlockedKeywords.Reset();

	TArray<FText> Errors;
	TestFalse(TEXT("Missing positive filter fails"),
		ValidateRunWorldCardInteractionDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("Helper reports missing positive filter"),
		Definition->GetConfigWarningReason(),
		FName(TEXT("MissingPositiveCardFilter")));
	TestTrue(TEXT("Missing positive filter has error"), Errors.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunWorldCardInteractionDefinitionValidationBlockedOnlySpec,
	"Wacom.Data.RunWorldCardInteractionDefinition.Validation.BlockedOnlyFilterReportsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunWorldCardInteractionDefinitionValidationBlockedOnlySpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunWorldCardInteractionDefinition> Definition(
		MakeValidRunWorldCardInteractionDefinition(GetTransientPackage()));
	Definition->AllowedCardDefinitions.Reset();
	Definition->AllowedCardIds.Reset();
	Definition->RequiredKeywords.Reset();
	Definition->BlockedKeywords.Reset();
	Definition->BlockedKeywords.AddTag(WacomTags::Card_Keyword_Weapon);

	TArray<FText> Errors;
	TestFalse(TEXT("Blocked-only filter fails"),
		ValidateRunWorldCardInteractionDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("Helper reports missing positive filter"),
		Definition->GetConfigWarningReason(),
		FName(TEXT("MissingPositiveCardFilter")));
	TestTrue(TEXT("Blocked-only filter has error"), Errors.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunWorldCardInteractionDefinitionValidationMissingRewardSpec,
	"Wacom.Data.RunWorldCardInteractionDefinition.Validation.MissingRewardReportsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunWorldCardInteractionDefinitionValidationMissingRewardSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunWorldCardInteractionDefinition> Definition(
		MakeValidRunWorldCardInteractionDefinition(GetTransientPackage()));
	Definition->Rewards.Reset();

	TArray<FText> Errors;
	TestFalse(TEXT("Missing reward fails"),
		ValidateRunWorldCardInteractionDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("Helper reports missing reward"),
		Definition->GetConfigWarningReason(),
		FName(TEXT("MissingReward")));
	TestTrue(TEXT("Missing reward has error"), Errors.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunWorldCardInteractionDefinitionValidationInvalidGoldSpec,
	"Wacom.Data.RunWorldCardInteractionDefinition.Validation.InvalidGoldRewardReportsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunWorldCardInteractionDefinitionValidationInvalidGoldSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunWorldCardInteractionDefinition> Definition(
		MakeValidRunWorldCardInteractionDefinition(GetTransientPackage()));
	if (Definition->Rewards.IsValidIndex(0))
	{
		Definition->Rewards[0].GoldAmount = 0;
	}

	TArray<FText> Errors;
	TestFalse(TEXT("Invalid gold reward fails"),
		ValidateRunWorldCardInteractionDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("Helper reports invalid gold"),
		Definition->GetConfigWarningReason(),
		FName(TEXT("InvalidGoldReward")));
	TestTrue(TEXT("Invalid gold has error"), Errors.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunWorldCardInteractionDefinitionValidationMissingCardRewardSpec,
	"Wacom.Data.RunWorldCardInteractionDefinition.Validation.MissingCardDefinitionReportsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunWorldCardInteractionDefinitionValidationMissingCardRewardSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunWorldCardInteractionDefinition> Definition(
		MakeValidRunWorldCardInteractionDefinition(GetTransientPackage()));
	FWacomRunWorldCardInteractionReward CardReward;
	CardReward.Type = EWacomRunWorldCardInteractionRewardType::Card;
	CardReward.CardDefinition = nullptr;
	Definition->Rewards = { CardReward };

	TArray<FText> Errors;
	TestFalse(TEXT("Missing card reward definition fails"),
		ValidateRunWorldCardInteractionDefinitionForTest(Definition.Get(), Errors));
	TestEqual(TEXT("Helper reports missing card definition"),
		Definition->GetConfigWarningReason(),
		FName(TEXT("MissingCardDefinition")));
	TestTrue(TEXT("Missing card definition has error"), Errors.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunWorldCardInteractionDefinitionAssetSpec,
	"Wacom.Data.RunWorldCardInteractionDefinition.DebugKeyGoldInteractionDefinitionAssetLoads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunWorldCardInteractionDefinitionAssetSpec::RunTest(
	const FString& /*Parameters*/)
{
	UWacomRunWorldCardInteractionDefinition* Definition =
		LoadDebugKeyGoldInteractionDefinition();
	if (!TestNotNull(TEXT("Generated debug key interaction definition"), Definition))
	{
		AddError(TEXT("Run WacomRegenerateContent before this asset validation test."));
		return false;
	}

	TArray<FText> Errors;
	TestTrue(TEXT("Generated definition validates"),
		ValidateRunWorldCardInteractionDefinitionForTest(Definition, Errors));
	TestEqual(TEXT("Interaction id"),
		Definition->InteractionId,
		FName(TEXT("WorldCardInteraction.DebugKeyGold3")));
	TestEqual(TEXT("One allowed definition"),
		Definition->AllowedCardDefinitions.Num(),
		1);
	TestNotNull(TEXT("Allowed debug key definition"),
		Definition->AllowedCardDefinitions.IsValidIndex(0)
			? Definition->AllowedCardDefinitions[0].Get()
			: nullptr);
	if (Definition->AllowedCardDefinitions.IsValidIndex(0)
		&& Definition->AllowedCardDefinitions[0])
	{
		TestEqual(TEXT("Allowed definition card id"),
			Definition->AllowedCardDefinitions[0]->CardId,
			FName(TEXT("DebugKey")));
	}
	TestEqual(TEXT("One allowed card id"),
		Definition->AllowedCardIds.Num(),
		1);
	if (Definition->AllowedCardIds.IsValidIndex(0))
	{
		TestEqual(TEXT("Allowed card id"),
			Definition->AllowedCardIds[0],
			FName(TEXT("DebugKey")));
	}
	TestEqual(TEXT("One reward"),
		Definition->Rewards.Num(),
		1);
	if (Definition->Rewards.IsValidIndex(0))
	{
		TestEqual(TEXT("Gold reward type"),
			Definition->Rewards[0].Type,
			EWacomRunWorldCardInteractionRewardType::Gold);
		TestEqual(TEXT("Gold reward amount"),
			Definition->Rewards[0].GoldAmount,
			3);
	}
	TestTrue(TEXT("Consumes source card"),
		Definition->bConsumeCardOnSuccess);
	TestEqual(TEXT("Preview prompt"),
		Definition->PreviewPromptText.ToString(),
		FString(TEXT("使用钥匙打开宝箱")));
	TestEqual(TEXT("Completed prompt"),
		Definition->CompletedPromptText.ToString(),
		FString(TEXT("宝箱已打开")));
	TestEqual(TEXT("Rejected prompt"),
		Definition->RejectedCardPromptText.ToString(),
		FString(TEXT("需要钥匙")));
	TestEqual(TEXT("Config prompt"),
		Definition->ConfigWarningPromptText.ToString(),
		FString(TEXT("场景交互配置异常")));
	TestEqual(TEXT("Source unavailable prompt"),
		Definition->SourceCardUnavailablePromptText.ToString(),
		FString(TEXT("这张卡无法用于交互")));
	TestEqual(TEXT("Generic failure prompt"),
		Definition->GenericFailurePromptText.ToString(),
		FString(TEXT("无法完成场景交互")));
	return true;
}
