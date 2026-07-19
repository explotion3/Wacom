// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"
#include "Engine/Blueprint.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

namespace WacomEnemySceneLegacyAuditSpec
{
	bool ContainsAnsiToken(const TArray<uint8>& Bytes, const ANSICHAR* Token)
	{
		const int32 TokenLength = FCStringAnsi::Strlen(Token);
		if (TokenLength <= 0 || Bytes.Num() < TokenLength)
		{
			return false;
		}

		for (int32 Offset = 0; Offset <= Bytes.Num() - TokenLength; ++Offset)
		{
			if (FMemory::Memcmp(Bytes.GetData() + Offset, Token, TokenLength) == 0)
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEnemySceneLegacyAuditSpec,
	"Wacom.Editor.EnemyScene.LegacyAudit.AllGamePackagesUseComponentArchitecture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEnemySceneLegacyAuditSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomEnemySceneLegacyAuditSpec;
	const ANSICHAR* LegacyTokens[] = {
		"WacomBattleEnemyPartActor",
		"WacomBattleEnemyPartHitBoundsComponent",
		"WacomBattleEnemyPartChildActorComponent",
		"WacomBattleEnemyPartVisualLayerComponent",
		"WacomBattleEnemyPartWorldTargetBridgeComponent",
		"WacomBattleEnemyPartPresentationComponent",
		"WacomBattleEnemyHostVisualComponent",
		"WacomBattleEnemyHostAnimationStyle",
		"EWacomBattleEnemyHostAuthoringMode",
		"EWacomBattleEnemyHostVisualMode"
	};

	TArray<FString> AssetFiles;
	IFileManager::Get().FindFilesRecursive(
		AssetFiles, *FPaths::ProjectContentDir(), TEXT("*.uasset"), true, false);
	IFileManager::Get().FindFilesRecursive(
		AssetFiles, *FPaths::ProjectContentDir(), TEXT("*.umap"), true, false);
	TestTrue(TEXT("Project contains packages to audit"), !AssetFiles.IsEmpty());

	for (const FString& AssetFile : AssetFiles)
	{
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *AssetFile))
		{
			AddError(FString::Printf(TEXT("Could not read package %s"), *AssetFile));
			continue;
		}
		for (const ANSICHAR* LegacyToken : LegacyTokens)
		{
			if (ContainsAnsiToken(Bytes, LegacyToken))
			{
				AddError(FString::Printf(
					TEXT("Legacy enemy scene token %s remains in %s"),
					ANSI_TO_TCHAR(LegacyToken),
					*AssetFile));
			}
		}
	}

	const TCHAR* HostBlueprintPaths[] = {
		TEXT("/Game/Wacom/Core/Enemy/BP_EnemyHost_TrainingWarrior.BP_EnemyHost_TrainingWarrior"),
		TEXT("/Game/Wacom/Core/Enemy/BP_EnemyHost_Snake.BP_EnemyHost_Snake"),
		TEXT("/Game/Wacom/Core/Enemy/BP_EnemyHost_SlimeTrio.BP_EnemyHost_SlimeTrio"),
		TEXT("/Game/Wacom/Core/Enemy/BP_SnakeHost_Debug.BP_SnakeHost_Debug")
	};
	for (const TCHAR* HostBlueprintPath : HostBlueprintPaths)
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, HostBlueprintPath);
		if (!TestNotNull(FString::Printf(TEXT("Host Blueprint loads: %s"), HostBlueprintPath), Blueprint)
			|| !Blueprint->GeneratedClass)
		{
			continue;
		}
		const AWacomBattleEnemyActor* Host = Cast<AWacomBattleEnemyActor>(
			Blueprint->GeneratedClass->GetDefaultObject());
		if (!TestNotNull(TEXT("Host Blueprint generates the component-native Host class"), Host))
		{
			continue;
		}
		const FWacomBattleSceneEnemyHostAuthoringReport Report =
			FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host);
		TestTrue(
			FString::Printf(
				TEXT("Host authoring report is Ready: %s (state=%s parts=%d missingVisual=%d duplicateLayer=%d invalidParent=%d multipleAnchor=%d emptyVisual=%d invalidStyle=%d terminalConflict=%d)"),
				HostBlueprintPath,
				*Report.AuthoringState.ToString(),
				Report.PartComponentCount,
				Report.MissingVisualLayerPartSlotIds.Num(),
				Report.DuplicateLayerIds.Num(),
				Report.InvalidParentComponentNames.Num(),
				Report.MultipleImpactAnchorPartSlotIds.Num(),
				Report.EmptyVisualPartSlotIds.Num(),
				Report.InvalidAnimationStylePartSlotIds.Num(),
				Report.TerminalAnimationConflictPartSlotIds.Num()),
			Report.bAuthoringReady);
	}

	const TCHAR* MapPackageNames[] = {
		TEXT("/Game/Wacom/Maps/L_Exploration"),
		TEXT("/Game/Wacom/Maps/Debug/L_RunExploration_Debug")
	};
	for (const TCHAR* MapPackageName : MapPackageNames)
	{
		UPackage* MapPackage = LoadPackage(nullptr, MapPackageName, LOAD_None);
		TestNotNull(FString::Printf(TEXT("Migrated map loads: %s"), MapPackageName), MapPackage);
	}

	return true;
}
