// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WacomBattleEnemyHostVisualComponent.generated.h"

class UPaperFlipbook;
class UPaperFlipbookComponent;
class UPaperSprite;
class UPaperSpriteComponent;
class UMaterialInterface;
class USceneComponent;
struct FWacomBattleEnemyActionPlaybackCallbacks;

/**
 * 生成并维护场景敌人 Host 的整体 PaperSprite / PaperFlipbook 视觉。
 *
 * 只负责 Host 整体视觉组件生命周期与统计；不拥有部位身份、HitBounds 或 Battle 目标绑定。
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent, ToolTip = "生成并维护场景敌人 Host 的整体 PaperSprite / PaperFlipbook 视觉。"))
class WACOMAPP_API UWacomBattleEnemyHostVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomBattleEnemyHostVisualComponent();

	void RefreshHostVisual(
		USceneComponent* AttachRoot,
		bool bUseFlipbook,
		UPaperSprite* HostSprite,
		UPaperFlipbook* HostFlipbook,
		const FVector& RelativeLocation,
		const FRotator& RelativeRotation,
		const FVector& RelativeScale3D,
		int32 SortOrder,
		const FLinearColor& Tint,
		UMaterialInterface* MaterialOverride,
		bool bCastShadow,
		bool bVisible,
		float FlipbookPlayRate,
		bool bLoopFlipbook,
		float FlipbookStartTimeSeconds,
		bool bAutoPlayFlipbook);
	void ClearGeneratedHostVisual();

	/** 原地播放一次 Host 语义动画；返回 false 时会同步完成 Completion。 */
	bool PlayRuntimeOneShot(
		UPaperFlipbook* Flipbook,
		float PlayRate,
		float ImpactNormalizedTime,
		FName IntentId,
		bool bTerminal,
		FWacomBattleEnemyActionPlaybackCallbacks&& Callbacks);

	/** 新战斗首次接管 Host 时，仅在残留 runtime 播放或终态存在时恢复 authored Idle。 */
	void ResetRuntimePlaybackToIdle();

	/** source/session 清理时结束当前 barrier；行动恢复 Idle，终态保持末帧。 */
	void CancelRuntimePlayback();

	UPaperSpriteComponent* GetGeneratedHostSpriteVisualComponent() const
	{
		return GeneratedHostSpriteVisualComponent;
	}

	UPaperFlipbookComponent* GetGeneratedHostFlipbookVisualComponent() const
	{
		return GeneratedHostFlipbookVisualComponent;
	}

	int32 GetGeneratedHostVisualComponentCount() const;
	int32 GetRegisteredHostVisualComponentCount() const;
	int32 GetVisibleHostVisualComponentCount() const;
	FName GetCurrentRuntimeClipName() const { return CurrentRuntimeClipName; }
	FName GetCurrentRuntimeIntentId() const { return CurrentRuntimeIntentId; }
	bool IsRuntimePlaybackActive() const { return bRuntimePlaybackActive; }
	bool IsRuntimeTerminalState() const { return bRuntimeTerminalState; }
	int32 GetRuntimePlaybackCount() const { return RuntimePlaybackCount; }
	int32 GetRuntimeWatchdogCompletionCount() const { return RuntimeWatchdogCompletionCount; }
	float GetCurrentRuntimeImpactNormalizedTime() const { return CurrentRuntimeImpactNormalizedTime; }
	bool HasRuntimeImpactFired() const { return bRuntimeImpactFired; }
	int32 GetRuntimeImpactCount() const { return RuntimeImpactCount; }
	int32 GetRuntimeWatchdogForcedImpactCount() const { return RuntimeWatchdogForcedImpactCount; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleRuntimeFlipbookFinished();

	void HandleRuntimeWatchdogExpired(uint64 ExpectedPlaybackSerial);
	void HandleRuntimeImpact(uint64 ExpectedPlaybackSerial);
	void CompleteRuntimePlayback(
		uint64 ExpectedPlaybackSerial,
		bool bWatchdogCompletion,
		bool bDeliverPendingImpact);
	void StopRuntimeWatchdog();
	void StopRuntimeImpactTimer();
	void RestoreAuthoredIdlePlayback();
	void CompletePendingCallback();

	UPROPERTY(Transient)
	TObjectPtr<UPaperSpriteComponent> GeneratedHostSpriteVisualComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPaperFlipbookComponent> GeneratedHostFlipbookVisualComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPaperFlipbook> AuthoredIdleFlipbook = nullptr;

	float AuthoredIdlePlayRate = 1.0f;
	float AuthoredIdleStartTimeSeconds = 0.0f;
	bool bAuthoredIdleLooping = true;
	bool bAuthoredIdleAutoPlay = true;
	bool bRuntimePlaybackActive = false;
	bool bRuntimeTerminalState = false;
	FName CurrentRuntimeClipName = NAME_None;
	FName CurrentRuntimeIntentId = NAME_None;
	int32 RuntimePlaybackCount = 0;
	int32 RuntimeWatchdogCompletionCount = 0;
	float CurrentRuntimeImpactNormalizedTime = 0.0f;
	bool bRuntimeImpactFired = false;
	int32 RuntimeImpactCount = 0;
	int32 RuntimeWatchdogForcedImpactCount = 0;
	uint64 PlaybackSerial = 0;
	uint64 ActivePlaybackSerial = 0;
	FTimerHandle RuntimeWatchdogTimerHandle;
	FTimerHandle RuntimeImpactTimerHandle;
	TFunction<void()> PendingRuntimeImpact;
	TFunction<void()> PendingRuntimeCompletion;
};
