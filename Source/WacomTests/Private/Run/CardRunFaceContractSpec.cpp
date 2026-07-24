// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "Validation/CardDefinitionValidation.h"

#include "UObject/StrongObjectPtr.h"

namespace WacomCardRunFaceContractSpec
{
	struct FSampleAssetExpectation
	{
		const TCHAR* ObjectPath;
		const TCHAR* CardId;
		EWacomRunCardTargetMode TargetMode;
		FGameplayTag ActionTag;
		const TCHAR* Description;
	};

	UCardDefinition* MakeValidBattleOnlyCard(UObject* Outer)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = TEXT("RunFace.Contract");
		Card->Rarity = WacomTags::Card_Rarity_White;
		return Card;
	}

	bool Validate(const UCardDefinition* Card, TArray<FText>& OutErrors)
	{
		return FWacomCardDefinitionValidation::Validate(Card, OutErrors);
	}

	TArray<FSampleAssetExpectation> SampleAssetExpectations()
	{
		return {
			{
				TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_AntennaSearch.DA_Card_Starter_AntennaSearch"),
				TEXT("Starter.AntennaSearch"),
				EWacomRunCardTargetMode::Route,
				WacomTags::Run_Card_Action_Reveal,
				TEXT("观察前方路线，揭示 1 个未知节点的内容。")
			},
			{
				TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_DebugKey.DA_Card_DebugKey"),
				TEXT("DebugKey"),
				EWacomRunCardTargetMode::WorldTarget,
				WacomTags::Run_Card_Action_Unlock,
				TEXT("解锁 1 个上锁的场景目标。")
			},
			{
				TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_MoltCut.DA_Card_Starter_MoltCut"),
				TEXT("Starter.MoltCut"),
				EWacomRunCardTargetMode::WorldTarget,
				WacomTags::Run_Card_Action_Break,
				TEXT("破坏 1 个可被工具拆除的场景目标。")
			},
			{
				TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_ChitinWard.DA_Card_Starter_ChitinWard"),
				TEXT("Starter.ChitinWard"),
				EWacomRunCardTargetMode::WorldTarget,
				WacomTags::Run_Card_Action_Feed,
				TEXT("向 1 个可接受供给的场景目标投喂。")
			}
		};
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataCardRunFaceDefaultsAndTagsSpec,
	"Wacom.Data.Card.RunFace.DefaultsAndTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataCardRunFaceDefaultsAndTagsSpec::RunTest(const FString& /*Parameters*/)
{
	const FWacomRunCardFaceDefinition Defaults;
	TestFalse(TEXT("RunFace is migration-safe and disabled by default"), Defaults.bEnabled);
	TestEqual(
		TEXT("Default target mode is ready for a world target"),
		Defaults.TargetMode,
		EWacomRunCardTargetMode::WorldTarget);
	TestEqual(
		TEXT("Default disposition exhausts only for the current room"),
		Defaults.UseDisposition,
		EWacomRunCardUseDisposition::ExhaustForCurrentRoom);
	TestEqual(TEXT("Default action magnitude is positive"), Defaults.PrimaryAction.Magnitude, 1);

	const FGameplayTag ActionRoot = WacomTags::Run_Card_Action;
	TestTrue(TEXT("Run action root is registered"), ActionRoot.IsValid());
	const TArray<FGameplayTag> ActionTags = {
		WacomTags::Run_Card_Action_Reveal,
		WacomTags::Run_Card_Action_Unlock,
		WacomTags::Run_Card_Action_Break,
		WacomTags::Run_Card_Action_Fix,
		WacomTags::Run_Card_Action_Ignite,
		WacomTags::Run_Card_Action_Feed
	};
	TestEqual(TEXT("Six concrete Run actions are registered"), ActionTags.Num(), 6);
	for (int32 Index = 0; Index < ActionTags.Num(); ++Index)
	{
		const FGameplayTag& ActionTag = ActionTags[Index];
		TestTrue(
			FString::Printf(TEXT("Action %d is registered"), Index),
			ActionTag.IsValid());
		TestTrue(
			FString::Printf(TEXT("Action %d belongs to the Run action root"), Index),
			ActionTag.MatchesTag(ActionRoot));
		TestFalse(
			FString::Printf(TEXT("Action %d is a concrete child"), Index),
			ActionTag.MatchesTagExact(ActionRoot));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataCardRunFaceValidationSpec,
	"Wacom.Data.Card.RunFace.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataCardRunFaceValidationSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomCardRunFaceContractSpec;

	TStrongObjectPtr<UCardDefinition> Card(MakeValidBattleOnlyCard(GetTransientPackage()));
	TArray<FText> Errors;

	Card->RunFace.TargetMode = EWacomRunCardTargetMode::None;
	Card->RunFace.PrimaryAction.Magnitude = 0;
	TestTrue(
		TEXT("Disabled RunFace skips migration validation without warnings or errors"),
		Validate(Card.Get(), Errors));
	TestTrue(TEXT("Disabled RunFace produces no errors"), Errors.IsEmpty());
	TestFalse(TEXT("Card reports no enabled RunFace"), Card->HasEnabledRunFace());

	Card->RunFace.bEnabled = true;
	Card->RunFace.Description = FText::FromString(TEXT("打开挡住路线的机关。"));
	Card->RunFace.TargetMode = EWacomRunCardTargetMode::WorldTarget;
	Card->RunFace.PrimaryAction.ActionTag = WacomTags::Run_Card_Action_Unlock;
	Card->RunFace.PrimaryAction.Magnitude = 2;
	TestTrue(TEXT("Valid enabled RunFace passes"), Validate(Card.Get(), Errors));
	TestTrue(TEXT("Card reports enabled RunFace"), Card->HasEnabledRunFace());

	Card->RunFace.Description = FText::GetEmpty();
	TestFalse(TEXT("Enabled RunFace requires a description"), Validate(Card.Get(), Errors));
	Card->RunFace.Description = FText::FromString(TEXT("打开挡住路线的机关。"));

	Card->RunFace.TargetMode = EWacomRunCardTargetMode::None;
	TestFalse(TEXT("Enabled RunFace requires a target mode"), Validate(Card.Get(), Errors));
	Card->RunFace.TargetMode = EWacomRunCardTargetMode::WorldTarget;

	Card->RunFace.PrimaryAction.ActionTag = FGameplayTag();
	TestFalse(TEXT("Enabled RunFace requires an action tag"), Validate(Card.Get(), Errors));
	Card->RunFace.PrimaryAction.ActionTag = WacomTags::Run_Card_Action;
	TestFalse(TEXT("Run action root is not a concrete action"), Validate(Card.Get(), Errors));
	Card->RunFace.PrimaryAction.ActionTag = WacomTags::Card_Keyword_Tool;
	TestFalse(TEXT("Action outside Run.Card.Action is rejected"), Validate(Card.Get(), Errors));
	Card->RunFace.PrimaryAction.ActionTag = WacomTags::Run_Card_Action_Unlock;

	Card->RunFace.PrimaryAction.Magnitude = 0;
	TestFalse(TEXT("Run action magnitude must be positive"), Validate(Card.Get(), Errors));
	Card->RunFace.PrimaryAction.Magnitude = -1;
	TestFalse(TEXT("Negative Run action magnitude is rejected"), Validate(Card.Get(), Errors));
	Card->RunFace.PrimaryAction.Magnitude = 1;
	TestTrue(TEXT("Restored valid RunFace passes"), Validate(Card.Get(), Errors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataCardRunFaceSampleAssetsSpec,
	"Wacom.Data.Card.RunFace.SampleAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataCardRunFaceSampleAssetsSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomCardRunFaceContractSpec;

	for (const FSampleAssetExpectation& Expected : SampleAssetExpectations())
	{
		UCardDefinition* Card =
			LoadObject<UCardDefinition>(nullptr, Expected.ObjectPath);
		if (!TestNotNull(
				*FString::Printf(TEXT("Sample card loads: %s"), Expected.ObjectPath),
				Card))
		{
			continue;
		}

		TestEqual(
			*FString::Printf(TEXT("%s keeps CardId"), Expected.CardId),
			Card->CardId,
			FName(Expected.CardId));
		TestTrue(
			*FString::Printf(TEXT("%s enables RunFace"), Expected.CardId),
			Card->HasEnabledRunFace());
		TestEqual(
			*FString::Printf(TEXT("%s target mode"), Expected.CardId),
			Card->RunFace.TargetMode,
			Expected.TargetMode);
		TestEqual(
			*FString::Printf(TEXT("%s action tag"), Expected.CardId),
			Card->RunFace.PrimaryAction.ActionTag,
			Expected.ActionTag);
		TestEqual(
			*FString::Printf(TEXT("%s magnitude"), Expected.CardId),
			Card->RunFace.PrimaryAction.Magnitude,
			1);
		TestEqual(
			*FString::Printf(TEXT("%s disposition"), Expected.CardId),
			Card->RunFace.UseDisposition,
			EWacomRunCardUseDisposition::ExhaustForCurrentRoom);
		TestEqual(
			*FString::Printf(TEXT("%s description"), Expected.CardId),
			Card->RunFace.Description.ToString(),
			FString(Expected.Description));
		TestTrue(
			*FString::Printf(TEXT("%s keeps shared name fallback"), Expected.CardId),
			Card->RunFace.DisplayNameOverride.IsEmpty());
		TestNull(
			*FString::Printf(TEXT("%s keeps shared illustration fallback"), Expected.CardId),
			Card->RunFace.IllustrationOverride.Get());
		TestNull(
			*FString::Printf(TEXT("%s keeps shared depth-map fallback"), Expected.CardId),
			Card->RunFace.IllustrationDepthMapOverride.Get());
	}
	return true;
}
