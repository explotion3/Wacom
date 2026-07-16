// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Wacom::ContentBuilder
{
	struct FEnemyPackArtPromotionResult
	{
		bool bSucceeded = false;
		bool bCopiedAssets = false;
		int32 ExpectedAssetCount = 0;
		TArray<FString> Errors;
	};

	/**
	 * 将本地 BattleWarrior Paper2D 素材的受控依赖闭包晋升为正式 TrainingWarrior 素材。
	 *
	 * 完整且合法的正式目标会直接复用；除非 bForceRefresh 为 true，不会重复复制。
	 * 本服务不会删除目标目录中的未知或额外资产。
	 */
	FEnemyPackArtPromotionResult PromoteTrainingWarriorArt(bool bForceRefresh);

	/**
	 * 只读验证正式 TrainingWarrior 素材是否完整、类型正确，且不依赖 ignored Content。
	 * 不读取或要求本地 /Game/Art 源素材。
	 */
	FEnemyPackArtPromotionResult ValidateTrainingWarriorFormalArt();
}
