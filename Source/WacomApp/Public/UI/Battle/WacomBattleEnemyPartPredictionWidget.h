// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Battle/WacomBattleEnemyPartPredictionTypes.h"
#include "WacomBattleEnemyPartPredictionWidget.generated.h"

class UBorder;
class UTextBlock;

/**
 * Battle 场景敌方部位的轻量预测 badge。
 *
 * 数据只来自场景 bridge 的 UI 预测 ViewData，不修改 BattleSession。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomBattleEnemyPartPredictionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy|Prediction")
	void SetPredictionView(const FWacomBattleEnemyPartPredictionView& InView);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Prediction")
	const FWacomBattleEnemyPartPredictionView& GetPredictionView() const { return CurrentView; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Scene Enemy|Prediction")
	void BP_OnPredictionViewChanged(const FWacomBattleEnemyPartPredictionView& InView);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BadgeBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MainTextBlock;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailTextBlock;

private:
	void ApplyPredictionViewToWidgets();
	FLinearColor BuildBadgeColor() const;

	UPROPERTY(Transient)
	FWacomBattleEnemyPartPredictionView CurrentView;
};
