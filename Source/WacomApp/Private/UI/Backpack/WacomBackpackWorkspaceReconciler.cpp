// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceReconciler.h"

#include "Components/CanvasPanel.h"
#include "UI/Backpack/WacomBackpackDeckCardListReconciler.h"
#include "UI/Backpack/WacomBackpackScreenPresenter.h"
#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
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

const FRunStorageCardView* FindPhysicalCard(
	const FRunBackpackStorageSnapshot& Snapshot,
	const FWacomBackpackZoneKey& Source,
	FGuid InstanceId)
{
	auto FindById = [InstanceId](TConstArrayView<FRunStorageCardView> Cards)
	{
		return Cards.FindByPredicate(
			[InstanceId](const FRunStorageCardView& Card)
			{
				return Card.Instance.InstanceId == InstanceId;
			});
	};
	switch (Source.Zone)
	{
	case EZoneKind::Backpack:
		return FindById(Snapshot.Flux.ContentCards);
	case EZoneKind::BattleDeck:
		return FindById(Snapshot.BattleDeckPhysicalCards);
	case EZoneKind::BurdenZone:
		return FindById(Snapshot.BurdenCards);
	case EZoneKind::SpecialZone:
		if (const FRunSpecialStorageView* Special = FindSpecial(Snapshot, Source.OwnerInstanceId))
		{
			return FindById(Special->ContentCards);
		}
		break;
	default:
		break;
	}
	return nullptr;
}

void AppendExpandedCards(
	const FRunBackpackStorageSnapshot& Snapshot,
	const FWacomBackpackZoneKey& Expanded,
	TArray<FWacomBackpackDeckCardListItem>& OutDesired)
{
	switch (Expanded.Zone)
	{
	case EZoneKind::BattleDeck:
		AppendPhysicalCards(Snapshot.BattleDeckPhysicalCards, OutDesired);
		for (const FRunStorageCardView& ProjectedView : Snapshot.BattleDeckProjectedCards)
		{
			FWacomBackpackDeckCardListItem Item;
			Item.CardView = ProjectedView;
			Item.Role = EWacomBackpackDeckCardListReuseRole::BattleDeckProjected;
			Item.ProjectedBadgeText = UWacomBackpackScreenPresenter::BuildBattleDeckProjectedFromBadgeText(
				ProjectedView,
				Snapshot);
			OutDesired.Add(MoveTemp(Item));
		}
		break;
	case EZoneKind::SpecialZone:
		if (const FRunSpecialStorageView* Special = FindSpecial(Snapshot, Expanded.OwnerInstanceId))
		{
			// 主卡只作为牌堆身份封面；实体仍由 Backpack/BattleDeck 拥有。
			AppendPhysicalCards(Special->ContentCards, OutDesired);
		}
		break;
	case EZoneKind::BurdenZone:
		AppendPhysicalCards(Snapshot.BurdenCards, OutDesired);
		break;
	default:
		break;
	}
}

bool SameIdentity(const FWacomBackpackZoneKey& A, const FWacomBackpackZoneKey& B)
{
	return A == B;
}

