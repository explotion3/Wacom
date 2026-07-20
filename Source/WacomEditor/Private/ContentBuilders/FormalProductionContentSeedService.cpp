// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/FormalProductionContentSeedService.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Cards/CardDefinition.h"
#include "ContentBuilders/ContentBuilderHelpers.h"
#include "Dom/JsonObject.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/RunEventDefinition.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Pickups/RunPickupDefinition.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Shops/ShopDefinition.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Validation/CardDefinitionValidation.h"
#include "Validation/EncounterDefinitionValidation.h"
#include "Validation/EnemyBehaviorDefinitionValidation.h"
#include "Validation/EnemyDefinitionValidation.h"
#include "Validation/EnemyPartDefinitionValidation.h"
#include "Validation/RunEventDefinitionValidation.h"
#include "Validation/RunPickupDefinitionValidation.h"
#include "Validation/ShopDefinitionValidation.h"

namespace Wacom::ContentBuilder::FormalProductionSeedPrivate
{
	constexpr int32 ReportSchemaVersion = 1;

	bool IsSelected(
		const FFormalProductionContentManifestEntry& Entry,
		const EFormalProductionContentGroup Group)
	{
		return Group == EFormalProductionContentGroup::All || Entry.Group == Group;
	}

	void AppendTextErrors(const TArray<FText>& TextErrors, TArray<FString>& OutErrors)
	{
		for (const FText& Error : TextErrors)
		{
			OutErrors.Add(Error.ToString());
		}
	}

	void NormalizeCardTunables(UCardDefinition& Actual, const UCardDefinition& Expected)
	{
		Actual.DisplayName = Expected.DisplayName;
		Actual.Description = Expected.Description;
		Actual.CardIllustration = Expected.CardIllustration;
		Actual.CardIllustrationDepthMap = Expected.CardIllustrationDepthMap;
		Actual.BaseCost = Expected.BaseCost;
		Actual.Rarity = Expected.Rarity;
		if (Actual.Effects.Num() == Expected.Effects.Num())
		{
			for (int32 Index = 0; Index < Actual.Effects.Num(); ++Index)
			{
				Actual.Effects[Index].Magnitude = Expected.Effects[Index].Magnitude;
			}
		}
	}

	void NormalizeBehaviorTunables(
		UEnemyBehaviorDefinition& Actual,
		const UEnemyBehaviorDefinition& Expected)
	{
		if (Actual.Phases.Num() != Expected.Phases.Num())
		{
			return;
		}
		for (int32 PhaseIndex = 0; PhaseIndex < Actual.Phases.Num(); ++PhaseIndex)
		{
			auto& ActualSets = Actual.Phases[PhaseIndex].IntentSets;
			const auto& ExpectedSets = Expected.Phases[PhaseIndex].IntentSets;
			if (ActualSets.Num() != ExpectedSets.Num())
			{
				continue;
			}
			for (int32 SetIndex = 0; SetIndex < ActualSets.Num(); ++SetIndex)
			{
				auto& ActualIntents = ActualSets[SetIndex].Intents;
				const auto& ExpectedIntents = ExpectedSets[SetIndex].Intents;
				if (ActualIntents.Num() != ExpectedIntents.Num())
				{
					continue;
				}
				for (int32 IntentIndex = 0; IntentIndex < ActualIntents.Num(); ++IntentIndex)
				{
					FIntentDefinition& ActualIntent = ActualIntents[IntentIndex].Intent;
					const FIntentDefinition& ExpectedIntent = ExpectedIntents[IntentIndex].Intent;
					ActualIntent.DisplayName = ExpectedIntent.DisplayName;
					ActualIntent.Initiative = ExpectedIntent.Initiative;
					ActualIntent.ResistanceValue = ExpectedIntent.ResistanceValue;
					if (ActualIntent.Effects.Num() == ExpectedIntent.Effects.Num())
					{
						for (int32 EffectIndex = 0;
							EffectIndex < ActualIntent.Effects.Num(); ++EffectIndex)
						{
							ActualIntent.Effects[EffectIndex].Magnitude =
								ExpectedIntent.Effects[EffectIndex].Magnitude;
							ActualIntent.Effects[EffectIndex].HandAffliction.TargetCardCount =
								ExpectedIntent.Effects[EffectIndex].HandAffliction.TargetCardCount;
						}
					}
				}
			}
		}
	}

	void NormalizeEventTunables(
		UWacomRunEventDefinition& Actual,
		const UWacomRunEventDefinition& Expected)
	{
		Actual.DisplayName = Expected.DisplayName;
		if (Actual.Nodes.Num() != Expected.Nodes.Num())
		{
			return;
		}
		for (int32 NodeIndex = 0; NodeIndex < Actual.Nodes.Num(); ++NodeIndex)
		{
			auto& ActualNode = Actual.Nodes[NodeIndex];
			const auto& ExpectedNode = Expected.Nodes[NodeIndex];
			ActualNode.TitleText = ExpectedNode.TitleText;
			ActualNode.BodyText = ExpectedNode.BodyText;
			if (ActualNode.Choices.Num() != ExpectedNode.Choices.Num())
			{
				continue;
			}
			for (int32 ChoiceIndex = 0; ChoiceIndex < ActualNode.Choices.Num(); ++ChoiceIndex)
			{
				auto& ActualChoice = ActualNode.Choices[ChoiceIndex];
				const auto& ExpectedChoice = ExpectedNode.Choices[ChoiceIndex];
				ActualChoice.LabelText = ExpectedChoice.LabelText;
				if (ActualChoice.Conditions.Num() == ExpectedChoice.Conditions.Num())
				{
					for (int32 ConditionIndex = 0;
						ConditionIndex < ActualChoice.Conditions.Num(); ++ConditionIndex)
					{
						ActualChoice.Conditions[ConditionIndex].Value =
							ExpectedChoice.Conditions[ConditionIndex].Value;
					}
				}
				if (ActualChoice.Effects.Num() == ExpectedChoice.Effects.Num())
				{
					for (int32 EffectIndex = 0;
						EffectIndex < ActualChoice.Effects.Num(); ++EffectIndex)
					{
						ActualChoice.Effects[EffectIndex].Value =
							ExpectedChoice.Effects[EffectIndex].Value;
					}
				}
			}
		}
	}

