// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceSceneBuilder.h"

#include "Cards/CardDefinition.h"
#include "UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"

#define LOCTEXT_NAMESPACE "WacomBackpackWorkspaceSceneBuilder"

namespace
{
const FRunSpecialStorageView* FindSpecial(
	const TMap<FGuid, const FRunSpecialStorageView*>& SpecialsByOwner,
	FGuid OwnerInstanceId)
{
	const FRunSpecialStorageView* const* Found = SpecialsByOwner.Find(OwnerInstanceId);
	return Found ? *Found : nullptr;
}

FText BuildProjectedBadge(
	const FRunStorageCardView& ProjectedCard,
	const TMap<FGuid, const FRunSpecialStorageView*>& SpecialsByOwner)
{
	const FRunSpecialStorageView* Source = FindSpecial(
		SpecialsByOwner, ProjectedCard.ZoneOwnerInstanceId);
	return Source && Source->OwnerCard.Instance.Definition
		? FText::Format(
			LOCTEXT("ProjectedFromBadgeFmt", "来自 {0}"),
			Source->OwnerCard.Instance.Definition->DisplayName)
		: FText::GetEmpty();
}

void AddCard(
	const FRunStorageCardView& Card,
	const FWacomBackpackZoneKey& DisplayZone,
	EWacomBackpackDeckCardListReuseRole Role,
	bool bInteractive,
	EWacomBackpackWorkspaceCardReadOnlyKind ReadOnlyKind,
	TArray<FWacomBackpackWorkspaceSceneCardEntry>& OutCards,
	const FText& Badge = FText::GetEmpty())
{
	FWacomBackpackWorkspaceSceneCardEntry& Item = OutCards.AddDefaulted_GetRef();
	Item.CardView = Card;
	Item.DisplayZone = DisplayZone;
	Item.Role = Role;
	Item.bWorkspaceInteractive = bInteractive;
	Item.ReadOnlyKind = ReadOnlyKind;
	Item.ProjectedBadgeText = Badge;
}

void AppendPileCards(
	const FRunBackpackStorageSnapshot& Snapshot,
	const TMap<FGuid, const FRunSpecialStorageView*>& SpecialsByOwner,
	const FWacomBackpackZonePileView& Pile,
	const FWacomBackpackWorkspaceCarryProjection& Carry,
	TArray<FWacomBackpackWorkspaceSceneCardEntry>& OutCards)
{
	const FWacomBackpackZoneKey Zone = FWacomBackpackZoneKey::Make(
		Pile.Zone, Pile.OwnerInstanceId);
	const auto IsInteractive = [&](const FRunStorageCardView& Card)
	{
		return Pile.bExpanded
			|| (Carry.bCarrying && Carry.SourceZone == Zone
				&& Carry.InstanceIds.Contains(Card.Instance.InstanceId));
	};

	switch (Pile.Zone)
	{
	case EZoneKind::BattleDeck:
		for (const FRunStorageCardView& Card : Snapshot.BattleDeckPhysicalCards)
		{
			AddCard(
				Card, Zone, EWacomBackpackDeckCardListReuseRole::PhysicalList,
				IsInteractive(Card), EWacomBackpackWorkspaceCardReadOnlyKind::None,
				OutCards);
		}
		for (const FRunStorageCardView& Card : Snapshot.BattleDeckProjectedCards)
		{
			AddCard(
				Card, Zone, EWacomBackpackDeckCardListReuseRole::BattleDeckProjected,
				false, EWacomBackpackWorkspaceCardReadOnlyKind::BattleProjection,
				OutCards,
				BuildProjectedBadge(Card, SpecialsByOwner));
		}
		break;
	case EZoneKind::SpecialZone:
		if (const FRunSpecialStorageView* Special = FindSpecial(
			SpecialsByOwner, Pile.OwnerInstanceId))
		{
			AddCard(
				Special->OwnerCard, Zone,
				EWacomBackpackDeckCardListReuseRole::SpecialOwner,
				false, EWacomBackpackWorkspaceCardReadOnlyKind::SpecialOwner,
				OutCards, LOCTEXT("SpecialOwnerBadge", "主卡"));
			for (const FRunStorageCardView& Card : Special->ContentCards)
			{
				AddCard(
					Card, Zone, EWacomBackpackDeckCardListReuseRole::SpecialContent,
					IsInteractive(Card), EWacomBackpackWorkspaceCardReadOnlyKind::None,
					OutCards);
			}
		}
		break;
	case EZoneKind::BurdenZone:
		for (const FRunStorageCardView& Card : Snapshot.BurdenCards)
		{
			AddCard(
				Card, Zone, EWacomBackpackDeckCardListReuseRole::PhysicalList,
				IsInteractive(Card), EWacomBackpackWorkspaceCardReadOnlyKind::None,
				OutCards);
		}
		break;
	default:
		break;
	}
}
}

