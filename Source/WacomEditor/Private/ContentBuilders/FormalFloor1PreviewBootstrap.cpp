// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/FormalFloor1PreviewBootstrap.h"

#include "Actors/WacomRunFloorSceneDescriptorActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "ContentBuilders/ContentBuilderHelpers.h"
#include "ContentBuilders/FormalFloor1ProductionSceneBuilder.h"
#include "Dom/JsonObject.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/WacomGameMode.h"
#include "GameFramework/WacomRunFloorPreviewGameMode.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Testing/WacomFormalFloor1PreviewBootstrapAutomationTestView.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "UI/Foundation/WacomExplorationHUD.h"
#include "UI/Menus/WacomJourneySummaryScreen.h"
#include "UObject/Package.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	const FString PreviewGameModePackage =
		TEXT("/Game/Wacom/Run/Preview/GM_WacomRunFloorPreview");
	const FString ProductionMapPackage =
		TEXT("/Game/Wacom/Maps/Run/L_Run_Floor_Main_01");
	const FString SourceGameModeClassPath =
		TEXT("/Game/Wacom/Core/GameModes/GM_Wacom.GM_Wacom_C");
	const FName PreviewPlayerStartName(TEXT("PlayerStart_FloorMain01Preview"));

	FString ObjectPathForPackage(const FString& PackagePath)
	{
		return PackagePath + TEXT(".")
			+ FPackageName::GetLongPackageAssetName(PackagePath);
	}

	FString DefaultReportPath()
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("FormalFloor1PreviewBootstrap"),
			TEXT("Spec016-preview-bootstrap.json"));
	}

	UClass* LoadSourceGameModeClass(TArray<FString>& OutErrors)
	{
		UClass* SourceClass = LoadObject<UClass>(nullptr, *SourceGameModeClassPath);
		if (!SourceClass || !SourceClass->IsChildOf(AWacomGameMode::StaticClass()))
		{
			OutErrors.Add(TEXT("Missing or invalid GM_Wacom source class"));
			return nullptr;
		}
		return SourceClass;
	}

	UBlueprint* LoadPreviewBlueprint()
	{
		return LoadObject<UBlueprint>(
			nullptr, *ObjectPathForPackage(PreviewGameModePackage));
	}

	bool ValidatePreviewBlueprintCollisionPolicy(
		const bool bParentMatches,
		const bool bGeneratedClassExists,
		const bool bCompileStateValid,
		const bool bCdoExists,
		const bool bConfigurationMatches,
		TArray<FString>& OutErrors)
	{
		if (!bParentMatches
			|| !bGeneratedClassExists
			|| !bCompileStateValid
			|| !bCdoExists
			|| !bConfigurationMatches)
		{
			OutErrors.Add(TEXT("Preview GameMode Blueprint parent/compile/CDO/configuration mismatch"));
			return false;
		}
		return true;
	}

	bool ValidatePreviewBlueprint(
		const UBlueprint& Blueprint,
		const AWacomGameMode& SourceCDO,
		TArray<FString>& OutErrors)
	{
		const AWacomRunFloorPreviewGameMode* PreviewCDO =
			Blueprint.GeneratedClass
				? Cast<AWacomRunFloorPreviewGameMode>(
					Blueprint.GeneratedClass->GetDefaultObject())
				: nullptr;
		const bool bConfigurationMatches = PreviewCDO
			&& PreviewCDO->PlayerControllerClass == SourceCDO.PlayerControllerClass
			&& PreviewCDO->DefaultPawnClass == SourceCDO.DefaultPawnClass
			&& PreviewCDO->DefaultCharacter == SourceCDO.DefaultCharacter
			&& PreviewCDO->BattleHUDClass == SourceCDO.BattleHUDClass
			&& PreviewCDO->ExplorationHUDClass == SourceCDO.ExplorationHUDClass
			&& PreviewCDO->JourneySummaryScreenClass
				== SourceCDO.JourneySummaryScreenClass
			&& PreviewCDO->DefaultJourneyDefinition == nullptr;
		return ValidatePreviewBlueprintCollisionPolicy(
			Blueprint.ParentClass == AWacomRunFloorPreviewGameMode::StaticClass(),
			Blueprint.GeneratedClass != nullptr,
			Blueprint.Status != BS_Error,
			PreviewCDO != nullptr,
			bConfigurationMatches,
			OutErrors);
	}

	UBlueprint* CreatePreviewBlueprint(
		const AWacomGameMode& SourceCDO,
		TArray<FString>& OutErrors)
	{
		UPackage* Package = CreatePackage(*PreviewGameModePackage);
		const FName AssetName(
			*FPackageName::GetLongPackageAssetName(PreviewGameModePackage));
		UBlueprint* Blueprint = Package
			? FKismetEditorUtilities::CreateBlueprint(
				AWacomRunFloorPreviewGameMode::StaticClass(),
				Package,
				AssetName,
				BPTYPE_Normal,
				UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass())
			: nullptr;
		AWacomRunFloorPreviewGameMode* PreviewCDO =
			Blueprint && Blueprint->GeneratedClass
				? Cast<AWacomRunFloorPreviewGameMode>(
					Blueprint->GeneratedClass->GetDefaultObject())
				: nullptr;
		if (!Blueprint || !PreviewCDO)
		{
			OutErrors.Add(TEXT("Could not create Preview GameMode Blueprint/CDO"));
			return nullptr;
		}

		PreviewCDO->Modify();
		PreviewCDO->PlayerControllerClass = SourceCDO.PlayerControllerClass;
		PreviewCDO->DefaultPawnClass = SourceCDO.DefaultPawnClass;
		PreviewCDO->DefaultCharacter = SourceCDO.DefaultCharacter;
		PreviewCDO->BattleHUDClass = SourceCDO.BattleHUDClass;
		PreviewCDO->ExplorationHUDClass = SourceCDO.ExplorationHUDClass;
		PreviewCDO->JourneySummaryScreenClass = SourceCDO.JourneySummaryScreenClass;
		PreviewCDO->DefaultJourneyDefinition = nullptr;
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (Blueprint->Status == BS_Error
			|| !ValidatePreviewBlueprint(*Blueprint, SourceCDO, OutErrors))
		{
			return nullptr;
		}
		return Blueprint;
	}

	struct FMapInspection
	{
		UWorld* World = nullptr;
		UWacomFloorMapDefinition* Floor = nullptr;
		AWacomRunMapNodeAnchorActor* EntryAnchor = nullptr;
		APlayerStart* PreviewPlayerStart = nullptr;
		int32 PlayerStartCount = 0;
		bool bSceneValid = false;
		bool bReadyForBootstrap = false;
		bool bConfigured = false;
	};

	bool IsFiniteTransform(const FTransform& Transform)
	{
		return !Transform.ContainsNaN();
	}

	struct FMapCollisionPolicyFacts
	{
		bool bEntryAnchorValid = true;
		bool bSceneContractValid = true;
		int32 PlayerStartCount = 0;
		bool bExactPreviewPlayerStart = false;
		bool bPreviewPlayerStartTransformMatches = true;
		bool bPreviewPlayerStartIsPlain = true;
		bool bWorldSettingsValid = true;
		bool bGameModeAllowed = true;
	};

	bool ValidateMapCollisionPolicy(
		const FMapCollisionPolicyFacts& Facts,
		TArray<FString>& OutErrors)
	{
		if (!Facts.bEntryAnchorValid)
		{
			OutErrors.Add(TEXT("Production map must have one finite Node.Entry Anchor"));
		}
		if (!Facts.bSceneContractValid)
		{
			OutErrors.Add(TEXT("Production map failed the Spec 015 scene contract"));
		}
		if (Facts.PlayerStartCount > 1
			|| (Facts.PlayerStartCount == 1
				&& !Facts.bExactPreviewPlayerStart))
		{
			OutErrors.Add(TEXT("Unexpected or duplicate PlayerStart in Production map"));
		}
		if (Facts.bExactPreviewPlayerStart
			&& !Facts.bPreviewPlayerStartTransformMatches)
		{
			OutErrors.Add(TEXT("Preview PlayerStart transform does not match Entry Anchor"));
		}
		if (Facts.bExactPreviewPlayerStart
			&& !Facts.bPreviewPlayerStartIsPlain)
		{
			OutErrors.Add(TEXT("Preview PlayerStart must not carry Run identity/ownership"));
		}
		if (!Facts.bWorldSettingsValid || !Facts.bGameModeAllowed)
		{
			OutErrors.Add(TEXT("Unexpected Production map GameMode override"));
		}
		return OutErrors.IsEmpty();
	}

	FMapInspection InspectProductionMap(
		UClass* SourceGameModeClass,
		UClass* PreviewGameModeClass,
		TArray<FString>& OutErrors)
	{
		FMapInspection Result;
		Result.World = UEditorLoadingAndSavingUtils::LoadMap(ProductionMapPackage);
		if (!Result.World)
		{
			OutErrors.Add(TEXT("Production Floor 1 map could not be loaded"));
			return Result;
		}

		AWacomRunFloorSceneDescriptorActor* Descriptor = nullptr;
		int32 DescriptorCount = 0;
		int32 EntryAnchorCount = 0;
		for (TActorIterator<AActor> It(Result.World); It; ++It)
		{
			AActor* Actor = *It;
			if (AWacomRunFloorSceneDescriptorActor* Candidate =
				Cast<AWacomRunFloorSceneDescriptorActor>(Actor))
			{
				Descriptor = Candidate;
				++DescriptorCount;
			}
			if (AWacomRunMapNodeAnchorActor* Anchor =
				Cast<AWacomRunMapNodeAnchorActor>(Actor))
			{
				if (Anchor->NodeId == TEXT("Node.Entry"))
				{
					Result.EntryAnchor = Anchor;
					++EntryAnchorCount;
				}
			}
			if (APlayerStart* PlayerStart = Cast<APlayerStart>(Actor))
			{
				++Result.PlayerStartCount;
				if (PlayerStart->GetFName() == PreviewPlayerStartName
					&& PlayerStart->GetActorLabel() == PreviewPlayerStartName.ToString())
				{
					Result.PreviewPlayerStart = PlayerStart;
				}
			}
		}
		if (DescriptorCount != 1 || !Descriptor
			|| !IsValid(Descriptor->FloorDefinition))
		{
			OutErrors.Add(TEXT("Production map must have one valid Floor Descriptor"));
			return Result;
		}
		Result.Floor = Descriptor->FloorDefinition;
		if (Result.Floor->FloorId != TEXT("Floor.Main.01"))
		{
			OutErrors.Add(TEXT("Production map Descriptor must reference Floor.Main.01"));
		}
		TArray<FString> SceneErrors;
		Result.bSceneValid =
			ValidateFormalFloor1ProductionWorld(
				*Result.World, *Result.Floor, SceneErrors);
		OutErrors.Append(SceneErrors);

		AWorldSettings* WorldSettings = Result.World->GetWorldSettings();
		UClass* CurrentGameMode = WorldSettings
			? WorldSettings->DefaultGameMode.Get()
			: nullptr;
		const bool bAllowedGameMode = !CurrentGameMode
			|| CurrentGameMode == SourceGameModeClass
			|| (PreviewGameModeClass && CurrentGameMode == PreviewGameModeClass);
		FMapCollisionPolicyFacts CollisionFacts;
		CollisionFacts.bEntryAnchorValid = EntryAnchorCount == 1
			&& Result.EntryAnchor
			&& IsFiniteTransform(Result.EntryAnchor->GetActorTransform());
		CollisionFacts.bSceneContractValid = Result.bSceneValid;
		CollisionFacts.PlayerStartCount = Result.PlayerStartCount;
		CollisionFacts.bExactPreviewPlayerStart =
			Result.PreviewPlayerStart != nullptr;
		CollisionFacts.bPreviewPlayerStartTransformMatches =
			!Result.PreviewPlayerStart || !Result.EntryAnchor
			|| Result.PreviewPlayerStart->GetActorTransform().Equals(
				Result.EntryAnchor->GetActorTransform(), 0.01);
		CollisionFacts.bPreviewPlayerStartIsPlain =
			!Result.PreviewPlayerStart
			|| (!Result.PreviewPlayerStart->FindComponentByClass<
					UWacomRunMapNodeBindingComponent>()
				&& !Result.PreviewPlayerStart->ActorHasTag(
					TEXT("Wacom.Generated.RunExploration")));
		CollisionFacts.bWorldSettingsValid = WorldSettings != nullptr;
		CollisionFacts.bGameModeAllowed = bAllowedGameMode;
		ValidateMapCollisionPolicy(CollisionFacts, OutErrors);

		Result.bReadyForBootstrap = OutErrors.IsEmpty();
		Result.bConfigured = Result.bReadyForBootstrap
			&& PreviewGameModeClass
			&& CurrentGameMode == PreviewGameModeClass
			&& Result.PlayerStartCount == 1
			&& Result.PreviewPlayerStart;
		return Result;
	}

	bool ConfigureProductionMap(
		FMapInspection& Inspection,
		UClass* PreviewGameModeClass,
		bool& bOutModified,
		TArray<FString>& OutErrors)
	{
		bOutModified = false;
		if (!Inspection.bReadyForBootstrap
			|| !Inspection.World
			|| !Inspection.EntryAnchor
			|| !PreviewGameModeClass)
		{
			OutErrors.Add(TEXT("Production map was not eligible for Preview mutation"));
			return false;
		}

		AWorldSettings* WorldSettings = Inspection.World->GetWorldSettings();
		if (WorldSettings->DefaultGameMode.Get() != PreviewGameModeClass)
		{
			WorldSettings->Modify();
			WorldSettings->DefaultGameMode = PreviewGameModeClass;
			bOutModified = true;
		}
		if (!Inspection.PreviewPlayerStart)
		{
			FActorSpawnParameters Params;
			Params.Name = PreviewPlayerStartName;
			Params.NameMode =
				FActorSpawnParameters::ESpawnActorNameMode::Requested;
			Params.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			APlayerStart* PlayerStart =
				Inspection.World->SpawnActor<APlayerStart>(
					APlayerStart::StaticClass(),
					Inspection.EntryAnchor->GetActorTransform(),
					Params);
			if (!PlayerStart)
			{
				OutErrors.Add(TEXT("Could not create Preview PlayerStart"));
				return false;
			}
			PlayerStart->SetActorLabel(PreviewPlayerStartName.ToString());
			Inspection.PreviewPlayerStart = PlayerStart;
			bOutModified = true;
		}
		return true;
	}

	struct FAssetInspectionFacts
	{
		UClass* SourceGameModeClass = nullptr;
		const AWacomGameMode* SourceCDO = nullptr;
		UBlueprint* PreviewBlueprint = nullptr;
		UClass* PreviewGameModeClass = nullptr;
		bool bPreviewExists = false;
		bool bMapExists = false;
		FMapInspection Map;
	};

	FAssetInspectionFacts PreflightAssets(TArray<FString>& OutErrors)
	{
		FAssetInspectionFacts Facts;
		Facts.SourceGameModeClass = LoadSourceGameModeClass(OutErrors);
		Facts.SourceCDO = Facts.SourceGameModeClass
			? Cast<AWacomGameMode>(
				Facts.SourceGameModeClass->GetDefaultObject())
			: nullptr;
		if (!Facts.SourceCDO)
		{
			OutErrors.Add(TEXT("GM_Wacom source CDO is unavailable"));
			return Facts;
		}

		Facts.bPreviewExists =
			FPackageName::DoesPackageExist(PreviewGameModePackage);
		if (Facts.bPreviewExists)
		{
			Facts.PreviewBlueprint = LoadPreviewBlueprint();
			if (!Facts.PreviewBlueprint
				|| !ValidatePreviewBlueprint(
					*Facts.PreviewBlueprint, *Facts.SourceCDO, OutErrors))
			{
				OutErrors.Add(TEXT("Existing Preview GameMode asset is not authoritative"));
				return Facts;
			}
			Facts.PreviewGameModeClass = Facts.PreviewBlueprint->GeneratedClass;
		}

		Facts.bMapExists = FPackageName::DoesPackageExist(ProductionMapPackage);
		if (!Facts.bMapExists)
		{
			OutErrors.Add(TEXT("Production Floor 1 map package is missing"));
			return Facts;
		}
		Facts.Map = InspectProductionMap(
			Facts.SourceGameModeClass,
			Facts.PreviewGameModeClass,
			OutErrors);
		return Facts;
	}

	FFormalFloor1PreviewBootstrapPassReport ApplyPass(const bool bAllowMutation)
	{
		FFormalFloor1PreviewBootstrapPassReport Report;
		TArray<FString> Errors;
		if (!ValidateFormalFloor1PreviewBootstrapManifest(Errors))
		{
			Report.Diagnostics.Append(Errors);
			Report.FailedCount = 1;
			return Report;
		}

		FAssetInspectionFacts Facts = PreflightAssets(Errors);
		if (!Errors.IsEmpty())
		{
			Report.Diagnostics.Append(Errors);
			Report.FailedCount = 1;
			return Report;
		}
		if (!bAllowMutation)
		{
			Report.ExistingCount = 1 + (Facts.bPreviewExists ? 1 : 0);
			return Report;
		}

		if (!Facts.bPreviewExists)
		{
			Facts.PreviewBlueprint = CreatePreviewBlueprint(*Facts.SourceCDO, Errors);
			if (!Facts.PreviewBlueprint
				|| !SaveAssetPackage(
					Facts.PreviewBlueprint->GetOutermost(),
					Facts.PreviewBlueprint,
					PreviewGameModePackage))
			{
				if (Errors.IsEmpty())
				{
					Errors.Add(TEXT("Preview GameMode package save failed"));
				}
				Report.Diagnostics.Append(Errors);
				Report.FailedCount = 1;
				return Report;
			}
			Facts.PreviewGameModeClass = Facts.PreviewBlueprint->GeneratedClass;
			++Report.CreatedCount;
			++Report.SavedCount;
			Report.SavedPackages.Add(PreviewGameModePackage);
		}
		else
		{
			++Report.ExistingCount;
		}

		// Re-inspect the map with the now-authoritative Preview class before mutation.
		Errors.Reset();
		Facts.Map = InspectProductionMap(
			Facts.SourceGameModeClass,
			Facts.PreviewGameModeClass,
			Errors);
		if (!Errors.IsEmpty())
		{
			Report.Diagnostics.Append(Errors);
			Report.FailedCount = 1;
			return Report;
		}

		bool bMapModified = false;
		if (!ConfigureProductionMap(
			Facts.Map, Facts.PreviewGameModeClass, bMapModified, Errors)
			|| !Errors.IsEmpty())
		{
			Report.Diagnostics.Append(Errors);
			Report.FailedCount = 1;
			return Report;
		}
		if (bMapModified)
		{
			if (!UEditorLoadingAndSavingUtils::SaveMap(
				Facts.Map.World, ProductionMapPackage))
			{
				Report.Diagnostics.Add(TEXT("Production map save failed"));
				Report.FailedCount = 1;
				return Report;
			}
			++Report.ModifiedCount;
			++Report.SavedCount;
			Report.SavedPackages.Add(ProductionMapPackage);
		}
		else
		{
			++Report.ExistingCount;
		}

		UPackage::WaitForAsyncFileWrites();
		Errors.Reset();
		FAssetInspectionFacts Reloaded = PreflightAssets(Errors);
		if (!Errors.IsEmpty()
			|| !Reloaded.bPreviewExists
			|| !Reloaded.Map.bConfigured)
		{
			Report.Diagnostics.Append(Errors);
			Report.Diagnostics.Add(TEXT("Post-save Preview bootstrap inspection failed"));
			Report.FailedCount = 1;
		}
		return Report;
	}

	TSharedRef<FJsonObject> PassToJson(
		const FFormalFloor1PreviewBootstrapPassReport& Pass)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetNumberField(TEXT("created"), Pass.CreatedCount);
		Json->SetNumberField(TEXT("modified"), Pass.ModifiedCount);
		Json->SetNumberField(TEXT("existing"), Pass.ExistingCount);
		Json->SetNumberField(TEXT("saved"), Pass.SavedCount);
		Json->SetNumberField(TEXT("failed"), Pass.FailedCount);
		TArray<TSharedPtr<FJsonValue>> Saved;
		for (const FString& Package : Pass.SavedPackages)
		{
			Saved.Add(MakeShared<FJsonValueString>(Package));
		}
		Json->SetArrayField(TEXT("savedPackages"), Saved);
		TArray<TSharedPtr<FJsonValue>> Diagnostics;
		for (const FString& Diagnostic : Pass.Diagnostics)
		{
			Diagnostics.Add(MakeShared<FJsonValueString>(Diagnostic));
		}
		Json->SetArrayField(TEXT("diagnostics"), Diagnostics);
		return Json;
	}

	bool WriteReportJson(const FFormalFloor1PreviewBootstrapReport& Report)
	{
		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetNumberField(TEXT("schemaVersion"), 1);
		Root->SetStringField(TEXT("timestampUtc"),
			FDateTime::UtcNow().ToIso8601());
		Root->SetNumberField(TEXT("exitCode"), Report.ExitCode);
		Root->SetStringField(TEXT("failureCategory"), Report.FailureCategory);
		Root->SetObjectField(TEXT("firstPass"), PassToJson(Report.FirstPass));
		Root->SetObjectField(TEXT("secondPass"), PassToJson(Report.SecondPass));
		TArray<TSharedPtr<FJsonValue>> Manifest;
		for (const FString& Package :
			GetFormalFloor1PreviewBootstrapPackageManifest())
		{
			Manifest.Add(MakeShared<FJsonValueString>(Package));
		}
		Root->SetArrayField(TEXT("manifest"), Manifest);

		IFileManager::Get().MakeDirectory(
			*FPaths::GetPath(Report.ReportPath), true);
		FString Output;
		const TSharedRef<TJsonWriter<>> Writer =
			TJsonWriterFactory<>::Create(&Output);
		return FJsonSerializer::Serialize(Root, Writer)
			&& FFileHelper::SaveStringToFile(Output, *Report.ReportPath);
	}
}

