// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleEnemyPanelViewData.h"
#include "WacomBattleEnemyInspectionPartRowWidget.generated.h"

class UButton;
class UTextBlock;
class UWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(
	FWacomBattleEnemyInspectionPartSelectedNative,
	const FBattlePartSlotIdentity&);

/** 双侧敌人详情左栏中的被动部位导航行。 */
UCLASS(Abstract, Blueprintable, meta = (ToolTip = "敌人详情左侧的被动部位导航行。只消费 Enemy Part ViewData，并把点击作为 UI 意图向上广播。"))
class WACOMAPP_API UWacomBattleEnemyInspectionPartRowWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetPartViewData(const FWacomBattleEnemyPartEntryViewData& InView);
	void SetSelected(bool bInSelected);
	const FWacomBattleEnemyPartEntryViewData& GetPartViewData() const { return CurrentView; }
	bool IsSelected() const { return bSelected; }

	FWacomBattleEnemyInspectionPartSelectedNative OnPartSelectedNative;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void RefreshPresentation();

	UFUNCTION()
	void HandleSelectClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PartSelectButton = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PartNameText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HpText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ShieldContainer = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ShieldText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> InitiativeText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> SelectionHighlight = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> DestroyedOverlay = nullptr;

	UPROPERTY(Transient)
	FWacomBattleEnemyPartEntryViewData CurrentView;

	bool bHasViewData = false;
	bool bSelected = false;
};
