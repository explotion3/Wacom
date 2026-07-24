// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleEnemyPanelViewData.h"
#include "WacomBattleEnemyInspectionWidget.generated.h"

class UButton;
class UImage;
class UPanelWidget;
class UProgressBar;
class UTextBlock;
class UWidget;
class UWidgetAnimation;
class UWacomBattleEnemyIntentPresentationStyle;
class UWacomBattleEnemyInspectionPartRowWidget;
class UWacomBattleIntentEffectRowWidget;
class UWacomBattleIntentTooltipWidget;
class UWacomBattleStatusIconListWidget;

DECLARE_MULTICAST_DELEGATE(FWacomBattleEnemyInspectionCloseRequestedNative);
DECLARE_MULTICAST_DELEGATE_OneParam(
	FWacomBattleEnemyInspectionSelectionRequestedNative,
	const FBattlePartSlotIdentity&);

/** BattleHUD 独占的非模态双侧敌人详情面板。 */
UCLASS(Abstract, Blueprintable, meta = (ToolTip = "战斗中的被动双侧敌人详情面板。只消费 Enemy Inspection ViewData，不读取或修改 BattleSession。"))
class WACOMAPP_API UWacomBattleEnemyInspectionWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	bool SetInspectionViewData(const FWacomBattleEnemyInspectionViewData& InView);
	void ClearInspectionViewData();
	void OpenInspection();
	void CloseInspection(bool bImmediate);
	bool IsInspectionOpen() const { return bOpen && !bClosing; }
	bool HasInspectionViewData() const { return bHasViewData; }
	const FWacomBattleEnemyInspectionViewData& GetInspectionViewData() const { return CurrentView; }

	void SetPartRowWidgetClass(TSubclassOf<UWacomBattleEnemyInspectionPartRowWidget> InClass);
	TSubclassOf<UWacomBattleEnemyInspectionPartRowWidget> GetPartRowWidgetClass() const
	{
		return PartRowWidgetClass;
	}
	void SetIntentPresentationStyle(UWacomBattleEnemyIntentPresentationStyle* InStyle);
	UWacomBattleEnemyIntentPresentationStyle* GetIntentPresentationStyle() const
	{
		return IntentPresentationStyle;
	}
	void SetIntentEffectRowWidgetClass(
		TSubclassOf<UWacomBattleIntentEffectRowWidget> InClass);
	void SetIntentTooltipWidgetClass(
		TSubclassOf<UWacomBattleIntentTooltipWidget> InClass);
	TSubclassOf<UWacomBattleIntentEffectRowWidget>
	GetIntentEffectRowWidgetClass() const
	{
		return IntentEffectRowWidgetClass;
	}
	TSubclassOf<UWacomBattleIntentTooltipWidget>
	GetIntentTooltipWidgetClass() const
	{
		return IntentTooltipWidgetClass;
	}

	FWacomBattleEnemyInspectionCloseRequestedNative OnCloseRequestedNative;
	FWacomBattleEnemyInspectionSelectionRequestedNative OnSelectionRequestedNative;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void SynchronizeProperties() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Inspection", meta = (ToolTip = "左侧部位导航使用的正式 WBP 类。"))
	TSubclassOf<UWacomBattleEnemyInspectionPartRowWidget> PartRowWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Inspection", meta = (ToolTip = "详情面板按稳定 IntentId 解析图标的 UI-only Style；不读取规则效果或名称推断。"))
	TObjectPtr<UWacomBattleEnemyIntentPresentationStyle> IntentPresentationStyle = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Inspection", meta = (AllowAbstract = "false", ToolTip = "档案完整 Intent 效果列表使用的被动 Row 类。"))
	TSubclassOf<UWacomBattleIntentEffectRowWidget> IntentEffectRowWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Inspection", meta = (AllowAbstract = "false", ToolTip = "档案 Intent 图标悬停时使用的被动 Tooltip 类。"))
	TSubclassOf<UWacomBattleIntentTooltipWidget> IntentTooltipWidgetClass;

private:
	void SyncPartRows();
	void ClearPartRows();
	void RefreshSelectedPartDetails();
	const FWacomBattleEnemyPartEntryViewData* FindSelectedPart() const;
	bool EnsureValidSelection();
	FName BuildPartRowKey(const FWacomBattleEnemyPartEntryViewData& PartView) const;
	void HandlePartRowSelected(const FBattlePartSlotIdentity& PartIdentity);
	void ScheduleRightPanelOpen();
	void PlayRightPanelOpen();
	void CancelRightPanelOpenTimer();
	void EnsureIntentTooltipBinding();
	void RefreshIntentPresentation(
		const FWacomBattleEnemyPartEntryViewData* Part);
	void ClearIntentPresentation();

	UFUNCTION()
	UWidget* HandleBuildIntentTooltipWidget();

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleCloseAnimationFinished();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> LeftPanel = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> RightPanel = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EnemyNameText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EnemyStateText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> PartNavigator = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SelectedPartNameText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HpBar = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HpText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ShieldContainer = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ShieldText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> InitiativeText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> IntentText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IntentIcon = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> IntentTooltipTarget = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> IntentEffectsList = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResistanceText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWacomBattleStatusIconListWidget> StatusList = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> DestroyedOverlay = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> OpenLeftAnimation = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> OpenRightAnimation = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> CloseAnimation = nullptr;

	UPROPERTY(Transient)
	FWacomBattleEnemyInspectionViewData CurrentView;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UWacomBattleEnemyInspectionPartRowWidget>> PartRows;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomBattleIntentEffectRowWidget>> IntentEffectRows;

	UPROPERTY(Transient)
	TObjectPtr<UWacomBattleIntentTooltipWidget> CachedIntentTooltipWidget = nullptr;

	FTimerHandle RightPanelOpenTimerHandle;

	bool bHasViewData = false;
	bool bOpen = false;
	bool bClosing = false;

	friend struct FWacomBattleEnemyInspectionWidgetTestAccess;
};
