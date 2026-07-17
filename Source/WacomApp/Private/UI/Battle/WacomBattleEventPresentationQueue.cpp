// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEventPresentationQueue.h"

#include "UI/Battle/WacomBattleEnemyActionPlaybackTypes.h"
#include "UI/Battle/WacomBattleHUDPresentationCoordinator.h"
#include "UI/Battle/WacomBattlePresentationTimerOwner.h"

namespace
{
	constexpr float EnemyPartHpEmptiedExtraDelay = 0.45f;
	constexpr float DamageTargetCueDuration = 0.30f;
	constexpr float DamageTargetCueConfirmationLeadSeconds = 0.14f;
	constexpr float EnemyPartDestroyedTargetCueDelay = 0.30f;

	int32 BuildTargetCueSeed(
		int32 EventSequence,
		const FBattlePartSlotIdentity& TargetPartKey,
		int32 Amount)
	{
		uint32 Hash = HashCombineFast(GetTypeHash(EventSequence), GetTypeHash(TargetPartKey));
		Hash = HashCombineFast(Hash, GetTypeHash(Amount));
		return static_cast<int32>(Hash & 0x7FFFFFFFu);
	}
}

FWacomBattleEventPresentationQueue::FWacomBattleEventPresentationQueue(
	FWacomBattleHUDPresentationCoordinator& InCoordinator,
	FWacomBattlePresentationTimerOwner& InTimerOwner)
	: Coordinator(InCoordinator)
	, TimerOwner(InTimerOwner)
{
}

FWacomBattleEventPresentationQueue::~FWacomBattleEventPresentationQueue()
{
	AbandonWithoutWorldAccess();
}

void FWacomBattleEventPresentationQueue::EnqueueEvents(const TArray<FBattleEvent>& Events)
{
	EnqueueEvents(
		Events,
		TArray<FBattlePresentationEnemyActionStep>(),
		INDEX_NONE,
		0.0f,
		false);
}

void FWacomBattleEventPresentationQueue::EnqueueEvents(
	const TArray<FBattleEvent>& Events,
	const TArray<FBattlePresentationEnemyActionStep>& EnemyActionSteps,
	int32 PresentationStackEntryId,
	float MinimumStackHoldSeconds,
	bool bTargetAlreadyConfirmed)
{
	if (Events.IsEmpty())
	{
		return;
	}

	bool bAddedPresentationStep = false;
	bool bAddedDamageConfirmationLead = false;
	TMap<FName, int32> LastDestroyedEventIndexByEnemySlot;
	for (int32 EventIndex = 0; EventIndex < Events.Num(); ++EventIndex)
	{
		const FBattleEvent& Event = Events[EventIndex];
		if (Event.Type == EBattleEventType::EnemyPartHpEmptied
			&& Event.ActorEnemyPartKey.IsValidKey())
		{
			LastDestroyedEventIndexByEnemySlot.Add(
				Event.ActorEnemyPartKey.GetEffectiveEnemyUnitSlotId(),
				EventIndex);
		}
	}

	for (int32 EventIndex = 0; EventIndex < Events.Num(); ++EventIndex)
	{
		const FBattleEvent& Event = Events[EventIndex];
		const FBattlePresentationEnemyActionStep* EnemyActionStep =
			Event.Type == EBattleEventType::EnemyPartActed
			? EnemyActionSteps.FindByPredicate(
				[Sequence = Event.Sequence](const FBattlePresentationEnemyActionStep& Candidate)
				{
					return Candidate.FirstEventSequence == Sequence;
				})
			: nullptr;
		if (!bTargetAlreadyConfirmed
			&& !bAddedDamageConfirmationLead
			&& Event.Type == EBattleEventType::DamageDealt
			&& Event.ActorEnemyPartKey.IsValidKey())
		{
			FWacomBattlePresentationStep LeadStep;
			LeadStep.Type = EWacomBattlePresentationStepType::Delay;
			LeadStep.EventSequence = Event.Sequence;
			LeadStep.SourceEventType = Event.Type;
			LeadStep.Duration = DamageTargetCueConfirmationLeadSeconds;
			Steps.Add(MoveTemp(LeadStep));
			bAddedPresentationStep = true;
			bAddedDamageConfirmationLead = true;
		}
		const int32* LastDestroyedEventIndex = Event.ActorEnemyPartKey.IsValidKey()
			? LastDestroyedEventIndexByEnemySlot.Find(
				Event.ActorEnemyPartKey.GetEffectiveEnemyUnitSlotId())
			: nullptr;
		bAddedPresentationStep |= BuildStepsForEvent(
			Event,
			EnemyActionStep,
			Event.Type == EBattleEventType::EnemyPartHpEmptied
				&& LastDestroyedEventIndex
				&& *LastDestroyedEventIndex == EventIndex);
	}

	if (PresentationStackEntryId != INDEX_NONE)
	{
		if (!bAddedPresentationStep && MinimumStackHoldSeconds > 0.0f)
		{
			FWacomBattlePresentationStep HoldStep;
			HoldStep.Type = EWacomBattlePresentationStepType::Delay;
			HoldStep.Duration = MinimumStackHoldSeconds;
			HoldStep.PresentationStackEntryId = PresentationStackEntryId;
			Steps.Add(MoveTemp(HoldStep));
		}

		FWacomBattlePresentationStep BoundaryStep;
		BoundaryStep.Type = EWacomBattlePresentationStepType::CardStackBoundary;
		BoundaryStep.PresentationStackEntryId = PresentationStackEntryId;
		Steps.Add(MoveTemp(BoundaryStep));
	}

	if (!bProcessing && !Steps.IsEmpty())
	{
		bProcessing = true;
		Coordinator.HandleQueueStarted();
		Advance();
		if (bProcessing && !Coordinator.GetWorld())
		{
			int32 SafetyCounter = 0;
			while (bProcessing && SafetyCounter++ < 128)
			{
				Advance();
			}
		}
		return;
	}

	if (Steps.IsEmpty())
	{
		FinishIfIdle();
	}
}

