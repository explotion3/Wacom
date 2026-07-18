// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "CoreMinimal.h"
#include "TimerManager.h"
#include "WacomBattleEnemyPanelViewData.h"
#include "WacomBattleEnemyPartEntryWidget.generated.h"

class UProgressBar;
class UButton;
class UImage;
class UTextBlock;
class UWidget;
class UWidgetAnimation;
class UWacomBattleEnemyIntentPresentationStyle;
class UWacomBattleStatusIconListWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(
	FWacomBattleEnemyPartInspectionRequestedNative,
	const FBattlePartSlotIdentity&);

/**
 * 敌人聚合面板中的单个部位条目。
 *
 * 该类只消费 FWacomBattleEnemyPartEntryViewData，并把值写入正式 WBP 绑定。
 * 它不读取 BattleSession、不拉取 Snapshot，也不创建运行时布局。
 */
UCLASS(Abstract, Blueprintable, meta = (ToolTip = "敌人面板中的被动部位条目。只渲染 ViewData；布局、皮肤和动画由正式 WBP 提供。"))
class WACOMAPP_API UWacomBattleEnemyPartEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Enemy Panel")
	virtual void SetPartEntryViewData(const FWacomBattleEnemyPartEntryViewData& InView);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "返回当前真实 Snapshot 部位展示数据。Action Preview 不会覆盖该值。"))
	const FWacomBattleEnemyPartEntryViewData& GetPartEntryViewData() const { return CurrentView; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "返回当前实际显示的数据。Action Preview 激活时返回预览值。"))
	const FWacomBattleEnemyPartEntryViewData& GetEffectivePartEntryViewData() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "应用规则层生成的行动预览，只覆盖显示，不触发真实伤害、护盾或破坏动画。"))
	void SetActionPreview(const FWacomBattleEnemyPartEntryViewData& InPreviewView);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "清除行动预览并恢复真实 Snapshot 显示。"))
	void ClearActionPreview();

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Enemy Panel")
	bool HasActionPreview() const { return bHasActionPreview; }

	/** 设置当前条目是否对应场景 hover 部位。 */
	void SetContextHighlighted(bool bHighlighted);

	/** Panel 创建新条目时设置一次错峰入场延迟。 */
	void SetIntroDelaySeconds(float InDelaySeconds);

	/** Panel 移除条目前取消弱 Timer 和正在播放的动画。 */
	void CancelPendingPresentation();

	/** Editor content builder 为正式紧凑 WBP 配置 UI-only Intent Style。 */
	void SetIntentPresentationStyle(UWacomBattleEnemyIntentPresentationStyle* InStyle);
	UWacomBattleEnemyIntentPresentationStyle* GetIntentPresentationStyle() const
	{
		return IntentPresentationStyle;
	}

	/** 仅由 HUD runtime 在 Idle 且无拖卡/预览/结算时启用。 */
	void SetInspectionInteractionEnabled(bool bEnabled);
	bool IsInspectionInteractionEnabled() const { return bInspectionInteractionEnabled; }

	FWacomBattleEnemyPartInspectionRequestedNative OnInspectionRequestedNative;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void RefreshPresentation();
	void ScheduleIntroAnimation();
	void PlayIntroAnimation();
	void PlayRealFactTransition(
		const FWacomBattleEnemyPartEntryViewData& PreviousView,
		const FWacomBattleEnemyPartEntryViewData& NewView);
	void RefreshContextPresentation(bool bPreviousContextActive);
	void RefreshInspectionInteraction();
	void CancelIntroTimer();

	UFUNCTION()
	void HandleInspectClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PartNameText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HpBar = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HpText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ShieldContainer = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ShieldFrame = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ShieldBadge = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ShieldText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> InitiativeText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> InitiativeDiamond = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> IntentDiamond = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IntentIcon = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> IntentText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResistanceText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> DetailsContainer = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWacomBattleStatusIconListWidget> StatusList = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusOverflowText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ContextHighlight = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ActionPreviewOverlay = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> DestroyedOverlay = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> DestroyedMark = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> InspectHitTarget = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> IntroAnimation = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> DamagePulseAnimation = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> ShieldPulseAnimation = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> DestroyedPulseAnimation = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> ContextHighlightAnimation = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> InitiativePulseAnimation = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> IntentChangedAnimation = nullptr;

	UPROPERTY(Transient)
	FWacomBattleEnemyPartEntryViewData CurrentView;

	UPROPERTY(Transient)
	FWacomBattleEnemyPartEntryViewData ActionPreviewView;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel|Preview", meta = (AllowPrivateAccess = "true", ToolTip = "Action Preview 激活时条目的整体透明度。单位：0-1；推荐 0.7-1.0，只影响表现。"))
	float ActionPreviewRenderOpacity = 0.82f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel|Intent", meta = (AllowPrivateAccess = "true", ToolTip = "紧凑敌人面板用于按稳定 IntentId 解析图标的 UI-only Style。为空时保留 WBP 默认图标，不影响规则。"))
	TObjectPtr<UWacomBattleEnemyIntentPresentationStyle> IntentPresentationStyle = nullptr;

	FTimerHandle IntroTimerHandle;
	float IntroDelaySeconds = 0.0f;
	bool bHasReceivedViewData = false;
	bool bHasActionPreview = false;
	bool bContextHighlighted = false;
	bool bInspectionInteractionEnabled = false;
	bool bIntroPending = false;
};
