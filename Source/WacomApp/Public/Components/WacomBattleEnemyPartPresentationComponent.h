// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Events/BattleEvent.h"
#include "UI/Battle/WacomBattleEnemyPartDragPredictionTypes.h"
#include "UI/Battle/WacomBattleEnemyPanelViewData.h"
#include "UI/Battle/WacomBattleEnemyPartPredictionTypes.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomBattleEnemyPartPresentationComponent.generated.h"

class UPrimitiveComponent;
class USceneComponent;
class UWidgetComponent;
class UWacomBattleEnemyPartImpactStyle;
class UWacomBattleEnemyPartTargetPreviewStyle;
class UWacomSettingsSubsystem;
class FWacomBattleEnemyPartCuePlayback;
class FWacomBattleEnemyPartImpactFeedbackController;
class FWacomBattleEnemyPartTargetPreviewPlayback;
class FWacomBattleEnemyPartTargetPreviewFeedbackController;
struct FEnemyPartSnapshot;
struct FWacomLocalSettingsSnapshot;
struct FWacomInteractionTargetHandle;
enum class EWacomRuntimeSettingsChangeReason : uint8;
#if WITH_AUTOMATION_TESTS
struct FWacomBattleEnemyPartPresentationTestAccess;
#endif

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyPartPresentationDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	bool bHasRuntimePartFacts = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FGuid RuntimePartInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	int32 CurrentInitiative = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	bool bRuntimePartDestroyed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FName CurrentIntentId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FName LastCueKind = TEXT("None");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	EBattleEventType LastCueType = EBattleEventType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	int32 LastCueAmount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	int32 CuePlayCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FName ActiveCueKind = TEXT("None");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	bool bCuePlaybackActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	float CuePlaybackProgress = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	float CuePlaybackDurationSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	bool bImpactAnchorReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FName ImpactAnchorName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FVector ImpactAnchorWorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FName ResolvedImpactStyleName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	bool bImpactFeedbackEnabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	bool bImpactNiagaraReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	bool bImpactEffectActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	float LastImpactIntensity = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	float LastImpactTargetDiameterCentimeters = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FName LastImpactEffectKind = TEXT("None");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	int32 LastImpactSeed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	int32 ImpactEffectPlayCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	int32 ImpactSoundRequestCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	bool bImpactReducedMotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	float ImpactDecorativeIntensity = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FName ResolvedTargetPreviewStyleName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	bool bTargetPreviewFeedbackEnabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	bool bTargetPreviewNiagaraReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	bool bTargetPreviewEffectActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	float TargetPreviewAmount = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	float TargetPreviewPulse = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FVector2D TargetPreviewSizeCentimeters = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	int32 TargetPreviewActivationCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	EWacomFirstPersonCardDragTargetFeedbackState DragPreviewState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	bool bDragPreviewActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FWacomBattleEnemyPartDragPredictionDebugInput LastDragPredictionDebugInput;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	bool bHoverActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FWacomBattleEnemyPartDragPredictionDebugInput LastHoverPredictionInput;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FName HoverReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FName HoverStableId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FGuid HoverWorldTargetId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FVector2D HoverScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FWacomBattleEnemyPartPredictionView PredictionView;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	bool bActionPreviewPartActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FWacomBattleEnemyPartEntryViewData ActionPreviewPartView;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FName PredictionWidgetName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FVector PredictionBadgeRelativeLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	FVector2D PredictionBadgeDrawSize = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	float PredictionBadgeScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	float PredictionBadgeZOffsetWhenVisible = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	bool bPredictionBadgeOffsetActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug")
	int32 BadgeLayoutStaggerIndex = INDEX_NONE;
};