void FWacomBattleEventPresentationQueue::EnqueueTargetCue(
	const FWacomBattlePresentationTargetCue& Cue)
{
	if (!Cue.TargetPartKey.IsValidSlot())
	{
		return;
	}

	FWacomBattlePresentationStep Step;
	Step.Type = EWacomBattlePresentationStepType::TargetCue;
	Step.SourceEventType = Cue.SourceEventType;
	Step.TargetCue = Cue;
	Steps.Add(MoveTemp(Step));
	if (!bProcessing)
	{
		bProcessing = true;
		Coordinator.HandleQueueStarted();
		Advance();
		if (bProcessing && !Coordinator.GetWorld())
		{
			int32 SafetyCounter = 0;
			while (bProcessing && SafetyCounter++ < 16)
			{
				Advance();
			}
		}
	}
}

#if WITH_AUTOMATION_TESTS
void FWacomBattleEventPresentationQueue::AdvanceForTest()
{
	Advance();
}
#endif

void FWacomBattleEventPresentationQueue::Clear()
{
	StopTimer();

	Steps.Reset();
	bProcessing = false;
	bAdvancing = false;
	bWaitingForSceneEnemyAnimation = false;
	bSceneEnemyAnimationImpactDelivered = false;
	ActiveSceneEnemyActionStep.Reset();
	++SceneEnemyAnimationBarrierSerial;
}

void FWacomBattleEventPresentationQueue::AbandonWithoutWorldAccess()
{
	TimerOwner.AbandonWithoutWorldAccess(
		FWacomBattlePresentationTimerKey::EventQueueAdvance());
	Steps.Reset();
	bProcessing = false;
	bAdvancing = false;
	bWaitingForSceneEnemyAnimation = false;
	bSceneEnemyAnimationImpactDelivered = false;
	ActiveSceneEnemyActionStep.Reset();
	++SceneEnemyAnimationBarrierSerial;
}

