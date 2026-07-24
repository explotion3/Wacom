// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardDetailMotionController.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class AWacomPlayerController;
class UWacomFirstPersonCardAnchorComponent;
class UWacomCardDetailPanel;

enum class EWacomRunFirstPersonCardDetailHoldReason : uint8
{
	None,
	Hover,
	Inspect
};

class FWacomRunFirstPersonCardDetailController
{
public:
	explicit FWacomRunFirstPersonCardDetailController(AWacomPlayerController& InPlayerController);

	bool IsVisible() const;
	bool IsPrewarmed() const;
	FText GetNameText() const;
	FVector2D GetLastPanelPosition() const { return MotionController.GetLastPanelPosition(); }

	UWacomCardDetailPanel* EnsurePanel();
	void PrewarmPanel();
	void RemovePanelFromViewport();
	void RefreshBinding();
	void UnbindBinding(UWacomFirstPersonCardAnchorComponent* Anchor);
	void UnbindCurrentBinding();

	bool CanReuseCurrentDetail(const FGuid& CardInstanceId) const;
	bool ShowExistingAtSlot(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView,
		EWacomRunFirstPersonCardDetailHoldReason HoldReason);
	bool ShowAtSlot(
		const FGuid& CardInstanceId,
		const FWacomCardDetailViewData& DetailData,
		const FWacomFirstPersonCardLayerSlotView& SlotView,
		EWacomRunFirstPersonCardDetailHoldReason HoldReason);
	void UpdateCurrentSlot(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HideForSource(const FGuid& CardInstanceId);
	void ForceHideAll();
	void TickMotion(float DeltaTime);

	void SetHoveredSlot(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void ClearHoveredStateForSource(const FGuid& CardInstanceId);
	bool GetHoveredSlot(
		FGuid& OutCardInstanceId,
		FWacomFirstPersonCardLayerSlotView& OutSlotView) const;
	bool IsInspectHeldForSource(const FGuid& CardInstanceId) const;
	void ClearInspectHold();

	void HandleCardHovered(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleCardUnhovered(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleHoveredCardLayoutUpdated(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	bool HandleInspectDragStartedOrUpdated(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView);
	void FinishInspectDetail(const FGuid& CardInstanceId);
	void HandleFaceInspectLocked(
		const FGuid& CardInstanceId,
		EWacomCardFaceContext FaceContext,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleFaceChanged(
		const FGuid& CardInstanceId,
		EWacomCardFaceContext FaceContext,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleFaceInspectClosed(const FGuid& CardInstanceId);

#if WITH_AUTOMATION_TESTS
	bool IsPendingShowForTest() const { return MotionController.IsPendingShowForTest(); }
	float GetPanelOpacityForTest() const { return MotionController.GetPanelOpacityForTest(); }
	int32 GetDetailDataApplyCountForTest() const { return MotionController.GetDetailDataApplyCountForTest(); }
#endif

private:
	void SetCurrentSource(
		const FGuid& CardInstanceId,
		EWacomRunFirstPersonCardDetailHoldReason HoldReason);
	void ClearCurrentSource();
	bool ShouldHandleCurrentSource() const;
	bool ShowAtSlotFromRunData(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView,
		EWacomRunFirstPersonCardDetailHoldReason HoldReason);
	bool ShowFaceInspectionDetail(
		const FGuid& CardInstanceId,
		EWacomCardFaceContext FaceContext,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	bool ShouldShowInspectDetail(const FWacomFirstPersonCardDragView& DragView) const;
	bool ShowInspectDetail(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView);
	FWacomFirstPersonCardDetailMotionConfig BuildMotionConfig() const;
	FVector2D GetViewportSize() const;

	AWacomPlayerController& PlayerController;
	TWeakObjectPtr<UWacomFirstPersonCardAnchorComponent> BoundAnchor;
	EWacomRunFirstPersonCardDetailHoldReason CurrentHoldReason =
		EWacomRunFirstPersonCardDetailHoldReason::None;
	FGuid HoveredSourceId;
	FWacomFirstPersonCardLayerSlotView HoveredSlot;
	bool bHasHoveredSlot = false;
	FWacomFirstPersonCardDetailMotionController MotionController;
};
