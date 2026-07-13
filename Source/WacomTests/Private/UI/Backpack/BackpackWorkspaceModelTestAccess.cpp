// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/BackpackWorkspaceModelTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceTypes.h"
#include "RunSession.h"

namespace
{
FWacomBackpackWorkspaceResolvedLayoutTestView ToTestView(
	const FWacomBackpackResolvedLayout& Layout)
{
	FWacomBackpackWorkspaceResolvedLayoutTestView View;
	View.CardCenter = Layout.CardCenter;
	View.AngleDegrees = Layout.AngleDegrees;
	View.LayerRank = Layout.LayerRank;
	return View;
}
}

FGuid FWacomBackpackWorkspaceModelTestAccess::NormalizeZoneOwner(
	EZoneKind Zone,
	FGuid OwnerInstanceId)
{
	return FWacomBackpackZoneKey::Make(Zone, OwnerInstanceId).OwnerInstanceId;
}

TArray<FWacomBackpackWorkspaceResolvedLayoutTestView>
FWacomBackpackWorkspaceModelTestAccess::BuildDefaultLayout(
	int32 CardCount,
	FVector2D WorkspaceSize,
	FVector2D CardSize,
	FVector2D Spacing,
	FVector2D Padding)
{
	TArray<FWacomBackpackWorkspaceResolvedLayoutTestView> Views;
	for (const FWacomBackpackResolvedLayout& Layout :
		FWacomBackpackWorkspaceLayoutSolver::BuildDefaultLayout(
			CardCount,
			WorkspaceSize,
			CardSize,
			Spacing,
			Padding))
	{
		Views.Add(ToTestView(Layout));
	}
	return Views;
}

FWacomBackpackWorkspaceResolvedLayoutTestView
FWacomBackpackWorkspaceModelTestAccess::ResolveManualLayout(
	FVector2D NormalizedPosition,
	float AngleDegrees,
	int32 LayerRank,
	FVector2D WorkspaceSize,
	FVector2D CardSize,
	float MinimumVisibleFraction)
{
	FWacomBackpackWorkspaceLayoutEntry Entry;
	Entry.NormalizedPosition = NormalizedPosition;
	Entry.AngleDegrees = AngleDegrees;
	Entry.LayerRank = LayerRank;
	Entry.bHasManualPlacement = true;
	return ToTestView(FWacomBackpackWorkspaceLayoutSolver::ResolveManualLayout(
		Entry,
		WorkspaceSize,
		CardSize,
		MinimumVisibleFraction));
}

int32 FWacomBackpackWorkspaceModelTestAccess::ArrangeAllAndCountRemainingManualLayouts(
	int32 LayoutCount)
{
	TMap<FGuid, FWacomBackpackWorkspaceLayoutEntry> Layouts;
	for (int32 Index = 0; Index < LayoutCount; ++Index)
	{
		FWacomBackpackWorkspaceLayoutEntry Entry;
		Entry.bHasManualPlacement = true;
		Entry.LayerRank = LayoutCount - Index;
		Layouts.Add(FGuid(Index + 1, 2, 3, 4), Entry);
	}
	FWacomBackpackWorkspaceLayoutSolver::ArrangeAll(Layouts);
	return Layouts.Num();
}

FWacomBackpackWorkspaceStateLifecycleTestView
FWacomBackpackWorkspaceModelTestAccess::RunStateLifecycleScenario()
{
	FWacomBackpackWorkspaceStateLifecycleTestView View;
	TStrongObjectPtr<URunSession> FirstRun(NewObject<URunSession>());
	TStrongObjectPtr<URunSession> SecondRun(NewObject<URunSession>());
	FWacomBackpackWorkspaceStateStore Store;
	const FWacomBackpackZoneKey BackpackZone = FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
	const FGuid ExistingCardId(1, 2, 3, 4);
	const FGuid NewCardId(5, 6, 7, 8);

	Store.BindToRun(FirstRun.Get());
	Store.SetActiveZone(BackpackZone);
	FWacomBackpackWorkspaceLayoutEntry Entry;
	Entry.bHasManualPlacement = true;
	Entry.NormalizedPosition = FVector2D(0.25f, 0.75f);
	Store.SetLayout(BackpackZone, ExistingCardId, Entry);

	Store.BindToRun(FirstRun.Get());
	View.bSameRunPreservedLayout = Store.FindLayout(BackpackZone, ExistingCardId) != nullptr;
	View.bActiveZonePreservedForSameRun = Store.GetActiveZone() == BackpackZone;

	Store.ReconcileZone(BackpackZone, MakeArrayView(&NewCardId, 1));
	View.bRemovedCardLayoutPruned = Store.FindLayout(BackpackZone, ExistingCardId) == nullptr;
	View.bNewCardHasNoManualLayout = Store.FindLayout(BackpackZone, NewCardId) == nullptr;

	Store.SetLayout(BackpackZone, NewCardId, Entry);
	Store.BindToRun(SecondRun.Get());
	View.bNewRunClearedLayouts = Store.GetManualLayoutCount(BackpackZone) == 0;
	View.bActiveZoneResetForNewRun = !Store.HasActiveZone();
	return View;
}

#endif
