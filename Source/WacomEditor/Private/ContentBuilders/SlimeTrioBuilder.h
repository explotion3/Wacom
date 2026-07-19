// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UBlueprint;
class UEncounterDefinition;
class UEnemyBehaviorDefinition;
class UEnemyDefinition;
class UEnemyPartDefinition;

namespace Wacom::ContentBuilder
{
	struct WACOMEDITOR_API FSlimeTrioBuildResult
	{
		TObjectPtr<UEnemyBehaviorDefinition> Behavior = nullptr;
		TObjectPtr<UEnemyPartDefinition> LeftPart = nullptr;
		TObjectPtr<UEnemyPartDefinition> CorePart = nullptr;
		TObjectPtr<UEnemyPartDefinition> RightPart = nullptr;
		TObjectPtr<UEnemyDefinition> Enemy = nullptr;
		TObjectPtr<UEncounterDefinition> Encounter = nullptr;
		TObjectPtr<UBlueprint> HostBlueprint = nullptr;
		bool bChanged = false;
		TArray<FString> Errors;

		bool IsSuccess() const
		{
			return Behavior && LeftPart && CorePart && RightPart
				&& Enemy && Encounter && HostBlueprint && Errors.IsEmpty();
		}
	};

	/** 幂等创建或更新 SlimeTrio 规则数据、单敌人 Encounter 与三部位 Host。 */
	WACOMEDITOR_API FSlimeTrioBuildResult BuildSlimeTrioContent();
}