FSlateRect MakeRect(FVector2D TopLeft, FVector2D Size)
{
	return FSlateRect(TopLeft.X, TopLeft.Y, TopLeft.X + Size.X, TopLeft.Y + Size.Y);
}
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
	UCanvasPanel* Canvas = Workspace.GetCardCanvas();
	if (!Canvas)
	{
		return;
	}
	const UWacomBackpackWorkspaceStyle* ResolvedStyle = Style
		? Style
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const FVector2D WorkspaceSize = Workspace.GetLayoutSpaceSize();

	// 先按 Snapshot 清理已消失的牌堆和展开状态。
	TArray<FWacomBackpackZoneKey> VisiblePileKeys;
	VisiblePileKeys.Add(FWacomBackpackZoneKey::Make(EZoneKind::BattleDeck));
	for (const FRunSpecialStorageView& Special : Snapshot.SpecialZones)
	{
		if (Special.OwnerCard.Instance.InstanceId.IsValid())
		{
			VisiblePileKeys.Add(FWacomBackpackZoneKey::Make(
				EZoneKind::SpecialZone,
				Special.OwnerCard.Instance.InstanceId));
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
			Snapshot,
			Expanded.Zone,
			Expanded.OwnerInstanceId,
			bHasExpanded);

	TArray<FVector2D> PileTopLefts;
	TArray<int32> PileLayerRanks;
	TArray<FSlateRect> Obstacles;
	TArray<FSlateRect> OccupiedPileHeaders;
	PileTopLefts.Reserve(PileViews.Num());
	PileLayerRanks.Reserve(PileViews.Num());
	Obstacles.Reserve(PileViews.Num() + 1);
	if (!Snapshot.BurdenCards.IsEmpty())
	{
		const FVector2D FixedBurdenTopLeft = FWacomBackpackWorkspaceLayoutSolver::SnapPileTopLeft(
			FVector2D(
				WorkspaceSize.X - ResolvedStyle->PileEdgeMarginPixels - ResolvedStyle->PileCollapsedSize.X,
				ResolvedStyle->PileEdgeMarginPixels),
			WorkspaceSize,
			ResolvedStyle->PileCollapsedSize,
			1.0f,
			ResolvedStyle->PileEdgeMarginPixels);
		OccupiedPileHeaders.Emplace(
			FixedBurdenTopLeft.X,
			FixedBurdenTopLeft.Y,
			FixedBurdenTopLeft.X + ResolvedStyle->PileCollapsedSize.X,
			FixedBurdenTopLeft.Y + 48.0f);
	}
	int32 MovablePileIndex = 0;
	for (const FWacomBackpackZonePileView& Pile : PileViews)
	{
		const FWacomBackpackZoneKey Key = FWacomBackpackZoneKey::Make(
			Pile.Zone,
			Pile.OwnerInstanceId);
		FVector2D TopLeft;
		int32 LayerRank = 0;
		if (!Pile.bMovable)
		{
			TopLeft = FVector2D(
				WorkspaceSize.X - ResolvedStyle->PileEdgeMarginPixels - ResolvedStyle->PileCollapsedSize.X,
				ResolvedStyle->PileEdgeMarginPixels);
			TopLeft = FWacomBackpackWorkspaceLayoutSolver::SnapPileTopLeft(
				TopLeft,
				WorkspaceSize,
				ResolvedStyle->PileCollapsedSize,
				1.0f,
				ResolvedStyle->PileEdgeMarginPixels);
			LayerRank = 100000;
		}
		else
		{
			const FWacomBackpackResolvedPileLayout Default =
				FWacomBackpackWorkspaceLayoutSolver::BuildDefaultPileLayout(
					MovablePileIndex++,
					WorkspaceSize,
					ResolvedStyle->PileCollapsedSize,
					ResolvedStyle->PileEdgeMarginPixels);
			TopLeft = Default.TopLeft;
			LayerRank = Default.LayerRank;
			if (const FWacomBackpackWorkspacePileLayoutEntry* Stored = StateStore.FindPileLayout(Key))
			{
				LayerRank = Stored->LayerRank;
				if (Stored->bHasManualPlacement)
				{
					TopLeft = FWacomBackpackWorkspaceLayoutSolver::ResolvePileTopLeft(
						*Stored,
						WorkspaceSize,
						ResolvedStyle->PileCollapsedSize,
						ResolvedStyle->PileEdgeMarginPixels);
				}
			}
			TopLeft = FWacomBackpackWorkspaceLayoutSolver::ResolvePileHeaderOverlap(
				TopLeft,
				WorkspaceSize,
				ResolvedStyle->PileCollapsedSize,
				FVector2D(ResolvedStyle->PileCollapsedSize.X, 48.0f),
				ResolvedStyle->PileSnapGridPixels,
				ResolvedStyle->PileEdgeMarginPixels,
				OccupiedPileHeaders);
			OccupiedPileHeaders.Emplace(
				TopLeft.X,
				TopLeft.Y,
				TopLeft.X + ResolvedStyle->PileCollapsedSize.X,
				TopLeft.Y + 48.0f);
		}
		PileTopLefts.Add(TopLeft);
		PileLayerRanks.Add(LayerRank);
		Obstacles.Add(MakeRect(TopLeft, ResolvedStyle->PileCollapsedSize));
	}
	// 固定销毁区占据工作台右下角，整理通量卡时避开其交互区域。
	Obstacles.Add(MakeRect(
		FVector2D(
			FMath::Max(0.0f, WorkspaceSize.X - 220.0f - ResolvedStyle->PileEdgeMarginPixels),
			FMath::Max(0.0f, WorkspaceSize.Y - 150.0f - ResolvedStyle->PileEdgeMarginPixels)),
		FVector2D(220.0f, 150.0f)));
	Workspace.ReconcilePiles(PileViews, PileTopLefts, PileLayerRanks);

	TArray<FWacomBackpackDeckCardListItem> FluxDesired;
	AppendPhysicalCards(Snapshot.Flux.ContentCards, FluxDesired);
	TArray<FWacomBackpackDeckCardListItem> ExpandedDesired;
	if (bHasExpanded)
	{
		AppendExpandedCards(Snapshot, Expanded, ExpandedDesired);
	}
	TArray<FWacomBackpackDeckCardListItem> HiddenCarryDesired;

	// 自动展开目标牌堆时，来源牌堆会收起；携带中的物理卡仍需保留到提交或取消。
	if (InteractionModel && InteractionModel->IsCarrying())
	{
		const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
		if (!bHasExpanded || !SameIdentity(Carry.SourceZone, Expanded))
		{
			for (const FGuid InstanceId : Carry.RemainingInstanceIds)
			{
				// 通量卡始终已经位于 FluxDesired，保持其分组索引；隐藏牌堆的
				// 物理卡则放到展开组之后，并替换目标牌堆中可能存在的同身份投影。
				if (Carry.SourceZone.Zone == EZoneKind::Backpack)
				{
					continue;
				}
				ExpandedDesired.RemoveAll(
					[InstanceId](const FWacomBackpackDeckCardListItem& Item)
					{
						return Item.CardView.Instance.InstanceId == InstanceId;
					});
				if (const FRunStorageCardView* Card = FindPhysicalCard(Snapshot, Carry.SourceZone, InstanceId))
				{
					AppendPhysicalCards(MakeArrayView(Card, 1), HiddenCarryDesired);
				}
			}
		}
	}
	TArray<FWacomBackpackDeckCardListItem> Desired;
	Desired.Reserve(FluxDesired.Num() + ExpandedDesired.Num() + HiddenCarryDesired.Num());
	Desired.Append(FluxDesired);
	const int32 FluxCardCount = Desired.Num();
	Desired.Append(ExpandedDesired);
	const int32 ExpandedCardCount = ExpandedDesired.Num();
	Desired.Append(HiddenCarryDesired);

	TArray<FGuid> FluxIds;
	FluxIds.Reserve(Snapshot.Flux.ContentCards.Num());
	for (const FRunStorageCardView& Card : Snapshot.Flux.ContentCards)
	{
		FluxIds.Add(Card.Instance.InstanceId);
	}
	const FWacomBackpackZoneKey FluxZone = FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
	StateStore.ReconcileZone(FluxZone, FluxIds);

	TArray<TObjectPtr<UWacomDeckCardWidget>> OrderedWidgets;
	FWacomBackpackDeckCardListReconciler::Reconcile(
		Canvas,
		Desired,
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
	TArray<FWacomBackpackResolvedLayout> Accordion;
	int32 ExpandedPileIndex = INDEX_NONE;
	if (bHasExpanded)
	{
		for (int32 Index = 0; Index < PileViews.Num(); ++Index)
		{
			if (FWacomBackpackZoneKey::Make(PileViews[Index].Zone, PileViews[Index].OwnerInstanceId) == Expanded)
			{
				ExpandedPileIndex = Index;
				break;
			}
		}
		if (PileTopLefts.IsValidIndex(ExpandedPileIndex))
		{
			Accordion = FWacomBackpackWorkspaceLayoutSolver::BuildAccordionLayout(
				ExpandedCardCount,
				PileTopLefts[ExpandedPileIndex],
				ResolvedStyle->PileCollapsedSize,
				WorkspaceSize,
				ResolvedStyle->CardRenderSize,
				ResolvedStyle->AccordionMinimumExposurePixels,
				ResolvedStyle->AccordionMaximumExposurePixels,
				ResolvedStyle->AccordionMaximumAngleDegrees,
				ResolvedStyle->PileEdgeMarginPixels);
		}
	}

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
		bool bExpandedCardLayout = false;
		if (Index < FluxCardCount && FluxDefaults.IsValidIndex(Index))
		{
			Resolved = FluxDefaults[Index];
			if (const FWacomBackpackWorkspaceLayoutEntry* Manual = StateStore.FindLayout(
				FluxZone,
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
		}
		else if (Index < FluxCardCount + ExpandedCardCount
			&& Accordion.IsValidIndex(Index - FluxCardCount))
		{
			Resolved = Accordion[Index - FluxCardCount];
			Resolved.LayerRank += 3000;
			bExpandedCardLayout = true;
			const FSlateRect CardRect(
				Resolved.CardCenter.X - ResolvedStyle->CardRenderSize.X * 0.5f,
				Resolved.CardCenter.Y - ResolvedStyle->CardRenderSize.Y * 0.5f - ResolvedStyle->AccordionHoverLiftPixels,
				Resolved.CardCenter.X + ResolvedStyle->CardRenderSize.X * 0.5f,
				Resolved.CardCenter.Y + ResolvedStyle->CardRenderSize.Y * 0.5f);
			if (!bHasExpandedBounds)
			{
				ExpandedBounds = CardRect;
				bHasExpandedBounds = true;
			}
			else
			{
				ExpandedBounds.Left = FMath::Min(ExpandedBounds.Left, CardRect.Left);
				ExpandedBounds.Top = FMath::Min(ExpandedBounds.Top, CardRect.Top);
				ExpandedBounds.Right = FMath::Max(ExpandedBounds.Right, CardRect.Right);
				ExpandedBounds.Bottom = FMath::Max(ExpandedBounds.Bottom, CardRect.Bottom);
			}
		}
		else
		{
			// 隐藏来源牌堆的剩余携带卡会立即被携带扇形覆盖。
			Resolved.CardCenter = InteractionModel && InteractionModel->IsCarrying()
				? InteractionModel->GetCarry().PointerPosition
				: WorkspaceSize * 0.5f;
			Resolved.LayerRank = 3000 + Index;
		}
		if (bExpandedCardLayout
			&& PileTopLefts.IsValidIndex(ExpandedPileIndex)
			&& !Workspace.HasCardBaseLayout(CardWidget->GetCardInstanceId()))
		{
			Workspace.PrimeCardBaseLayout(
				*CardWidget,
				PileTopLefts[ExpandedPileIndex] + ResolvedStyle->PileCollapsedSize * 0.5f,
				ResolvedStyle->CardRenderSize,
				0.0f,
				Resolved.LayerRank);
		}
		Workspace.ApplyCardBaseLayout(
			*CardWidget,
			Resolved.CardCenter,
			ResolvedStyle->CardRenderSize,
			Resolved.AngleDegrees,
			Resolved.LayerRank);
		CardWidget->SetMoveEnabled(
			Desired[Index].Role != EWacomBackpackDeckCardListReuseRole::BattleDeckProjected);
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
