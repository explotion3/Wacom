// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomExitBattlePostRunBarrierAutomationTestView.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/WacomExitBattlePostRunBarrier.h"

struct FWacomExitBattlePostRunBarrierAutomationTestView::FImpl
{
	explicit FImpl(
		ABattleTriggerActor& ResolvedEncounterTrigger,
		TFunction<void()>&& OnReady)
		: Barrier(MakeShared<FExitBattlePostRunBarrierState>(MoveTemp(OnReady)))
	{
		Barrier->SetResolvedEncounterTrigger(&ResolvedEncounterTrigger);
	}

	TSharedRef<FExitBattlePostRunBarrierState> Barrier;
};

FWacomExitBattlePostRunBarrierAutomationTestView::
	FWacomExitBattlePostRunBarrierAutomationTestView(
		ABattleTriggerActor& ResolvedEncounterTrigger,
		TFunction<void()>&& OnReady)
	: Impl(MakeUnique<FImpl>(ResolvedEncounterTrigger, MoveTemp(OnReady)))
{
}

FWacomExitBattlePostRunBarrierAutomationTestView::
	~FWacomExitBattlePostRunBarrierAutomationTestView() = default;

void FWacomExitBattlePostRunBarrierAutomationTestView::MarkReturnCompleted()
{
	Impl->Barrier->MarkReturnCompleted();
}

void FWacomExitBattlePostRunBarrierAutomationTestView::MarkExitBattlePostRunReady()
{
	Impl->Barrier->MarkExitBattlePostRunReady();
}

#endif