	void NormalizeTunables(UObject& Actual, const UObject& Expected)
	{
		if (UCardDefinition* ActualCard = Cast<UCardDefinition>(&Actual))
		{
			NormalizeCardTunables(*ActualCard, *CastChecked<UCardDefinition>(&Expected));
		}
		else if (UEnemyBehaviorDefinition* ActualBehavior =
			Cast<UEnemyBehaviorDefinition>(&Actual))
		{
			NormalizeBehaviorTunables(*ActualBehavior,
				*CastChecked<UEnemyBehaviorDefinition>(&Expected));
		}
		else if (UEnemyPartDefinition* ActualPart = Cast<UEnemyPartDefinition>(&Actual))
		{
			const auto* ExpectedPart = CastChecked<UEnemyPartDefinition>(&Expected);
			ActualPart->DisplayName = ExpectedPart->DisplayName;
			ActualPart->MaxHp = ExpectedPart->MaxHp;
			ActualPart->ExperienceReward = ExpectedPart->ExperienceReward;
		}
		else if (UEnemyDefinition* ActualEnemy = Cast<UEnemyDefinition>(&Actual))
		{
			ActualEnemy->DisplayName = CastChecked<UEnemyDefinition>(&Expected)->DisplayName;
		}
		else if (UEncounterDefinition* ActualEncounter = Cast<UEncounterDefinition>(&Actual))
		{
			ActualEncounter->DisplayName =
				CastChecked<UEncounterDefinition>(&Expected)->DisplayName;
		}
		else if (UWacomRunEventDefinition* ActualEvent =
			Cast<UWacomRunEventDefinition>(&Actual))
		{
			NormalizeEventTunables(*ActualEvent,
				*CastChecked<UWacomRunEventDefinition>(&Expected));
		}
		else if (UWacomRunPickupDefinition* ActualPickup =
			Cast<UWacomRunPickupDefinition>(&Actual))
		{
			ActualPickup->GoldAmount =
				CastChecked<UWacomRunPickupDefinition>(&Expected)->GoldAmount;
		}
		else if (UShopDefinition* ActualShop = Cast<UShopDefinition>(&Actual))
		{
			const auto* ExpectedShop = CastChecked<UShopDefinition>(&Expected);
			ActualShop->DisplayName = ExpectedShop->DisplayName;
			if (ActualShop->Offers.Num() == ExpectedShop->Offers.Num())
			{
				for (int32 Index = 0; Index < ActualShop->Offers.Num(); ++Index)
				{
					ActualShop->Offers[Index].Price = ExpectedShop->Offers[Index].Price;
				}
			}
		}
	}

	UObject* ResolveObject(
		const FString& PackagePath,
		TMap<FString, UObject*>& ObjectsByPackage)
	{
		if (UObject** Found = ObjectsByPackage.Find(PackagePath))
		{
			return *Found;
		}
		UObject* Loaded = LoadObject<UObject>(
			nullptr, *FormalProductionObjectPathForPackage(PackagePath));
		if (Loaded)
		{
			ObjectsByPackage.Add(PackagePath, Loaded);
		}
		return Loaded;
	}

	bool ValidateActualAgainstExpected(
		const FFormalProductionContentProfile& Profile,
		UObject& Actual,
		const FFormalProductionContentManifestEntry& Entry,
		TMap<FString, UObject*>& ObjectsByPackage,
		const bool bStrict,
		TArray<FString>& OutErrors)
	{
		if (!ValidateFormalProductionObjectWithSharedRules(Actual, OutErrors))
		{
			return false;
		}
		TStrongObjectPtr<UObject> Expected;
		if (!BuildFormalProductionExpectedObject(
			Profile, Entry, ObjectsByPackage, Expected, OutErrors))
		{
			return false;
		}
		return CompareFormalProductionEditableProperties(
			Actual, *Expected.Get(), bStrict, OutErrors);
	}

