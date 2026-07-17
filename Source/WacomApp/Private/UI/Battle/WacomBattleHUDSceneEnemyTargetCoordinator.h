// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Battle/WacomBattleCardPresentationHelper.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"

class AWacomBattleEnemyActor;
class AWacomBattleEnemyPartActor;
class FWacomBattleHUDRuntime;
class UWacomBattleEnemyPartPresentationComponent;
class UWacomBattleEnemyPartWorldTargetBridgeComponent;
struct FBattleSnapshot;
struct FBattleCardActionPreview;
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
	void PlaySceneEnemyActionAnimation(
		const FBattlePartSlotIdentity& ActingPartKey,
		FName IntentId,
		TFunction<void()>&& Completion);
	void PlayHostDestroyedAnimation(
		FName EnemySlotId,
		TFunction<void()>&& Completion);
	void ClearRetiringHosts(bool bCancelPendingPlayback);
	void ApplyActionPreviewToEnemyPanels(
		const TArray<FWacomBattleEnemyPartEntryViewData>& PreviewParts,
		bool bApplyScenePartPreview = true) const;
	void ClearActionPreviewFromEnemyPanels() const;

	bool CanUpdateHoverProbe() const;
	void TickHoverProbe(float DeltaTime);
	void UpdateHoverProbe();
	void ClearHoverProbe(FName Reason, bool bClearFirstPersonTargetPreviewLayer = true);
	FWacomBattleEnemyPartDragPredictionDebugInput BuildHoverPredictionInput(
		const FWacomInteractionTargetHandle& TargetHandle) const;

	int32 GetRegisteredBridgeCount() const { return SceneEnemyPartWorldTargets.Num(); }
	int32 GetRegistryRevision() const { return RegistryRevision; }

private:
	struct FSceneEnemyRuntimePartEntry
	{
		TWeakObjectPtr<AWacomBattleEnemyPartActor> PartActor;
		FBattlePartSlotIdentity ObservedIdentity;
	};

	struct FSceneEnemyHostEntry
	{
		TWeakObjectPtr<AWacomBattleEnemyActor> Host;
		FName ObservedEnemySlotId = NAME_None;
		uint32 ObservedTopologyRevision = 0;
		TArray<FSceneEnemyRuntimePartEntry> RuntimeParts;
	};

	struct FSceneEnemyPartWorldTargetEntry
	{
		TWeakObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent> Bridge;
		TWeakObjectPtr<UWacomBattleEnemyPartPresentationComponent> Presentation;
		FBattlePartSlotIdentity RegisteredTargetIdentity;
		bool bPresentationTargetRegistered = false;
	};

	struct FRetiringSceneEnemyHostEntry
	{
		TWeakObjectPtr<AWacomBattleEnemyActor> Host;
		FName ObservedEnemySlotId = NAME_None;
		bool bAllPartsDestroyed = false;
		TArray<FSceneEnemyRuntimePartEntry> RuntimeParts;
	};

	FWacomBattleHUDRuntime& Runtime;
	TArray<FSceneEnemyHostEntry> SceneEnemyHosts;
	TArray<FRetiringSceneEnemyHostEntry> RetiringSceneEnemyHosts;
	TArray<FSceneEnemyPartWorldTargetEntry> SceneEnemyPartWorldTargets;
	TWeakObjectPtr<UWacomBattleEnemyPartPresentationComponent> HoveredPresentation;
	TWeakObjectPtr<AWacomBattleEnemyActor> HoveredEnemyHost;
	FWacomInteractionTargetHandle HoveredHandle;
	float HoverProbeElapsedSeconds = 0.0f;
	int32 RegistryRevision = 0;

	bool HasSameSceneEnemyHosts(const TArray<AWacomBattleEnemyActor*>& InHosts) const;
	bool IsRegistryTopologyCurrent() const;
	void ClearActiveWorldTargets(FName Reason);
	void RetireWorldTargetsForBattleEnd(const FBattleSnapshot& Snapshot);
	bool IsActiveEnemyAllPartsDestroyed(FName EnemySlotId) const;
	void ClearRegistryEntries(FName Reason);
	void ClearPresentationTargetRegistration(FSceneEnemyPartWorldTargetEntry& Entry);
	void EnsurePresentationTargetRegistration(
		FSceneEnemyPartWorldTargetEntry& Entry,
		UWacomBattleEnemyPartPresentationComponent& Presentation,
		const FBattlePartSlotIdentity& TargetIdentity);
	bool TryBuildHoverTargetPreviewContext(
		const FWacomInteractionTargetHandle& TargetHandle,
		FBattleSnapshot& OutSnapshot,
		const FHandCardSnapshot*& OutSourceSnapshot,
		FBattleCardActionPreview& OutActionPreview,
		FBattleCardTargetPreview& OutTargetPreview,
		FWacomBattleEnemyPartDragPredictionDebugInput& OutPredictionInput) const;
	bool TryFindPendingTargetingCardSlot(FWacomFirstPersonCardLayerSlotView& OutSlotView) const;
	void ApplyHoverTargetPreview(
		const FWacomBattleCardTargetPreviewPresentation& TargetPreviewPresentation,
		bool bHasTargetPreviewContext) const;
	void ApplyActionPreviewToSceneParts(const TArray<FWacomBattleEnemyPartEntryViewData>& PreviewParts) const;
	void ClearActionPreviewFromSceneParts() const;
};
