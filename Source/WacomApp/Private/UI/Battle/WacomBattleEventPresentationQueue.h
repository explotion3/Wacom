// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"

class FWacomBattleHUDPresentationCoordinator;

enum class EWacomBattlePresentationStepType : uint8
{
	TargetCue,
	Delay,
	KnockdownChoiceDialog,
	BattleEndSignal,
	CardStackBoundary,
	HostAnimation,
};

struct FWacomBattlePresentationStep
{
	EWacomBattlePresentationStepType Type = EWacomBattlePresentationStepType::Delay;
	int32 EventSequence = 0;
	EBattleEventType SourceEventType = EBattleEventType::None;
	FWacomBattlePresentationTargetCue TargetCue;
	float Duration = 0.0f;
	int32 PresentationStackEntryId = INDEX_NONE;
	FName EnemySlotId = NAME_None;
	FName IntentId = NAME_None;
	bool bDestroyedHostAnimation = false;
};

class FWacomBattleEventPresentationQueue
	: public TSharedFromThis<FWacomBattleEventPresentationQueue>
{
public:
	explicit FWacomBattleEventPresentationQueue(FWacomBattleHUDPresentationCoordinator& InCoordinator);
	~FWacomBattleEventPresentationQueue();

	void EnqueueEvents(const TArray<FBattleEvent>& Events);
	void EnqueueEvents(
		const TArray<FBattleEvent>& Events,
		int32 PresentationStackEntryId,
		float MinimumStackHoldSeconds,
		bool bTargetAlreadyConfirmed = false);
	void EnqueueTargetCue(const FWacomBattlePresentationTargetCue& Cue);
	void Clear();
	void AbandonWithoutWorldAccess();

	bool IsBusy() const { return bProcessing || Steps.Num() > 0; }
	int32 GetPendingStepCount() const { return Steps.Num(); }
#if WITH_AUTOMATION_TESTS
	void AdvanceForTest();
#endif

private:
	FWacomBattleHUDPresentationCoordinator& Coordinator;
	TArray<FWacomBattlePresentationStep> Steps;
	FTimerHandle StepTimerHandle;
	bool bProcessing = false;
	bool bAdvancing = false;
	bool bWaitingForHostAnimation = false;
	uint64 HostAnimationBarrierSerial = 0;

	bool BuildStepsForEvent(const FBattleEvent& Event, bool bLastDestroyedEventForEnemy);
	void ScheduleNextStep(float DelaySeconds);
	void StopTimer();
	void Advance();
	void FinishIfIdle();
	void CompleteHostAnimationBarrier(uint64 ExpectedSerial);
};
