// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceReconciler.h"

#include "Components/CanvasPanel.h"
#include "Components/PanelWidget.h"
#include "UI/Backpack/WacomBackpackDeckCardListReconciler.h"
#include "UI/Backpack/WacomBackpackScreenPresenter.h"
#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"
#include "UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"

#define LOCTEXT_NAMESPACE "WacomBackpackWorkspaceReconciler"

namespace
{
const FRunSpecialStorageView* FindSpecial(
	const FRunBackpackStorageSnapshot& Snapshot,
	FGuid OwnerInstanceId)
{
	return Snapshot.SpecialZones.FindByPredicate(
		[OwnerInstanceId](const FRunSpecialStorageView& Candidate)
		{
			return Candidate.OwnerCard.Instance.InstanceId == OwnerInstanceId;
		});
}

bool IsCarriedPhysicalCard(
	const FWacomBackpackWorkspaceInteractionModel* InteractionModel,
	const FWacomBackpackZoneKey& Zone,
	FGuid InstanceId)
{
	if (!InteractionModel || !InteractionModel->IsCarrying())
	{
		return false;
	}
	const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
	return Carry.SourceZone == Zone && Carry.RemainingInstanceIds.Contains(InstanceId);
}

void AddCardItem(
	const FRunStorageCardView& Card,
	const FWacomBackpackZoneKey& DisplayZone,
	EWacomBackpackDeckCardListReuseRole Role,
	bool bInteractive,
	EWacomBackpackWorkspaceCardReadOnlyKind ReadOnlyKind,
	TArray<FWacomBackpackDeckCardListItem>& OutDesired,
	const FText& Badge = FText::GetEmpty())
{
	FWacomBackpackDeckCardListItem Item;
	Item.CardView = Card;
	Item.DisplayZone = DisplayZone;
	Item.Role = Role;
	Item.bWorkspaceInteractive = bInteractive;
	Item.ReadOnlyKind = ReadOnlyKind;
	Item.ProjectedBadgeText = Badge;
	OutDesired.Add(MoveTemp(Item));
}

void AppendPileCards(
	const FRunBackpackStorageSnapshot& Snapshot,
	const FWacomBackpackZonePileView& Pile,
	const FWacomBackpackWorkspaceInteractionModel* InteractionModel,
	TArray<FWacomBackpackDeckCardListItem>& OutDesired)
{
	const FWacomBackpackZoneKey Zone = FWacomBackpackZoneKey::Make(
		Pile.Zone, Pile.OwnerInstanceId);
	auto PhysicalInteractive = [&](const FRunStorageCardView& Card)
	{
		return Pile.bExpanded
			|| IsCarriedPhysicalCard(InteractionModel, Zone, Card.Instance.InstanceId);
	};

	switch (Pile.Zone)
	{
	case EZoneKind::BattleDeck:
		for (const FRunStorageCardView& Card : Snapshot.BattleDeckPhysicalCards)
		{
			AddCardItem(
				Card, Zone, EWacomBackpackDeckCardListReuseRole::PhysicalList,
				PhysicalInteractive(Card), EWacomBackpackWorkspaceCardReadOnlyKind::None,
				OutDesired);
		}
		for (const FRunStorageCardView& Card : Snapshot.BattleDeckProjectedCards)
		{
			AddCardItem(
				Card, Zone, EWacomBackpackDeckCardListReuseRole::BattleDeckProjected,
				false, EWacomBackpackWorkspaceCardReadOnlyKind::BattleProjection,
				OutDesired,
				UWacomBackpackScreenPresenter::BuildBattleDeckProjectedFromBadgeText(Card, Snapshot));
		}
		break;
	case EZoneKind::SpecialZone:
		if (const FRunSpecialStorageView* Special = FindSpecial(Snapshot, Pile.OwnerInstanceId))
		{
			AddCardItem(
				Special->OwnerCard, Zone, EWacomBackpackDeckCardListReuseRole::SpecialOwner,
				false, EWacomBackpackWorkspaceCardReadOnlyKind::SpecialOwner,
				OutDesired, LOCTEXT("SpecialOwnerBadge", "主卡"));
			for (const FRunStorageCardView& Card : Special->ContentCards)
			{
				AddCardItem(
					Card, Zone, EWacomBackpackDeckCardListReuseRole::SpecialContent,
					PhysicalInteractive(Card), EWacomBackpackWorkspaceCardReadOnlyKind::None,
					OutDesired);
			}
		}
		break;
	case EZoneKind::BurdenZone:
		for (const FRunStorageCardView& Card : Snapshot.BurdenCards)
		{
			AddCardItem(
				Card, Zone, EWacomBackpackDeckCardListReuseRole::PhysicalList,
				false, EWacomBackpackWorkspaceCardReadOnlyKind::BurdenLocked,
				OutDesired);
		}
		break;
	default:
		break;
	}
}

FSlateRect CardBounds(
	TConstArrayView<FWacomBackpackResolvedLayout> Layouts,
	FVector2D CardSize,
	float HoverLift)
{
	if (Layouts.IsEmpty())
	{
		return FSlateRect();
	}
	FSlateRect Result(
		Layouts[0].CardCenter.X - CardSize.X * 0.5f,
		Layouts[0].CardCenter.Y - CardSize.Y * 0.5f - HoverLift,
		Layouts[0].CardCenter.X + CardSize.X * 0.5f,
		Layouts[0].CardCenter.Y + CardSize.Y * 0.5f);
	for (const FWacomBackpackResolvedLayout& Layout : Layouts)
	{
		Result.Left = FMath::Min(Result.Left, Layout.CardCenter.X - CardSize.X * 0.5f);
		Result.Top = FMath::Min(Result.Top, Layout.CardCenter.Y - CardSize.Y * 0.5f - HoverLift);
		Result.Right = FMath::Max(Result.Right, Layout.CardCenter.X + CardSize.X * 0.5f);
		Result.Bottom = FMath::Max(Result.Bottom, Layout.CardCenter.Y + CardSize.Y * 0.5f);
	}
	return Result;
}

struct FPileGroup
{
	FWacomBackpackZoneKey Zone;
	int32 StartIndex = 0;
	int32 CardCount = 0;
	FWacomBackpackResolvedPileContentLayout Layout;
};
}

