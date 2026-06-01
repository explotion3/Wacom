// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEventPresentationQueue.h"

#include "UI/Battle/BattleHUD.h"

#include "Engine/World.h"

namespace
{
	constexpr float EnemyPartHpEmptiedExtraDelay = 0.45f;
	constexpr float DamageTargetCueDelay = 0.18f;
	constexpr float EnemyPartDestroyedTargetCueDelay = 0.30f;
}

FWacomBattleEventPresentationQueue::FWacomBattleEventPresentationQueue(UBattleHUD& InHUD)
	: HUD(&InHUD)
{
}

FWacomBattleEventPresentationQueue::~FWacomBattleEventPresentationQueue()
{
	Clear();
}

void FWacomBattleEventPresentationQueue::EnqueueEvents(const TArray<FBattleEvent>& Events)
{
	EnqueueEvents(Events, INDEX_NONE, 0.0f);
}

void FWacomBattleEventPresentationQueue::EnqueueEvents(
	const TArray<FBattleEvent>& Events,
	int32 PresentationStackEntryId,
	float MinimumStackHoldSeconds)
{
	if (Events.IsEmpty())
	{
		return;
	}

	bool bAddedPresentationStep = false;
	for (const FBattleEvent& Event : Events)
	{
		bAddedPresentationStep |= BuildStepsForEvent(Event);
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
		UBattleHUD* StrongHUD = HUD.Get();
		if (StrongHUD)
		{
			StrongHUD->HandleBattlePresentationQueueStarted();
		}
		Advance();
		if (bProcessing && StrongHUD && !StrongHUD->GetWorld())
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

#if WITH_AUTOMATION_TESTS
void FWacomBattleEventPresentationQueue::AdvanceForTest()
{
	Advance();
}
#endif

void FWacomBattleEventPresentationQueue::Clear()
{
	if (UBattleHUD* StrongHUD = HUD.Get())
	{
		StopTimer();
	}

	Steps.Reset();
	bProcessing = false;
	bAdvancing = false;
}

bool FWacomBattleEventPresentationQueue::BuildStepsForEvent(const FBattleEvent& Event)
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
	if ((Event.Type == EBattleEventType::DamageDealt || Event.Type == EBattleEventType::EnemyPartHpEmptied)
		&& Event.ActorInstanceId.IsValid())
	{
		FWacomBattlePresentationStep CueStep;
		CueStep.Type = EWacomBattlePresentationStepType::TargetCue;
		CueStep.EventSequence = Event.Sequence;
		CueStep.SourceEventType = Event.Type;
		CueStep.TargetCue.SourceEventType = Event.Type;
		CueStep.TargetCue.TargetPartInstanceId = Event.ActorInstanceId;
		CueStep.TargetCue.Amount = Event.Amount;
		CueStep.TargetCue.Duration = Event.Type == EBattleEventType::EnemyPartHpEmptied
			? EnemyPartDestroyedTargetCueDelay
			: DamageTargetCueDelay;
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
	UBattleHUD* StrongHUD = HUD.Get();
	if (!StrongHUD)
	{
		Clear();
		return;
	}

	if (UWorld* World = StrongHUD->GetWorld())
	{
		World->GetTimerManager().ClearTimer(StepTimerHandle);
		StepTimerHandle = FTimerHandle();

		const float SafeDelay = FMath::Max(0.01f, DelaySeconds);
		World->GetTimerManager().SetTimer(
			StepTimerHandle,
			FTimerDelegate::CreateRaw(this, &FWacomBattleEventPresentationQueue::Advance),
			SafeDelay,
			false);
		return;
	}

	return;
}

void FWacomBattleEventPresentationQueue::StopTimer()
{
	if (UBattleHUD* StrongHUD = HUD.Get())
	{
		if (UWorld* World = StrongHUD->GetWorld())
		{
			World->GetTimerManager().ClearTimer(StepTimerHandle);
		}
	}
}

void FWacomBattleEventPresentationQueue::Advance()
{
	if (bAdvancing)
	{
		ScheduleNextStep(0.01f);
		return;
	}

	TGuardValue<bool> AdvancingGuard(bAdvancing, true);

	UBattleHUD* StrongHUD = HUD.Get();
	if (!StrongHUD)
	{
		Clear();
		return;
	}
	TSharedPtr<FWacomBattleEventPresentationQueue> SelfKeepAlive =
		StrongHUD->GetBattlePresentationQueueSelfKeepAlive();

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
		StrongHUD->PlayBattlePresentationCue(Step.TargetCue);
		NextDelay = Step.TargetCue.Duration;
		break;

	case EWacomBattlePresentationStepType::Delay:
		NextDelay = Step.Duration;
		break;

	case EWacomBattlePresentationStepType::KnockdownChoiceDialog:
		StrongHUD->PushPendingKnockdownChoiceDialog();
		NextDelay = 0.0f;
		break;

	case EWacomBattlePresentationStepType::BattleEndSignal:
		StrongHUD->HandleBattlePresentationBattleEndStep();
		NextDelay = 0.0f;
		break;

	case EWacomBattlePresentationStepType::CardStackBoundary:
		StrongHUD->BeginBattlePresentationStackEntryExit(Step.PresentationStackEntryId);
		NextDelay = 0.0f;
		break;

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

void FWacomBattleEventPresentationQueue::FinishIfIdle()
{
	if (!bProcessing)
	{
		return;
	}

	bProcessing = false;
	StopTimer();
	if (UBattleHUD* StrongHUD = HUD.Get())
	{
		StrongHUD->HandleBattlePresentationQueueFinished();
	}
}
