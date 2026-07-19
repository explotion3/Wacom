// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UBlueprint;
class UCardDefinition;
class UEncounterDefinition;
class UEnemyBehaviorDefinition;
class UEnemyDefinition;
class UEnemyPartDefinition;

namespace Wacom::ContentBuilder
{
	struct WACOMEDITOR_API FSnakeBuildResult
	{
		TObjectPtr<UCardDefinition> RewardCard = nullptr;
		TObjectPtr<UEnemyBehaviorDefinition> Behavior = nullptr;
		TObjectPtr<UEnemyPartDefinition> HeadPart = nullptr;
		TObjectPtr<UEnemyPartDefinition> BodyPart = nullptr;
		TObjectPtr<UEnemyPartDefinition> TailPart = nullptr;
		TObjectPtr<UEnemyDefinition> Enemy = nullptr;
		TObjectPtr<UEncounterDefinition> Encounter = nullptr;
		TObjectPtr<UBlueprint> HostBlueprint = nullptr;
		TObjectPtr<UBlueprint> DebugHostBlueprint = nullptr;
		bool bChanged = false;
		TArray<FString> Errors;

		bool IsSuccess() const
		{
			return RewardCard && Behavior && HeadPart && BodyPart && TailPart
				&& Enemy && Encounter && HostBlueprint && DebugHostBlueprint
				&& Errors.IsEmpty();
		}
	};

	/** 幂等创建或更新 Snake 规则数据、单蛇 Encounter 与正式 Multi-Part Host。 */
	WACOMEDITOR_API FSnakeBuildResult BuildSnakeContent();
}
