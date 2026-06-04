// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Events/BattleEvent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/Battle/WacomBattleEnemyPartPredictionTypes.h"
#include "UI/Battle/WacomBattleEnemyPartStatusBadgeTypes.h"
#include "WacomBattleEnemyPartWorldTargetBridgeComponent.generated.h"

class UBattleHUD;
class UPrimitiveComponent;
class UWacomInteractionTargetComponent;
class UWidgetComponent;
struct FBattleSnapshot;
struct FBattleTargetSelectionView;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyPartDragPredictionDebugInput
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "当前拖卡预测是否带有源卡，只用于场景部位目标诊断。"))
	bool bHasSourceCard = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "当前拖卡预测使用的源卡实例 ID。"))
	FGuid SourceCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "源卡当前运行时费用，用于排查先机预测。"))
	int32 SourceCardRuntimeCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "源卡是否为迅捷卡，用于排查先机预测。"))
	bool bSourceCardSwift = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "当前拖卡 preview 是否允许提交。"))
	bool bPreviewCanSubmit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "当前拖卡 preview 的拒绝原因。"))
	FName PreviewRejectReason = NAME_None;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyPartWorldTargetDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "敌方部位稳定 PartId，对应制作数据，只用于 PIE / 蓝图诊断。"))
	FName PartId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "当前战斗运行时部位实例 ID。"))
	FGuid PartInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "Bridge 当前是否绑定到战斗 Snapshot 中的部位。"))
	bool bBoundToSnapshot = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "Bridge 当前是否注册到 BattleHUD 的 scene enemy target registry。"))
	bool bRegisteredWithBattleHUD = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	bool bHasRuntimePartFacts = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FGuid RuntimePartInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FText RuntimePartDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	int32 CurrentHp = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	int32 MaxHp = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	int32 Shield = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	int32 CurrentInitiative = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	bool bRuntimePartDestroyed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FName CurrentIntentId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FText CurrentIntentDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	int32 CurrentIntentInitiative = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	int32 CurrentIntentResistanceValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	bool bTargetable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FName TargetDisabledReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FName LastBindResult = TEXT("NotAttempted");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FName LastCueKind = TEXT("None");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	EBattleEventType LastCueType = EBattleEventType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	int32 LastCueAmount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	int32 CuePlayCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	EWacomFirstPersonCardDragTargetFeedbackState DragPreviewState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	bool bDragPreviewActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FWacomBattleEnemyPartDragPredictionDebugInput LastDragPredictionDebugInput;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	bool bHoverActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FName HoverReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FName HoverStableId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FGuid HoverWorldTargetId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FVector2D HoverScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FWacomBattleEnemyPartPredictionView PredictionView;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FWacomBattleEnemyPartStatusBadgeView StatusBadgeView;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FName PredictionWidgetName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FName StatusBadgeWidgetName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FVector PredictionBadgeRelativeLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FVector StatusBadgeRelativeLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FVector2D PredictionBadgeDrawSize = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FVector2D StatusBadgeDrawSize = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	float PredictionBadgeScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	float StatusBadgeScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	float StatusBadgeOpacity = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	float DestroyedStatusBadgeOpacity = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	float CurrentStatusBadgeAppliedOpacity = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	float PredictionBadgeZOffsetWhenVisible = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	bool bPredictionBadgeOffsetActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	int32 BadgeLayoutStaggerIndex = INDEX_NONE;
};

