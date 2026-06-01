// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Battle/WacomBattleEnemyPartStatusBadgeTypes.h"
#include "WacomBattleEnemyPartStatusBadgeWidget.generated.h"

class UBorder;
class UHorizontalBox;
class UOverlay;
class UTextBlock;
class UWacomProgressBar;

/**
 * Battle 场景敌方部位的常驻状态 badge。
 *
 * 数据只来自 Battle Snapshot 的只读 ViewData，不修改 BattleSession。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomBattleEnemyPartStatusBadgeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy|Status")
	void SetStatusBadgeView(const FWacomBattleEnemyPartStatusBadgeView& InView);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Status")
	const FWacomBattleEnemyPartStatusBadgeView& GetStatusBadgeView() const { return CurrentView; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Scene Enemy|Status")
	void BP_OnStatusBadgeViewChanged(const FWacomBattleEnemyPartStatusBadgeView& InView);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BadgeBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PartNameTextBlock;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomProgressBar> HpBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> CoreRow;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InitiativeTextBlock;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> IntentTextBlock;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShieldTextBlock;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusTextBlock;

private:
	void ApplyStatusBadgeViewToWidgets();
	FLinearColor BuildBadgeColor() const;

	UPROPERTY(Transient)
	FWacomBattleEnemyPartStatusBadgeView CurrentView;
};
