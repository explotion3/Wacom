// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleEnemyActionPlaybackTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

class AWacomBattleEnemyActor;
class UWacomBattleEnemyPartComponent;
struct FBattleSnapshot;
struct FWacomBattleEnemyPartDragPredictionDebugInput;
enum class EWacomFirstPersonCardDragTargetFeedbackState : uint8;

/** Non-reflected automation seam for the App-private enemy scene runtime. */
struct WACOMAPP_API FWacomEnemySceneRuntimeAutomationTestView
{
	static void InitializeBinding(
		AWacomBattleEnemyActor& Host,
		FName EncounterId,
		FName EnemySlotId);
	static bool SyncPart(
		AWacomBattleEnemyActor& Host,
		UWacomBattleEnemyPartComponent& Part,
		const FBattleSnapshot& Snapshot);
	static void SetRegisteredAndTargetable(
		AWacomBattleEnemyActor& Host,
		UWacomBattleEnemyPartComponent& Part,
		bool bRegistered,
		bool bTargetable);
	static void PlayAction(
		AWacomBattleEnemyActor& Host,
		UWacomBattleEnemyPartComponent& Part,
		FName IntentId,
		FWacomBattleEnemyActionPlaybackCallbacks&& Callbacks);
	static void CancelAction(
		AWacomBattleEnemyActor& Host,
		UWacomBattleEnemyPartComponent& Part);
	static void SetHoverPrediction(
		AWacomBattleEnemyActor& Host,
		UWacomBattleEnemyPartComponent& Part,
		const FWacomBattleEnemyPartDragPredictionDebugInput& PredictionInput);
	static void ClearHoverPrediction(
		AWacomBattleEnemyActor& Host,
		UWacomBattleEnemyPartComponent& Part);
	static void SetDragTargetPreview(
		AWacomBattleEnemyActor& Host,
		UWacomBattleEnemyPartComponent& Part,
		EWacomFirstPersonCardDragTargetFeedbackState PreviewState);
	static void ClearDragTargetPreview(
		AWacomBattleEnemyActor& Host,
		UWacomBattleEnemyPartComponent& Part);
	static FName GetDesiredTargetPreviewKind(
		const UWacomBattleEnemyPartComponent& Part);
};

#endif
