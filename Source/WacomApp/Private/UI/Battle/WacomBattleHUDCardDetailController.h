// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "UI/Battle/BattleHUD.h"

class UBattleHUD;
class UCardWidget;
class UWacomCardDetailPanel;
struct FWacomCardDetailViewData;
struct FWacomFirstPersonCardLayerSlotView;

class FWacomBattleHUDCardDetailController
{
public:
	explicit FWacomBattleHUDCardDetailController(UBattleHUD& InHUD);

	bool IsVisible() const;
	FText GetNameText() const;

	void HandleHandCardHovered(UCardWidget* SourceWidget);
	void HandleHandCardUnhovered(UCardWidget* SourceWidget);
	bool ShowForCardWidget(UCardWidget* SourceWidget);
	bool ShowAtAnchor(
		const FWacomCardDetailViewData& DetailData,
		const FVector2D& AnchorPosition,
		const FVector2D& AnchorSize);
	void HideAll();
	void HideLegacyForSource(UCardWidget* SourceWidget);
	void HideFirstPersonForSource(const FGuid& CardInstanceId);
	bool IsFirstPersonInspectDetailActiveForSource(const FGuid& CardInstanceId) const;

	UWacomCardDetailPanel* EnsureLegacyPanel();
	UWacomCardDetailPanel* EnsureFirstPersonPanel();
	void EnsureLayer();
	void PositionLegacyNear(UCardWidget* SourceWidget);
	void PositionLegacyBesideAnchor(const FVector2D& AnchorPosition, const FVector2D& AnchorSize);
	bool ShowFirstPersonAtSlot(
		const FWacomCardDetailViewData& DetailData,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void PositionFirstPersonBesideSlot(const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HideFirstPerson();
	void TickMotion(float DeltaTime);

	bool BeginMotionShow(UBattleHUD::ECardDetailHost Host);
	void RequestMotionShow(UBattleHUD::ECardDetailHost Host);
	void RequestMotionHide(UBattleHUD::ECardDetailHost Host, bool bImmediate);
	void ForceHideHost(UBattleHUD::ECardDetailHost Host);
	void ForceHideAll();
	UWacomCardDetailPanel* GetPanelForHost(UBattleHUD::ECardDetailHost Host) const;
	bool UpdateMotionTarget(UBattleHUD::ECardDetailHost Host);
	bool ComputeLegacyTarget(UCardWidget* SourceWidget, FVector2D& OutPosition);
	bool ComputeFirstPersonTarget(const FWacomFirstPersonCardLayerSlotView& SlotView, FVector2D& OutPosition);
	FVector2D ComputeStablePosition(
		const FVector2D& AnchorPosition,
		const FVector2D& AnchorSize,
		const FVector2D& LayerSize,
		const FVector2D& PanelSize,
		float DetailPadding);
	void ApplyMotionVisual(UBattleHUD::ECardDetailHost Host, const FVector2D& Position, float Opacity);
	void CollapseHost(UBattleHUD::ECardDetailHost Host);
	bool IsMotionSource(UBattleHUD::ECardDetailHost Host, UCardWidget* SourceWidget) const;
	bool IsMotionSource(UBattleHUD::ECardDetailHost Host, const FGuid& CardInstanceId) const;
	FVector2D GetFirstPersonViewportSize() const;

	void SetFirstPersonSource(const FGuid& CardInstanceId);
	void ClearFirstPersonSource();
	bool IsCurrentFirstPersonSource(const FGuid& CardInstanceId) const;
	void ClearLegacySource();
	void UpdateFirstPersonSlot(const FWacomFirstPersonCardLayerSlotView& SlotView);
	FVector2D GetLastFirstPersonPanelPosition() const { return LastFirstPersonCardDetailPanelPosition; }
	void RemoveFirstPersonPanelFromViewport();

private:
	struct FCardDetailMotionState
	{
		UBattleHUD::ECardDetailHost ActiveHost = UBattleHUD::ECardDetailHost::None;
		TWeakObjectPtr<UCardWidget> ActiveLegacySource;
		FGuid ActiveFirstPersonSourceId;
		FWacomFirstPersonCardLayerSlotView ActiveFirstPersonSlot;
		bool bHasActiveFirstPersonSlot = false;
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

	UBattleHUD& HUD;
	TWeakObjectPtr<UCardWidget> CurrentLegacySource;
	FGuid CurrentFirstPersonSourceId;
	FVector2D LastFirstPersonCardDetailPanelPosition = FVector2D::ZeroVector;
	FCardDetailMotionState MotionState;
	bool bLoggedMissingCardDetailLayer = false;
};
