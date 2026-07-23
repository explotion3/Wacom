// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "PlayerStatusBar.generated.h"

class UWacomProgressBar;
class UWacomBattleStatusIconListWidget;
class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTextBlock;
class UWidget;
class USoundBase;
class UWacomSettingsSubsystem;
class FWacomBattlePlayerVitalsPlayback;
struct FWacomLocalSettingsSnapshot;
enum class EWacomRuntimeSettingsChangeReason : uint8;

struct WACOMAPP_API FWacomPlayerStatusBarAutomationTestView
{
	float CurrentHpPercent = 0.0f;
	float DamageTrailPercent = 0.0f;
	float PreviewHpPercent = 0.0f;
	float LowHealthAmount = 0.0f;
	float DamagePulseAmount = 0.0f;
	float ShieldPulseAmount = 0.0f;
	float ShieldScale = 1.0f;
	int32 DisplayHp = 0;
	int32 DisplayShield = 0;
	int32 PreviewMode = 0;
	bool bHasActionPreview = false;
	bool bShieldVisible = false;
	bool bPlaybackActive = false;
	bool bReducedMotion = false;
};

UCLASS(Blueprintable, meta = (ToolTip = "Battle 玩家状态条 Widget。继承 UWacomBattleWidgetBase，只根据 Snapshot 显示玩家 HP、Shield 和 runtime 状态图标，不提交玩家命令或修改 BattleSession。"))
class WACOMAPP_API UPlayerStatusBar : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	virtual ~UPlayerStatusBar() override;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Player Status", meta = (ToolTip = "应用战斗行动预览。只覆盖本控件显示，不修改 BattleSession；ClearActionPreview 后恢复最近一次 Snapshot。"))
	void SetActionPreview(const FPlayerSnapshot& ProjectedPlayer);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Player Status", meta = (ToolTip = "清除战斗行动预览，恢复最近一次 Snapshot 显示。"))
	void ClearActionPreview();

	/** 只播放一次敌方行动命中反馈；不修改 BattleSession 或 Snapshot 真相。 */
	void PlayEnemyActionImpactFeedback(
		const FPlayerSnapshot& PreviousPlayer,
		const FPlayerSnapshot& CurrentPlayer);

	/** 控制玩家状态图标是否可被鼠标检视；拖卡、TargetSelect 和 Action Preview 期间关闭。 */
	void SetStatusInspectionEnabled(bool bEnabled);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeRefreshFromSnapshot(const struct FBattleSnapshot& Snap) override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 仅供 C++ fallback 使用；正式 V2 WBP 使用 VitalsTrackImage。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomProgressBar> HpBar;

	UPROPERTY(meta = (BindWidgetOptional, ToolTip = "玩家生命、预测区段、延迟伤害和护盾外框使用的 V2 材质图像。正式 WBP 必须提供；缺失时回退到 HpBar。"))
	TObjectPtr<UImage> VitalsTrackImage;

	UPROPERTY(meta = (BindWidgetOptional, ToolTip = "玩家生命数值文本。显示为 当前 HP / 最大 HP；Action Preview 时显示预测值。"))
	TObjectPtr<UTextBlock> HpValueText;

	UPROPERTY(meta = (BindWidgetOptional, ToolTip = "护盾数字的固定布局根节点。护盾为零时使用 Hidden 保留布局空间，避免生命条宽度跳动。"))
	TObjectPtr<UWidget> ShieldValueRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShieldText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomBattleStatusIconListWidget> StatusList;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Player Status|Authoring", meta = (ToolTip = "护盾为 0 时是否隐藏 ShieldText。只影响玩家状态条显示，不改变 BattleSession 中的护盾数值。"))
	bool bHideShieldWhenZero = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Player Status|Vitals", meta = (ToolTip = "生命低于该比例时切换为低生命颜色。单位：0-1；默认 0.25，推荐 0.15–0.35；只影响表现，不改变规则。"))
	float LowHealthThreshold = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Player Status|Vitals", meta = (ToolTip = "生命下降后，延迟伤害区保持不动的时间。单位：秒；默认 0.08，推荐 0.04–0.14；不影响权威数值更新时间。"))
	float DamageTrailHoldSeconds = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Player Status|Vitals", meta = (ToolTip = "延迟伤害区从旧生命收束到新生命的时间。单位：秒；默认 0.32，推荐 0.20–0.50；不影响规则。"))
	float DamageTrailRecoverySeconds = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Player Status|Vitals", meta = (ToolTip = "护盾命中和破裂反馈总时长。单位：秒；默认 0.16，推荐 0.10–0.26。"))
	float ImpactFeedbackDurationSeconds = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Player Status|Vitals", meta = (ToolTip = "护盾反馈压缩峰值倍率。默认 0.94，推荐 0.88–0.98；只影响 ShieldValueRoot，不影响布局。"))
	float ShieldCompressionScale = 0.94f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Player Status|Vitals", meta = (ToolTip = "护盾反馈回弹峰值倍率。默认 1.08，推荐 1.02–1.16；只影响 ShieldValueRoot，不影响布局。"))
	float ShieldReboundScale = 1.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Player Status|Impact",
		meta = (ToolTip = "玩家 HP 因敌方行动下降时播放的可选 2D 音效。为空时保持静音，不影响状态条脉冲。"))
	TObjectPtr<USoundBase> DamageImpactSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Player Status|Impact",
		meta = (ToolTip = "敌方行动只削减玩家护盾时播放的可选 2D 音效。为空时保持静音；若同次命中也扣 HP，则优先使用 DamageImpactSound。"))
	TObjectPtr<USoundBase> ShieldImpactSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Player Status|Impact",
		meta = (ToolTip = "敌方行动命中音效音量倍率。单位：线性倍率；推荐 0.6–1.2，不影响布局或规则。"))
	float ImpactSoundVolume = 1.0f;