	bool WriteReportJson(
		const FFormalProductionContentProfile& Profile,
		FFormalProductionContentBuildReport& Report)
	{
		if (Report.ReportPath.IsEmpty())
		{
			Report.ReportPath = FPaths::ProjectSavedDir()
				/ Profile.ReportFolder
				/ FString::Printf(TEXT("%s-report.json"),
					*FormalProductionGroupToString(Report.Options.Group));
		}
		else if (FPaths::IsRelative(Report.ReportPath))
		{
			Report.ReportPath = FPaths::ConvertRelativePathToFull(
				FPaths::ProjectDir(), Report.ReportPath);
		}
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Report.ReportPath), true);

		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetNumberField(TEXT("schemaVersion"), ReportSchemaVersion);
		Root->SetStringField(TEXT("timestampUtc"), FDateTime::UtcNow().ToIso8601());
		Root->SetStringField(TEXT("projectPath"),
			FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()));
		Root->SetStringField(TEXT("group"),
			FormalProductionGroupToString(Report.Options.Group));
		Root->SetBoolField(TEXT("seedMissing"), Report.Options.bSeedMissing);
		Root->SetBoolField(TEXT("compareSeedDefaults"),
			Report.Options.bCompareSeedDefaults);
		Root->SetNumberField(TEXT("manifestCount"), Report.ManifestCount);
		Root->SetNumberField(TEXT("selectedCount"), Report.SelectedCount);
		Root->SetNumberField(TEXT("createdCount"), Report.CreatedCount);
		Root->SetNumberField(TEXT("existingCount"), Report.ExistingCount);
		Root->SetNumberField(TEXT("missingCount"), Report.MissingCount);
		Root->SetNumberField(TEXT("failedCount"), Report.FailedCount);
		Root->SetNumberField(TEXT("savedCount"), Report.SavedCount);
		Root->SetNumberField(TEXT("exitCode"), Report.ExitCode);
		Root->SetStringField(TEXT("failureCategory"), Report.FailureCategory);
		TArray<TSharedPtr<FJsonValue>> JsonEntries;
		for (const FFormalProductionContentEntryReport& Entry : Report.Entries)
		{
			TSharedRef<FJsonObject> JsonEntry = MakeShared<FJsonObject>();
			JsonEntry->SetStringField(TEXT("package"), Entry.PackagePath);
			JsonEntry->SetStringField(TEXT("class"), Entry.ClassName);
			JsonEntry->SetStringField(TEXT("stableId"), Entry.StableId.ToString());
			JsonEntry->SetStringField(TEXT("state"),
				FormalProductionStateToString(Entry.State));
			JsonEntry->SetBoolField(TEXT("saved"), Entry.bSaved);
			TArray<TSharedPtr<FJsonValue>> Diagnostics;
			for (const FString& Diagnostic : Entry.Diagnostics)
			{
				Diagnostics.Add(MakeShared<FJsonValueString>(Diagnostic));
			}
			JsonEntry->SetArrayField(TEXT("diagnostics"), Diagnostics);
			JsonEntries.Add(MakeShared<FJsonValueObject>(JsonEntry));
		}
		Root->SetArrayField(TEXT("entries"), JsonEntries);
		FString Json;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
		return FJsonSerializer::Serialize(Root, Writer)
			&& FFileHelper::SaveStringToFile(Json, *Report.ReportPath);
	}
}

namespace Wacom::ContentBuilder
{
	FString FormalProductionGroupToString(const EFormalProductionContentGroup Group)
	{
		switch (Group)
		{
		case EFormalProductionContentGroup::Cards: return TEXT("Cards");
		case EFormalProductionContentGroup::EnemyGraph: return TEXT("EnemyGraph");
		case EFormalProductionContentGroup::NodeDefinitions: return TEXT("NodeDefinitions");
		case EFormalProductionContentGroup::All:
		default: return TEXT("All");
		}
	}

	FString FormalProductionStateToString(const EFormalProductionContentEntryState State)
	{
		switch (State)
		{
		case EFormalProductionContentEntryState::NotProcessed: return TEXT("NotProcessed");
		case EFormalProductionContentEntryState::Missing: return TEXT("Missing");
		case EFormalProductionContentEntryState::Existing: return TEXT("Existing");
		case EFormalProductionContentEntryState::Created: return TEXT("Created");
		case EFormalProductionContentEntryState::Failed: return TEXT("Failed");
		default: return TEXT("Unknown");
		}
	}

	FString FormalProductionObjectPathForPackage(const FString& PackagePath)
	{
		return PackagePath + TEXT(".")
			+ FPackageName::GetLongPackageAssetName(PackagePath);
	}

	bool ParseFormalProductionContentOptions(
		const TArray<FString>& Arguments,
		FFormalProductionContentOptions& OutOptions,
		TArray<FString>& OutErrors)
	{
		OutOptions = FFormalProductionContentOptions();
		bool bSawGroup = false;
		bool bSawReport = false;
		for (FString Argument : Arguments)
		{
			while (Argument.RemoveFromStart(TEXT("-"))) {}
			if (Argument.Equals(TEXT("SeedMissing"), ESearchCase::IgnoreCase))
			{
				if (OutOptions.bSeedMissing)
				{
					OutErrors.Add(TEXT("SeedMissing was specified more than once"));
				}
				OutOptions.bSeedMissing = true;
				continue;
			}
			if (Argument.Equals(TEXT("CompareSeedDefaults"), ESearchCase::IgnoreCase))
			{
				if (OutOptions.bCompareSeedDefaults)
				{
					OutErrors.Add(TEXT("CompareSeedDefaults was specified more than once"));
				}
				OutOptions.bCompareSeedDefaults = true;
				continue;
			}
			FString Value;
			if (Argument.StartsWith(TEXT("Group="), ESearchCase::IgnoreCase))
			{
				Value = Argument.Mid(FCString::Strlen(TEXT("Group=")));
				if (bSawGroup)
				{
					OutErrors.Add(TEXT("Group was specified more than once"));
				}
				bSawGroup = true;
				if (Value.Equals(TEXT("Cards"), ESearchCase::IgnoreCase))
				{
					OutOptions.Group = EFormalProductionContentGroup::Cards;
				}
				else if (Value.Equals(TEXT("EnemyGraph"), ESearchCase::IgnoreCase))
				{
					OutOptions.Group = EFormalProductionContentGroup::EnemyGraph;
				}
				else if (Value.Equals(TEXT("NodeDefinitions"), ESearchCase::IgnoreCase))
				{
					OutOptions.Group = EFormalProductionContentGroup::NodeDefinitions;
				}
				else if (Value.Equals(TEXT("All"), ESearchCase::IgnoreCase))
				{
					OutOptions.Group = EFormalProductionContentGroup::All;
				}
				else
				{
					OutErrors.Add(FString::Printf(TEXT("Invalid Group: %s"), *Value));
				}
				continue;
			}
			if (Argument.StartsWith(TEXT("Report="), ESearchCase::IgnoreCase))
			{
				Value = Argument.Mid(FCString::Strlen(TEXT("Report=")));
				if (bSawReport)
				{
					OutErrors.Add(TEXT("Report was specified more than once"));
				}
				bSawReport = true;
				OutOptions.ReportPath = Value.TrimQuotes();
				if (OutOptions.ReportPath.IsEmpty())
				{
					OutErrors.Add(TEXT("Report path cannot be empty"));
				}
				continue;
			}
			OutErrors.Add(FString::Printf(TEXT("Unknown argument: %s"), *Argument));
		}
		return OutErrors.IsEmpty();
	}

