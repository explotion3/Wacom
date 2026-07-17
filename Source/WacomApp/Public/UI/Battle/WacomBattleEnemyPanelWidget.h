// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "CoreMinimal.h"
#include "WacomBattleEnemyPanelViewData.h"
#include "WacomBattleEnemyPanelWidget.generated.h"

class UPanelWidget;
class UTextBlock;
class UWidget;
class UWacomBattleEnemyPartEntryWidget;

/**
 * 单个 Scene Enemy Host 的头顶聚合面板。
 *
 * 每个实例只渲染一个 FWacomBattleEnemyPanelViewData。C++ 负责稳定部位条目复用，
 * 正式 WBP 负责全部布局、皮肤和动画。
 */
UCLASS(Abstract, Blueprintable, meta = (ToolTip = "单个 Scene Enemy Host 的被动聚合面板。只消费一个 Enemy ViewData，布局由正式 WBP 提供。"))
class WACOMAPP_API UWacomBattleEnemyPanelWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Enemy Panel")
	void SetEnemyPanelViewData(const FWacomBattleEnemyPanelViewData& InView);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Enemy Panel")
	void ClearEnemyPanelViewData();

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Enemy Panel")
	bool HasEnemyPanelViewData() const { return bHasCurrentView; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Enemy Panel")
	const FWacomBattleEnemyPanelViewData& GetEnemyPanelViewData() const { return CurrentView; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "应用规则层生成的部位行动预览，只覆盖现有部位条目的显示。"))
	bool SetActionPreviewPartViews(const TArray<FWacomBattleEnemyPartEntryViewData>& InPreviewParts);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "清除全部部位行动预览。"))
	void ClearActionPreview();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "设置当前场景 hover 的稳定 PartSlotId；NAME_None 表示清除。"))
	void SetHoveredPartSlotId(FName InPartSlotId);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "设置面板内每个部位条目使用的正式 WBP 类。运行时切换会清空并重建条目。"))
	void SetPartEntryWidgetClass(TSubclassOf<UWacomBattleEnemyPartEntryWidget> InWidgetClass);

	TSubclassOf<UWacomBattleEnemyPartEntryWidget> GetPartEntryWidgetClass() const
	{
		return PartEntryWidgetClass;
	}

	/** Editor builder 为单部位紧凑 Panel 配置：常态隐藏名称和聚合 Initiative。 */
	void SetCompactSinglePartPresentation(bool bInCompact)
	{
		bCompactSinglePartPresentation = bInCompact;
		RefreshHeader();
	}
	bool IsCompactSinglePartPresentation() const
	{
		return bCompactSinglePartPresentation;
	}

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "面板内每个部位条目使用的正式 WBP 类。必须继承 UWacomBattleEnemyPartEntryWidget。"))
	TSubclassOf<UWacomBattleEnemyPartEntryWidget> PartEntryWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "单部位紧凑 Panel 开启后，常态隐藏敌人名称和聚合 Initiative；hover 或 Action Preview 时只展开敌人名称。多部位正式 WBP 应保持关闭。"))
	bool bCompactSinglePartPresentation = false;

private:
	void RefreshHeader();
	void SyncPartEntries();
	void ClearPartEntries();
	void RefreshContextHighlight();
	UWacomBattleEnemyPartEntryWidget* FindOrCreatePartEntryWidget(
		const FWacomBattleEnemyPartEntryViewData& PartView);
	FName BuildPartEntryWidgetKey(const FWacomBattleEnemyPartEntryViewData& PartView) const;
	bool DoesPartBelongToCurrentEnemy(const FWacomBattleEnemyPartEntryViewData& PartView) const;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EnemyNameText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EnemyInitiativeText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> PartList = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> PanelContextHighlight = nullptr;

	UPROPERTY(Transient)
	FWacomBattleEnemyPanelViewData CurrentView;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UWacomBattleEnemyPartEntryWidget>> PartEntryWidgets;

	UPROPERTY(Transient)
	TSet<FName> AnimatedPartEntryKeys;

	FName HoveredPartSlotId = NAME_None;
	bool bHasCurrentView = false;
	bool bHasActionPreview = false;
	bool bSyncingPartEntries = false;
};