void FWacomBackpackWorkspaceReconciler::Reconcile(
	UWacomBackpackWorkspaceWidget& Workspace,
	const FRunBackpackStorageSnapshot& Snapshot,
	FWacomBackpackWorkspaceStateStore& StateStore,
	const FWacomBackpackWorkspaceInteractionModel* InteractionModel,
	const UWacomBackpackWorkspaceStyle* Style,
	TFunctionRef<UWacomDeckCardWidget*(const FRunStorageCardView&)> CreateWidget,
	TFunctionRef<void(UWacomDeckCardWidget*)> OnRemovedWidget,
	TArray<TObjectPtr<UWacomDeckCardWidget>>* OutOrderedWidgets)
{
	UCanvasPanel* StaticCanvas = Workspace.GetCardCanvas();
	UCanvasPanel* CarryCanvas = Workspace.GetCarryCanvas();
	UCanvasPanel* CarryActiveCanvas = Workspace.GetCarryActiveCanvas();
	UCanvasPanel* SettlementCanvas = Workspace.GetSettlementCanvas();
	if (!StaticCanvas)
	{
		return;
	}
	const UWacomBackpackWorkspaceStyle* ResolvedStyle = Style
		? Style
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const FVector2D WorkspaceSize = Workspace.GetLayoutSpaceSize();
	const FVector2D HeaderSize(
		FMath::Max(260.0f, ResolvedStyle->PileCollapsedSize.X),
		48.0f);
	const FVector2D DefaultPileFootprint(
		HeaderSize.X,
		HeaderSize.Y + ResolvedStyle->CardRenderSize.Y + 24.0f);

	TArray<FWacomBackpackZoneKey> VisiblePileKeys;
	VisiblePileKeys.Add(FWacomBackpackZoneKey::Make(EZoneKind::BattleDeck));
	for (const FRunSpecialStorageView& Special : Snapshot.SpecialZones)
	{
		if (Special.OwnerCard.Instance.InstanceId.IsValid())
		{
			VisiblePileKeys.Add(FWacomBackpackZoneKey::Make(
				EZoneKind::SpecialZone, Special.OwnerCard.Instance.InstanceId));
		}
	}
	if (!Snapshot.BurdenCards.IsEmpty())
	{
		VisiblePileKeys.Add(FWacomBackpackZoneKey::Make(EZoneKind::BurdenZone));
	}
	StateStore.ReconcilePiles(VisiblePileKeys);

	const TOptional<FWacomBackpackZoneKey>& ExpandedOptional = StateStore.GetExpandedPile();
	const bool bHasExpanded = ExpandedOptional.IsSet();
	const FWacomBackpackZoneKey Expanded = bHasExpanded
		? ExpandedOptional.GetValue()
		: FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
	TArray<FWacomBackpackZonePileView> PileViews =
		UWacomBackpackScreenPresenter::BuildWorkspacePileViews(
			Snapshot, Expanded.Zone, Expanded.OwnerInstanceId, bHasExpanded);

	TArray<FWacomBackpackDeckCardListItem> Desired;
	Desired.Reserve(
		Snapshot.Flux.ContentCards.Num()
		+ Snapshot.BattleDeckPhysicalCards.Num()
		+ Snapshot.BattleDeckProjectedCards.Num()
		+ Snapshot.BurdenCards.Num()
		+ Snapshot.SpecialZones.Num());
	const FWacomBackpackZoneKey FluxZone = FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
	for (const FRunStorageCardView& Card : Snapshot.Flux.ContentCards)
	{
		AddCardItem(
			Card, FluxZone, EWacomBackpackDeckCardListReuseRole::PhysicalList,
			true, EWacomBackpackWorkspaceCardReadOnlyKind::None, Desired);
	}
	const int32 FluxCardCount = Desired.Num();

	TArray<FSlateRect> PileFrameRects;
	TArray<FSlateRect> PileHeaderRects;
	TArray<int32> PileLayerRanks;
	TArray<FSlateRect> OccupiedHeaders;
	TArray<FSlateRect> Obstacles;
	TArray<FPileGroup> PileGroups;
	int32 MovablePileIndex = 0;
	for (const FWacomBackpackZonePileView& Pile : PileViews)
	{
		const FWacomBackpackZoneKey Key = FWacomBackpackZoneKey::Make(
			Pile.Zone, Pile.OwnerInstanceId);
		FVector2D HeaderTopLeft;
		int32 LayerRank = 0;
		if (!Pile.bMovable)
		{
			HeaderTopLeft = FVector2D(
				WorkspaceSize.X - ResolvedStyle->PileEdgeMarginPixels - HeaderSize.X,
				ResolvedStyle->PileEdgeMarginPixels);
			LayerRank = 100000;
		}
		else
		{
			const FWacomBackpackResolvedPileLayout Default =
				FWacomBackpackWorkspaceLayoutSolver::BuildDefaultPileLayout(
					MovablePileIndex++, WorkspaceSize, DefaultPileFootprint,
					ResolvedStyle->PileEdgeMarginPixels);
			HeaderTopLeft = Default.TopLeft;
			LayerRank = Default.LayerRank;
			if (const FWacomBackpackWorkspacePileLayoutEntry* Stored = StateStore.FindPileLayout(Key))
			{
				LayerRank = Stored->LayerRank;
				if (Stored->bHasManualPlacement)
				{
					HeaderTopLeft = FWacomBackpackWorkspaceLayoutSolver::ResolvePileTopLeft(
						*Stored, WorkspaceSize, HeaderSize, ResolvedStyle->PileEdgeMarginPixels);
				}
			}
			HeaderTopLeft = FWacomBackpackWorkspaceLayoutSolver::ResolvePileHeaderOverlap(
				HeaderTopLeft, WorkspaceSize, HeaderSize, HeaderSize,
				ResolvedStyle->PileSnapGridPixels, ResolvedStyle->PileEdgeMarginPixels,
				OccupiedHeaders);
		}

		const int32 StartIndex = Desired.Num();
		AppendPileCards(Snapshot, Pile, InteractionModel, Desired);
		const int32 PileCardCount = Desired.Num() - StartIndex;
		FWacomBackpackResolvedPileContentLayout Layout =
			FWacomBackpackWorkspaceLayoutSolver::BuildPileContentLayout(
				PileCardCount,
				HeaderTopLeft,
				HeaderSize,
				WorkspaceSize,
				ResolvedStyle->CardRenderSize,
				Pile.bExpanded,
				ResolvedStyle->PileCollapsedExposurePixels,
				ResolvedStyle->AdaptiveStripExposurePixels,
				ResolvedStyle->AdaptiveStripFocusSeparationPixels,
				ResolvedStyle->PileEdgeMarginPixels,
				ResolvedStyle->ExpandedCardHoverLiftPixels);
		OccupiedHeaders.Add(Layout.HeaderRect);
		PileFrameRects.Add(Layout.FrameRect);
		PileHeaderRects.Add(Layout.HeaderRect);
		PileLayerRanks.Add(LayerRank);
		Obstacles.Add(Layout.FrameRect);
		FPileGroup& Group = PileGroups.AddDefaulted_GetRef();
		Group.Zone = Key;
		Group.StartIndex = StartIndex;
		Group.CardCount = PileCardCount;
		Group.Layout = MoveTemp(Layout);
	}
	Obstacles.Emplace(
		FMath::Max(0.0f, WorkspaceSize.X - 220.0f - ResolvedStyle->PileEdgeMarginPixels),
		FMath::Max(0.0f, WorkspaceSize.Y - 150.0f - ResolvedStyle->PileEdgeMarginPixels),
		WorkspaceSize.X - ResolvedStyle->PileEdgeMarginPixels,
		WorkspaceSize.Y - ResolvedStyle->PileEdgeMarginPixels);
	Workspace.ReconcilePiles(PileViews, PileFrameRects, PileHeaderRects, PileLayerRanks);

	TArray<FGuid> FluxIds;
	for (const FRunStorageCardView& Card : Snapshot.Flux.ContentCards)
	{
		FluxIds.Add(Card.Instance.InstanceId);
	}
	StateStore.ReconcileZone(FluxZone, FluxIds);

	TArray<TObjectPtr<UWacomDeckCardWidget>> OrderedWidgets;
	TArray<UPanelWidget*> SearchPanels;
	SearchPanels.Add(StaticCanvas);
	if (CarryCanvas)
	{
		SearchPanels.Add(CarryCanvas);
	}
	if (CarryActiveCanvas)
	{
		SearchPanels.Add(CarryActiveCanvas);
	}
	if (SettlementCanvas)
	{
		// Settlement retains the authoritative Widget while its outer pose converges.
		// Excluding it makes the next Snapshot refresh create a duplicate static card.
		SearchPanels.Add(SettlementCanvas);
	}
	FWacomBackpackDeckCardListReconciler::ReconcileAcrossPanels(
		SearchPanels,
		StaticCanvas,
		Desired,
		[&Workspace](const UWacomDeckCardWidget* Widget)
		{
			return Workspace.ShouldPreserveCardParent(Widget);
		},
		CreateWidget,
		OnRemovedWidget,
		&OrderedWidgets);

	const TArray<FWacomBackpackResolvedLayout> FluxDefaults =
		FWacomBackpackWorkspaceLayoutSolver::BuildDefaultLayoutAvoidingRectangles(
			FluxCardCount,
			WorkspaceSize,
			ResolvedStyle->CardRenderSize,
			ResolvedStyle->DefaultCardSpacing,
			ResolvedStyle->WorkspacePadding,
			Obstacles);
	FSlateRect ExpandedBounds;
	bool bHasExpandedBounds = false;
	for (int32 Index = 0; Index < OrderedWidgets.Num(); ++Index)
	{
		UWacomDeckCardWidget* CardWidget = OrderedWidgets[Index];
		if (!CardWidget || !Desired.IsValidIndex(Index))
		{
			continue;
		}
		FWacomBackpackResolvedLayout Resolved;
		bool bFoundLayout = false;
		if (Index < FluxCardCount && FluxDefaults.IsValidIndex(Index))
		{
			Resolved = FluxDefaults[Index];
			bFoundLayout = true;
			if (const FWacomBackpackWorkspaceLayoutEntry* Manual =
				StateStore.FindLayout(FluxZone, CardWidget->GetCardInstanceId()))
			{
				if (Manual->bHasManualPlacement)
				{
					Resolved = FWacomBackpackWorkspaceLayoutSolver::ResolveManualLayout(
						*Manual, WorkspaceSize, ResolvedStyle->CardRenderSize,
						ResolvedStyle->MinimumVisibleFraction);
				}
			}
		}
		else
		{
			for (const FPileGroup& Group : PileGroups)
			{
				const int32 LocalIndex = Index - Group.StartIndex;
				if (LocalIndex >= 0 && LocalIndex < Group.CardCount
					&& Group.Layout.Cards.IsValidIndex(LocalIndex))
				{
					Resolved = Group.Layout.Cards[LocalIndex];
					Resolved.LayerRank += 3000;
					bFoundLayout = true;
					if (bHasExpanded && Group.Zone == Expanded)
					{
						ExpandedBounds = Group.Layout.FrameRect;
						bHasExpandedBounds = !Group.Layout.Cards.IsEmpty();
					}
					break;
				}
			}
		}
		if (!bFoundLayout)
		{
			continue;
		}
		if (!Workspace.HasCardBaseLayout(*CardWidget))
		{
			Workspace.PrimeCardBaseLayout(
				*CardWidget, Resolved.CardCenter, ResolvedStyle->CardRenderSize,
				Resolved.AngleDegrees, Resolved.LayerRank);
		}
		Workspace.ApplyCardBaseLayout(
			*CardWidget, Resolved.CardCenter, ResolvedStyle->CardRenderSize,
			Resolved.AngleDegrees, Resolved.LayerRank);
		CardWidget->SetWorkspaceInteractionEnabled(Desired[Index].bWorkspaceInteractive);
		CardWidget->SetWorkspaceReadOnlyKind(Desired[Index].ReadOnlyKind);
	}

	bool bAppliedExpandedFocusContract = false;
	if (bHasExpanded)
	{
		for (const FPileGroup& Group : PileGroups)
		{
			if (Group.Zone != Expanded || Group.CardCount <= 0)
			{
				continue;
			}
			TArray<FWacomBackpackExpandedPileFocusCard> FocusCards;
			FocusCards.Reserve(Group.CardCount);
			for (int32 LocalIndex = 0; LocalIndex < Group.CardCount; ++LocalIndex)
			{
				const int32 OrderedIndex = Group.StartIndex + LocalIndex;
				if (!OrderedWidgets.IsValidIndex(OrderedIndex)
					|| !Group.Layout.Cards.IsValidIndex(LocalIndex)
					|| !Group.Layout.FocusHitBands.IsValidIndex(LocalIndex))
				{
					continue;
				}
				FWacomBackpackExpandedPileFocusCard& FocusCard = FocusCards.AddDefaulted_GetRef();
				FocusCard.Card = OrderedWidgets[OrderedIndex];
				FocusCard.NeutralCenter = Group.Layout.Cards[LocalIndex].CardCenter;
				FocusCard.NeutralAngleDegrees = Group.Layout.Cards[LocalIndex].AngleDegrees;
				FocusCard.NeutralLayerRank = Group.Layout.Cards[LocalIndex].LayerRank + 3000;
				FocusCard.NeutralHitBand = Group.Layout.FocusHitBands[LocalIndex];
				FocusCard.CurrentHitBand = FocusCard.NeutralHitBand;
			}
			Workspace.SetExpandedPileFocusContract(
				Expanded.Zone,
				Expanded.OwnerInstanceId,
				Group.Layout.HeaderRect,
				Group.Layout.FocusCorridorRect,
				FocusCards);
			bAppliedExpandedFocusContract = true;
			break;
		}
	}
	if (!bAppliedExpandedFocusContract)
	{
		Workspace.SetExpandedPileFocusContract(
			EZoneKind::Backpack, FGuid(), FSlateRect(), FSlateRect(), {});
	}

	if (bHasExpanded && bHasExpandedBounds)
	{
		Workspace.SetExpandedContentBounds(Expanded.Zone, Expanded.OwnerInstanceId, ExpandedBounds);
	}
	else
	{
		Workspace.SetExpandedContentBounds(EZoneKind::Backpack, FGuid(), FSlateRect());
	}
	Workspace.SetPresentedContentZone(Expanded.Zone, Expanded.OwnerInstanceId);
	Workspace.SetManualLayoutCount(StateStore.GetManualLayoutCount(FluxZone));
	Workspace.SetEmptyStateVisible(Snapshot.Flux.ContentCards.IsEmpty());
	if (OutOrderedWidgets)
	{
		*OutOrderedWidgets = MoveTemp(OrderedWidgets);
	}
}

#undef LOCTEXT_NAMESPACE
