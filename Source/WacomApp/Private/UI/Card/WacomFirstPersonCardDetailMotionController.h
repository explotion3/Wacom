// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class UWacomCardDetailPanel;

struct FWacomFirstPersonCardDetailMotionConfig
{
	bool bEnableReadabilityPolish = true;
	FVector2D PanelEstimatedSize = FVector2D(360.0f, 420.0f);
	float DetailPadding = 12.0f;
	FVector2D AnchorBaseSize = FVector2D(296.0f, 420.0f);
	float HoverDelaySeconds = 0.10f;
	float FadeInSpeed = 18.0f;
	float FadeOutSpeed = 24.0f;
	float FollowSpeed = 24.0f;
	float PositionResetDistancePixels = 420.0f;
	float AppearStartScale = 0.97f;
	float SideSwitchHysteresisPixels = 72.0f;
};

class WACOMAPP_API FWacomFirstPersonCardDetailMotionController
{
public:
	bool IsVisible(const UWacomCardDetailPanel* Panel) const;
	bool IsPrewarmed(const UWacomCardDetailPanel* Panel) const;
	FText GetNameText(const UWacomCardDetailPanel* Panel) const;
	FVector2D GetLastPanelPosition() const { return LastPanelPosition; }

	void PrewarmPanel(
		UWacomCardDetailPanel& Panel,
		const FWacomFirstPersonCardDetailMotionConfig& Config);
	bool CanReuseCurrentDetail(const FGuid& CardInstanceId, const UWacomCardDetailPanel* Panel) const;
	bool ShowExistingAtSlot(
		UWacomCardDetailPanel& Panel,
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView,
		const FWacomFirstPersonCardDetailMotionConfig& Config,
		const FVector2D& ViewportSize);
	bool ShowAtSlot(
		UWacomCardDetailPanel& Panel,
		const FGuid& CardInstanceId,
		const FWacomCardDetailViewData& DetailData,
		const FWacomFirstPersonCardLayerSlotView& SlotView,
		const FWacomFirstPersonCardDetailMotionConfig& Config,
		const FVector2D& ViewportSize);
	void UpdateCurrentSlot(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView,
		UWacomCardDetailPanel* Panel,
		const FWacomFirstPersonCardDetailMotionConfig& Config,
		const FVector2D& ViewportSize);
	void HideForSource(
		const FGuid& CardInstanceId,
		UWacomCardDetailPanel* Panel,
		const FWacomFirstPersonCardDetailMotionConfig& Config);
	void ForceHideAll(UWacomCardDetailPanel* Panel);
	void TickMotion(
		float DeltaTime,
		UWacomCardDetailPanel* Panel,
		const FWacomFirstPersonCardDetailMotionConfig& Config,
		const FVector2D& ViewportSize);

	void SetCurrentSource(const FGuid& CardInstanceId);
	void ClearCurrentSource();
	FGuid GetCurrentSource() const { return CurrentSourceId; }
	bool IsCurrentSource(const FGuid& CardInstanceId) const;
	bool IsMotionSource(const FGuid& CardInstanceId) const;
	bool GetActiveSlotForSource(
		const FGuid& CardInstanceId,
		FWacomFirstPersonCardLayerSlotView& OutSlotView) const;
	bool ComputeTarget(
		const FWacomFirstPersonCardLayerSlotView& SlotView,
		const FWacomFirstPersonCardDetailMotionConfig& Config,
		const FVector2D& ViewportSize,
		FVector2D& OutPosition);
	FVector2D ComputeStablePosition(
		const FVector2D& AnchorPosition,
		const FVector2D& AnchorSize,
		const FVector2D& LayerSize,
		const FVector2D& PanelSize,
		float DetailPadding,
		float SideSwitchHysteresisPixels);

#if WITH_AUTOMATION_TESTS
	bool IsPendingShowForTest() const { return MotionState.bPendingShow; }
	float GetPanelOpacityForTest() const { return MotionState.VisualOpacity; }
	int32 GetDetailDataApplyCountForTest() const { return DetailDataApplyCountForTest; }
	FVector2D GetLastAppliedPanelLayoutPositionForTest() const
	{
		return LastAppliedPanelLayoutPositionForTest;
	}
#endif

private:
	struct FMotionState
	{
		FGuid ActiveSourceId;
		FWacomFirstPersonCardLayerSlotView ActiveSlot;
		bool bHasActiveSlot = false;
		bool bPendingShow = false;
		bool bWantsVisible = false;
		float PendingElapsedSeconds = 0.0f;
		float VisualOpacity = 0.0f;
		FVector2D TargetPosition = FVector2D::ZeroVector;
		FVector2D VisualPosition = FVector2D::ZeroVector;
		bool bHasTargetPosition = false;
		bool bHasVisualPosition = false;
		bool bResetPosition = true;
		int32 StableSide = 0;
	};

	void PositionBesideSlot(
		UWacomCardDetailPanel& Panel,
		const FWacomFirstPersonCardLayerSlotView& SlotView,
		const FWacomFirstPersonCardDetailMotionConfig& Config,
		const FVector2D& ViewportSize);
	bool BeginMotionShow(
		UWacomCardDetailPanel& Panel,
		const FWacomFirstPersonCardDetailMotionConfig& Config,
		const FVector2D& ViewportSize);
	void RequestMotionShow(
		UWacomCardDetailPanel& Panel,
		const FWacomFirstPersonCardDetailMotionConfig& Config);
	void RequestMotionHide(
		UWacomCardDetailPanel* Panel,
		const FWacomFirstPersonCardDetailMotionConfig& Config,
		bool bImmediate);
	bool UpdateMotionTarget(
		const FWacomFirstPersonCardDetailMotionConfig& Config,
		const FVector2D& ViewportSize);
	FVector2D ComputeImmediatePosition(
		const FVector2D& AnchorPosition,
		const FVector2D& AnchorSize,
		const FVector2D& LayerSize,
		const FVector2D& PanelSize,
		float DetailPadding) const;
	void ApplyMotionVisual(
		UWacomCardDetailPanel& Panel,
		const FVector2D& Position,
		float Opacity,
		const FWacomFirstPersonCardDetailMotionConfig& Config,
		float PresentationScale);
	void CollapsePanel(UWacomCardDetailPanel* Panel);

	FGuid CurrentSourceId;
	FVector2D LastPanelPosition = FVector2D::ZeroVector;
	FMotionState MotionState;

#if WITH_AUTOMATION_TESTS
	int32 DetailDataApplyCountForTest = 0;
	FVector2D LastAppliedPanelLayoutPositionForTest = FVector2D::ZeroVector;
#endif
};
