// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "WacomBattleEnemyPanelViewData.h"
#include "WacomBattleEnemyPanelWidget.generated.h"

class UTextBlock;
class UVerticalBox;
struct FEnemySnapshot;

UCLASS(Blueprintable, meta = (ToolTip = "BattleHUD 的敌人总面板。只渲染 FWacomBattleEnemyPanelViewData。"))
class WACOMAPP_API UWacomBattleEnemyPanelWidget : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Enemy Panel")
	void SetEnemyPanelViewData(const TArray<FWacomBattleEnemyPanelViewData>& InViews);

	static FWacomBattleEnemyPanelViewData BuildEnemyPanelViewDataFromSnapshot(
		const FBattleSnapshot& Snap,
		const FEnemySnapshot& Enemy);

	static TArray<FWacomBattleEnemyPanelViewData> BuildEnemyPanelViewDataListFromSnapshot(const FBattleSnapshot& Snap);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeRefreshFromSnapshot(const FBattleSnapshot& Snap) override;

private:
	void RebuildEnemyWidgets();
	FText BuildEnemyHeaderText(const FWacomBattleEnemyPanelViewData& View) const;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RootBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> EnemyListBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyTextBlock = nullptr;

	UPROPERTY(Transient)
	TArray<FWacomBattleEnemyPanelViewData> CurrentViews;
};
