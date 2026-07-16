// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AWacomBattleEnemyActor;
class AWacomBattleEnemyPartActor;

enum class EWacomBattleSceneEnemyPartSyncOperation : uint8
{
	AddMissingPart,
	UpdateDerivedPartId,
};

struct WACOMAPP_API FWacomBattleSceneEnemyHostIdentityAudit
{
	TArray<FName> UnknownPartIds;
	TArray<FName> UnknownPartSlotIds;
	TArray<FName> MissingDefinitionPartIds;
	TArray<FName> MissingDefinitionPartSlotIds;
	TArray<FName> DuplicatePartSlotIds;
	TArray<FName> PartDefinitionMismatchSlotIds;
	TArray<FString> SurplusPartActorNames;
};

struct WACOMAPP_API FWacomBattleSceneEnemyPartSyncPlanEntry
{
	EWacomBattleSceneEnemyPartSyncOperation Operation =
		EWacomBattleSceneEnemyPartSyncOperation::AddMissingPart;
	FName PartSlotId = NAME_None;
	FName DerivedPartId = NAME_None;
	TWeakObjectPtr<AWacomBattleEnemyPartActor> ExistingPartActor;
};

/**
 * Pure, read-only snapshot of a Scene Enemy Host's authoring configuration.
 *
 * Building this report may inspect live ChildActor instances and Blueprint component
 * templates, but it must never refresh visuals, write identity, create components or
 * dirty packages. WacomEditor consumes SyncPlan explicitly when the author requests it.
 */
struct WACOMAPP_API FWacomBattleSceneEnemyHostAuthoringReport
{
	FName AuthoringState = NAME_None;
	bool bAuthoringReady = false;
	bool bHasEnemyDefinition = false;
	bool bHasValidDefinitionParts = false;
	bool bUsingHostVisual = false;
	bool bHostAnimationStyleApplicable = true;
	int32 PartActorCount = 0;

	TArray<FName> AttachedPartIds;
	TArray<FName> AttachedPartSlotIds;
	TArray<FName> StableSceneTargetIds;
	TArray<FName> InvalidDefinitionPartSlotIds;
	TArray<FName> MissingVisualLayerPartSlotIds;
	FWacomBattleSceneEnemyHostIdentityAudit IdentityAudit;
	TArray<FWacomBattleSceneEnemyPartSyncPlanEntry> SyncPlan;

	int32 GetAddMissingPartCount() const;
	int32 GetUpdateDerivedPartIdCount() const;
};

class WACOMAPP_API FWacomBattleSceneEnemyHostAuthoringEvaluator
{
public:
	static FWacomBattleSceneEnemyHostAuthoringReport Build(
		const AWacomBattleEnemyActor& EnemyActor);
};
