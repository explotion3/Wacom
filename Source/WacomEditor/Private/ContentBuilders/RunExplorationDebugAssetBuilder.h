// Copyright Wacom. All Rights Reserved.

#pragma once

class UWacomFloorMapDefinition;
class UWacomJourneyDefinition;

namespace Wacom::ContentBuilder
{
	struct FRunExplorationDebugAssetBuildResult
	{
		UWacomJourneyDefinition* Journey = nullptr;
		UWacomFloorMapDefinition* Floor = nullptr;
		bool bPathBlueprintsBuilt = false;
		bool bRuntimeAssetsConfigured = false;
		bool bExplorationWorldMigrated = false;

		bool IsDataOk() const { return Journey && Floor && bPathBlueprintsBuilt; }
		bool IsOk() const { return IsDataOk() && bRuntimeAssetsConfigured && bExplorationWorldMigrated; }
	};

	/** 创建/更新稳定的单层 Debug Journey、Floor 和新 Run Path 制作 BP。 */
	FRunExplorationDebugAssetBuildResult BuildRunExplorationDebugAssets();
}
