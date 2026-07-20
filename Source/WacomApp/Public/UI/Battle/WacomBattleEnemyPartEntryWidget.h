// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "CoreMinimal.h"
#include "TimerManager.h"
#include "WacomBattleEnemyPanelViewData.h"
#include "WacomBattleEnemyPartEntryWidget.generated.h"

class UButton;
class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTextBlock;
class USizeBox;
class UWidget;
class UWidgetAnimation;
class UWacomSettingsSubsystem;
class UWacomBattleEnemyIntentPresentationStyle;
class UWacomBattleStatusIconListWidget;
struct FWacomLocalSettingsSnapshot;
enum class EWacomRuntimeSettingsChangeReason : uint8;

/** Scene Enemy 连续生命条中的部位边缘语义。 */
enum class EWacomBattleEnemySegmentRole : uint8
{
	Single,
	First,
	Middle,
	Last,
};

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

	/** Panel 按 Definition 顺序注入分段位置；不改变稳定 Part identity。 */
	void SetSegmentLayout(int32 InPartIndex, int32 InPartCount);

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
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void ResolveAuthoredBindings();
	void RefreshPresentation();
	void ApplyVitalsMaterialPresentation();
	bool EnsureVitalsMaterial();
	void RestoreVitalsMaterial();
	void CaptureOutgoingIntent(const FWacomBattleEnemyPartEntryViewData& PreviousView);
	void ScheduleIntroAnimation();
	void PlayIntroAnimation();
	void PlayRealFactTransition(
		const FWacomBattleEnemyPartEntryViewData& PreviousView,
		const FWacomBattleEnemyPartEntryViewData& NewView);
	void RefreshContextPresentation(bool bPreviousContextActive);
	void RefreshInspectionInteraction();
	void CancelIntroTimer();
	void BindRuntimeSettings();
	void UnbindRuntimeSettings();
	void HandleRuntimeSettingsChanged(
		const FWacomLocalSettingsSnapshot& Snapshot,
		EWacomRuntimeSettingsChangeReason Reason);
	void PlaySemanticAnimation(UWidgetAnimation* Animation, float AuthoredDurationSeconds);
	float ResolveWorldTimeSeconds() const;
	static float ResolveHpPercent(const FWacomBattleEnemyPartEntryViewData& View);
	static EWacomBattleEnemySegmentRole ResolveSegmentRole(int32 PartIndex, int32 PartCount);

	UFUNCTION()
	void HandleInspectClicked();

	UFUNCTION()
	void HandleIntentChangeAnimationFinished();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> PartEntryRoot = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> VitalsTrackImage = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ShieldValueRoot = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> InitiativeSocket = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> IntentSocket = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> OutgoingIntentIcon = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ContextSurface = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> DestroyedSurface = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HpText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ShieldText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> InitiativeText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IntentIcon = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWacomBattleStatusIconListWidget> StatusList = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusOverflowText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> DestroyedMark = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> InspectHitTarget = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> IntroAnimation = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> DamageImpactAnimation = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> ShieldImpactAnimation = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> ShieldBreakAnimation = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> InitiativeStepAnimation = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> IntentChangeAnimation = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> ContextAnimation = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> DestroyedAnimation = nullptr;

	UPROPERTY(Transient)
	FWacomBattleEnemyPartEntryViewData CurrentView;

	UPROPERTY(Transient)
	FWacomBattleEnemyPartEntryViewData ActionPreviewView;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel|Intent", meta = (AllowPrivateAccess = "true", ToolTip = "紧凑敌人面板用于按稳定 IntentId 解析图标的 UI-only Style。为空时保留 WBP 默认图标，不影响规则。"))
	TObjectPtr<UWacomBattleEnemyIntentPresentationStyle> IntentPresentationStyle = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel|Motion", meta = (AllowPrivateAccess = "true", ToolTip = "HP 真实下降后保留旧值残影的时间，单位：秒；推荐 0.05-0.15。只影响表现，不改变规则数值。"))
	float DamageTrailHoldSeconds = 0.09f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel|Motion", meta = (AllowPrivateAccess = "true", ToolTip = "HP 伤害残影收缩到权威值的时间，单位：秒；推荐 0.15-0.30。只影响表现。"))
	float DamageTrailRecoverySeconds = 0.22f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> VitalsMaterialInstance = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> VitalsSourceMaterial = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<UWacomSettingsSubsystem> BoundSettingsSubsystem;

	FDelegateHandle RuntimeSettingsChangedHandle;

	FTimerHandle IntroTimerHandle;
	float IntroDelaySeconds = 0.0f;
	float DamageTrailStartPercent = 0.0f;
	float DamageStartTimeSeconds = -1000.0f;
	float ShieldImpactStartTimeSeconds = -1000.0f;
	float RuntimeFlashIntensity = 1.0f;
	int32 SegmentIndex = 0;
	int32 SegmentCount = 1;
	EWacomBattleEnemySegmentRole SegmentRole = EWacomBattleEnemySegmentRole::Single;
	bool bHasReceivedViewData = false;
	bool bHasActionPreview = false;
	bool bContextHighlighted = false;
	bool bInspectionInteractionEnabled = false;
	bool bIntroPending = false;
	bool bRuntimeSimplifiedMotion = false;

	friend struct FWacomBattleEnemyPartEntryWidgetTestAccess;
};
