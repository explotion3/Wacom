// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Resolution/BattleTargetValidationResult.h"
#include "Runtime/BattleEnemyKeys.h"
#include "Runtime/BattlePartSlotIdentity.h"

enum class EWacomBattleCardPreviewTargetKind : uint8
{
	None,
	EnemyPart,
	HandCard,
};

enum class EWacomBattleCardPreviewEffectSkipReason : uint8
{
	None,
	ConditionFailed,
	UnsupportedTarget,
	InvalidTarget,
};

struct WACOMBATTLE_API FBattleCardTargetPreviewEffect
{
	int32 EffectIndex = INDEX_NONE;
	FGameplayTag EffectType;
	FGameplayTag Target;

	bool bSkipped = false;
	EWacomBattleCardPreviewEffectSkipReason SkipReason =
		EWacomBattleCardPreviewEffectSkipReason::None;

	bool bHasMagnitude = false;
	int32 Magnitude = 0;

	bool bTargetsSelectedHandCard = false;
	bool bHasTargetHandCardCostPreview = false;
	int32 TargetHandCardRuntimeCostBefore = 0;
	int32 TargetHandCardRuntimeCostAfter = 0;

	bool bWouldDiscardTargetHandCard = false;
	bool bWouldExhaustTargetHandCard = false;
	bool bWouldGainTargetHandCardKeyword = false;
	FGameplayTag TargetHandCardKeyword;
};

/**
 * Read-only preview facts for one source hand card and one candidate target.
 *
 * The structure is intentionally C++ only. WacomBattle owns rule facts; UI
 * modules may translate these facts into view data but must not recompute them.
 */
struct WACOMBATTLE_API FBattleCardTargetPreview
{
	bool bHasPreview = false;
	FWacomBattleTargetValidationResult Validation;

	EWacomBattleCardPreviewTargetKind TargetKind =
		EWacomBattleCardPreviewTargetKind::None;

	FGuid SourceCardInstanceId;
	int32 SourceCardRuntimeCost = 0;
	bool bSourceCardSwift = false;

	FGuid TargetEnemyPartInstanceId;
	FBattlePartSlotIdentity TargetEnemyPartIdentity;
	FBattleEnemyPartKey TargetEnemyPartKey;

	FGuid TargetHandCardInstanceId;
	bool bHasTargetHandCardCostPreview = false;
	int32 TargetHandCardRuntimeCostBefore = 0;
	int32 TargetHandCardRuntimeCostAfter = 0;
	bool bWouldDiscardTargetHandCard = false;
	bool bWouldExhaustTargetHandCard = false;
	bool bWouldGainTargetHandCardKeyword = false;
	FGameplayTag TargetHandCardKeyword;

	TArray<FBattleCardTargetPreviewEffect> Effects;
};
