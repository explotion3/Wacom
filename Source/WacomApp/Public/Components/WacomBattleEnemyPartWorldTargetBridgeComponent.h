// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Events/BattleEvent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "WacomBattleEnemyPartWorldTargetBridgeComponent.generated.h"

class UBattleHUD;
class UPrimitiveComponent;
class UWacomInteractionTargetComponent;
struct FBattleSnapshot;
struct FBattleTargetSelectionView;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyPartDragPredictionDebugInput
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	bool bHasSourceCard = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	FGuid SourceCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	int32 SourceCardRuntimeCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	bool bSourceCardSwift = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	bool bPreviewCanSubmit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	FName PreviewRejectReason = NAME_None;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyPartWorldTargetDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	FName PartId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	FGuid PartInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	bool bBoundToSnapshot = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	bool bRegisteredWithBattleHUD = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	bool bHasRuntimePartFacts = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	FGuid RuntimePartInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	int32 CurrentInitiative = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	bool bRuntimePartDestroyed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	FName CurrentIntentId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	FText CurrentIntentDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	int32 CurrentIntentInitiative = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	int32 CurrentIntentResistanceValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	bool bTargetable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	FName TargetDisabledReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	FName LastBindResult = TEXT("NotAttempted");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	FName LastCueKind = TEXT("None");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	EBattleEventType LastCueType = EBattleEventType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	int32 LastCueAmount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	int32 CuePlayCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	EWacomFirstPersonCardDragTargetFeedbackState DragPreviewState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	bool bDragPreviewActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target")
	FWacomBattleEnemyPartDragPredictionDebugInput LastDragPredictionDebugInput;
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

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|World Target")
	FWacomBattleEnemyPartWorldTargetDebugView GetBattleWorldTargetDebugView() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|World Target|Debug")
	FString GetBattleWorldTargetDebugSummary() const;

	UFUNCTION(CallInEditor, Category = "Wacom|Battle|World Target|Debug")
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
	void BeginScaleFeedback(float ScaleMultiplier, float HoldSeconds);
	void ClearScaleFeedback();
	void RestoreBaseScaleIfNeeded();
	void StopFeedbackTimer();
	void UpdateInteractionTargetComponent();

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
	EWacomFirstPersonCardDragTargetFeedbackState DragPreviewState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	FWacomBattleEnemyPartDragPredictionDebugInput LastDragPredictionDebugInput;
	int32 CurrentInitiative = 0;
	FName CurrentIntentId = NAME_None;
	FText CurrentIntentDisplayName;
	int32 CurrentIntentInitiative = 0;
	int32 CurrentIntentResistanceValue = 0;
	FName TargetDisabledReason = NAME_None;
	FName LastBindResult = TEXT("NotAttempted");
	FName LastCueKind = TEXT("None");
	EBattleEventType LastCueType = EBattleEventType::None;
	int32 LastCueAmount = 0;
	int32 CuePlayCount = 0;
	FTimerHandle FeedbackTimerHandle;
};