namespace Wacom::ContentBuilder
{
	const TArray<FString>& GetFormalFloor1PreviewBootstrapPackageManifest()
	{
		static const TArray<FString> Manifest =
		{
			PreviewGameModePackage,
			ProductionMapPackage,
		};
		return Manifest;
	}

	bool ValidateFormalFloor1PreviewBootstrapManifest(
		TArray<FString>& OutErrors)
	{
		const TArray<FString>& Manifest =
			GetFormalFloor1PreviewBootstrapPackageManifest();
		if (Manifest.Num() != 2)
		{
			OutErrors.Add(TEXT("Preview bootstrap manifest must contain exactly two packages"));
		}
		TSet<FString> Unique;
		for (const FString& Package : Manifest)
		{
			if (!FPackageName::IsValidLongPackageName(Package)
				|| !Package.StartsWith(TEXT("/Game/Wacom/")))
			{
				OutErrors.Add(TEXT("Invalid Preview package: ") + Package);
			}
			if (Unique.Contains(Package))
			{
				OutErrors.Add(TEXT("Duplicate Preview package: ") + Package);
			}
			Unique.Add(Package);
		}
		return OutErrors.IsEmpty();
	}

	FFormalFloor1PreviewBootstrapPassReport
	InspectFormalFloor1PreviewBootstrapAssets()
	{
		return ApplyPass(false);
	}

