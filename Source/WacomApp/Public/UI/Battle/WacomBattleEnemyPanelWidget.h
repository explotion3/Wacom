// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "WacomBattleEnemyPanelViewData.h"
#include "WacomBattleEnemyPanelWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UBorder;
class UWacomBattleEnemyPartEntryWidget;
struct FEnemySnapshot;

USTRUCT()
struct FWacomBattleEnemyPanelEnemyWidgetState
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> EnemyBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> EnemyBorder = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeaderText = nullptr;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UWacomBattleEnemyPartEntryWidget>> PartEntryWidgets;

	UPROPERTY(Transient)
	TSet<FName> AnimatedPartEntryKeys;
};

UCLASS(Blueprintable, meta = (ToolTip = "BattleHUD 的敌人总面板。只渲染 FWacomBattleEnemyPanelViewData。"))
class WACOMAPP_API UWacomBattleEnemyPanelWidget : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Enemy Panel")
	void SetEnemyPanelViewData(const TArray<FWacomBattleEnemyPanelViewData>& InViews);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "设置敌人面板内每个部位条目使用的 Widget 类。通常在 WBP 默认值中指定；运行时切换会清空并重建条目缓存。"))
	void SetPartEntryWidgetClass(TSubclassOf<UWacomBattleEnemyPartEntryWidget> InWidgetClass);

	static FWacomBattleEnemyPanelViewData BuildEnemyPanelViewDataFromSnapshot(
		const FBattleSnapshot& Snap,
		const FEnemySnapshot& Enemy);

	static TArray<FWacomBattleEnemyPanelViewData> BuildEnemyPanelViewDataListFromSnapshot(const FBattleSnapshot& Snap);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeRefreshFromSnapshot(const FBattleSnapshot& Snap) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "敌人面板内每个部位条目使用的 Widget 类。默认使用 C++ fallback；正式 WBP 可指定 BP_WacomBattleEnemyPartEntryWidget。"))
	TSubclassOf<UWacomBattleEnemyPartEntryWidget> PartEntryWidgetClass;

private:
	void SyncEnemyWidgets();
	FWacomBattleEnemyPanelEnemyWidgetState& FindOrCreateEnemyWidgetState(const FWacomBattleEnemyPanelViewData& View);
	UWacomBattleEnemyPartEntryWidget* FindOrCreatePartEntryWidget(
		FWacomBattleEnemyPanelEnemyWidgetState& EnemyState,
		const FWacomBattleEnemyPartEntryViewData& PartView);
	FText BuildEnemyHeaderText(const FWacomBattleEnemyPanelViewData& View) const;
	FName BuildEnemyWidgetKey(const FWacomBattleEnemyPanelViewData& View) const;
	FName BuildPartEntryWidgetKey(const FWacomBattleEnemyPartEntryViewData& PartView) const;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RootBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> EnemyListBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyTextBlock = nullptr;

	UPROPERTY(Transient)
	TArray<FWacomBattleEnemyPanelViewData> CurrentViews;

	UPROPERTY(Transient)
	TMap<FName, FWacomBattleEnemyPanelEnemyWidgetState> EnemyWidgetStates;

	bool bSyncingEnemyWidgets = false;
};
