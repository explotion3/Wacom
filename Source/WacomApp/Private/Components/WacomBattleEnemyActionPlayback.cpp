// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyActionPlayback.h"

#include "Engine/World.h"
#include "TimerManager.h"

FWacomBattleEnemyActionPlayback::~FWacomBattleEnemyActionPlayback()
{
	Abandon();
}

bool FWacomBattleEnemyActionPlayback::Begin(
	FWacomBattleEnemyActionPlaybackRequest&& Request)
{
	const bool bHasImpact = Request.ImpactNormalizedTime.IsSet();
	const float ImpactNormalizedTime = bHasImpact
		? Request.ImpactNormalizedTime.GetValue()
		: 0.0f;
	UWorld* World = Request.LifetimeOwner
		? Request.LifetimeOwner->GetWorld()
		: nullptr;
	if (!Request.LifetimeOwner
		|| !World
		|| !FMath::IsFinite(Request.DurationSeconds)
		|| Request.DurationSeconds <= 0.0f
		|| !FMath::IsFinite(Request.WatchdogGraceSeconds)
		|| Request.WatchdogGraceSeconds < 0.0f
		|| (bHasImpact
			&& (!FMath::IsFinite(ImpactNormalizedTime)
				|| ImpactNormalizedTime < 0.0f
				|| ImpactNormalizedTime > 1.0f))
		|| !Request.StartVisual
		|| !Request.FinalizeVisual)
	{
		Request.Callbacks.CompleteImmediately();
		return false;
	}

	Cancel(true);

	LifetimeOwner = Request.LifetimeOwner;
	PendingImpact = MoveTemp(Request.Callbacks.OnImpact);
	PendingCompletion = MoveTemp(Request.Callbacks.OnCompleted);
	PendingVisualFinalizer = MoveTemp(Request.FinalizeVisual);
	View.bActive = true;
	View.bImpactFired = false;
	View.ImpactNormalizedTime = ImpactNormalizedTime;
	ActivePlaybackSerial = ++PlaybackSerial;
	const uint64 StartedSerial = ActivePlaybackSerial;

	if (!Request.StartVisual())
	{
		Finish(
			StartedSerial,
			EWacomBattleEnemyActionPlaybackFinishReason::Natural,
			true,
			true);
		return false;
	}
	++View.PlaybackCount;

	if (bHasImpact && PendingImpact)
	{
		const float ImpactSeconds = Request.DurationSeconds * ImpactNormalizedTime;
		if (ImpactSeconds <= 0.0f)
		{
			HandleImpact(StartedSerial);
		}
		else
		{
			World->GetTimerManager().SetTimer(
				ImpactTimerHandle,
				FTimerDelegate::CreateWeakLambda(
					Request.LifetimeOwner,
					[this, StartedSerial]()
					{
						HandleImpact(StartedSerial);
					}),
				FMath::Max(0.01f, ImpactSeconds),
				false);
		}
	}

	if (!View.bActive || ActivePlaybackSerial != StartedSerial)
	{
		return true;
	}

	World->GetTimerManager().SetTimer(
		WatchdogTimerHandle,
		FTimerDelegate::CreateWeakLambda(
			Request.LifetimeOwner,
			[this, StartedSerial]()
			{
				HandleWatchdog(StartedSerial);
			}),
		Request.DurationSeconds + Request.WatchdogGraceSeconds,
		false);
	return true;
}

void FWacomBattleEnemyActionPlayback::NotifyFinished()
{
	Finish(
		ActivePlaybackSerial,
		EWacomBattleEnemyActionPlaybackFinishReason::Natural,
		true,
		true);
}

void FWacomBattleEnemyActionPlayback::Cancel(bool bRestoreAuthoredVisual)
{
	if (!View.bActive)
	{
		ClearTimers();
		PendingImpact = nullptr;
		return;
	}

	Finish(
		ActivePlaybackSerial,
		EWacomBattleEnemyActionPlaybackFinishReason::Cancelled,
		bRestoreAuthoredVisual,
		false);
}

void FWacomBattleEnemyActionPlayback::HandleImpact(uint64 ExpectedSerial)
{
	if (!View.bActive
		|| ExpectedSerial != ActivePlaybackSerial
		|| View.bImpactFired
		|| !PendingImpact)
	{
		return;
	}

	if (UObject* Owner = LifetimeOwner.Get())
	{
		if (UWorld* World = Owner->GetWorld())
		{
			World->GetTimerManager().ClearTimer(ImpactTimerHandle);
		}
	}
	ImpactTimerHandle = FTimerHandle();
	View.bImpactFired = true;
	++View.ImpactCount;
	TFunction<void()> Callback = MoveTemp(PendingImpact);
	PendingImpact = nullptr;
	Callback();
}

void FWacomBattleEnemyActionPlayback::HandleWatchdog(uint64 ExpectedSerial)
{
	Finish(
		ExpectedSerial,
		EWacomBattleEnemyActionPlaybackFinishReason::Watchdog,
		true,
		true);
}

void FWacomBattleEnemyActionPlayback::Finish(
	uint64 ExpectedSerial,
	EWacomBattleEnemyActionPlaybackFinishReason Reason,
	bool bRestoreAuthoredVisual,
	bool bDeliverPendingImpact)
{
	if (!View.bActive || ExpectedSerial != ActivePlaybackSerial)
	{
		return;
	}

	ClearTimers();
	const bool bHadPendingImpact = !View.bImpactFired && PendingImpact;
	if (Reason == EWacomBattleEnemyActionPlaybackFinishReason::Watchdog)
	{
		++View.WatchdogCompletionCount;
		if (bDeliverPendingImpact && bHadPendingImpact)
		{
			++View.WatchdogForcedImpactCount;
		}
	}

	if (bDeliverPendingImpact)
	{
		HandleImpact(ExpectedSerial);
		if (!View.bActive || ExpectedSerial != ActivePlaybackSerial)
		{
			return;
		}
	}
	else
	{
		PendingImpact = nullptr;
	}

	TFunction<void(const FWacomBattleEnemyActionPlaybackFinishContext&)> VisualFinalizer =
		MoveTemp(PendingVisualFinalizer);
	TFunction<void()> Completion = MoveTemp(PendingCompletion);
	PendingImpact = nullptr;
	PendingCompletion = nullptr;
	PendingVisualFinalizer = nullptr;
	View.bActive = false;
	++PlaybackSerial;

	if (VisualFinalizer)
	{
		VisualFinalizer(FWacomBattleEnemyActionPlaybackFinishContext{
			Reason,
			bRestoreAuthoredVisual });
	}
	if (Completion)
	{
		Completion();
	}
}

void FWacomBattleEnemyActionPlayback::ClearTimers()
{
	if (UObject* Owner = LifetimeOwner.Get())
	{
		if (UWorld* World = Owner->GetWorld())
		{
			World->GetTimerManager().ClearTimer(ImpactTimerHandle);
			World->GetTimerManager().ClearTimer(WatchdogTimerHandle);
		}
	}
	ImpactTimerHandle = FTimerHandle();
	WatchdogTimerHandle = FTimerHandle();
}

void FWacomBattleEnemyActionPlayback::Abandon()
{
	ClearTimers();
	PendingImpact = nullptr;
	PendingCompletion = nullptr;
	PendingVisualFinalizer = nullptr;
	View.bActive = false;
	++PlaybackSerial;
}
