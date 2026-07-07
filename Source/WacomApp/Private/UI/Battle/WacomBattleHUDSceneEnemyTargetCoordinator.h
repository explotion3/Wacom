// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Battle/WacomBattleCardPresentationHelper.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"

class AWacomBattleEnemyActor;
class FWacomBattleHUDRuntime;
class UWacomBattleEnemyPartPresentationComponent;
class UWacomBattleEnemyPartWorldTargetBridgeComponent;
struct FBattleSnapshot;
struct FBattleCardTargetPreview;
struct FHandCardSnapshot;
struct FWacomFirstPersonCardLayerSlotView;
struct FWacomBattlePresentationTargetCue;
struct FWacomBattleEnemyPartDragPredictionDebugInput;

class FWacomBattleHUDSceneEnemyTargetCoordinator
{
public:
	explicit FWacomBattleHUDSceneEnemyTargetCoordinator(FWacomBattleHUDRuntime& InRuntime);

	void SetSceneEnemyHosts(const TArray<AWacomBattleEnemyActor*>& InHosts);
	bool HasSceneEnemyHost() const;
	bool IsSceneEnemyHostInCurrentRegistry(const AWacomBattleEnemyActor* Host) const;

	bool IsWorldTargetInCurrentRegistry(const FWacomInteractionTargetHandle& TargetHandle) const;
	UWacomBattleEnemyPartWorldTargetBridgeComponent* ResolveWorldTargetBridge(
		const FWacomInteractionTargetHandle& TargetHandle) const;
	UWacomBattleEnemyPartPresentationComponent* ResolveWorldTargetPresentation(
		const FWacomInteractionTargetHandle& TargetHandle) const;
	bool IsBridgeInCurrentRegistry(const UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge) const;

	void RebuildRegistry();
	void SyncWorldTargets(const FBattleSnapshot& Snapshot);
	void ClearWorldTargets();

	bool CanUpdateHoverProbe() const;
	void TickHoverProbe(float DeltaTime);
	void UpdateHoverProbe();
	void ClearHoverProbe(FName Reason, bool bClearFirstPersonTargetPreviewLayer = true);
	FWacomBattleEnemyPartDragPredictionDebugInput BuildHoverPredictionInput(
		const FWacomInteractionTargetHandle& TargetHandle) const;

	int32 GetRegisteredBridgeCount() const { return SceneEnemyPartWorldTargets.Num(); }

private:
	struct FSceneEnemyPartWorldTargetEntry
	{
		TWeakObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent> Bridge;
		TWeakObjectPtr<UWacomBattleEnemyPartPresentationComponent> Presentation;
	};

	FWacomBattleHUDRuntime& Runtime;
	TArray<TWeakObjectPtr<AWacomBattleEnemyActor>> SceneEnemyHosts;
	TArray<FSceneEnemyPartWorldTargetEntry> SceneEnemyPartWorldTargets;
	TWeakObjectPtr<UWacomBattleEnemyPartPresentationComponent> HoveredPresentation;
	TWeakObjectPtr<AWacomBattleEnemyActor> HoveredEnemyHost;
	FWacomInteractionTargetHandle HoveredHandle;
	float HoverProbeElapsedSeconds = 0.0f;

	bool TryBuildHoverTargetPreviewContext(
		const FWacomInteractionTargetHandle& TargetHandle,
		FBattleSnapshot& OutSnapshot,
		const FHandCardSnapshot*& OutSourceSnapshot,
		FBattleCardTargetPreview& OutTargetPreview,
		FWacomBattleEnemyPartDragPredictionDebugInput& OutPredictionInput) const;
	bool TryFindPendingTargetingCardSlot(FWacomFirstPersonCardLayerSlotView& OutSlotView) const;
	void ApplyHoverTargetPreview(
		const FWacomBattleCardTargetPreviewPresentation& TargetPreviewPresentation,
		bool bHasTargetPreviewContext) const;
};
