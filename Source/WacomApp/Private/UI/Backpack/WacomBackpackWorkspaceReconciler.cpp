// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceReconciler.h"

#include "Components/CanvasPanel.h"
#include "UI/Backpack/WacomBackpackDeckCardListReconciler.h"
#include "UI/Backpack/WacomBackpackScreenPresenter.h"
#include "UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"
#include "UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"

namespace
{
void AppendPhysicalCards(
	TConstArrayView<FRunStorageCardView> Source,
	TArray<FWacomBackpackDeckCardListItem>& OutDesired)
{
	for (const FRunStorageCardView& CardView : Source)
	{
		FWacomBackpackDeckCardListItem Item;
		Item.CardView = CardView;
		Item.Role = EWacomBackpackDeckCardListReuseRole::PhysicalList;
		OutDesired.Add(MoveTemp(Item));
	}
}

TArray<FWacomBackpackDeckCardListItem> BuildDesiredCards(
	const FRunBackpackStorageSnapshot& Snapshot,
	const FWacomBackpackZoneKey& ActiveZone)
{
	TArray<FWacomBackpackDeckCardListItem> Desired;
	switch (ActiveZone.Zone)
	{
	case EZoneKind::Backpack:
		AppendPhysicalCards(Snapshot.Flux.ContentCards, Desired);
		break;
	case EZoneKind::BattleDeck:
		AppendPhysicalCards(Snapshot.BattleDeckPhysicalCards, Desired);
		for (const FRunStorageCardView& ProjectedView : Snapshot.BattleDeckProjectedCards)
		{
			FWacomBackpackDeckCardListItem Item;
			Item.CardView = ProjectedView;
			Item.Role = EWacomBackpackDeckCardListReuseRole::BattleDeckProjected;
			Item.ProjectedBadgeText = UWacomBackpackScreenPresenter::BuildBattleDeckProjectedFromBadgeText(
				ProjectedView,
				Snapshot);
			Desired.Add(MoveTemp(Item));
		}
		break;
	case EZoneKind::BurdenZone:
		AppendPhysicalCards(Snapshot.BurdenCards, Desired);
		break;
	case EZoneKind::SpecialZone:
		if (const FRunSpecialStorageView* SpecialView = Snapshot.SpecialZones.FindByPredicate(
			[&ActiveZone](const FRunSpecialStorageView& Candidate)
			{
				return Candidate.OwnerCard.Instance.InstanceId == ActiveZone.OwnerInstanceId;
			}))
		{
			AppendPhysicalCards(MakeArrayView(&SpecialView->OwnerCard, 1), Desired);
			AppendPhysicalCards(SpecialView->ContentCards, Desired);
		}
		break;
	default:
		break;
	}
	return Desired;
}
}

void FWacomBackpackWorkspaceReconciler::Reconcile(
	UWacomBackpackWorkspaceWidget& Workspace,
	const FRunBackpackStorageSnapshot& Snapshot,
	const FWacomBackpackZoneKey& ActiveZone,
	FWacomBackpackWorkspaceStateStore& StateStore,
	const UWacomBackpackWorkspaceStyle* Style,
	TFunctionRef<UWacomDeckCardWidget*(const FRunStorageCardView&)> CreateWidget,
	TFunctionRef<void(UWacomDeckCardWidget*)> OnRemovedWidget,
	TArray<TObjectPtr<UWacomDeckCardWidget>>* OutOrderedWidgets)
{
	UCanvasPanel* CardCanvas = Workspace.GetCardCanvas();
	if (!CardCanvas)
	{
		return;
	}

	const UWacomBackpackWorkspaceStyle* ResolvedStyle = Style
		? Style
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	TArray<FWacomBackpackDeckCardListItem> Desired = BuildDesiredCards(Snapshot, ActiveZone);
	TArray<FGuid> VisibleIds;
	VisibleIds.Reserve(Desired.Num());
	for (const FWacomBackpackDeckCardListItem& Item : Desired)
	{
		VisibleIds.Add(Item.CardView.Instance.InstanceId);
	}
	StateStore.ReconcileZone(ActiveZone, VisibleIds);

	TArray<TObjectPtr<UWacomDeckCardWidget>> OrderedWidgets;
	FWacomBackpackDeckCardListReconciler::Reconcile(
		CardCanvas,
		Desired,
		CreateWidget,
		OnRemovedWidget,
		&OrderedWidgets);

	const FVector2D WorkspaceSize = Workspace.GetLayoutSpaceSize();
	const TArray<FWacomBackpackResolvedLayout> Defaults = FWacomBackpackWorkspaceLayoutSolver::BuildDefaultLayout(
		OrderedWidgets.Num(),
		WorkspaceSize,
		ResolvedStyle->CardRenderSize,
		ResolvedStyle->DefaultCardSpacing,
		ResolvedStyle->WorkspacePadding);

	for (int32 Index = 0; Index < OrderedWidgets.Num(); ++Index)
	{
		UWacomDeckCardWidget* CardWidget = OrderedWidgets[Index];
		if (!CardWidget || !Defaults.IsValidIndex(Index))
		{
			continue;
		}

		FWacomBackpackResolvedLayout Resolved = Defaults[Index];
		if (const FWacomBackpackWorkspaceLayoutEntry* Manual = StateStore.FindLayout(
			ActiveZone,
			CardWidget->GetCardInstanceId()))
		{
			if (Manual->bHasManualPlacement)
			{
				Resolved = FWacomBackpackWorkspaceLayoutSolver::ResolveManualLayout(
					*Manual,
					WorkspaceSize,
					ResolvedStyle->CardRenderSize,
					ResolvedStyle->MinimumVisibleFraction);
			}
		}
		Workspace.ApplyCardLayout(
			*CardWidget,
			Resolved.CardCenter,
			ResolvedStyle->CardRenderSize,
			Resolved.AngleDegrees,
			Resolved.LayerRank);
		CardWidget->SetMoveEnabled(Desired[Index].Role != EWacomBackpackDeckCardListReuseRole::BattleDeckProjected);
	}

	Workspace.SetActiveZone(ActiveZone.Zone, ActiveZone.OwnerInstanceId);
	Workspace.SetManualLayoutCount(StateStore.GetManualLayoutCount(ActiveZone));
	Workspace.SetEmptyStateVisible(OrderedWidgets.IsEmpty());
	if (OutOrderedWidgets)
	{
		*OutOrderedWidgets = MoveTemp(OrderedWidgets);
	}
}
