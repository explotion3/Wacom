// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UI/Battle/WacomBattleEnemyActionPlaybackTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomBattleEnemySceneRuntimeComponent.generated.h"

class UWacomBattleEnemyPartComponent;
struct FBattleSnapshot;
struct FEnemyPartSnapshot;
struct FWacomBattleEnemyPartRuntimeDebugView;
struct FWacomBattlePresentationTargetCue;
struct FWacomInteractionTargetHandle;

/**
 * Host 内唯一的场景敌人运行时所有者。
 *
 * 不可由 Blueprint 添加；它只管理 typed Part hierarchy 的绑定、反馈、预测、
 * Action/Destroyed/terminal playback 和清理，不拥有任何 authored Visual Component。
 */
UCLASS(NotBlueprintable, NotBlueprintType, Transient)
class UWacomBattleEnemySceneRuntimeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomBattleEnemySceneRuntimeComponent();
	virtual ~UWacomBattleEnemySceneRuntimeComponent() override;

	void RefreshTypedHierarchy();
	void NotifyTypedHierarchyChanged();
	void GetOrderedPartComponents(TArray<UWacomBattleEnemyPartComponent*>& OutParts) const;
	uint32 GetTopologyRevision() const;

	void InitializeRuntimeSceneBinding(FName EncounterId, FName EnemySlotId);
	bool ApplyPartSnapshotFacts(
		UWacomBattleEnemyPartComponent& Part,
		const FEnemyPartSnapshot* SnapshotPart,
		bool bTargetSelectionActive,
		bool bTargetable,
		FName TargetDisabledReason);
	void ClearPartBattleBinding(UWacomBattleEnemyPartComponent& Part, bool bClearRuntimeFacts = true);
	void ClearAllBattleBindings(bool bClearRuntimeFacts = true);
	bool IsPartBound(const UWacomBattleEnemyPartComponent& Part) const;
	bool IsPartRegisteredWithHUD(const UWacomBattleEnemyPartComponent& Part) const;
	void SetPartRegisteredWithHUD(UWacomBattleEnemyPartComponent& Part, bool bRegistered);
	void SetPartTargetable(
		UWacomBattleEnemyPartComponent& Part,
		bool bTargetable,
		FName DisabledReason);

	FWacomInteractionTargetHandle BuildWorldTargetHandle(
		const UWacomBattleEnemyPartComponent& Part) const;
	FWacomBattleEnemyPartRuntimeDebugView BuildPartDebugView(
		const UWacomBattleEnemyPartComponent& Part) const;

	void PlayPartActionAnimation(
		UWacomBattleEnemyPartComponent& Part,
		FName IntentId,
		FWacomBattleEnemyActionPlaybackCallbacks&& Callbacks);
	void PlayEnemyDestroyedAnimation(TFunction<void()>&& Completion);
	void CancelPartActionAnimation(UWacomBattleEnemyPartComponent& Part, bool bRestoreAuthoredVisual = true);
	void CancelAllPlayback(bool bRestoreAuthoredVisual = true);
	void ResetRuntimeScenePresentationForBattle();
	void RetireRuntimeEncounterPresentation();
	bool IsRuntimeRetired() const;

	void PlayPartPresentationCue(
		UWacomBattleEnemyPartComponent& Part,
		const FWacomBattlePresentationTargetCue& Cue);
	void ForceCompletePartPresentationCue(UWacomBattleEnemyPartComponent& Part);
	void ClearPartPresentation(UWacomBattleEnemyPartComponent& Part, FName Reason);
	void SetPartDragTargetPreviewState(
		UWacomBattleEnemyPartComponent& Part,
		EWacomFirstPersonCardDragTargetFeedbackState PreviewState);
	void ClearPartDragTargetPreviewState(UWacomBattleEnemyPartComponent& Part);
	void SetPartHoverProbeState(
		UWacomBattleEnemyPartComponent& Part,
		const FWacomInteractionTargetHandle& TargetHandle,
		FName Reason);
	void ClearPartHoverProbeState(UWacomBattleEnemyPartComponent& Part, FName Reason);

	int32 ApplyPartDestroyedVisualState(UWacomBattleEnemyPartComponent& Part);
	void RestorePartAuthoredVisualState(UWacomBattleEnemyPartComponent& Part);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleActiveFlipbookFinished();

	struct FImpl;
	FImpl* Impl = nullptr;
};
