// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "WacomBattleEnemyPanelViewData.h"
#include "WacomBattleEnemyPartEntryWidget.generated.h"

class UTextBlock;
class UVerticalBox;

UCLASS(Blueprintable, meta = (ToolTip = "敌人面板中的单个部位条目。只渲染 FWacomBattleEnemyPartEntryViewData。"))
class WACOMAPP_API UWacomBattleEnemyPartEntryWidget : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Enemy Panel")
	void SetPartEntryViewData(const FWacomBattleEnemyPartEntryViewData& InView);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeRefreshFromSnapshot(const FBattleSnapshot& Snap) override;

private:
	void RefreshText();
	FText BuildStatusText() const;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RootBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PartNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatsText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> IntentText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText = nullptr;

	UPROPERTY(Transient)
	FWacomBattleEnemyPartEntryViewData CurrentView;
};
