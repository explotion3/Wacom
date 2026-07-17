// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleEnemyActionPlaybackTypes.h"

class UObject;

enum class EWacomBattleEnemyActionPlaybackFinishReason : uint8
{
	Natural,
	Watchdog,
	Cancelled,
};

struct FWacomBattleEnemyActionPlaybackFinishContext
{
	EWacomBattleEnemyActionPlaybackFinishReason Reason =
		EWacomBattleEnemyActionPlaybackFinishReason::Natural;
	bool bRestoreAuthoredVisual = true;
};

struct FWacomBattleEnemyActionPlaybackRequest
{
	UObject* LifetimeOwner = nullptr;
	float DurationSeconds = 0.0f;
	TOptional<float> ImpactNormalizedTime;
	float WatchdogGraceSeconds = 0.10f;
	FWacomBattleEnemyActionPlaybackCallbacks Callbacks;
	TFunction<bool()> StartVisual;
	TFunction<void(const FWacomBattleEnemyActionPlaybackFinishContext&)> FinalizeVisual;
};

struct FWacomBattleEnemyActionPlaybackView
{
	bool bActive = false;
	float ImpactNormalizedTime = 0.0f;
	bool bImpactFired = false;
	int32 PlaybackCount = 0;
	int32 ImpactCount = 0;
	int32 WatchdogCompletionCount = 0;
	int32 WatchdogForcedImpactCount = 0;
};

/**
 * Shared non-reflected lifecycle for enemy one-shot playback.
 *
 * Visual adapters own Flipbook switching and restoration. This module owns
 * timers, serial invalidation, exactly-once Impact/Complete callbacks and
 * lifecycle diagnostics.
 */
class FWacomBattleEnemyActionPlayback
{
public:
	~FWacomBattleEnemyActionPlayback();

	bool Begin(FWacomBattleEnemyActionPlaybackRequest&& Request);
	void NotifyFinished();
	void Cancel(bool bRestoreAuthoredVisual = true);

	const FWacomBattleEnemyActionPlaybackView& GetView() const { return View; }

private:
	void HandleImpact(uint64 ExpectedSerial);
	void HandleWatchdog(uint64 ExpectedSerial);
	void Finish(
		uint64 ExpectedSerial,
		EWacomBattleEnemyActionPlaybackFinishReason Reason,
		bool bRestoreAuthoredVisual,
		bool bDeliverPendingImpact);
	void ClearTimers();
	void Abandon();

	FWacomBattleEnemyActionPlaybackView View;
	TWeakObjectPtr<UObject> LifetimeOwner;
	uint64 PlaybackSerial = 0;
	uint64 ActivePlaybackSerial = 0;
	FTimerHandle ImpactTimerHandle;
	FTimerHandle WatchdogTimerHandle;
	TFunction<void()> PendingImpact;
	TFunction<void()> PendingCompletion;
	TFunction<void(const FWacomBattleEnemyActionPlaybackFinishContext&)> PendingVisualFinalizer;
};