	void TokenizeFormalProductionCommandletParams(
		const FString& Params,
		TArray<FString>& OutArguments)
	{
		OutArguments.Reset();
		const TCHAR* Cursor = *Params;
		FString Token;
		while (FParse::Token(Cursor, Token, true))
		{
			FString Canonical = Token;
			while (Canonical.RemoveFromStart(TEXT("-"))) {}
			const bool bEngineArgument =
				Canonical.StartsWith(TEXT("run="), ESearchCase::IgnoreCase)
				|| Canonical.StartsWith(TEXT("abslog="), ESearchCase::IgnoreCase)
				|| Canonical.StartsWith(TEXT("log="), ESearchCase::IgnoreCase)
				|| Canonical.StartsWith(TEXT("ddc-"), ESearchCase::IgnoreCase)
				|| Canonical.Equals(TEXT("Unattended"), ESearchCase::IgnoreCase)
				|| Canonical.Equals(TEXT("NoPause"), ESearchCase::IgnoreCase)
				|| Canonical.Equals(TEXT("NoSplash"), ESearchCase::IgnoreCase)
				|| Canonical.Equals(TEXT("NullRHI"), ESearchCase::IgnoreCase)
				|| Canonical.Equals(TEXT("NoSound"), ESearchCase::IgnoreCase)
				|| Canonical.Equals(TEXT("NoDreamShaderEditorBridge"), ESearchCase::IgnoreCase)
				|| Canonical.Equals(TEXT("Multiprocess"), ESearchCase::IgnoreCase)
				|| Canonical.Equals(TEXT("stdout"), ESearchCase::IgnoreCase)
				|| Canonical.Equals(TEXT("FullStdOutLogOutput"), ESearchCase::IgnoreCase);
			if (bEngineArgument)
			{
				continue;
			}
			OutArguments.Add(MoveTemp(Token));
		}
	}

	bool ValidateFormalProductionContentManifest(
		const FFormalProductionContentProfile& Profile,
		TArray<FString>& OutErrors)
	{
		if (!Profile.Manifest)
		{
			OutErrors.Add(TEXT("Content profile has no manifest"));
			return false;
		}
		const auto& Manifest = *Profile.Manifest;
		const int32 ExpectedTotal = Profile.ExpectedCardsCount
			+ Profile.ExpectedEnemyGraphCount + Profile.ExpectedNodeDefinitionsCount;
		if (Manifest.Num() != ExpectedTotal)
		{
			OutErrors.Add(FString::Printf(TEXT("Expected %d manifest entries, got %d"),
				ExpectedTotal, Manifest.Num()));
		}
		TSet<FString> Packages;
		TSet<FName> StableIds;
		TSet<FString> ObjectPaths;
		TMap<UClass*, int32> ClassCounts;
		int32 CardsCount = 0;
		int32 EnemyGraphCount = 0;
		int32 NodeDefinitionsCount = 0;
		for (const auto& Entry : Manifest)
		{
			if (!FPackageName::IsValidLongPackageName(Entry.PackagePath))
			{
				OutErrors.Add(FString::Printf(TEXT("Invalid package path: %s"),
					*Entry.PackagePath));
			}
			if (!Entry.PackagePath.StartsWith(TEXT("/Game/Wacom/Data/")))
			{
				OutErrors.Add(FString::Printf(TEXT("Package outside formal data root: %s"),
					*Entry.PackagePath));
			}
			if (Packages.Contains(Entry.PackagePath))
			{
				OutErrors.Add(FString::Printf(TEXT("Duplicate package: %s"),
					*Entry.PackagePath));
			}
			Packages.Add(Entry.PackagePath);
			const FString ObjectPath = FormalProductionObjectPathForPackage(Entry.PackagePath);
			if (ObjectPaths.Contains(ObjectPath))
			{
				OutErrors.Add(FString::Printf(TEXT("Duplicate object path: %s"), *ObjectPath));
			}
			ObjectPaths.Add(ObjectPath);
			if (Entry.StableId.IsNone() || StableIds.Contains(Entry.StableId))
			{
				OutErrors.Add(FString::Printf(TEXT("Missing or duplicate stable id: %s"),
					*Entry.StableId.ToString()));
			}
			StableIds.Add(Entry.StableId);
			if (!Entry.AssetClass)
			{
				OutErrors.Add(FString::Printf(TEXT("Missing class: %s"), *Entry.PackagePath));
			}
			else
			{
				ClassCounts.FindOrAdd(Entry.AssetClass)++;
			}
			switch (Entry.Group)
			{
			case EFormalProductionContentGroup::Cards: ++CardsCount; break;
			case EFormalProductionContentGroup::EnemyGraph: ++EnemyGraphCount; break;
			case EFormalProductionContentGroup::NodeDefinitions: ++NodeDefinitionsCount; break;
			case EFormalProductionContentGroup::All:
				OutErrors.Add(FString::Printf(
					TEXT("Manifest entry cannot use All group: %s"), *Entry.PackagePath));
				break;
			}
		}
		if (CardsCount != Profile.ExpectedCardsCount
			|| EnemyGraphCount != Profile.ExpectedEnemyGraphCount
			|| NodeDefinitionsCount != Profile.ExpectedNodeDefinitionsCount)
		{
			OutErrors.Add(FString::Printf(TEXT("Group counts are %d/%d/%d, expected %d/%d/%d"),
				CardsCount, EnemyGraphCount, NodeDefinitionsCount,
				Profile.ExpectedCardsCount, Profile.ExpectedEnemyGraphCount,
				Profile.ExpectedNodeDefinitionsCount));
		}
		for (const TPair<UClass*, int32>& Expected : Profile.ExpectedClassCounts)
		{
			const int32 Actual = ClassCounts.FindRef(Expected.Key);
			if (Actual != Expected.Value)
			{
				OutErrors.Add(FString::Printf(TEXT("Class %s count is %d, expected %d"),
					*GetNameSafe(Expected.Key), Actual, Expected.Value));
			}
		}
		for (const FString& ReadOnlyPackage : Profile.ReadOnlyDependencies)
		{
			if (Packages.Contains(ReadOnlyPackage))
			{
				OutErrors.Add(FString::Printf(TEXT("Read-only dependency is writable: %s"),
					*ReadOnlyPackage));
			}
		}
		if (Profile.ValidateProfileSpecific)
		{
			Profile.ValidateProfileSpecific(OutErrors);
		}
		return OutErrors.IsEmpty();
	}