/**
 * 场景敌人部位表现组件。
 *
 * 只负责语义 cue 生命周期、hover/drag preview 缩放和预测 widget。
 * 不负责 Battle 目标身份绑定，也不向 BattleSession 提交命令。
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent, ToolTip = "负责场景敌人部位的表现层：语义 Cue 生命周期、hover/拖卡预览和预测 Widget。"))
class WACOMAPP_API UWacomBattleEnemyPartPresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomBattleEnemyPartPresentationComponent();
	virtual ~UWacomBattleEnemyPartPresentationComponent() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation", meta = (ToolTip = "用于播放确认、伤害、破坏和可选目标提示的 Primitive；为空时自动使用 Owner 上第一个 PrimitiveComponent。"))
	TObjectPtr<UPrimitiveComponent> VisualTargetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation", meta = (ToolTip = "用于播放确认、伤害、破坏和可选目标提示的场景组件。优先于 VisualTargetComponent；适合把多个 2D 视觉层作为一组缩放。"))
	TObjectPtr<USceneComponent> FeedbackTargetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation", meta = (ToolTip = "TargetSelect 中可选部位的轻量提示缩放倍率。", ClampMin = "1.0", ClampMax = "1.5", UIMin = "1.0", UIMax = "1.2"))
	float TargetableAffordanceScale = 1.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation", meta = (ToolTip = "鼠标悬停该部位时的轻量探测缩放倍率。只表示当前 hover 目标，不影响战斗规则。", ClampMin = "1.0", ClampMax = "1.5", UIMin = "1.0", UIMax = "1.15"))
	float HoverProbeScale = 1.04f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation", meta = (ToolTip = "是否允许更新场景部位预测 Widget。只影响 UI，不影响战斗规则。"))
	bool bEnablePredictionDisplay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation", meta = (ToolTip = "预测 Widget 的整体渲染缩放。只影响 UI。", ClampMin = "0.25", ClampMax = "2.0", UIMin = "0.6", UIMax = "1.2"))
	float PredictionBadgeScale = 0.92f;




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation", meta = (ToolTip = "预测 Widget 显示时额外向上错开的距离。只影响 UI。", ClampMin = "0.0", ClampMax = "300.0", UIMin = "0.0", UIMax = "100.0"))
	float PredictionBadgeZOffsetWhenVisible = 42.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation", meta = (ToolTip = "默认 cue 保持时间，单位秒。", ClampMin = "0.01", ClampMax = "2.0", UIMin = "0.05", UIMax = "0.5"))
	float CueHoldSeconds = 0.14f;

	void CacheRuntimePartFacts(FName InPartId, const FEnemyPartSnapshot& Part);
	void ClearRuntimePartFacts();
	void PlayBattlePresentationCue(const FWacomBattlePresentationTargetCue& Cue);
	void ForceCompleteBattlePresentationCue();
	void ResetBattlePresentationFeedback();
	void SetPresentationTargets(
		USceneComponent* InFeedbackTarget,
		USceneComponent* InImpactAnchor,
		UPrimitiveComponent* InImpactExtentSource);
	void SetImpactFeedbackStyle(
		UWacomBattleEnemyPartImpactStyle* InImpactStyle,
		bool bInImpactFeedbackEnabled);
	void SetTargetPreviewFeedbackStyle(
		UWacomBattleEnemyPartTargetPreviewStyle* InTargetPreviewStyle,
		bool bInTargetPreviewFeedbackEnabled);
	void SetTargetableAffordance(bool bInTargetable);
	void SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState PreviewState,
		const FWacomBattleEnemyPartDragPredictionDebugInput& PredictionDebugInput =
			FWacomBattleEnemyPartDragPredictionDebugInput());
	void ClearDragTargetPreviewState();
	void SetHoverProbeState(
		const FWacomInteractionTargetHandle& TargetHandle,
		FName Reason,
		const FWacomBattleEnemyPartDragPredictionDebugInput& PredictionInput =
			FWacomBattleEnemyPartDragPredictionDebugInput());
	void ClearHoverProbeState(FName Reason = NAME_None);
	void SetActionPreviewPartView(const FWacomBattleEnemyPartEntryViewData& InPreviewView);
	void ClearActionPreviewPartView();
	void SetPredictionWidgetComponent(UWidgetComponent* InPredictionWidgetComponent);
	void SetBadgeLayoutDebugState(int32 InStaggerIndex);
	void ClearPredictionDisplay(FName Reason = NAME_None);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Presentation|Debug", meta = (ToolTip = "获取当前场景敌人部位表现层调试快照；不影响战斗规则。"))
	FWacomBattleEnemyPartPresentationDebugView GetBattleEnemyPartPresentationDebugView() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
#if WITH_AUTOMATION_TESTS
	friend struct FWacomBattleEnemyPartPresentationTestAccess;
#endif

	struct FCuePlaybackDeleter
	{
		void operator()(FWacomBattleEnemyPartCuePlayback* Playback) const;
	};

	struct FImpactFeedbackControllerDeleter
	{
		void operator()(FWacomBattleEnemyPartImpactFeedbackController* Controller) const;
	};

	struct FTargetPreviewPlaybackDeleter
	{
		void operator()(FWacomBattleEnemyPartTargetPreviewPlayback* Playback) const;
	};

	struct FTargetPreviewFeedbackControllerDeleter
	{
		void operator()(FWacomBattleEnemyPartTargetPreviewFeedbackController* Controller) const;
	};

	USceneComponent* ResolveFeedbackTargetComponent() const;
	USceneComponent* ResolveImpactAnchorComponent() const;
	UPrimitiveComponent* ResolveImpactExtentSourceComponent() const;
	UPrimitiveComponent* ResolveVisualTargetComponent() const;
	void RefreshPredictionDisplay();
	void ApplyPredictionViewToWidget();
	FWacomBattleEnemyPartPredictionView BuildPredictionView(
		const FWacomBattleEnemyPartDragPredictionDebugInput& PredictionInput) const;
	FWacomBattleEnemyPartPredictionView BuildActionPreviewPredictionView() const;
	void ApplyPersistentScaleState();
	void ApplyPersistentScaleMultiplier(float ScaleMultiplier);
	void ResetCuePlayback();
	void ResetTargetPreviewPlayback(bool bDestroyComponent);
	void RefreshComponentTickEnabled();
	void BindRuntimeSettings();
	void UnbindRuntimeSettings();
	void HandleRuntimeSettingsChanged(
		const FWacomLocalSettingsSnapshot& Snapshot,
		EWacomRuntimeSettingsChangeReason Reason);
	void RestoreBaseScaleIfNeeded();

	UPROPERTY(Transient)
	FGuid RuntimePartInstanceId;

	UPROPERTY(Transient)
	FVector CachedBaseScale = FVector::OneVector;

	UPROPERTY(Transient)
	TWeakObjectPtr<USceneComponent> CachedFeedbackTarget;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> ImpactAnchorComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> ImpactExtentSourceComponent;

	UPROPERTY(Transient)
	TObjectPtr<UWacomBattleEnemyPartImpactStyle> ImpactFeedbackStyle;

	UPROPERTY(Transient)
	TObjectPtr<UWacomBattleEnemyPartTargetPreviewStyle> TargetPreviewFeedbackStyle;

	UPROPERTY(Transient)
	TWeakObjectPtr<UWidgetComponent> PredictionWidgetComponent;

	UPROPERTY(Transient)

	FName PartId = NAME_None;
	bool bHasRuntimePartFacts = false;
	bool bRuntimePartDestroyed = false;
	bool bTargetable = false;
	bool bHasCachedBaseScale = false;
	bool bDragPreviewActive = false;
	bool bHoverProbeActive = false;
	bool bActionPreviewPartActive = false;
	EWacomFirstPersonCardDragTargetFeedbackState DragPreviewState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	FWacomBattleEnemyPartDragPredictionDebugInput LastDragPredictionDebugInput;
	FWacomBattleEnemyPartDragPredictionDebugInput LastHoverPredictionInput;
	FWacomBattleEnemyPartEntryViewData ActionPreviewPartView;
	FWacomBattleEnemyPartPredictionView CurrentPredictionView;
	FVector PredictionBadgeBaseRelativeLocation = FVector::ZeroVector;
	bool bHasPredictionBadgeBaseRelativeLocation = false;
	FName HoverReason = NAME_None;
	FName HoverStableId = NAME_None;
	FGuid HoverWorldTargetId;
	FVector2D HoverScreenPosition = FVector2D::ZeroVector;
	int32 BadgeLayoutStaggerIndex = INDEX_NONE;
	int32 CurrentInitiative = 0;
	FName CurrentIntentId = NAME_None;
	FName LastCueKind = TEXT("None");
	EBattleEventType LastCueType = EBattleEventType::None;
	int32 LastCueAmount = 0;
	int32 CuePlayCount = 0;
	bool bImpactFeedbackEnabled = false;
	bool bTargetPreviewFeedbackEnabled = false;
	float RuntimeDecorativeFlashIntensityScale = 1.0f;
	bool bRuntimeSimplifiedMotion = false;
	FDelegateHandle RuntimeSettingsChangedHandle;
	TUniquePtr<FWacomBattleEnemyPartCuePlayback, FCuePlaybackDeleter> CuePlayback;
	TUniquePtr<FWacomBattleEnemyPartImpactFeedbackController, FImpactFeedbackControllerDeleter>
		ImpactFeedbackController;
	TUniquePtr<FWacomBattleEnemyPartTargetPreviewPlayback, FTargetPreviewPlaybackDeleter>
		TargetPreviewPlayback;
	TUniquePtr<FWacomBattleEnemyPartTargetPreviewFeedbackController, FTargetPreviewFeedbackControllerDeleter>
		TargetPreviewFeedbackController;
};
