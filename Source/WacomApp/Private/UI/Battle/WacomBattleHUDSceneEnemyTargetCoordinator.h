// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Battle/WacomBattleCardPresentationHelper.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"

class AWacomBattleEnemyActor;
class FWacomBattleHUDRuntime;
class UWacomBattleEnemyPartComponent;
struct FBattleSnapshot;
struct FBattleCardActionPreview;
struct FBattleCardTargetPreview;
struct FHandCardSnapshot;
struct FWacomFirstPersonCardLayerSlotView;
struct FWacomBattleEnemyActionPlaybackCallbacks;
struct FWacomBattleEnemyPartDragPredictionDebugInput;

/** HUD 对 typed Enemy Part hierarchy 的唯一 registry 与表现路由。 */
class FWacomBattleHUDSceneEnemyTargetCoordinator
{
public:
	explicit FWacomBattleHUDSceneEnemyTargetCoordinator(FWacomBattleHUDRuntime& InRuntime);
	~FWacomBattleHUDSceneEnemyTargetCoordinator();

	void SetSceneEnemyHosts(const TArray<AWacomBattleEnemyActor*>& InHosts);
	bool HasSceneEnemyHost() const;
	bool IsSceneEnemyHostInCurrentRegistry(const AWacomBattleEnemyActor* Host) const;
	bool IsWorldTargetInCurrentRegistry(const FWacomInteractionTargetHandle& Handle) const;
	UWacomBattleEnemyPartComponent* ResolvePartComponent(
		const FWacomInteractionTargetHandle& Handle) const;
	bool IsPartInCurrentRegistry(const UWacomBattleEnemyPartComponent* Part) const;

	void RebuildRegistry();
	void SyncWorldTargets(const FBattleSnapshot& Snapshot);
	void ClearWorldTargets();
	void PlaySceneEnemyActionAnimation(
		const FBattlePartSlotIdentity& ActingPartKey,
		FName IntentId,
		FWacomBattleEnemyActionPlaybackCallbacks&& Callbacks);
	void PlayEnemyDestroyedAnimation(FName EnemySlotId, TFunction<void()>&& Completion);
	void ClearRetiringHosts(bool bCancelPendingPlayback);

	void ApplyActionPreviewToEnemyPanels(
		const TArray<FWacomBattleEnemyPartEntryViewData>& PreviewParts,
		bool bApplyScenePartPreview = true) const;
	void ClearActionPreviewFromEnemyPanels() const;
	void RefreshEnemyPanelInspectionInteraction() const;

	bool CanUpdateHoverProbe() const;
	void TickHoverProbe(float DeltaTime);
	void UpdateHoverProbe();
	void ClearHoverProbe(FName Reason, bool bClearFirstPersonTargetPreviewLayer = true);
	FWacomBattleEnemyPartDragPredictionDebugInput BuildHoverPredictionInput(
		const FWacomInteractionTargetHandle& TargetHandle) const;

	int32 GetRegisteredPartCount() const { return RegisteredParts.Num(); }
	int32 GetRegistryRevision() const { return RegistryRevision; }

private:
	struct FPartEntry
	{
		TWeakObjectPtr<UWacomBattleEnemyPartComponent> Part;
		FBattlePartSlotIdentity ObservedIdentity;
		bool bPresentationTargetRegistered = false;
	};

	struct FHostEntry
	{
		TWeakObjectPtr<AWacomBattleEnemyActor> Host;
		FName ObservedEnemySlotId = NAME_None;
		uint32 ObservedTopologyRevision = 0;
		TArray<FPartEntry> Parts;
	};

	struct FRetiringHostEntry
	{
		TWeakObjectPtr<AWacomBattleEnemyActor> Host;
		FName ObservedEnemySlotId = NAME_None;
		bool bAllPartsDestroyed = false;
		TArray<FPartEntry> Parts;
	};

	FWacomBattleHUDRuntime& Runtime;
	TArray<FHostEntry> SceneEnemyHosts;
	TArray<FRetiringHostEntry> RetiringSceneEnemyHosts;
	TArray<FPartEntry*> RegisteredParts;
	TWeakObjectPtr<UWacomBattleEnemyPartComponent> HoveredPart;
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
	void EnsurePresentationTargetRegistration(FPartEntry& Entry);
	void ClearPresentationTargetRegistration(FPartEntry& Entry);
	void RebuildRegisteredPartPointers();

	bool TryBuildHoverTargetPreviewContext(
		const FWacomInteractionTargetHandle& TargetHandle,
		FBattleSnapshot& OutSnapshot,
		const FHandCardSnapshot*& OutSourceSnapshot,
		FBattleCardActionPreview& OutActionPreview,
		FBattleCardTargetPreview& OutTargetPreview,
		FWacomBattleEnemyPartDragPredictionDebugInput& OutPredictionInput) const;
	bool TryFindPendingTargetingCardSlot(FWacomFirstPersonCardLayerSlotView& OutSlotView) const;
	void ApplyHoverTargetPreview(
		const FWacomBattleCardTargetPreviewPresentation& Presentation,
		bool bHasTargetPreviewContext) const;
	void ApplyActionPreviewToSceneParts(
		const TArray<FWacomBattleEnemyPartEntryViewData>& PreviewParts) const;
	void ClearActionPreviewFromSceneParts() const;
	void BindHostInspectionDelegate(AWacomBattleEnemyActor& Host);
	void UnbindHostInspectionDelegate(AWacomBattleEnemyActor& Host);
	void HandleEnemyPanelInspectionRequested(
		AWacomBattleEnemyActor* Host,
		const FBattlePartSlotIdentity& PartIdentity);
};