	int32 RunFormalFloor1PreviewBootstrap(
		FFormalFloor1PreviewBootstrapReport* OutReport)
	{
		FFormalFloor1PreviewBootstrapReport Report;
		Report.ReportPath = DefaultReportPath();
		Report.FirstPass = ApplyPass(true);
		if (!Report.FirstPass.IsOk())
		{
			Report.ExitCode = 1;
			Report.FailureCategory = TEXT("FirstPass");
		}
		else
		{
			Report.SecondPass = ApplyPass(true);
			if (!Report.SecondPass.IsIdempotent())
			{
				Report.ExitCode = 1;
				Report.FailureCategory = TEXT("Idempotence");
			}
		}
		if (!WriteReportJson(Report))
		{
			Report.ExitCode = 2;
			Report.FailureCategory = TEXT("ReportWrite");
		}

		UE_LOG(LogTemp, Display,
			TEXT("[FormalFloor1PreviewBootstrap] First(Created=%d Modified=%d Existing=%d Saved=%d Failed=%d) Second(Created=%d Modified=%d Existing=%d Saved=%d Failed=%d) Report=%s Exit=%d"),
			Report.FirstPass.CreatedCount,
			Report.FirstPass.ModifiedCount,
			Report.FirstPass.ExistingCount,
			Report.FirstPass.SavedCount,
			Report.FirstPass.FailedCount,
			Report.SecondPass.CreatedCount,
			Report.SecondPass.ModifiedCount,
			Report.SecondPass.ExistingCount,
			Report.SecondPass.SavedCount,
			Report.SecondPass.FailedCount,
			*Report.ReportPath,
			Report.ExitCode);
		for (const FString& Diagnostic : Report.FirstPass.Diagnostics)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[FormalFloor1PreviewBootstrap] %s"), *Diagnostic);
		}
		for (const FString& Diagnostic : Report.SecondPass.Diagnostics)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[FormalFloor1PreviewBootstrap] %s"), *Diagnostic);
		}
		if (OutReport)
		{
			*OutReport = MoveTemp(Report);
		}
		return OutReport ? OutReport->ExitCode : Report.ExitCode;
	}
}

