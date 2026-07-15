// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomRunPathBranchSelectionAutomationTestView.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Actors/WacomRunPathBranchTargetActor.h"
#include "GameFramework/WacomRunPathBranchSelectionController.h"

struct FWacomRunPathBranchSelectionAutomationTestView::FImpl
{
	FWacomRunPathBranchSelectionController Controller;
};

FWacomRunPathBranchSelectionAutomationTestView::
	FWacomRunPathBranchSelectionAutomationTestView()
	: Impl(MakeUnique<FImpl>())
{
}

FWacomRunPathBranchSelectionAutomationTestView::
	~FWacomRunPathBranchSelectionAutomationTestView() = default;

void FWacomRunPathBranchSelectionAutomationTestView::Initialize(
	AWacomPlayerController& Owner,
	const TArray<AWacomRunPathBranchTargetActor*>& Targets)
{
	TArray<TWeakObjectPtr<AWacomRunPathBranchTargetActor>> WeakTargets;
	for (AWacomRunPathBranchTargetActor* Target : Targets)
	{
		WeakTargets.Add(Target);
	}
	Impl->Controller.Initialize(Owner, WeakTargets);
}

void FWacomRunPathBranchSelectionAutomationTestView::Shutdown()
{
	Impl->Controller.Shutdown();
}

void FWacomRunPathBranchSelectionAutomationTestView::ApplyChoiceState(
	const int32 SnapshotVersion,
	const TArray<FName>& LegalEdgeIds)
{
	FWacomRunRouteChoiceState State;
	State.SnapshotVersion = SnapshotVersion;
	State.Mode = EWacomRunRouteChoiceMode::ChoiceRequired;
	State.LegalEdgeIds = LegalEdgeIds;
	Impl->Controller.ApplyRouteChoiceState(State);
}

void FWacomRunPathBranchSelectionAutomationTestView::SetPresentationEnabled(
	const bool bEnabled)
{
	Impl->Controller.SetPresentationEnabled(bEnabled);
}

bool FWacomRunPathBranchSelectionAutomationTestView::ShiftFocus(
	const int32 Direction)
{
	return Impl->Controller.ShiftFocus(Direction);
}

bool FWacomRunPathBranchSelectionAutomationTestView::ConfirmFocused()
{
	return Impl->Controller.ConfirmFocused();
}

bool FWacomRunPathBranchSelectionAutomationTestView::SelectTarget(AActor* Target)
{
	return Impl->Controller.TrySelectHitActor(Target);
}

bool FWacomRunPathBranchSelectionAutomationTestView::IsPresentationValid() const
{
	return Impl->Controller.IsPresentationValid();
}

FName FWacomRunPathBranchSelectionAutomationTestView::GetFocusedEdgeId() const
{
	return Impl->Controller.GetFocusedEdgeId();
}

#endif
