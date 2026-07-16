// Copyright Wacom. All Rights Reserved.

#pragma once

class UWacomFloorMapDefinition;
class UWacomJourneyDefinition;
class UBlueprint;
class UWorld;

namespace Wacom::ContentBuilder
{
	struct FRunExplorationDebugSharedDependencies
	{
		FString PlayerBlueprintObjectPath =
			TEXT("/Game/Wacom/Core/Player/BP_WacomPlayerCharacter.BP_WacomPlayerCharacter");
		FString AnchorBlueprintObjectPath =
			TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunMapNodeAnchorActor.BP_WacomRunMapNodeAnchorActor");
		FString PathBlueprintObjectPath =
			TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunPathSegmentActor.BP_WacomRunPathSegmentActor");
		FString BranchBlueprintObjectPath =
			TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunPathBranchTargetActor.BP_WacomRunPathBranchTargetActor");
	};

	struct FRunExplorationDebugAssetBuildResult
	{
		UWacomJourneyDefinition* DebugJourney = nullptr;
		UWacomFloorMapDefinition* DebugFloor = nullptr;
		UBlueprint* DebugGameMode = nullptr;
		UWorld* DebugWorld = nullptr;
		bool bSharedDependenciesValid = false;
		bool bDataValidationPassed = false;
		bool bSceneValidationPassed = false;
		bool bOwnedAssetsSaved = false;

		bool IsDataOk() const
		{
			return DebugJourney && DebugFloor && bDataValidationPassed;
		}
		bool IsOk() const
		{
			return bSharedDependenciesValid && IsDataOk() && DebugGameMode
				&& DebugWorld && bSceneValidationPassed && bOwnedAssetsSaved;
		}
	};

	/** 创建/更新稳定的单层 Debug Journey、Floor、GameMode 与地图夹具。 */
	FRunExplorationDebugAssetBuildResult BuildRunExplorationDebugAssets();

	/** 仅供非反射自动化视图注入依赖路径；生产入口使用固定项目路径。 */
	FRunExplorationDebugAssetBuildResult BuildRunExplorationDebugAssets(
		const FRunExplorationDebugSharedDependencies& Dependencies);
}
