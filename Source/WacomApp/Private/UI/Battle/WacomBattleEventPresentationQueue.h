// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"

class UBattleHUD;

enum class EWacomBattlePresentationStepType : uint8
{
	Toast,
	Delay,
	KnockdownChoiceDialog,
	BattleEndSignal,
};

struct FWacomBattlePresentationStep
{
	EWacomBattlePresentationStepType Type = EWacomBattlePresentationStepType::Toast;
	int32 EventSequence = 0;
	EBattleEventType SourceEventType = EBattleEventType::None;
	FBattleEventPresentationView View;
	float Duration = 0.0f;
};

class FWacomBattleEventPresentationQueue
{
public:
	explicit FWacomBattleEventPresentationQueue(UBattleHUD& InHUD);
	~FWacomBattleEventPresentationQueue();

	void EnqueueEvents(const TArray<FBattleEvent>& Events);
	void Clear();

	bool IsBusy() const { return bProcessing || Steps.Num() > 0; }
	int32 GetPendingStepCount() const { return Steps.Num(); }
#if WITH_AUTOMATION_TESTS
	void AdvanceForTest();
#endif

private:
	TWeakObjectPtr<UBattleHUD> HUD;
	TArray<FWacomBattlePresentationStep> Steps;
	FTimerHandle StepTimerHandle;
	bool bProcessing = false;
	bool bAdvancing = false;

	void BuildStepsForEvent(const FBattleEvent& Event);
	void ScheduleNextStep(float DelaySeconds);
	void StopTimer();
	void Advance();
	void FinishIfIdle();
};