/**
 * Battle enemy part 到通用 World interaction target 的桥接组件。
 *
 * 本组件只负责 Battle 专用的 PartId -> PartInstanceId 绑定和目标表现反馈；
 * 通用命中身份仍由同 Actor 上的 UWacomInteractionTargetComponent 提供。
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent, ToolTip = "把场景 Actor 绑定为当前战斗敌方部位 World Target，并接收目标确认、伤害、破坏表现 cue。"))
class WACOMAPP_API UWacomBattleEnemyPartWorldTargetBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomBattleEnemyPartWorldTargetBridgeComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "稳定敌方部位 ID，对应 UEnemyPartDefinition::PartId。进入战斗后 Bridge 会把它解析成当前运行时 PartInstanceId。"))
	FName PartId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "用于播放确认、伤害、破坏和可选目标提示的 Primitive；为空时自动使用 Owner 上第一个 PrimitiveComponent。"))
	TObjectPtr<UPrimitiveComponent> VisualTargetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "是否在同步时自动把同 Actor 上的 WacomInteractionTargetComponent 标记为 Battle EnemyPart target。"))
	bool bAutoConfigureInteractionTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "目标确认 cue 的缩放倍率。", ClampMin = "1.0", ClampMax = "2.0", UIMin = "1.0", UIMax = "1.3"))
	float TargetConfirmPulseScale = 1.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "伤害 cue 的缩放倍率。", ClampMin = "1.0", ClampMax = "2.0", UIMin = "1.0", UIMax = "1.4"))
	float DamagePulseScale = 1.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "破坏 cue 的缩放倍率。", ClampMin = "1.0", ClampMax = "2.5", UIMin = "1.0", UIMax = "1.6"))
	float DestroyedPulseScale = 1.32f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "TargetSelect 中可选部位的轻量提示缩放倍率。", ClampMin = "1.0", ClampMax = "1.5", UIMin = "1.0", UIMax = "1.2"))
	float TargetableAffordanceScale = 1.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "第一人称卡牌拖拽指向该部位时的轻量预览缩放倍率。", ClampMin = "1.0", ClampMax = "1.5", UIMin = "1.0", UIMax = "1.2"))
	float DragTargetPreviewScale = 1.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "鼠标悬停该部位时的轻量探测缩放倍率。只表示当前 hover 目标，不影响战斗规则。", ClampMin = "1.0", ClampMax = "1.5", UIMin = "1.0", UIMax = "1.15"))
	float HoverProbeScale = 1.04f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "是否允许 Bridge 更新场景部位预测 Widget。只影响 UI，不影响战斗规则。"))
	bool bEnablePredictionDisplay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "是否允许 Bridge 更新场景部位常驻状态 Badge。只影响 UI，不影响战斗规则。"))
	bool bEnableStatusBadgeDisplay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "预测 Widget 的整体渲染缩放。由 PartActor facade 同步；只影响 UI。", ClampMin = "0.25", ClampMax = "2.0", UIMin = "0.6", UIMax = "1.2"))
	float PredictionBadgeScale = 0.92f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "状态 Badge 的整体渲染缩放。由 PartActor facade 同步；只影响 UI。", ClampMin = "0.25", ClampMax = "2.0", UIMin = "0.6", UIMax = "1.2"))
	float StatusBadgeScale = 0.86f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "普通状态 Badge 透明度。由 PartActor facade 同步；只影响 UI。", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.35", UIMax = "1.0"))
	float StatusBadgeOpacity = 0.92f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "破坏状态 Badge 透明度。由 PartActor facade 同步；破坏态仍显示但弱化。", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.25", UIMax = "1.0"))
	float DestroyedStatusBadgeOpacity = 0.58f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "预测 Widget 显示时额外向上错开的距离。由 PartActor facade 同步；只影响 UI。", ClampMin = "0.0", ClampMax = "300.0", UIMin = "0.0", UIMax = "100.0"))
	float PredictionBadgeZOffsetWhenVisible = 42.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "默认 cue 保持时间，单位秒。", ClampMin = "0.01", ClampMax = "2.0", UIMin = "0.05", UIMax = "0.5"))
	float CueHoldSeconds = 0.14f;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|World Target")
	void SetPartId(FName InPartId);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|World Target")
	FGuid GetPartInstanceId() const { return PartInstanceId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|World Target")
	bool IsBoundToBattlePart() const { return bBoundToSnapshot && PartInstanceId.IsValid(); }

	bool SyncFromBattleHUD(UBattleHUD& HUD, const FBattleSnapshot& Snapshot, const FBattleTargetSelectionView& TargetSelectionView);
	void ClearBattleBinding();
	void PlayBattlePresentationCue(const FWacomBattlePresentationTargetCue& Cue);
	void SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState PreviewState,
		const FWacomBattleEnemyPartDragPredictionDebugInput& PredictionDebugInput =
			FWacomBattleEnemyPartDragPredictionDebugInput());
	void ClearDragTargetPreviewState();
	void SetHoverProbeState(
		const struct FWacomInteractionTargetHandle& TargetHandle,
		FName Reason,
		const FWacomBattleEnemyPartDragPredictionDebugInput& PredictionInput =
			FWacomBattleEnemyPartDragPredictionDebugInput());
	void ClearHoverProbeState(FName Reason = NAME_None);
	void SetPredictionWidgetComponent(UWidgetComponent* InPredictionWidgetComponent);
	void SetStatusBadgeWidgetComponent(UWidgetComponent* InStatusBadgeWidgetComponent);
	void SetBadgeLayoutDebugState(int32 InStaggerIndex);
	void ClearPredictionDisplay(FName Reason = NAME_None);
	void ClearStatusBadgeDisplay(FName Reason = NAME_None);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "获取当前 Battle World Target 的只读调试快照；只用于 PIE / 蓝图排查，不影响战斗规则。"))
	FWacomBattleEnemyPartWorldTargetDebugView GetBattleWorldTargetDebugView() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "获取当前 Battle World Target 的单行调试摘要；用于排查部位绑定、hover、预测和 cue 状态。"))
	FString GetBattleWorldTargetDebugSummary() const;

	UFUNCTION(CallInEditor, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "在编辑器或 PIE 中把当前 Battle World Target 调试摘要写入 Output Log；不改变战斗或 UI 状态。"))
	void LogBattleWorldTargetDebugSummary() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UWacomInteractionTargetComponent* ResolveInteractionTargetComponent() const;
	UPrimitiveComponent* ResolveVisualTargetComponent() const;
	void CacheRuntimePartFacts(const struct FEnemyPartSnapshot& Part);
	void ClearRuntimePartFacts();
	void ClearBattleBindingInternal(bool bClearRuntimeFacts);
	void RegisterWithBattleHUD(UBattleHUD& HUD);
	void UnregisterFromBattleHUD();
	void ApplyTargetableAffordance(bool bInTargetable);
	void RefreshPredictionDisplay();
	void ApplyPredictionViewToWidget();
	void RefreshStatusBadgeDisplay();
	void ApplyStatusBadgeViewToWidget();
	FWacomBattleEnemyPartPredictionView BuildPredictionView(
		const FWacomBattleEnemyPartDragPredictionDebugInput& PredictionInput) const;
	FWacomBattleEnemyPartStatusBadgeView BuildStatusBadgeView() const;
	FText BuildStatusBadgeStatusText() const;
	void ApplyPersistentScaleState();
	void BeginScaleFeedback(float ScaleMultiplier, float HoldSeconds);
	void ClearScaleFeedback();
	void RestoreBaseScaleIfNeeded();
	void StopFeedbackTimer();
	void UpdateInteractionTargetComponent();

#if WITH_AUTOMATION_TESTS
public:
	void ApplyTargetableAffordanceForTest(bool bInTargetable) { ApplyTargetableAffordance(bInTargetable); }
private:
#endif

	UPROPERTY(Transient)
	TWeakObjectPtr<UBattleHUD> RegisteredBattleHUD;

	UPROPERTY(Transient)
	FGuid PartInstanceId;

	UPROPERTY(Transient)
	FGuid RuntimePartInstanceId;

	UPROPERTY(Transient)
	FVector CachedBaseScale = FVector::OneVector;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPrimitiveComponent> CachedVisualTarget;

	bool bBoundToSnapshot = false;
	bool bRegisteredWithBattleHUD = false;
	bool bHasRuntimePartFacts = false;
	bool bRuntimePartDestroyed = false;
	bool bTargetable = false;
	bool bHasCachedBaseScale = false;
	bool bDragPreviewActive = false;
	bool bHoverProbeActive = false;
	EWacomFirstPersonCardDragTargetFeedbackState DragPreviewState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	FWacomBattleEnemyPartDragPredictionDebugInput LastDragPredictionDebugInput;
	FWacomBattleEnemyPartDragPredictionDebugInput LastHoverPredictionInput;
	FWacomBattleEnemyPartPredictionView CurrentPredictionView;
	FWacomBattleEnemyPartStatusBadgeView CurrentStatusBadgeView;
	TWeakObjectPtr<UWidgetComponent> PredictionWidgetComponent;
	TWeakObjectPtr<UWidgetComponent> StatusBadgeWidgetComponent;
	FVector PredictionBadgeBaseRelativeLocation = FVector::ZeroVector;
	bool bHasPredictionBadgeBaseRelativeLocation = false;
	FName HoverReason = NAME_None;
	FName HoverStableId = NAME_None;
	FGuid HoverWorldTargetId;
	FVector2D HoverScreenPosition = FVector2D::ZeroVector;
	int32 BadgeLayoutStaggerIndex = INDEX_NONE;
	int32 CurrentInitiative = 0;
	FName CurrentIntentId = NAME_None;
	FText RuntimePartDisplayName;
	int32 CurrentHp = 0;
	int32 MaxHp = 0;
	int32 Shield = 0;
	FText CurrentIntentDisplayName;
	int32 CurrentIntentInitiative = 0;
	int32 CurrentIntentResistanceValue = 0;
	FGameplayTagContainer RuntimeStatuses;
	TMap<FGameplayTag, int32> RuntimeStatusStacks;
	FName TargetDisabledReason = NAME_None;
	FName LastBindResult = TEXT("NotAttempted");
	FName LastCueKind = TEXT("None");
	EBattleEventType LastCueType = EBattleEventType::None;
	int32 LastCueAmount = 0;
	int32 CuePlayCount = 0;
	FTimerHandle FeedbackTimerHandle;
};
