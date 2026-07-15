// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

class AActor;
class AWacomPlayerController;
class AWacomRunPathBranchTargetActor;

/** Narrow non-reflected automation seam for the App-private branch selector. */
struct WACOMAPP_API FWacomRunPathBranchSelectionAutomationTestView
{
	FWacomRunPathBranchSelectionAutomationTestView();
	~FWacomRunPathBranchSelectionAutomationTestView();

	FWacomRunPathBranchSelectionAutomationTestView(
		const FWacomRunPathBranchSelectionAutomationTestView&) = delete;
	FWacomRunPathBranchSelectionAutomationTestView& operator=(
		const FWacomRunPathBranchSelectionAutomationTestView&) = delete;

	void Initialize(
		AWacomPlayerController& Owner,
		const TArray<AWacomRunPathBranchTargetActor*>& Targets);
	void Shutdown();
	void ApplyChoiceState(int32 SnapshotVersion, const TArray<FName>& LegalEdgeIds);
	void SetPresentationEnabled(bool bEnabled);
	bool ShiftFocus(int32 Direction);
	bool ConfirmFocused();
	bool SelectTarget(AActor* Target);
	bool IsPresentationValid() const;
	FName GetFocusedEdgeId() const;

private:
	struct FImpl;
	TUniquePtr<FImpl> Impl;
};

#endif