bool FWacomBattleEventPresentationQueue::BuildStepsForEvent(
	const FBattleEvent& Event,
	const FBattlePresentationEnemyActionStep* EnemyActionStep,
	bool bLastDestroyedEventForEnemy)
{
	if (Event.Type == EBattleEventType::KnockdownChoiceRequested)
	{
		FWacomBattlePresentationStep Step;
		Step.Type = EWacomBattlePresentationStepType::KnockdownChoiceDialog;
		Step.EventSequence = Event.Sequence;
		Step.SourceEventType = Event.Type;
		Steps.Add(MoveTemp(Step));
		return true;
	}

	bool bAddedStep = false;
	if (Event.Type == EBattleEventType::EnemyPartActed
		&& Event.ActorEnemyPartKey.IsValidKey())
	{
		FWacomBattlePresentationStep SceneEnemyAnimationStep;
		SceneEnemyAnimationStep.Type = EWacomBattlePresentationStepType::SceneEnemyAnimation;
		SceneEnemyAnimationStep.EventSequence = Event.Sequence;
		SceneEnemyAnimationStep.SourceEventType = Event.Type;
		SceneEnemyAnimationStep.ActingPartKey =
			FBattlePartSlotIdentity::FromEnemyPartKey(Event.ActorEnemyPartKey);
		SceneEnemyAnimationStep.IntentId = Event.IntentId;
		SceneEnemyAnimationStep.bSkipSceneEnemyAnimation = Event.Count <= 0;
		if (EnemyActionStep && EnemyActionStep->IsValid())
		{
			SceneEnemyAnimationStep.EnemyActionStep = *EnemyActionStep;
		}
		Steps.Add(MoveTemp(SceneEnemyAnimationStep));
		bAddedStep = true;
	}
	if ((Event.Type == EBattleEventType::DamageDealt || Event.Type == EBattleEventType::EnemyPartHpEmptied)
		&& Event.ActorEnemyPartKey.IsValidKey())
	{
		FWacomBattlePresentationStep CueStep;
		CueStep.Type = EWacomBattlePresentationStepType::TargetCue;
		CueStep.EventSequence = Event.Sequence;
		CueStep.SourceEventType = Event.Type;
		CueStep.TargetCue.SourceEventType = Event.Type;
		CueStep.TargetCue.CueKind = Event.Type == EBattleEventType::EnemyPartHpEmptied
			? EWacomBattlePresentationTargetCueKind::EnemyPartHpEmptied
			: EWacomBattlePresentationTargetCueKind::DamageDealt;
		CueStep.TargetCue.TargetPartKey = FBattlePartSlotIdentity::FromEnemyPartKey(Event.ActorEnemyPartKey);
		CueStep.TargetCue.Amount = Event.Amount;
		CueStep.TargetCue.Duration = Event.Type == EBattleEventType::EnemyPartHpEmptied
			? EnemyPartDestroyedTargetCueDelay
			: DamageTargetCueDuration;
		CueStep.TargetCue.Seed = BuildTargetCueSeed(
			Event.Sequence,
			CueStep.TargetCue.TargetPartKey,
			Event.Amount);
		Steps.Add(MoveTemp(CueStep));
		bAddedStep = true;
	}

	if (Event.Type == EBattleEventType::EnemyPartHpEmptied)
	{
		FWacomBattlePresentationStep DelayStep;
		DelayStep.Type = EWacomBattlePresentationStepType::Delay;
		DelayStep.EventSequence = Event.Sequence;
		DelayStep.SourceEventType = Event.Type;
		DelayStep.Duration = EnemyPartHpEmptiedExtraDelay;
		Steps.Add(MoveTemp(DelayStep));
		bAddedStep = true;

		if (bLastDestroyedEventForEnemy && Event.ActorEnemyPartKey.IsValidKey())
		{
			FWacomBattlePresentationStep SceneEnemyAnimationStep;
			SceneEnemyAnimationStep.Type = EWacomBattlePresentationStepType::SceneEnemyAnimation;
			SceneEnemyAnimationStep.EventSequence = Event.Sequence;
			SceneEnemyAnimationStep.SourceEventType = Event.Type;
			SceneEnemyAnimationStep.ActingPartKey =
				FBattlePartSlotIdentity::FromEnemyPartKey(Event.ActorEnemyPartKey);
			SceneEnemyAnimationStep.bDestroyedHostAnimation = true;
			Steps.Add(MoveTemp(SceneEnemyAnimationStep));
		}
	}

	if (Event.Type == EBattleEventType::BattleEnded)
	{
		FWacomBattlePresentationStep BattleEndStep;
		BattleEndStep.Type = EWacomBattlePresentationStepType::BattleEndSignal;
		BattleEndStep.EventSequence = Event.Sequence;
		BattleEndStep.SourceEventType = Event.Type;
		Steps.Add(MoveTemp(BattleEndStep));
		bAddedStep = true;
	}

	return bAddedStep;
}

void FWacomBattleEventPresentationQueue::ScheduleNextStep(float DelaySeconds)
{
	if (UWorld* World = Coordinator.GetWorld())
	{
		const float SafeDelay = FMath::Max(0.01f, DelaySeconds);
		const TWeakPtr<FWacomBattleEventPresentationQueue> WeakThis = AsShared();
		TimerOwner.ScheduleOnce(
			World,
			FWacomBattlePresentationTimerKey::EventQueueAdvance(),
			SafeDelay,
			[WeakThis]()
			{
				if (const TSharedPtr<FWacomBattleEventPresentationQueue> Pinned = WeakThis.Pin())
				{
					Pinned->Advance();
				}
			});
		return;
	}

	return;
}

void FWacomBattleEventPresentationQueue::StopTimer()
{
	TimerOwner.Cancel(FWacomBattlePresentationTimerKey::EventQueueAdvance());
}