	bool ValidateFormalProductionObjectWithSharedRules(
		UObject& Object,
		TArray<FString>& OutErrors)
	{
		TArray<FText> Errors;
		bool bValid = false;
		if (const UCardDefinition* Card = Cast<UCardDefinition>(&Object))
		{
			bValid = FWacomCardDefinitionValidation::Validate(Card, Errors);
		}
		else if (const UEnemyPartDefinition* Part = Cast<UEnemyPartDefinition>(&Object))
		{
			bValid = FWacomEnemyPartDefinitionValidation::Validate(
				Part, Errors, EWacomEnemyPartValidationProfile::FormalProduction);
		}
		else if (const UEnemyBehaviorDefinition* Behavior =
			Cast<UEnemyBehaviorDefinition>(&Object))
		{
			bValid = FWacomEnemyBehaviorDefinitionValidation::Validate(Behavior, Errors);
		}
		else if (const UEnemyDefinition* Enemy = Cast<UEnemyDefinition>(&Object))
		{
			bValid = FWacomEnemyDefinitionValidation::Validate(Enemy, Errors);
		}
		else if (const UEncounterDefinition* Encounter = Cast<UEncounterDefinition>(&Object))
		{
			bValid = FWacomEncounterDefinitionValidation::Validate(Encounter, Errors);
		}
		else if (const UWacomRunEventDefinition* Event =
			Cast<UWacomRunEventDefinition>(&Object))
		{
			bValid = FWacomRunEventDefinitionValidation::Validate(Event, Errors);
		}
		else if (const UWacomRunPickupDefinition* Pickup =
			Cast<UWacomRunPickupDefinition>(&Object))
		{
			bValid = FWacomRunPickupDefinitionValidation::Validate(Pickup, Errors);
		}
		else if (const UShopDefinition* Shop = Cast<UShopDefinition>(&Object))
		{
			bValid = FWacomShopDefinitionValidation::Validate(Shop, Errors);
		}
		else
		{
			OutErrors.Add(FString::Printf(TEXT("No validator for class %s"),
				*GetNameSafe(Object.GetClass())));
			return false;
		}
		FormalProductionSeedPrivate::AppendTextErrors(Errors, OutErrors);
		return bValid;
	}

	bool CompareFormalProductionEditableProperties(
		const UObject& Actual,
		const UObject& Expected,
		const bool bStrict,
		TArray<FString>& OutErrors)
	{
		if (Actual.GetClass() != Expected.GetClass())
		{
			OutErrors.Add(TEXT("Actual and expected classes differ"));
			return false;
		}
		TStrongObjectPtr<UObject> Comparable(
			DuplicateObject(&Actual, GetTransientPackage()));
		if (!Comparable.IsValid())
		{
			OutErrors.Add(TEXT("Could not duplicate object for read-only comparison"));
			return false;
		}
		if (!bStrict)
		{
			FormalProductionSeedPrivate::NormalizeTunables(*Comparable.Get(), Expected);
		}
		for (TFieldIterator<FProperty> It(Actual.GetClass()); It; ++It)
		{
			FProperty* Property = *It;
			if (!Property->HasAnyPropertyFlags(CPF_Edit)
				|| Property->HasAnyPropertyFlags(
					CPF_Transient | CPF_DuplicateTransient | CPF_NonPIEDuplicateTransient))
			{
				continue;
			}
			if (!Property->Identical_InContainer(Comparable.Get(), &Expected, PPF_None))
			{
				OutErrors.Add(FString::Printf(TEXT("%s mismatch: %s"),
					bStrict ? TEXT("Seed default") : TEXT("Stable structure"),
					*Property->GetName()));
			}
		}
		return OutErrors.IsEmpty();
	}

