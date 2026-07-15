// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomActivatableWidget.h"
#include "WacomExplorationHUD.generated.h"

class UTextBlock;
class UProgressBar;
class UWacomRunViewModel;
class UWacomRunViewModelProvider;
#if WITH_AUTOMATION_TESTS
struct FWacomExplorationHUDInputTestAccess;
#endif

/**
 * 探索关卡 HUD（C++ fallback 布局 + 读 ViewModel）。
 *
 * 数据流：
 *   RunSession 写 → OnRunStateChangedNative
 *     → Provider 监听 → 灌 ViewModel 字段 + 广播 OnRunViewModelRefreshedNative
 *     → 本 widget 收到 → RefreshFromViewModel 读 ViewModel 的字段 → SetText
 *
 * 本 widget 不直接订阅 RunSession，也不读 RunState。只认 ViewModel + Provider。
 *
 * WBP 子类可替代布局并通过 ViewBinding 绑定字段；C++ 保留 fallback 和输入协议。
 *
 * 输入：GameAndUI / NoCapture；激活后下一帧把 Slate 焦点交给游戏视口，
 * 保持可见鼠标的同时让移动与鼠标视角无需额外点击即可生效。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomExplorationHUD : public UWacomActivatableWidget
{
	GENERATED_BODY()

public:
	UWacomExplorationHUD(const FObjectInitializer& ObjectInitializer);

	/**
	 * 显示/隐藏交互 Toast（"按 E 战斗"）。由 PlayerController 在候选 Trigger 列表
	 * 变化时调用。Toast 文字由调用方传入，便于将来扩展（如"按 E 拾取"）。
	 */
	void SetInteractToastVisible(bool bVisible, const FText& Message = FText::GetEmpty());

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	/** 从当前 ViewModel 全量刷新 SetText / SetPercent。 */
	void RefreshFromViewModel();

	/** Provider 的全量刷新广播回调。 */
	void HandleViewModelRefreshed();

	/** 订阅 Provider（如果还没订阅）+ 刷新一次。 */
	void TrySubscribeAndRefresh();

private:
	void QueueGameViewportFocus();
	void ApplyGameViewportFocus();
	void CancelPendingGameViewportFocus();

	UWacomRunViewModelProvider* GetProvider() const;
	UWacomRunViewModel* GetViewModel() const;

	UPROPERTY(Transient)
	TObjectPtr<UWacomRunViewModelProvider> SubscribedProvider = nullptr;

	FTimerHandle PendingGameViewportFocusTimerHandle;
	bool bGameViewportFocusPending = false;

	// ---- C++ 默认布局占位（WBP 可替代）----

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PhaseText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> NodeText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DayText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> FingerText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ExpText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> ExpBar;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PressureTotalText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HungerText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> WoundText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> FatigueText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> BurdenText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DecayText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MisdeedText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> BloodlustText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DisabilityText;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HintText;

	/** 交互 Toast Border（容器 + 背景）。BindWidgetOptional：WBP 可覆盖。 */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<class UBorder> InteractToastBg;
	/** 交互 Toast 文字。BindWidgetOptional。 */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> InteractToastText;

#if WITH_AUTOMATION_TESTS
	friend struct FWacomExplorationHUDInputTestAccess;
#endif
};
