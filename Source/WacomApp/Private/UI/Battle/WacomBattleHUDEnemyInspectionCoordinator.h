// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleEnemyPanelViewData.h"

class FWacomBattleHUDRuntime;
class UWacomBattleEnemyInspectionWidget;

/** BattleHUD 独占的非模态 Scene Enemy 详情生命周期。 */
class FWacomBattleHUDEnemyInspectionCoordinator
{
public:
	explicit FWacomBattleHUDEnemyInspectionCoordinator(FWacomBattleHUDRuntime& InRuntime);
	~FWacomBattleHUDEnemyInspectionCoordinator();

	bool ToggleInspection(
		const FWacomBattleEnemyPanelViewData& EnemyView,
		const FBattlePartSlotIdentity& PartIdentity);
	void UpdateEnemyView(const FWacomBattleEnemyPanelViewData& EnemyView);
	bool TryCloseInspection();
	void CloseInspection(bool bImmediate);
	void Shutdown();

	bool IsInspectionOpen() const;
	bool IsInspectingEnemySlot(FName EnemySlotId) const;
	const FBattlePartSlotIdentity& GetSelectedPartIdentity() const
	{
		return SelectedPartIdentity;
	}
	UWacomBattleEnemyInspectionWidget* GetWidget() const { return InspectionWidget.Get(); }

private:
	UWacomBattleEnemyInspectionWidget* EnsureWidget();
	void BindWidgetDelegates(UWacomBattleEnemyInspectionWidget& Widget);
	void HandleCloseRequested();
	void HandleSelectionRequested(const FBattlePartSlotIdentity& PartIdentity);

	FWacomBattleHUDRuntime& Runtime;
	TWeakObjectPtr<UWacomBattleEnemyInspectionWidget> InspectionWidget;
	FName InspectedEnemySlotId = NAME_None;
	FBattlePartSlotIdentity SelectedPartIdentity;
	bool bLoggedMissingWidgetClass = false;
};