TArray<FWacomBackpackZonePileView> FWacomBackpackWorkspaceSceneBuilder::BuildPileViews(
	const FRunBackpackStorageSnapshot& Snapshot,
	const TOptional<FWacomBackpackZoneKey>& ExpandedPile)
{
	const auto IsExpanded = [&ExpandedPile](EZoneKind Zone, FGuid Owner)
	{
		return ExpandedPile.IsSet()
			&& ExpandedPile.GetValue() == FWacomBackpackZoneKey::Make(Zone, Owner);
	};

	TArray<FWacomBackpackZonePileView> Result;
	FWacomBackpackZonePileView& Battle = Result.AddDefaulted_GetRef();
	Battle.Zone = EZoneKind::BattleDeck;
	Battle.Title = LOCTEXT("WorkspaceBattlePile", "备战区");
	Battle.CardCount = Snapshot.BattleDeckPhysicalCards.Num();
	Battle.Capacity = Snapshot.BattleDeckCapacity;
	Battle.ProjectedCount = Snapshot.BattleDeckProjectedCards.Num();
	Battle.bHasCapacity = true;
	Battle.bExpanded = IsExpanded(Battle.Zone, FGuid());

	for (const FRunSpecialStorageView& Special : Snapshot.SpecialZones)
	{
		const FGuid OwnerId = Special.OwnerCard.Instance.InstanceId;
		if (!OwnerId.IsValid())
		{
			continue;
		}
		FWacomBackpackZonePileView& Pile = Result.AddDefaulted_GetRef();
		Pile.Zone = EZoneKind::SpecialZone;
		Pile.OwnerInstanceId = OwnerId;
		Pile.Title = Special.OwnerCard.Instance.Definition
			? Special.OwnerCard.Instance.Definition->DisplayName
			: LOCTEXT("UnknownSpecialPile", "特殊存放区");
		Pile.CardCount = Special.ContentCards.Num();
		Pile.Capacity = Special.Capacity;
		Pile.bHasCapacity = true;
		Pile.bExpanded = IsExpanded(Pile.Zone, OwnerId);
	}

	if (!Snapshot.BurdenCards.IsEmpty())
	{
		FWacomBackpackZonePileView& Burden = Result.AddDefaulted_GetRef();
		Burden.Zone = EZoneKind::BurdenZone;
		Burden.Title = LOCTEXT("WorkspaceBurdenPile", "负重区");
		Burden.CardCount = Snapshot.BurdenCards.Num();
		Burden.bMovable = true;
		Burden.bAcceptsExternalCardDrop = false;
		Burden.bWarning = true;
		Burden.bExpanded = IsExpanded(Burden.Zone, FGuid());
	}
	return Result;
}

FText FWacomBackpackWorkspaceSceneBuilder::BuildBattleDeckProjectedBadge(
	const FRunStorageCardView& ProjectedCard,
	const FRunBackpackStorageSnapshot& Snapshot)
{
	for (const FRunSpecialStorageView& Special : Snapshot.SpecialZones)
	{
		if (Special.OwnerCard.Instance.InstanceId == ProjectedCard.ZoneOwnerInstanceId
			&& Special.OwnerCard.Instance.Definition)
		{
			return FText::Format(
				LOCTEXT("ProjectedFromBadgeFmt", "来自 {0}"),
				Special.OwnerCard.Instance.Definition->DisplayName);
		}
	}
	return FText::GetEmpty();
}

