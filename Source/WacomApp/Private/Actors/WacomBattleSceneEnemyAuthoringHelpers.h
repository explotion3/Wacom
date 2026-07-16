// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace WacomBattleSceneEnemyAuthoring
{
	struct FHostPartIdentityAudit
	{
		TArray<FName> UnknownPartIds;
		TArray<FName> UnknownPartSlotIds;
		TArray<FName> MissingDefinitionPartIds;
		TArray<FName> MissingDefinitionPartSlotIds;
		TArray<FName> DuplicatePartSlotIds;
		TArray<FName> PartDefinitionMismatchSlotIds;
		TArray<FString> SurplusPartActorNames;
	};

	bool ShouldValidateHostPlacementActor(const AWacomBattleEnemyActor& EnemyActor);
	bool ShouldValidatePartPlacementActor(const AWacomBattleEnemyPartActor& PartActor);

	const TCHAR* GetHostVisualModeDebugString(EWacomBattleEnemyHostVisualMode VisualMode);
	const TCHAR* GetHostAuthoringModeDebugString(EWacomBattleEnemyHostAuthoringMode AuthoringMode);
	TMap<FName, int32> BuildDefinitionPartOrder(const UEnemyDefinition* EnemyDefinition);
	FHostPartIdentityAudit BuildHostPartIdentityAudit(
		const UEnemyDefinition* EnemyDefinition,
		const TArray<AWacomBattleEnemyPartActor*>& PartActors);
	FName BuildHostAuthoringStateName(
		const UEnemyDefinition* EnemyDefinition,
		int32 PartActorCount,
		const FHostPartIdentityAudit& Audit);
	FString FormatHostDebugSummary(const FWacomBattleSceneEnemyDebugView& View);

	FName BuildVisualAuthoringModeName(
		const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers,
		bool bHostVisualContextActive);
	FName BuildPartAuthoringStateName(
		FName PartId,
		FName PartSlotId,
		const FVector& HitBoundsExtent,
		FName VisualAuthoringMode);
	FString FormatPartDebugSummary(const FWacomBattleSceneEnemyPartDebugView& View);

#if WITH_EDITOR
	EDataValidationResult ValidateHostPlacement(
		const AWacomBattleEnemyActor& EnemyActor,
		FDataValidationContext& Context,
		EDataValidationResult BaseResult);
	EDataValidationResult ValidatePartPlacement(
		const AWacomBattleEnemyPartActor& PartActor,
		FDataValidationContext& Context,
		EDataValidationResult BaseResult);
#endif
}
