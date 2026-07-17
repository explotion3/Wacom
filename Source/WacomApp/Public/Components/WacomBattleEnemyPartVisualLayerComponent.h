// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UI/Battle/WacomBattleEnemyPartVisualLayerTypes.h"
#include "WacomBattleEnemyPartVisualLayerComponent.generated.h"

class UPaperFlipbook;
class UPaperFlipbookComponent;
class UPaperSpriteComponent;
class USceneComponent;

struct FWacomBattleEnemyPartRuntimeVisualLayer
{
	int32 LayerIndex = INDEX_NONE;
	FName LayerId = NAME_None;
	EWacomBattleEnemyPartVisualLayerMode LayerMode =
		EWacomBattleEnemyPartVisualLayerMode::StaticSprite;
	TWeakObjectPtr<UPaperSpriteComponent> SpriteComponent;
	TWeakObjectPtr<UPaperFlipbookComponent> FlipbookComponent;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyPartVisualLayerDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	bool bUsingVisualLayers = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 VisualLayerCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 GeneratedVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 RegisteredVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 VisibleVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 GeneratedStaticVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 GeneratedFlipbookVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 RegisteredStaticVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 RegisteredFlipbookVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 VisibleStaticVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 VisibleFlipbookVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 MissingVisualLayerAssetCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 MissingVisualLayerSpriteCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 MissingVisualLayerFlipbookCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	TArray<FName> VisualLayerIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	TArray<FName> DuplicateVisualLayerIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	TArray<FName> VisualLayerAssetNames;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 DestroyedVisualResourceCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	bool bRuntimeDestroyedStateApplied = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 RuntimeDestroyedVisualLayerCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 RuntimeDestroyedVisualApplyCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	FName CurrentRuntimeActionLayerId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	FName CurrentRuntimeActionClipName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	FName CurrentRuntimeActionIntentId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	bool bRuntimeActionPlaybackActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 RuntimeActionPlaybackCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 RuntimeActionWatchdogCompletionCount = 0;
};

/**
 * 生成并维护敌人部位的 PaperSprite / PaperFlipbook 视觉层。
 *
 * 只负责表现组件生命周期与统计；不拥有命中体、Battle 身份或 HUD 命令。
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent, ToolTip = "生成并维护场景敌人部位的 PaperSprite / PaperFlipbook 视觉层。"))
class WACOMAPP_API UWacomBattleEnemyPartVisualLayerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomBattleEnemyPartVisualLayerComponent();

	void RefreshVisualLayers(
		const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers,
		USceneComponent* AttachRoot);
	void ClearGeneratedVisualLayers();
	/** 在精确 Flipbook Layer 上原地播放一次部位行动；返回 false 时同步完成 Completion。 */
	bool PlayRuntimeActionOneShot(
		const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers,
		FName TargetVisualLayerId,
		UPaperFlipbook* Flipbook,
		float PlayRate,
		FName IntentId,
		TFunction<void()>&& Completion);
	/** 清理当前行动 barrier；按需恢复 authored layer，Destroyed 终态应用时不会短暂恢复。 */
	void CancelRuntimeActionPlayback(bool bRestoreAuthoredLayer = true);
	int32 ApplyRuntimeDestroyedState(
		const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers);
	void RestoreRuntimeAuthoredState(
		const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers);
	FWacomBattleEnemyPartVisualLayerDebugView BuildVisualLayerDebugView(
		const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers) const;
	bool IsRuntimeDestroyedStateApplied() const { return bRuntimeDestroyedStateApplied; }
	bool IsRuntimeActionPlaybackActive() const { return bRuntimeActionPlaybackActive; }
	FName GetCurrentRuntimeActionLayerId() const { return CurrentRuntimeActionLayerId; }
	FName GetCurrentRuntimeActionClipName() const { return CurrentRuntimeActionClipName; }
	FName GetCurrentRuntimeActionIntentId() const { return CurrentRuntimeActionIntentId; }
	int32 GetRuntimeActionPlaybackCount() const { return RuntimeActionPlaybackCount; }
	int32 GetRuntimeActionWatchdogCompletionCount() const
	{
		return RuntimeActionWatchdogCompletionCount;
	}

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleRuntimeActionFlipbookFinished();

	void HandleRuntimeActionWatchdogExpired(uint64 ExpectedPlaybackSerial);
	void CompleteRuntimeActionPlayback(
		uint64 ExpectedPlaybackSerial,
		bool bWatchdogCompletion,
		bool bRestoreAuthoredLayer);
	void StopRuntimeActionWatchdog();
	void RestoreActiveRuntimeActionLayer();
	void UnbindRuntimeActionFinishedDelegate();
	void CompletePendingRuntimeActionCallback();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPaperSpriteComponent>> GeneratedVisualLayerComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPaperFlipbookComponent>> GeneratedFlipbookVisualLayerComponents;

	TArray<FWacomBattleEnemyPartRuntimeVisualLayer> RuntimeVisualLayers;
	bool bRuntimeDestroyedStateApplied = false;
	int32 RuntimeDestroyedVisualLayerCount = 0;
	int32 RuntimeDestroyedVisualApplyCount = 0;

	TWeakObjectPtr<UPaperFlipbookComponent> ActiveRuntimeActionComponent;
	UPROPERTY(Transient)
	TObjectPtr<UPaperFlipbook> AuthoredRuntimeActionFlipbook = nullptr;
	float AuthoredRuntimeActionPlayRate = 1.0f;
	float AuthoredRuntimeActionStartTimeSeconds = 0.0f;
	bool bAuthoredRuntimeActionLooping = true;
	bool bAuthoredRuntimeActionAutoPlay = true;
	bool bRuntimeActionPlaybackActive = false;
	FName CurrentRuntimeActionLayerId = NAME_None;
	FName CurrentRuntimeActionClipName = NAME_None;
	FName CurrentRuntimeActionIntentId = NAME_None;
	int32 RuntimeActionPlaybackCount = 0;
	int32 RuntimeActionWatchdogCompletionCount = 0;
	uint64 RuntimeActionPlaybackSerial = 0;
	uint64 ActiveRuntimeActionPlaybackSerial = 0;
	FTimerHandle RuntimeActionWatchdogTimerHandle;
	TFunction<void()> PendingRuntimeActionCompletion;
};
