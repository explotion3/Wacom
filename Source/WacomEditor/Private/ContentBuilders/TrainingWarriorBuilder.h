// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UBlueprint;
class UCardDefinition;
class UEncounterDefinition;
class UEnemyBehaviorDefinition;
class UEnemyDefinition;
class UEnemyPartDefinition;
class UWacomBattleEnemyHostAnimationStyle;

namespace Wacom::ContentBuilder
{
	struct FTrainingWarriorBuildResult
	{
		TObjectPtr<UCardDefinition> RewardCard = nullptr;
		TObjectPtr<UEnemyPartDefinition> BodyPart = nullptr;
		TObjectPtr<UEnemyBehaviorDefinition> Behavior = nullptr;
		TObjectPtr<UEnemyDefinition> Enemy = nullptr;
		TObjectPtr<UEncounterDefinition> Encounter = nullptr;
		TObjectPtr<UWacomBattleEnemyHostAnimationStyle> AnimationStyle = nullptr;
		TObjectPtr<UBlueprint> HostBlueprint = nullptr;
		bool bChanged = false;
		TArray<FString> Errors;

		bool IsSuccess() const
		{
			return RewardCard && BodyPart && Behavior && Enemy && Encounter
				&& AnimationStyle && HostBlueprint && Errors.IsEmpty();
		}
	};

	/** 幂等创建或更新正式 TrainingWarrior DataAsset、动画 Style 与 Host prefab。 */
	FTrainingWarriorBuildResult BuildTrainingWarriorContent();
}
