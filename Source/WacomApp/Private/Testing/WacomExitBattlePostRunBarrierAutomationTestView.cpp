// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomExitBattlePostRunBarrierAutomationTestView.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/WacomExitBattlePostRunBarrier.h"

struct FWacomExitBattlePostRunBarrierAutomationTestView::FImpl
{
	explicit FImpl(
		TFunction<void()>&& OnResolvedEncounterRetirement,
		TFunction<void()>&& OnReady)
		: Barrier(MakeShared<FExitBattlePostRunBarrierState>(MoveTemp(OnReady)))
	{
		Barrier->SetResolvedEncounterRetirement(
			MoveTemp(OnResolvedEncounterRetirement));
	}

	TSharedRef<FExitBattlePostRunBarrierState> Barrier;
};

FWacomExitBattlePostRunBarrierAutomationTestView::
	FWacomExitBattlePostRunBarrierAutomationTestView(
		TFunction<void()>&& OnResolvedEncounterRetirement,
		TFunction<void()>&& OnReady)
	: Impl(MakeUnique<FImpl>(
		MoveTemp(OnResolvedEncounterRetirement),
		MoveTemp(OnReady)))
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