FWacomFormalFloor1PreviewBootstrapAutomationSummary
FWacomFormalFloor1PreviewBootstrapAutomationTestView::InspectRealAssets()
{
	using namespace Wacom::ContentBuilder;
	FWacomFormalFloor1PreviewBootstrapAutomationSummary Summary;
	Summary.PackagePaths = GetFormalFloor1PreviewBootstrapPackageManifest();
	Summary.ManifestCount = Summary.PackagePaths.Num();
	Summary.bPreviewBlueprintExists =
		FPackageName::DoesPackageExist(PreviewGameModePackage);
	Summary.bMapExists = FPackageName::DoesPackageExist(ProductionMapPackage);

	TArray<FString> Errors;
	FAssetInspectionFacts Facts = PreflightAssets(Errors);
	Summary.bMapConfigured = Facts.Map.bConfigured;
	Summary.bMapReadyForBootstrap = Facts.Map.bReadyForBootstrap;
	Summary.PlayerStartCount = Facts.Map.PlayerStartCount;
	Summary.ExistingCount = (Facts.bPreviewExists ? 1 : 0)
		+ (Facts.bMapExists ? 1 : 0);
	Summary.MissingCount = Summary.ManifestCount - Summary.ExistingCount;
	Summary.FailedCount = Errors.IsEmpty() ? 0 : 1;
	Summary.SavedCount = 0;
	Summary.Diagnostics = MoveTemp(Errors);
	return Summary;
}

