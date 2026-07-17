// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

class ABattleTriggerActor;

/** Non-reflected automation seam for the App-private ExitBattle barrier. */
struct WACOMAPP_API FWacomExitBattlePostRunBarrierAutomationTestView
{
	FWacomExitBattlePostRunBarrierAutomationTestView(
		ABattleTriggerActor& ResolvedEncounterTrigger,
		TFunction<void()>&& OnReady);
	~FWacomExitBattlePostRunBarrierAutomationTestView();

	FWacomExitBattlePostRunBarrierAutomationTestView(
		const FWacomExitBattlePostRunBarrierAutomationTestView&) = delete;
	FWacomExitBattlePostRunBarrierAutomationTestView& operator=(
		const FWacomExitBattlePostRunBarrierAutomationTestView&) = delete;

	void MarkReturnCompleted();
	void MarkExitBattlePostRunReady();

private:
	struct FImpl;
	TUniquePtr<FImpl> Impl;
};

#endif