FWacomBackpackWorkspaceScene FWacomBackpackWorkspaceSceneBuilder::Build(
	const FRunBackpackStorageSnapshot& Snapshot,
	FWacomBackpackWorkspaceStateStore& StateStore,
	const FWacomBackpackWorkspaceCarryProjection& Carry,
	const UWacomBackpackWorkspaceStyle& Style,
	FVector2D WorkspaceSize)
{
	FWacomBackpackWorkspaceScene Scene;
	Scene.ExpandedZone = FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
	Scene.bFluxEmpty = Snapshot.Flux.ContentCards.IsEmpty();

	TMap<FGuid, const FRunSpecialStorageView*> SpecialsByOwner;
	TArray<FWacomBackpackZoneKey> VisiblePileKeys;
	VisiblePileKeys.Reserve(Snapshot.SpecialZones.Num() + 2);
	VisiblePileKeys.Add(FWacomBackpackZoneKey::Make(EZoneKind::BattleDeck));
	for (const FRunSpecialStorageView& Special : Snapshot.SpecialZones)
	{
		const FGuid OwnerId = Special.OwnerCard.Instance.InstanceId;
		if (OwnerId.IsValid())
		{
			SpecialsByOwner.Add(OwnerId, &Special);
			VisiblePileKeys.Add(FWacomBackpackZoneKey::Make(EZoneKind::SpecialZone, OwnerId));
		}
	}
	if (!Snapshot.BurdenCards.IsEmpty())
	{
		VisiblePileKeys.Add(FWacomBackpackZoneKey::Make(EZoneKind::BurdenZone));
	}
	StateStore.ReconcilePiles(VisiblePileKeys);
	const TOptional<FWacomBackpackZoneKey> Expanded = StateStore.GetExpandedPile();
	Scene.bHasExpandedPile = Expanded.IsSet();
	if (Expanded.IsSet())
	{
		Scene.ExpandedZone = Expanded.GetValue();
	}

	TArray<FWacomBackpackZonePileView> PileViews = BuildPileViews(Snapshot, Expanded);
	Scene.Cards.Reserve(
		Snapshot.Flux.ContentCards.Num()
		+ Snapshot.BattleDeckPhysicalCards.Num()
		+ Snapshot.BattleDeckProjectedCards.Num()
		+ Snapshot.BurdenCards.Num()
		+ Snapshot.SpecialZones.Num());
	const FWacomBackpackZoneKey FluxZone = FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
	for (const FRunStorageCardView& Card : Snapshot.Flux.ContentCards)
	{
		AddCard(
			Card, FluxZone, EWacomBackpackDeckCardListReuseRole::PhysicalList,
			true, EWacomBackpackWorkspaceCardReadOnlyKind::None, Scene.Cards);
	}
	Scene.FluxCardCount = Scene.Cards.Num();

	const FVector2D CardDisplaySize = Style.GetCardDisplaySize();
	const FVector2D HeaderSize(FMath::Max(260.0f, Style.PileCollapsedSize.X), 48.0f);
	const FVector2D DefaultPileFootprint(
		HeaderSize.X, HeaderSize.Y + CardDisplaySize.Y + 24.0f);
	TArray<FSlateRect> OccupiedHeaders;
	TArray<FSlateRect> Obstacles;
	int32 MovablePileIndex = 0;
	for (const FWacomBackpackZonePileView& PileView : PileViews)
	{
		FWacomBackpackWorkspaceScenePileEntry& Pile = Scene.Piles.AddDefaulted_GetRef();
		Pile.View = PileView;
		Pile.Zone = FWacomBackpackZoneKey::Make(PileView.Zone, PileView.OwnerInstanceId);
		FVector2D HeaderTopLeft;
		if (!PileView.bMovable)
		{
			HeaderTopLeft = FVector2D(
				WorkspaceSize.X - Style.PileEdgeMarginPixels - HeaderSize.X,
				Style.PileEdgeMarginPixels);
			Pile.LayerRank = 100000;
		}
		else
		{
			const FWacomBackpackResolvedPileLayout Default =
				FWacomBackpackWorkspaceLayoutSolver::BuildDefaultPileLayout(
					MovablePileIndex++, WorkspaceSize, DefaultPileFootprint,
					Style.PileEdgeMarginPixels);
			HeaderTopLeft = Default.TopLeft;
			Pile.LayerRank = Default.LayerRank;
			if (const FWacomBackpackWorkspacePileLayoutEntry* Stored =
				StateStore.FindPileLayout(Pile.Zone))
			{
				Pile.LayerRank = Stored->LayerRank;
				if (Stored->bHasManualPlacement)
				{
					HeaderTopLeft = FWacomBackpackWorkspaceLayoutSolver::ResolvePileTopLeft(
						*Stored, WorkspaceSize, HeaderSize, Style.PileEdgeMarginPixels);
				}
			}
			HeaderTopLeft = FWacomBackpackWorkspaceLayoutSolver::ResolvePileHeaderOverlap(
				HeaderTopLeft, WorkspaceSize, HeaderSize, HeaderSize,
				Style.PileSnapGridPixels, Style.PileEdgeMarginPixels, OccupiedHeaders);
		}

		Pile.CardStartIndex = Scene.Cards.Num();
		AppendPileCards(Snapshot, SpecialsByOwner, PileView, Carry, Scene.Cards);
		Pile.CardCount = Scene.Cards.Num() - Pile.CardStartIndex;
		Pile.ContentLayout = FWacomBackpackWorkspaceLayoutSolver::BuildPileContentLayout(
			Pile.CardCount, HeaderTopLeft, HeaderSize, WorkspaceSize,
			CardDisplaySize, PileView.bExpanded,
			Style.PileCollapsedExposurePixels, Style.HandLensFullGapPixels,
			Style.HandLensCompressedExposurePixels,
			Style.HandLensMinimumExposurePixels,
			Style.HandLensPromotionOverlapTolerancePixels, Style.PileEdgeMarginPixels,
			Style.ExpandedCardHoverLiftPixels);
		Pile.FrameRect = Pile.ContentLayout.FrameRect;
		Pile.HeaderRect = Pile.ContentLayout.HeaderRect;
		OccupiedHeaders.Add(Pile.HeaderRect);
		Obstacles.Add(Pile.FrameRect);
	}
	Obstacles.Emplace(
		FMath::Max(0.0f, WorkspaceSize.X - 220.0f - Style.PileEdgeMarginPixels),
		FMath::Max(0.0f, WorkspaceSize.Y - 150.0f - Style.PileEdgeMarginPixels),
		WorkspaceSize.X - Style.PileEdgeMarginPixels,
		WorkspaceSize.Y - Style.PileEdgeMarginPixels);

	TArray<FGuid> FluxIds;
	FluxIds.Reserve(Snapshot.Flux.ContentCards.Num());
	for (const FRunStorageCardView& Card : Snapshot.Flux.ContentCards)
	{
		FluxIds.Add(Card.Instance.InstanceId);
	}
	StateStore.ReconcileZone(FluxZone, FluxIds);
	Scene.ManualFluxLayoutCount = StateStore.GetManualLayoutCount(FluxZone);

	Scene.CardLayouts.SetNum(Scene.Cards.Num());
	const TArray<FWacomBackpackResolvedLayout> FluxDefaults =
		FWacomBackpackWorkspaceLayoutSolver::BuildDefaultLayoutAvoidingRectangles(
			Scene.FluxCardCount, WorkspaceSize, CardDisplaySize,
			Style.DefaultCardSpacing, Style.WorkspacePadding, Obstacles);
	for (int32 Index = 0; Index < Scene.FluxCardCount; ++Index)
	{
		if (!FluxDefaults.IsValidIndex(Index))
		{
			continue;
		}
		FWacomBackpackResolvedLayout Resolved = FluxDefaults[Index];
		const FGuid InstanceId = Scene.Cards[Index].CardView.Instance.InstanceId;
		if (const FWacomBackpackWorkspaceLayoutEntry* Manual =
			StateStore.FindLayout(FluxZone, InstanceId))
		{
			if (Manual->bHasManualPlacement)
			{
				Resolved = FWacomBackpackWorkspaceLayoutSolver::ResolveManualLayout(
					*Manual, WorkspaceSize, CardDisplaySize,
					Style.MinimumVisibleFraction);
			}
		}
		Scene.CardLayouts[Index] = Resolved;
	}
	for (const FWacomBackpackWorkspaceScenePileEntry& Pile : Scene.Piles)
	{
		for (int32 LocalIndex = 0; LocalIndex < Pile.CardCount; ++LocalIndex)
		{
			const int32 CardIndex = Pile.CardStartIndex + LocalIndex;
			if (!Scene.CardLayouts.IsValidIndex(CardIndex)
				|| !Pile.ContentLayout.Cards.IsValidIndex(LocalIndex))
			{
				continue;
			}
			Scene.CardLayouts[CardIndex] = Pile.ContentLayout.Cards[LocalIndex];
			Scene.CardLayouts[CardIndex].LayerRank += 3000;
		}
		if (Scene.bHasExpandedPile && Pile.Zone == Scene.ExpandedZone
			&& !Pile.ContentLayout.Cards.IsEmpty())
		{
			Scene.ExpandedBounds = Pile.FrameRect;
			Scene.bHasExpandedBounds = true;
		}
	}
	return Scene;
}

#undef LOCTEXT_NAMESPACE