void FWacomBattleEventPresentationQueue::Advance()
{
	if (bWaitingForSceneEnemyAnimation)
	{
		return;
	}
	if (bAdvancing)
	{
		ScheduleNextStep(0.01f);
		return;
	}

	TGuardValue<bool> AdvancingGuard(bAdvancing, true);

	TSharedPtr<FWacomBattleEventPresentationQueue> SelfKeepAlive = AsShared();

	if (Steps.IsEmpty())
	{
		FinishIfIdle();
		return;
	}

	FWacomBattlePresentationStep Step = MoveTemp(Steps[0]);
	Steps.RemoveAt(0);

	float NextDelay = 0.0f;
	switch (Step.Type)
	{
	case EWacomBattlePresentationStepType::TargetCue:
		Coordinator.HandleTargetCueStep(Step.TargetCue);
		NextDelay = Step.TargetCue.Duration;
		break;

	case EWacomBattlePresentationStepType::Delay:
		NextDelay = Step.Duration;
		break;

	case EWacomBattlePresentationStepType::KnockdownChoiceDialog:
		Coordinator.HandleKnockdownChoiceDialogStep();
		NextDelay = 0.0f;
		break;

	case EWacomBattlePresentationStepType::BattleEndSignal:
		Coordinator.HandleBattleEndStep();
		NextDelay = 0.0f;
		break;

	case EWacomBattlePresentationStepType::CardStackBoundary:
		Coordinator.HandleCardStackBoundaryStep(Step.PresentationStackEntryId);
		NextDelay = 0.0f;
		break;

	case EWacomBattlePresentationStepType::SceneEnemyAnimation:
	{
		const uint64 BarrierSerial = ++SceneEnemyAnimationBarrierSerial;
		bWaitingForSceneEnemyAnimation = true;
		bSceneEnemyAnimationImpactDelivered = false;
		ActiveSceneEnemyActionStep = MoveTemp(Step.EnemyActionStep);
		TWeakPtr<FWacomBattleEventPresentationQueue> WeakThis = AsShared();
		if (Step.bSkipSceneEnemyAnimation)
		{
			DeliverSceneEnemyAnimationImpact(BarrierSerial);
			CompleteSceneEnemyAnimationBarrier(BarrierSerial);
		}
		else
		{
			FWacomBattleEnemyActionPlaybackCallbacks Callbacks;
			Callbacks.OnImpact = [WeakThis, BarrierSerial]()
			{
				if (const TSharedPtr<FWacomBattleEventPresentationQueue> Pinned = WeakThis.Pin())
				{
					Pinned->DeliverSceneEnemyAnimationImpact(BarrierSerial);
				}
			};
			Callbacks.OnCompleted = [WeakThis, BarrierSerial]()
			{
				if (const TSharedPtr<FWacomBattleEventPresentationQueue> Pinned = WeakThis.Pin())
				{
					Pinned->CompleteSceneEnemyAnimationBarrier(BarrierSerial);
				}
			};
			Coordinator.HandleSceneEnemyAnimationStep(
				Step.ActingPartKey,
				Step.IntentId,
				Step.bDestroyedHostAnimation,
				MoveTemp(Callbacks));
		}
		if (bWaitingForSceneEnemyAnimation)
		{
			return;
		}
		NextDelay = 0.0f;
		break;
	}

	default:
		break;
	}

	if (!bProcessing)
	{
		return;
	}

	if (Steps.IsEmpty())
	{
		if (NextDelay > 0.0f)
		{
			ScheduleNextStep(NextDelay);
			return;
		}

		FinishIfIdle();
		return;
	}

	ScheduleNextStep(NextDelay);
}

void FWacomBattleEventPresentationQueue::CompleteSceneEnemyAnimationBarrier(uint64 ExpectedSerial)
{
	if (!bProcessing
		|| !bWaitingForSceneEnemyAnimation
		|| ExpectedSerial != SceneEnemyAnimationBarrierSerial)
	{
		return;
	}

	bWaitingForSceneEnemyAnimation = false;
	ActiveSceneEnemyActionStep.Reset();
	if (!bAdvancing)
	{
		Advance();
	}
}

void FWacomBattleEventPresentationQueue::DeliverSceneEnemyAnimationImpact(uint64 ExpectedSerial)
{
	if (!bProcessing
		|| !bWaitingForSceneEnemyAnimation
		|| ExpectedSerial != SceneEnemyAnimationBarrierSerial
		|| bSceneEnemyAnimationImpactDelivered)
	{
		return;
	}

	bSceneEnemyAnimationImpactDelivered = true;
	if (ActiveSceneEnemyActionStep.IsSet())
	{
		Coordinator.HandleSceneEnemyActionImpact(ActiveSceneEnemyActionStep.GetValue());
	}
}

void FWacomBattleEventPresentationQueue::FinishIfIdle()
{
	if (!bProcessing)
	{
		return;
	}

	bProcessing = false;
	StopTimer();
	Coordinator.HandleQueueFinished();
}
