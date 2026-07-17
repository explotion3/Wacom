// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Runtime/BattlePartSlotIdentity.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"

class FWacomBattleHUDPresentationCoordinator;
class FWacomBattlePresentationTimerOwner;

enum class EWacomBattlePresentationStepType : uint8
{
	TargetCue,
	Delay,
	KnockdownChoiceDialog,
	BattleEndSignal,
	CardStackBoundary,
	SceneEnemyAnimation,
};

struct FWacomBattlePresentationStep
{
	EWacomBattlePresentationStepType Type = EWacomBattlePresentationStepType::Delay;
	int32 EventSequence = 0;
	EBattleEventType SourceEventType = EBattleEventType::None;
	FWacomBattlePresentationTargetCue TargetCue;
	float Duration = 0.0f;
	int32 PresentationStackEntryId = INDEX_NONE;
	FBattlePartSlotIdentity ActingPartKey;
	FName IntentId = NAME_None;
	TOptional<FBattlePresentationEnemyActionStep> EnemyActionStep;
	bool bSkipSceneEnemyAnimation = false;
	bool bDestroyedHostAnimation = false;
};

class FWacomBattleEventPresentationQueue
	: public TSharedFromThis<FWacomBattleEventPresentationQueue>
{
public:
	FWacomBattleEventPresentationQueue(
		FWacomBattleHUDPresentationCoordinator& InCoordinator,
		FWacomBattlePresentationTimerOwner& InTimerOwner);
	~FWacomBattleEventPresentationQueue();

	void EnqueueEvents(const TArray<FBattleEvent>& Events);
	void EnqueueEvents(
		const TArray<FBattleEvent>& Events,
		const TArray<FBattlePresentationEnemyActionStep>& EnemyActionSteps,
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
	FWacomBattlePresentationTimerOwner& TimerOwner;
	TArray<FWacomBattlePresentationStep> Steps;
	bool bProcessing = false;
	bool bAdvancing = false;
	bool bWaitingForSceneEnemyAnimation = false;
	bool bSceneEnemyAnimationImpactDelivered = false;
	TOptional<FBattlePresentationEnemyActionStep> ActiveSceneEnemyActionStep;
	uint64 SceneEnemyAnimationBarrierSerial = 0;

	bool BuildStepsForEvent(
		const FBattleEvent& Event,
		const FBattlePresentationEnemyActionStep* EnemyActionStep,
		bool bLastDestroyedEventForEnemy);
	void ScheduleNextStep(float DelaySeconds);
	void StopTimer();
	void Advance();
	void FinishIfIdle();
	void CompleteSceneEnemyAnimationBarrier(uint64 ExpectedSerial);
	void DeliverSceneEnemyAnimationImpact(uint64 ExpectedSerial);
};