	bool BuildFormalProductionExpectedObject(
		const FFormalProductionContentProfile& Profile,
		const FFormalProductionContentManifestEntry& Entry,
		TMap<FString, UObject*>& ObjectsByPackage,
		TStrongObjectPtr<UObject>& OutExpected,
		TArray<FString>& OutErrors)
	{
		OutExpected = TStrongObjectPtr<UObject>(NewObject<UObject>(
			GetTransientPackage(), Entry.AssetClass, NAME_None, RF_Transient));
		if (!OutExpected.IsValid())
		{
			OutErrors.Add(FString::Printf(TEXT("Could not allocate expected object for %s"),
				*Entry.PackagePath));
			return false;
		}
		const FFormalProductionResolveObject Resolver =
			[&ObjectsByPackage](const FString& PackagePath)
			{
				return FormalProductionSeedPrivate::ResolveObject(
					PackagePath, ObjectsByPackage);
			};
		if (!Profile.ConfigureExpected
			|| !Profile.ConfigureExpected(*OutExpected.Get(), Entry, Resolver, OutErrors))
		{
			if (OutErrors.IsEmpty())
			{
				OutErrors.Add(FString::Printf(TEXT("No seed configuration for %s"),
					*Entry.StableId.ToString()));
			}
			return false;
		}
		return ValidateFormalProductionObjectWithSharedRules(*OutExpected.Get(), OutErrors);
	}

	bool ValidateFormalProductionLoadedAssets(
		const FFormalProductionContentProfile& Profile,
		const bool bCompareSeedDefaults,
		TArray<FString>& OutErrors)
	{
		if (!ValidateFormalProductionContentManifest(Profile, OutErrors))
		{
			return false;
		}
		TMap<FString, UObject*> ObjectsByPackage;
		for (const auto& Entry : *Profile.Manifest)
		{
			UObject* Asset = LoadObject<UObject>(
				nullptr, *FormalProductionObjectPathForPackage(Entry.PackagePath));
			if (!Asset)
			{
				OutErrors.Add(FString::Printf(TEXT("%s: asset failed to load"),
					*Entry.PackagePath));
				continue;
			}
			if (Asset->GetClass() != Entry.AssetClass)
			{
				OutErrors.Add(FString::Printf(TEXT("%s: class is %s, expected %s"),
					*Entry.PackagePath, *GetNameSafe(Asset->GetClass()),
					*GetNameSafe(Entry.AssetClass)));
				continue;
			}
			ObjectsByPackage.Add(Entry.PackagePath, Asset);
		}
		if (ObjectsByPackage.Num() != Profile.Manifest->Num())
		{
			return false;
		}
		for (const auto& Entry : *Profile.Manifest)
		{
			TArray<FString> EntryErrors;
			UObject* Asset = ObjectsByPackage.FindRef(Entry.PackagePath);
			if (!Asset || !FormalProductionSeedPrivate::ValidateActualAgainstExpected(
				Profile, *Asset, Entry, ObjectsByPackage,
				bCompareSeedDefaults, EntryErrors))
			{
				for (const FString& Error : EntryErrors)
				{
					OutErrors.Add(Entry.StableId.ToString() + TEXT(": ") + Error);
				}
			}
		}
		return OutErrors.IsEmpty();
	}

	bool ValidateFormalProductionDependencyClosure(
		const FFormalProductionContentProfile& Profile,
		TArray<FString>& OutErrors)
	{
		if (!Profile.Manifest)
		{
			OutErrors.Add(TEXT("Dependency closure requires a manifest."));
			return false;
		}

		TSet<FName> AllowedPackages;
		for (const FFormalProductionContentManifestEntry& Entry : *Profile.Manifest)
		{
			AllowedPackages.Add(FName(*Entry.PackagePath));
		}
		for (const FString& Dependency : Profile.ReadOnlyDependencies)
		{
			AllowedPackages.Add(FName(*Dependency));
		}

		FAssetRegistryModule& AssetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
		for (const FFormalProductionContentManifestEntry& RootEntry : *Profile.Manifest)
		{
			TArray<FName> Queue = {FName(*RootEntry.PackagePath)};
			TSet<FName> Seen;
			for (int32 Index = 0; Index < Queue.Num(); ++Index)
			{
				const FName PackageName = Queue[Index];
				if (PackageName.IsNone() || Seen.Contains(PackageName))
				{
					continue;
				}
				Seen.Add(PackageName);
				const FString PackageString = PackageName.ToString();
				if (PackageString.StartsWith(TEXT("/Game/"))
					&& !AllowedPackages.Contains(PackageName))
				{
					OutErrors.Add(FString::Printf(
						TEXT("Forbidden Production dependency: %s -> %s"),
						*RootEntry.PackagePath,
						*PackageString));
					continue;
				}

				TArray<FAssetDependency> Dependencies;
				AssetRegistry.GetDependencies(
					FAssetIdentifier(PackageName),
					Dependencies,
					UE::AssetRegistry::EDependencyCategory::Package);
				for (const FAssetDependency& Dependency : Dependencies)
				{
					const FName DependencyPackage = Dependency.AssetId.PackageName;
					if (!DependencyPackage.IsNone()
						&& DependencyPackage.ToString().StartsWith(TEXT("/Game/"))
						&& !Seen.Contains(DependencyPackage))
					{
						Queue.Add(DependencyPackage);
					}
				}
			}
		}
		return OutErrors.IsEmpty();
	}

