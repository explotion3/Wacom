// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AWacomBattleEnemyActor;
class UWacomBattleEnemyPartComponent;

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
	TArray<FString> SurplusPartComponentNames;
};

struct WACOMAPP_API FWacomBattleSceneEnemyPartSyncPlanEntry
{
	EWacomBattleSceneEnemyPartSyncOperation Operation =
		EWacomBattleSceneEnemyPartSyncOperation::AddMissingPart;
	FName PartSlotId = NAME_None;
	FName DerivedPartId = NAME_None;
	TWeakObjectPtr<UWacomBattleEnemyPartComponent> ExistingPartComponent;
};

/**
 * Scene Enemy Host 的纯只读组件制作报告。
 *
 * Evaluator 只读取 EnemyDefinition 与 typed SCS/实例组件层级；不会刷新视觉、
 * 写身份、创建组件、Modify 对象或标记 Package。Editor 同步服务显式消费 SyncPlan。
 */
struct WACOMAPP_API FWacomBattleSceneEnemyHostAuthoringReport
{
	FName AuthoringState = NAME_None;
	bool bAuthoringReady = false;
	bool bHasEnemyDefinition = false;
	bool bHasValidDefinitionParts = false;
	int32 PartComponentCount = 0;
	int32 FlipbookLayerCount = 0;
	int32 SpriteLayerCount = 0;

	TArray<FName> AttachedPartIds;
	TArray<FName> AttachedPartSlotIds;
	TArray<FName> StableSceneTargetIds;
	TArray<FName> InvalidDefinitionPartSlotIds;
	TArray<FName> MissingVisualLayerPartSlotIds;
	TArray<FName> MissingInteractionLayerPartSlotIds;
	TArray<FName> AmbiguousInteractionLayerPartSlotIds;
	TArray<FName> InteractionCollisionNotReadyPartSlotIds;
	TArray<FName> UnexpectedVisualCollisionPartSlotIds;
	TArray<FName> InteractionVisualLayerIds;
	TArray<FName> DuplicateLayerIds;
	TArray<FString> InvalidParentComponentNames;
	TArray<FName> MultipleImpactAnchorPartSlotIds;
	TArray<FName> EmptyVisualPartSlotIds;
	TArray<FName> InvalidAnimationStylePartSlotIds;
	TArray<FName> TerminalAnimationConflictPartSlotIds;
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