bool FWacomFormalFloor1PreviewBootstrapAutomationTestView::ValidateManifest(
	TArray<FString>& OutErrors)
{
	return Wacom::ContentBuilder::
		ValidateFormalFloor1PreviewBootstrapManifest(OutErrors);
}

bool FWacomFormalFloor1PreviewBootstrapAutomationTestView::
ValidateCollisionPolicyMatrix(TArray<FString>& OutErrors)
{
	auto ExpectBlueprintRejected = [&OutErrors](
		const TCHAR* CaseName,
		const bool bParent,
		const bool bGenerated,
		const bool bCompile,
		const bool bCdo,
		const bool bConfiguration)
	{
		TArray<FString> Errors;
		if (ValidatePreviewBlueprintCollisionPolicy(
			bParent, bGenerated, bCompile, bCdo, bConfiguration, Errors)
			|| Errors.IsEmpty())
		{
			OutErrors.Add(FString::Printf(
				TEXT("Blueprint collision policy accepted %s"), CaseName));
		}
	};
	ExpectBlueprintRejected(TEXT("wrong parent"), false, true, true, true, true);
	ExpectBlueprintRejected(TEXT("compile error"), true, true, false, true, true);
	ExpectBlueprintRejected(TEXT("configuration drift"), true, true, true, true, false);

	auto ExpectMapRejected = [&OutErrors](
		const TCHAR* CaseName,
		const FMapCollisionPolicyFacts& Facts)
	{
		TArray<FString> Errors;
		if (ValidateMapCollisionPolicy(Facts, Errors) || Errors.IsEmpty())
		{
			OutErrors.Add(FString::Printf(
				TEXT("Map collision policy accepted %s"), CaseName));
		}
	};
	FMapCollisionPolicyFacts Facts;
	Facts.bEntryAnchorValid = false;
	ExpectMapRejected(TEXT("invalid Entry Anchor"), Facts);
	Facts = FMapCollisionPolicyFacts();
	Facts.bSceneContractValid = false;
	ExpectMapRejected(TEXT("scene contract drift"), Facts);
	Facts = FMapCollisionPolicyFacts();
	Facts.PlayerStartCount = 1;
	ExpectMapRejected(TEXT("unexpected PlayerStart"), Facts);
	Facts = FMapCollisionPolicyFacts();
	Facts.PlayerStartCount = 2;
	Facts.bExactPreviewPlayerStart = true;
	ExpectMapRejected(TEXT("duplicate PlayerStart"), Facts);
	Facts = FMapCollisionPolicyFacts();
	Facts.PlayerStartCount = 1;
	Facts.bExactPreviewPlayerStart = true;
	Facts.bPreviewPlayerStartTransformMatches = false;
	ExpectMapRejected(TEXT("PlayerStart transform drift"), Facts);
	Facts = FMapCollisionPolicyFacts();
	Facts.PlayerStartCount = 1;
	Facts.bExactPreviewPlayerStart = true;
	Facts.bPreviewPlayerStartIsPlain = false;
	ExpectMapRejected(TEXT("PlayerStart identity contamination"), Facts);
	Facts = FMapCollisionPolicyFacts();
	Facts.bGameModeAllowed = false;
	ExpectMapRejected(TEXT("unexpected GameMode"), Facts);

	TArray<FString> ValidErrors;
	const bool bValidFactsAccepted =
		ValidatePreviewBlueprintCollisionPolicy(
			true, true, true, true, true, ValidErrors)
		&& ValidateMapCollisionPolicy(
			FMapCollisionPolicyFacts(), ValidErrors);
	if (!bValidFactsAccepted || !ValidErrors.IsEmpty())
	{
		OutErrors.Add(TEXT("Collision policy rejected a valid pre-seed state"));
	}
	return OutErrors.IsEmpty();
}