	int32 RunFormalProductionContentSeedService(
		const FFormalProductionContentProfile& Profile,
		const TArray<FString>& Arguments,
		FFormalProductionContentBuildReport* OutReport)
	{
		using namespace FormalProductionSeedPrivate;
		FFormalProductionContentBuildReport Report;
		TArray<FString> ParseErrors;
		if (!ParseFormalProductionContentOptions(Arguments, Report.Options, ParseErrors))
		{
			Report.ExitCode = 2;
			Report.FailureCategory = TEXT("Arguments");
			for (const FString& Error : ParseErrors)
			{
				UE_LOG(LogTemp, Error, TEXT("[%s] %s"), *Profile.LogLabel, *Error);
			}
			if (OutReport) { *OutReport = Report; }
			return Report.ExitCode;
		}
		Report.ReportPath = Report.Options.ReportPath;
		Report.ManifestCount = Profile.Manifest ? Profile.Manifest->Num() : 0;
		TArray<FString> ManifestErrors;
		if (!ValidateFormalProductionContentManifest(Profile, ManifestErrors))
		{
			Report.ExitCode = 1;
			Report.FailureCategory = TEXT("Manifest");
			for (const FString& Error : ManifestErrors)
			{
				UE_LOG(LogTemp, Error, TEXT("[%s] %s"), *Profile.LogLabel, *Error);
			}
			if (!WriteReportJson(Profile, Report))
			{
				Report.ExitCode = 3;
				Report.FailureCategory = TEXT("ReportWrite");
			}
			if (OutReport) { *OutReport = Report; }
			return Report.ExitCode;
		}

		const auto& Manifest = *Profile.Manifest;
		TArray<int32> SelectedManifestIndices;
		TMap<FString, UObject*> LoadedObjects;
		for (int32 ManifestIndex = 0; ManifestIndex < Manifest.Num(); ++ManifestIndex)
		{
			const auto& Entry = Manifest[ManifestIndex];
			if (FPackageName::DoesPackageExist(Entry.PackagePath))
			{
				if (UObject* Existing = LoadObject<UObject>(
					nullptr, *FormalProductionObjectPathForPackage(Entry.PackagePath)))
				{
					LoadedObjects.Add(Entry.PackagePath, Existing);
				}
			}
			if (!IsSelected(Entry, Report.Options.Group))
			{
				continue;
			}
			SelectedManifestIndices.Add(ManifestIndex);
			auto& EntryReport = Report.Entries.AddDefaulted_GetRef();
			EntryReport.PackagePath = Entry.PackagePath;
			EntryReport.ClassName = GetNameSafe(Entry.AssetClass);
			EntryReport.StableId = Entry.StableId;
		}
		Report.SelectedCount = SelectedManifestIndices.Num();

		bool bPreflightFailed = false;
		for (int32 SelectedIndex = 0; SelectedIndex < SelectedManifestIndices.Num(); ++SelectedIndex)
		{
			const auto& Entry = Manifest[SelectedManifestIndices[SelectedIndex]];
			auto& EntryReport = Report.Entries[SelectedIndex];
			if (!FPackageName::DoesPackageExist(Entry.PackagePath))
			{
				EntryReport.State = EFormalProductionContentEntryState::Missing;
				++Report.MissingCount;
				continue;
			}
			UObject* Existing = LoadedObjects.FindRef(Entry.PackagePath);
			if (!Existing)
			{
				EntryReport.State = EFormalProductionContentEntryState::Failed;
				EntryReport.Diagnostics.Add(
					TEXT("Package exists but the expected object failed to load"));
				++Report.FailedCount;
				bPreflightFailed = true;
				continue;
			}
			if (Existing->GetClass() != Entry.AssetClass)
			{
				EntryReport.State = EFormalProductionContentEntryState::Failed;
				EntryReport.Diagnostics.Add(FString::Printf(
					TEXT("Wrong class %s; expected %s"),
					*GetNameSafe(Existing->GetClass()), *GetNameSafe(Entry.AssetClass)));
				++Report.FailedCount;
				bPreflightFailed = true;
			}
			else
			{
				EntryReport.State = EFormalProductionContentEntryState::Existing;
			}
		}

		if (Report.Options.bSeedMissing && !bPreflightFailed)
		{
			TMap<FString, UObject*> PreflightObjects = LoadedObjects;
			TArray<TStrongObjectPtr<UObject>> PreflightKeepAlive;
			for (int32 SelectedIndex = 0;
				SelectedIndex < SelectedManifestIndices.Num(); ++SelectedIndex)
			{
				const auto& Entry = Manifest[SelectedManifestIndices[SelectedIndex]];
				TStrongObjectPtr<UObject> Expected(nullptr);
				TArray<FString> Errors;
				if (!BuildFormalProductionExpectedObject(
					Profile, Entry, PreflightObjects, Expected, Errors))
				{
					auto& EntryReport = Report.Entries[SelectedIndex];
					EntryReport.State = EFormalProductionContentEntryState::Failed;
					EntryReport.Diagnostics.Append(Errors);
					++Report.FailedCount;
					bPreflightFailed = true;
					break;
				}
				if (!PreflightObjects.Contains(Entry.PackagePath))
				{
					PreflightObjects.Add(Entry.PackagePath, Expected.Get());
					PreflightKeepAlive.Add(MoveTemp(Expected));
				}
			}
		}

		if (!Report.Options.bSeedMissing)
		{
			for (int32 SelectedIndex = 0;
				SelectedIndex < SelectedManifestIndices.Num(); ++SelectedIndex)
			{
				const auto& Entry = Manifest[SelectedManifestIndices[SelectedIndex]];
				auto& EntryReport = Report.Entries[SelectedIndex];
				if (EntryReport.State == EFormalProductionContentEntryState::Missing)
				{
					EntryReport.Diagnostics.Add(TEXT("Package is missing in inspect-only mode"));
					continue;
				}
				if (EntryReport.State == EFormalProductionContentEntryState::Failed)
				{
					continue;
				}
				TArray<FString> Errors;
				UObject* Existing = LoadedObjects.FindRef(Entry.PackagePath);
				if (!Existing || !ValidateActualAgainstExpected(
					Profile, *Existing, Entry, LoadedObjects,
					Report.Options.bCompareSeedDefaults, Errors))
				{
					EntryReport.State = EFormalProductionContentEntryState::Failed;
					EntryReport.Diagnostics.Append(Errors);
					++Report.FailedCount;
				}
				else
				{
					++Report.ExistingCount;
				}
			}
			Report.ExitCode = Report.FailedCount == 0 && Report.MissingCount == 0 ? 0 : 1;
			Report.FailureCategory = Report.ExitCode == 0 ? TEXT("") : TEXT("Validation");
		}
		else if (bPreflightFailed)
		{
			Report.ExitCode = 1;
			Report.FailureCategory = TEXT("Preflight");
		}
		else
		{
			Report.MissingCount = 0;
			for (int32 SelectedIndex = 0;
				SelectedIndex < SelectedManifestIndices.Num(); ++SelectedIndex)
			{
				const auto& Entry = Manifest[SelectedManifestIndices[SelectedIndex]];
				auto& EntryReport = Report.Entries[SelectedIndex];
				if (EntryReport.State == EFormalProductionContentEntryState::Existing)
				{
					TArray<FString> Errors;
					UObject* Existing = LoadedObjects.FindRef(Entry.PackagePath);
					if (!Existing || !ValidateActualAgainstExpected(
						Profile, *Existing, Entry, LoadedObjects,
						Report.Options.bCompareSeedDefaults, Errors))
					{
						EntryReport.State = EFormalProductionContentEntryState::Failed;
						EntryReport.Diagnostics.Append(Errors);
						++Report.FailedCount;
						Report.ExitCode = 1;
						Report.FailureCategory = TEXT("Validation");
						break;
					}
					++Report.ExistingCount;
					continue;
				}

				UPackage* Package = CreatePackage(*Entry.PackagePath);
				const FName AssetName(*FPackageName::GetLongPackageAssetName(Entry.PackagePath));
				UObject* Asset = Package
					? NewObject<UObject>(Package, Entry.AssetClass, AssetName,
						RF_Public | RF_Standalone | RF_Transactional)
					: nullptr;
				TArray<FString> Errors;
				const FFormalProductionResolveObject Resolver =
					[&LoadedObjects](const FString& PackagePath)
					{
						return ResolveObject(PackagePath, LoadedObjects);
					};
				if (!Package || !Asset || !Profile.ConfigureExpected
					|| !Profile.ConfigureExpected(*Asset, Entry, Resolver, Errors)
					|| !ValidateFormalProductionObjectWithSharedRules(*Asset, Errors))
				{
					EntryReport.State = EFormalProductionContentEntryState::Failed;
					EntryReport.Diagnostics.Append(Errors);
					if (EntryReport.Diagnostics.IsEmpty())
					{
						EntryReport.Diagnostics.Add(TEXT("Could not create/configure asset"));
					}
					++Report.FailedCount;
					Report.ExitCode = 3;
					Report.FailureCategory = TEXT("Create");
					break;
				}
				LoadedObjects.Add(Entry.PackagePath, Asset);
				TArray<FString> ComparisonErrors;
				if (!ValidateActualAgainstExpected(
					Profile, *Asset, Entry, LoadedObjects, true, ComparisonErrors))
				{
					EntryReport.State = EFormalProductionContentEntryState::Failed;
					EntryReport.Diagnostics.Append(ComparisonErrors);
					++Report.FailedCount;
					Report.ExitCode = 3;
					Report.FailureCategory = TEXT("CreateValidation");
					break;
				}
				if (!SaveAssetPackage(Package, Asset, Entry.PackagePath))
				{
					EntryReport.State = EFormalProductionContentEntryState::Failed;
					EntryReport.Diagnostics.Add(TEXT("SavePackage failed"));
					++Report.FailedCount;
					Report.ExitCode = 3;
					Report.FailureCategory = TEXT("Save");
					break;
				}
				UPackage::WaitForAsyncFileWrites();
				UObject* Reloaded = LoadObject<UObject>(
					nullptr, *FormalProductionObjectPathForPackage(Entry.PackagePath));
				if (!Reloaded || Reloaded->GetClass() != Entry.AssetClass)
				{
					EntryReport.State = EFormalProductionContentEntryState::Failed;
					EntryReport.Diagnostics.Add(TEXT("Post-save load/class check failed"));
					++Report.FailedCount;
					Report.ExitCode = 3;
					Report.FailureCategory = TEXT("Reload");
					break;
				}
				EntryReport.State = EFormalProductionContentEntryState::Created;
				EntryReport.bSaved = true;
				++Report.CreatedCount;
				++Report.SavedCount;
			}
			if (Report.ExitCode == 0 && Report.FailedCount == 0)
			{
				Report.FailureCategory.Reset();
			}
		}

		if (!WriteReportJson(Profile, Report))
		{
			UE_LOG(LogTemp, Error, TEXT("[%s] Failed to write report: %s"),
				*Profile.LogLabel, *Report.ReportPath);
			Report.ExitCode = 3;
			Report.FailureCategory = TEXT("ReportWrite");
		}
		UE_LOG(LogTemp, Display,
			TEXT("[%s] Group=%s Created=%d Existing=%d Missing=%d Failed=%d Saved=%d Report=%s Exit=%d"),
			*Profile.LogLabel, *FormalProductionGroupToString(Report.Options.Group),
			Report.CreatedCount, Report.ExistingCount, Report.MissingCount,
			Report.FailedCount, Report.SavedCount, *Report.ReportPath, Report.ExitCode);
		if (OutReport)
		{
			*OutReport = Report;
		}
		return Report.ExitCode;
	}
}
