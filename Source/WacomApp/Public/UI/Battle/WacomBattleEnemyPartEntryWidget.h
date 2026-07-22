// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "CoreMinimal.h"
#include "Templates/PimplPtr.h"
#include "TimerManager.h"
#include "WacomBattleEnemyPanelViewData.h"
#include "WacomBattleEnemyPartEntryWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class USizeBox;
class UWidget;
class UWidgetAnimation;
class UWacomBattleEnemyIntentPresentationStyle;
class UWacomBattleStatusIconListWidget;
class FWacomBattleEnemyPartPresentationState;
class FWacomBattleEnemyVitalsMaterialAdapter;
struct FWacomBattleEnemyPartPresentationUpdate;
enum class EWacomBattleEnemyMotionCue : uint8;

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
	UWacomBattleEnemyPartEntryWidget(const FObjectInitializer& ObjectInitializer);
	virtual ~UWacomBattleEnemyPartEntryWidget() override;

	/** Editor Inspector 读取正式紧凑 WBP 的 UI-only Intent Style。 */
	UWacomBattleEnemyIntentPresentationStyle* GetIntentPresentationStyle() const
	{
		return IntentPresentationStyle;
	}

	FWacomBattleEnemyPartInspectionRequestedNative OnInspectionRequestedNative;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void SetPartEntryViewData(const FWacomBattleEnemyPartEntryViewData& InView);
	const FWacomBattleEnemyPartEntryViewData& GetPartEntryViewData() const;
	const FWacomBattleEnemyPartEntryViewData& GetEffectivePartEntryViewData() const;
	void SetActionPreview(const FWacomBattleEnemyPartEntryViewData& InPreviewView);
	void ClearActionPreview();
	bool HasActionPreview() const;
	void SetContextHighlighted(bool bHighlighted);
	void SetIntroDelaySeconds(float InDelaySeconds);
	void SetSegmentLayout(int32 InPartIndex, int32 InPartCount);
	void CancelPendingPresentation();
	void SetInspectionInteractionEnabled(bool bEnabled);
	bool IsInspectionInteractionEnabled() const { return bInspectionInteractionEnabled; }
	void ApplyRuntimePresentationPolicy(bool bSimplifiedMotion, float FlashIntensity);

	void RefreshPresentation();
	void ApplyVitalsMaterialPresentation();
	void CaptureOutgoingIntent(const FWacomBattleEnemyPartEntryViewData& PreviousView);
	void ScheduleIntroAnimation();
	void PlayIntroAnimation();
	void RouteMotionCues(const FWacomBattleEnemyPartPresentationUpdate& Update);
	void RouteMotionCue(EWacomBattleEnemyMotionCue Cue);
	void RefreshInspectionInteraction();
	void CancelIntroTimer();
	void PlaySemanticAnimation(UWidgetAnimation* Animation, float AuthoredDurationSeconds);
	float ResolveWorldTimeSeconds() const;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel|Intent", meta = (AllowPrivateAccess = "true", ToolTip = "紧凑敌人面板用于按稳定 IntentId 解析图标的 UI-only Style。为空时保留 WBP 默认图标，不影响规则。"))
	TObjectPtr<UWacomBattleEnemyIntentPresentationStyle> IntentPresentationStyle = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel|Motion", meta = (AllowPrivateAccess = "true", ToolTip = "HP 真实下降后保留旧值残影的时间，单位：秒；推荐 0.05-0.15。只影响表现，不改变规则数值。"))
	float DamageTrailHoldSeconds = 0.09f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel|Motion", meta = (AllowPrivateAccess = "true", ToolTip = "HP 伤害残影收缩到权威值的时间，单位：秒；推荐 0.15-0.30。只影响表现。"))
	float DamageTrailRecoverySeconds = 0.22f;

	TPimplPtr<FWacomBattleEnemyPartPresentationState> PresentationState;
	TPimplPtr<FWacomBattleEnemyVitalsMaterialAdapter> VitalsMaterialAdapter;
	FTimerHandle IntroTimerHandle;
	float IntroDelaySeconds = 0.0f;
	bool bInspectionInteractionEnabled = false;
	bool bIntroPending = false;

	friend class UWacomBattleEnemyPanelWidget;
	friend struct FWacomBattleEnemyPartEntryWidgetTestAccess;
};