private:
	struct FVitalsPlaybackDeleter
	{
		void operator()(FWacomBattlePlayerVitalsPlayback* Playback) const;
	};

	void RefreshDisplay();
	void RefreshFromPlayerSnapshot(const FPlayerSnapshot& PlayerView);
	void RefreshStatusInspectionInteraction();
	UWacomBattleStatusIconListWidget* ResolveStatusListWidget();
	bool EnsureVitalsMaterial();
	void ApplyVitalsPresentation();
	void RestoreVitalsMaterial();
	void CaptureShieldAuthoredTransform();
	void RestoreShieldAuthoredTransform();
	void BindRuntimeSettings();
	void UnbindRuntimeSettings();
	void HandleRuntimeSettingsChanged(
		const FWacomLocalSettingsSnapshot& Snapshot,
		EWacomRuntimeSettingsChangeReason Reason);
	FWacomPlayerStatusBarAutomationTestView BuildAutomationTestView() const;

	UPROPERTY(Transient)
	FPlayerSnapshot BasePlayerView;

	UPROPERTY(Transient)
	FPlayerSnapshot ActionPreviewPlayerView;

	bool bHasBasePlayerView = false;
	bool bHasActionPreview = false;
	bool bStatusInspectionAllowedByRuntime = true;
	bool bRuntimeSimplifiedMotion = false;
	float RuntimeFlashIntensity = 1.0f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> VitalsMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> VitalsSourceMaterial;

	TUniquePtr<FWacomBattlePlayerVitalsPlayback, FVitalsPlaybackDeleter> VitalsPlayback;
	TWeakObjectPtr<UWacomSettingsSubsystem> BoundSettingsSubsystem;
	FDelegateHandle RuntimeSettingsChangedHandle;
	FWidgetTransform ShieldAuthoredTransform;
	FVector2D ShieldAuthoredPivot = FVector2D(0.5f, 0.5f);
	bool bCapturedShieldAuthoredTransform = false;

#if WITH_AUTOMATION_TESTS
	friend struct FWacomPlayerStatusBarTestAccess;
#endif
};
