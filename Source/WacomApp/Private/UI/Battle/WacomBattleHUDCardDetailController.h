// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardDetailMotionController.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class FWacomBattleHUDRuntime;
class UBattleHUD;
class UWacomCardDetailPanel;
enum class EWacomBattleHUDCardDetailHost : uint8;
struct FWacomFirstPersonCardDetailPanelHostContext;
struct FWacomCardDetailViewData;
struct FWacomFirstPersonCardLayerSlotView;

class FWacomBattleHUDCardDetailController
{
public:
	explicit FWacomBattleHUDCardDetailController(FWacomBattleHUDRuntime& InRuntime);

	bool IsVisible() const;
	FText GetNameText() const;

	void HideAll();
	void HideFirstPersonForSource(const FGuid& CardInstanceId);
	bool IsFirstPersonInspectDetailActiveForSource(const FGuid& CardInstanceId) const;

	UWacomCardDetailPanel* EnsureFirstPersonPanel();
	bool ShowFirstPersonAtSlot(
		const FWacomCardDetailViewData& DetailData,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void PositionFirstPersonBesideSlot(const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HideFirstPerson();
	void TickMotion(float DeltaTime);

	void ForceHideHost(EWacomBattleHUDCardDetailHost Host);
	void ForceHideAll();
	bool ComputeFirstPersonTarget(const FWacomFirstPersonCardLayerSlotView& SlotView, FVector2D& OutPosition);
	FVector2D ComputeStablePosition(
		const FVector2D& AnchorPosition,
		const FVector2D& AnchorSize,
		const FVector2D& LayerSize,
		const FVector2D& PanelSize,
		float DetailPadding);
	FVector2D GetFirstPersonViewportSize() const;

	void SetFirstPersonSource(const FGuid& CardInstanceId);
	void ClearFirstPersonSource();
	bool IsCurrentFirstPersonSource(const FGuid& CardInstanceId) const;
	void UpdateFirstPersonSlot(const FWacomFirstPersonCardLayerSlotView& SlotView);
	FVector2D GetLastFirstPersonPanelPosition() const { return MotionController.GetLastPanelPosition(); }
	void RemoveFirstPersonPanelFromViewport();

private:
	FWacomFirstPersonCardDetailMotionConfig BuildMotionConfig() const;
	FWacomFirstPersonCardDetailPanelHostContext BuildPanelHostContext() const;

	FWacomBattleHUDRuntime& Runtime;
	FWacomFirstPersonCardDetailMotionController MotionController;
};
