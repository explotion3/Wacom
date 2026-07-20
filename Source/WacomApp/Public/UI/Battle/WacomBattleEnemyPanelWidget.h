// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "CoreMinimal.h"
#include "WacomBattleEnemyPanelViewData.h"
#include "WacomBattleEnemyPanelWidget.generated.h"

class UPanelWidget;
class USizeBox;
class UWacomBattleEnemyPartEntryWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(
	FWacomBattleEnemyPanelInspectionRequestedNative,
	const FBattlePartSlotIdentity&);

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

	/** WBP 子类的类默认值：0 表示由分段数量决定宽度，正数表示固定铭牌宽度。 */
	void SetFixedPanelWidth(float InWidth) { FixedPanelWidth = FMath::Max(0.0f, InWidth); }
	float GetFixedPanelWidth() const { return FixedPanelWidth; }

	/** HUD runtime 的事件驱动输入门禁；禁用时整块头顶面板点击穿透。 */
	void SetInspectionInteractionEnabled(bool bEnabled);
	bool IsInspectionInteractionEnabled() const { return bInspectionInteractionEnabled; }

	FWacomBattleEnemyPanelInspectionRequestedNative OnInspectionRequestedNative;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "面板内每个部位条目使用的正式 WBP 类。必须继承 UWacomBattleEnemyPartEntryWidget。"))
	TSubclassOf<UWacomBattleEnemyPartEntryWidget> PartEntryWidgetClass;

private:
	void ResolveAuthoredBindings();
	void ApplyAuthoredGeometry();
	void SyncPartEntries();
	void ClearPartEntries();
	void ApplyInspectionInteractionState();
	void HandlePartInspectionRequested(const FBattlePartSlotIdentity& PartIdentity);
	UWacomBattleEnemyPartEntryWidget* FindOrCreatePartEntryWidget(
		const FWacomBattleEnemyPartEntryViewData& PartView);
	FName BuildPartEntryWidgetKey(const FWacomBattleEnemyPartEntryViewData& PartView) const;
	bool DoesPartBelongToCurrentEnemy(const FWacomBattleEnemyPartEntryViewData& PartView) const;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> PanelRoot = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> PartList = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel|Layout", meta = (AllowPrivateAccess = "true", ToolTip = "头顶敌人铭牌的固定宽度，单位：Slate Unit。0 表示按部位数量与每段最小宽度自然增长；单部位正式 WBP 推荐 268。"))
	float FixedPanelWidth = 0.0f;

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
	bool bInspectionInteractionEnabled = false;
};
