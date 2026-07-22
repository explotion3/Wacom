// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"

namespace
{
bool DoesCardBodyIntersectMarquee(
	const FWacomBackpackWorkspaceCardHitRecord& Card,
	const FVector2D& MarqueeMinimum,
	const FVector2D& MarqueeMaximum)
{
	if (Card.CardSize.X <= UE_SMALL_NUMBER || Card.CardSize.Y <= UE_SMALL_NUMBER)
	{
		return Card.CardCenter.X >= MarqueeMinimum.X
			&& Card.CardCenter.X <= MarqueeMaximum.X
			&& Card.CardCenter.Y >= MarqueeMinimum.Y
			&& Card.CardCenter.Y <= MarqueeMaximum.Y;
	}

	const FVector2D MarqueeCenter = (MarqueeMinimum + MarqueeMaximum) * 0.5f;
	const FVector2D MarqueeHalfSize = (MarqueeMaximum - MarqueeMinimum) * 0.5f;
	const FVector2D CardHalfSize(
		FMath::Abs(Card.CardSize.X) * 0.5f,
		FMath::Abs(Card.CardSize.Y) * 0.5f);
	const float AngleRadians = FMath::DegreesToRadians(Card.AngleDegrees);
	const FVector2D CardAxisX(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians));
	const FVector2D CardAxisY(-CardAxisX.Y, CardAxisX.X);
	const FVector2D CenterDelta = Card.CardCenter - MarqueeCenter;
	const auto IsSeparated = [](float CenterDistance, float FirstRadius, float SecondRadius)
	{
		return FMath::Abs(CenterDistance) > FirstRadius + SecondRadius + UE_KINDA_SMALL_NUMBER;
	};

	// SAT between the axis-aligned marquee and the card's screen-space oriented
	// rectangle. Inclusive comparisons make an exact edge touch a valid hit.
	if (IsSeparated(
		CenterDelta.X,
		MarqueeHalfSize.X,
		FMath::Abs(CardAxisX.X) * CardHalfSize.X
			+ FMath::Abs(CardAxisY.X) * CardHalfSize.Y)
		|| IsSeparated(
			CenterDelta.Y,
			MarqueeHalfSize.Y,
			FMath::Abs(CardAxisX.Y) * CardHalfSize.X
				+ FMath::Abs(CardAxisY.Y) * CardHalfSize.Y)
		|| IsSeparated(
			FVector2D::DotProduct(CenterDelta, CardAxisX),
			MarqueeHalfSize.X * FMath::Abs(CardAxisX.X)
				+ MarqueeHalfSize.Y * FMath::Abs(CardAxisX.Y),
			CardHalfSize.X)
		|| IsSeparated(
			FVector2D::DotProduct(CenterDelta, CardAxisY),
			MarqueeHalfSize.X * FMath::Abs(CardAxisY.X)
				+ MarqueeHalfSize.Y * FMath::Abs(CardAxisY.Y),
			CardHalfSize.Y))
	{
		return false;
	}
	return true;
}
}

const FWacomBackpackWorkspaceCardHitRecord* FWacomBackpackWorkspaceInteractionModel::FindCard(
	FGuid InstanceId) const
{
	return AvailableCards.FindByPredicate(
		[InstanceId](const FWacomBackpackWorkspaceCardHitRecord& Card)
		{
			return Card.InstanceId == InstanceId;
		});
}

bool FWacomBackpackWorkspaceInteractionModel::IsMovable(FGuid InstanceId) const
{
	const FWacomBackpackWorkspaceCardHitRecord* Card = FindCard(InstanceId);
	return Card && Card->bMovable;
}

void FWacomBackpackWorkspaceInteractionModel::ReplaceSelection(TConstArrayView<FGuid> InstanceIds)
{
	Selection.OrderedSelectedInstanceIds.Reset();
	Selection.bHasSourceZone = false;
	for (const FGuid InstanceId : InstanceIds)
	{
		const FWacomBackpackWorkspaceCardHitRecord* Card = FindCard(InstanceId);
		if (!Card || !Card->bMovable
			|| (Selection.bHasSourceZone && !(Card->SourceZone == Selection.SourceZone))
			|| Selection.OrderedSelectedInstanceIds.Contains(InstanceId))
		{
			continue;
		}
		if (!Selection.bHasSourceZone)
		{
			Selection.SourceZone = Card->SourceZone;
			Selection.bHasSourceZone = true;
		}
		Selection.OrderedSelectedInstanceIds.Add(InstanceId);
	}
	Selection.AnchorInstanceId = Selection.OrderedSelectedInstanceIds.IsEmpty()
		? FGuid()
		: Selection.OrderedSelectedInstanceIds.Last();
}

void FWacomBackpackWorkspaceInteractionModel::NormalizeSelection()
{
	Selection.OrderedSelectedInstanceIds.RemoveAll(
		[this](FGuid InstanceId)
		{
			const FWacomBackpackWorkspaceCardHitRecord* Card = FindCard(InstanceId);
			return !Card || !Card->bMovable
				|| (Selection.bHasSourceZone && !(Card->SourceZone == Selection.SourceZone));
		});
	if (Selection.OrderedSelectedInstanceIds.IsEmpty())
	{
		Selection.bHasSourceZone = false;
	}
	if (!Selection.OrderedSelectedInstanceIds.Contains(Selection.AnchorInstanceId))
	{
		Selection.AnchorInstanceId = Selection.OrderedSelectedInstanceIds.IsEmpty()
			? FGuid()
			: Selection.OrderedSelectedInstanceIds.Last();
	}
}

void FWacomBackpackWorkspaceInteractionModel::ReconcileCards(
	TConstArrayView<FWacomBackpackWorkspaceCardHitRecord> Cards)
{
	AvailableCards.Reset(Cards.Num());
	TMap<FGuid, FWacomBackpackZoneKey> MovableSources;
	for (const FWacomBackpackWorkspaceCardHitRecord& Card : Cards)
	{
		// Browse-only display identities never belong in the selection/carry table.
		// Keeping this defensive filter here prevents a future caller from restoring
		// the old "first matching InstanceId wins" ambiguity.
		if (!Card.bMovable || !Card.InstanceId.IsValid())
		{
			continue;
		}

		if (const FWacomBackpackZoneKey* ExistingSource = MovableSources.Find(Card.InstanceId))
		{
			ensureMsgf(
				*ExistingSource == Card.SourceZone,
				TEXT("Backpack physical card %s was projected as movable from two sources."),
				*Card.InstanceId.ToString());
			continue;
		}

		MovableSources.Add(Card.InstanceId, Card.SourceZone);
		AvailableCards.Add(Card);
	}
	NormalizeSelection();
	if (IsCarrying())
	{
		const bool bCarryInvalid = Carry.RemainingInstanceIds.ContainsByPredicate(
			[this](FGuid InstanceId)
			{
				const FWacomBackpackWorkspaceCardHitRecord* Card = FindCard(InstanceId);
				return !Card || !Card->bMovable || !(Card->SourceZone == Carry.SourceZone);
			});
		if (bCarryInvalid)
		{
			CancelTransientState();
		}
	}
}

void FWacomBackpackWorkspaceInteractionModel::ReconcileCards(
	const FWacomBackpackZoneKey& InActiveZone,
	TConstArrayView<FWacomBackpackWorkspaceCardHitRecord> Cards)
{
	TArray<FWacomBackpackWorkspaceCardHitRecord> Normalized(Cards);
	for (FWacomBackpackWorkspaceCardHitRecord& Card : Normalized)
	{
		Card.SourceZone = InActiveZone;
	}
	ReconcileCards(Normalized);
}

void FWacomBackpackWorkspaceInteractionModel::UpdateCardHitLayouts(
	TConstArrayView<FWacomBackpackWorkspaceCardHitRecord> Cards)
{
	for (const FWacomBackpackWorkspaceCardHitRecord& Update : Cards)
	{
		FWacomBackpackWorkspaceCardHitRecord* Existing = AvailableCards.FindByPredicate(
			[&Update](const FWacomBackpackWorkspaceCardHitRecord& Card)
			{
				return Card.InstanceId == Update.InstanceId
					&& Card.SourceZone == Update.SourceZone;
			});
		if (Existing)
		{
			Existing->CardCenter = Update.CardCenter;
			Existing->CardSize = Update.CardSize;
			Existing->AngleDegrees = Update.AngleDegrees;
			Existing->LayerRank = Update.LayerRank;
		}
	}
}

void FWacomBackpackWorkspaceInteractionModel::ClickCard(FGuid InstanceId, bool bControlDown)
{
	if (IsCarrying() || !IsMovable(InstanceId))
	{
		return;
	}
	const FWacomBackpackWorkspaceCardHitRecord* Clicked = FindCard(InstanceId);
	if (!Clicked)
	{
		return;
	}
	if (!bControlDown || (Selection.bHasSourceZone && !(Selection.SourceZone == Clicked->SourceZone)))
	{
		ReplaceSelection(MakeArrayView(&InstanceId, 1));
		return;
	}
	if (!Selection.bHasSourceZone)
	{
		Selection.SourceZone = Clicked->SourceZone;
		Selection.bHasSourceZone = true;
	}
	if (Selection.OrderedSelectedInstanceIds.Remove(InstanceId) == 0)
	{
		Selection.OrderedSelectedInstanceIds.Add(InstanceId);
	}
	NormalizeSelection();
	Selection.AnchorInstanceId = Selection.OrderedSelectedInstanceIds.IsEmpty()
		? FGuid()
		: InstanceId;
}

void FWacomBackpackWorkspaceInteractionModel::ClickBlank()
{
	if (!IsCarrying())
	{
		ReplaceSelection(TConstArrayView<FGuid>());
	}
}

void FWacomBackpackWorkspaceInteractionModel::BeginMarquee(FVector2D Start, bool bControlDown)
{
	BeginMarquee(
		Selection.bHasSourceZone ? Selection.SourceZone : FWacomBackpackZoneKey::Make(EZoneKind::Backpack),
		Start,
		bControlDown);
}

void FWacomBackpackWorkspaceInteractionModel::BeginMarquee(
	const FWacomBackpackZoneKey& SourceZone,
	FVector2D Start,
	bool bControlDown)
{
	if (IsCarrying() || IsPileMoving())
	{
		return;
	}
	if (!Selection.bHasSourceZone || !(Selection.SourceZone == SourceZone))
	{
		Selection = FWacomBackpackWorkspaceSelectionState();
	}
	Selection.SourceZone = SourceZone;
	Selection.bHasSourceZone = true;
	Selection.MarqueeStart = Start;
	Selection.MarqueeCurrent = Start;
	Selection.MarqueeMode = bControlDown
		? EWacomBackpackSelectionMode::Toggle
		: EWacomBackpackSelectionMode::Replace;
	Selection.bMarqueeActive = true;
	MarqueeStartSelection = Selection.OrderedSelectedInstanceIds;
	bMouseCaptured = true;
	Mode = EWacomBackpackWorkspaceInteractionMode::Marquee;
}

void FWacomBackpackWorkspaceInteractionModel::UpdateMarquee(FVector2D Current)
{
	if (Selection.bMarqueeActive)
	{
		Selection.MarqueeCurrent = Current;
	}
}

void FWacomBackpackWorkspaceInteractionModel::CompleteMarquee()
{
	if (!Selection.bMarqueeActive)
	{
		return;
	}
	const FVector2D Minimum(
		FMath::Min(Selection.MarqueeStart.X, Selection.MarqueeCurrent.X),
		FMath::Min(Selection.MarqueeStart.Y, Selection.MarqueeCurrent.Y));
	const FVector2D Maximum(
		FMath::Max(Selection.MarqueeStart.X, Selection.MarqueeCurrent.X),
		FMath::Max(Selection.MarqueeStart.Y, Selection.MarqueeCurrent.Y));
	TArray<FGuid> Hits;
	for (const FWacomBackpackWorkspaceCardHitRecord& Card : AvailableCards)
	{
		if (Card.bMovable && Selection.bHasSourceZone && Card.SourceZone == Selection.SourceZone
			&& DoesCardBodyIntersectMarquee(Card, Minimum, Maximum))
		{
			Hits.Add(Card.InstanceId);
		}
	}
	if (Selection.MarqueeMode == EWacomBackpackSelectionMode::Replace)
	{
		ReplaceSelection(Hits);
	}
	else
	{
		Selection.OrderedSelectedInstanceIds = MarqueeStartSelection;
		for (const FGuid HitId : Hits)
		{
			if (Selection.OrderedSelectedInstanceIds.Remove(HitId) == 0)
			{
				Selection.OrderedSelectedInstanceIds.Add(HitId);
			}
		}
		NormalizeSelection();
	}
	Selection.bMarqueeActive = false;
	MarqueeStartSelection.Reset();
	bMouseCaptured = false;
	Mode = EWacomBackpackWorkspaceInteractionMode::Idle;
}

void FWacomBackpackWorkspaceInteractionModel::SelectAllMovable()
{
	SelectAllMovable(Selection.bHasSourceZone
		? Selection.SourceZone
		: FWacomBackpackZoneKey::Make(EZoneKind::Backpack));
}

void FWacomBackpackWorkspaceInteractionModel::SelectAllMovable(
	const FWacomBackpackZoneKey& SourceZone)
{
	if (IsCarrying())
	{
		return;
	}
	TArray<FGuid> MovableIds;
	for (const FWacomBackpackWorkspaceCardHitRecord& Card : AvailableCards)
	{
		if (Card.bMovable && Card.SourceZone == SourceZone)
		{
			MovableIds.Add(Card.InstanceId);
		}
	}
	ReplaceSelection(MovableIds);
}

void FWacomBackpackWorkspaceInteractionModel::SetCardPressActive(bool bActive)
{
	if (!IsCarrying() && !IsMarqueeActive() && !IsPileMoving())
	{
		Mode = bActive
			? EWacomBackpackWorkspaceInteractionMode::CardPress
			: EWacomBackpackWorkspaceInteractionMode::Idle;
	}
}

bool FWacomBackpackWorkspaceInteractionModel::BeginPileMove(
	const FWacomBackpackZoneKey& Zone,
	FVector2D PointerStart,
	FVector2D PileStart)
{
	if (!Zone.IsValid() || IsCarrying() || IsMarqueeActive())
	{
		return false;
	}
	Selection = FWacomBackpackWorkspaceSelectionState();
	PileMove = FWacomBackpackWorkspacePileMoveState();
	PileMove.Zone = Zone;
	PileMove.PointerStart = PointerStart;
	PileMove.PileStart = PileStart;
	PileMove.CurrentPosition = PileStart;
	PileMove.bActive = true;
	bMouseCaptured = true;
	Mode = EWacomBackpackWorkspaceInteractionMode::PileMove;
	return true;
}

void FWacomBackpackWorkspaceInteractionModel::UpdatePileMove(FVector2D PointerPosition)
{
	if (PileMove.bActive)
	{
		PileMove.CurrentPosition = PileMove.PileStart + (PointerPosition - PileMove.PointerStart);
	}
}

FWacomBackpackWorkspacePileMoveState FWacomBackpackWorkspaceInteractionModel::CompletePileMove()
{
	const FWacomBackpackWorkspacePileMoveState Completed = PileMove;
	PileMove = FWacomBackpackWorkspacePileMoveState();
	bMouseCaptured = false;
	Mode = EWacomBackpackWorkspaceInteractionMode::Idle;
	return Completed;
}

bool FWacomBackpackWorkspaceInteractionModel::BeginCarry(
	FGuid DraggedInstanceId,
	FVector2D PointerPosition,
	uint64 SourceStorageRevision)
{
	const FWacomBackpackWorkspaceCardHitRecord* Dragged = FindCard(DraggedInstanceId);
	if (IsCarrying() || !Dragged || !Dragged->bMovable)
	{
		return false;
	}
	if (!IsSelected(DraggedInstanceId))
	{
		ReplaceSelection(MakeArrayView(&DraggedInstanceId, 1));
	}
	TArray<FGuid> Ordered = Selection.OrderedSelectedInstanceIds;
	Ordered.Sort([this](const FGuid& Left, const FGuid& Right)
	{
		const FWacomBackpackWorkspaceCardHitRecord* LeftCard = FindCard(Left);
		const FWacomBackpackWorkspaceCardHitRecord* RightCard = FindCard(Right);
		const int32 LeftLayer = LeftCard ? LeftCard->LayerRank : 0;
		const int32 RightLayer = RightCard ? RightCard->LayerRank : 0;
		if (LeftLayer != RightLayer)
		{
			return LeftLayer < RightLayer;
		}
		return Left.ToString(EGuidFormats::Digits) < Right.ToString(EGuidFormats::Digits);
	});
	if (Ordered.IsEmpty())
	{
		return false;
	}
	Carry = FWacomBackpackWorkspaceCarryState();
	Carry.RemainingInstanceIds = MoveTemp(Ordered);
	Carry.DefaultIndex = Carry.RemainingInstanceIds.Num() - 1;
	Carry.CurrentIndex = Carry.DefaultIndex;
	Carry.PointerPosition = PointerPosition;
	Carry.bInitialReleaseGuardArmed = true;
	Carry.bMouseCaptured = true;
	Carry.SourceZone = Dragged->SourceZone;
	Carry.SourceStorageRevision = SourceStorageRevision;
	bMouseCaptured = true;
	Selection.bMarqueeActive = false;
	Mode = EWacomBackpackWorkspaceInteractionMode::Carry;
	return true;
}

void FWacomBackpackWorkspaceInteractionModel::UpdateCarryPointer(FVector2D PointerPosition)
{
	if (IsCarrying())
	{
		Carry.PointerPosition = PointerPosition;
	}
}

void FWacomBackpackWorkspaceInteractionModel::StepCurrentByWheel(float WheelDelta)
{
	if (!IsCarrying() || FMath::IsNearlyZero(WheelDelta))
	{
		return;
	}
	const int32 Direction = WheelDelta > 0.0f ? -1 : 1;
	Carry.CurrentIndex = FMath::Clamp(
		Carry.CurrentIndex + Direction, 0, Carry.RemainingInstanceIds.Num() - 1);
}

void FWacomBackpackWorkspaceInteractionModel::NotifyReleaseGestureStarted()
{
	if (IsCarrying())
	{
		Carry.bInitialReleaseGuardArmed = false;
	}
}

FWacomBackpackWorkspaceReleaseIntent FWacomBackpackWorkspaceInteractionModel::BuildReleaseIntent(
	bool bReleaseAll,
	EWacomBackpackWorkspaceReleaseTargetKind TargetKind,
	const FWacomBackpackZoneKey& TargetZone)
{
	FWacomBackpackWorkspaceReleaseIntent Intent;
	Intent.bReleaseAll = bReleaseAll;
	Intent.TargetKind = TargetKind;
	Intent.TargetZone = TargetZone;
	if (!IsCarrying())
	{
		return Intent;
	}
	if (Carry.bInitialReleaseGuardArmed)
	{
		Carry.bInitialReleaseGuardArmed = false;
		Intent.bConsumedByInitialReleaseGuard = true;
		return Intent;
	}
	if (bReleaseAll)
	{
		Intent.InstanceIds = Carry.RemainingInstanceIds;
	}
	else if (Carry.RemainingInstanceIds.IsValidIndex(Carry.CurrentIndex))
	{
		Intent.InstanceIds.Add(Carry.RemainingInstanceIds[Carry.CurrentIndex]);
	}
	return Intent;
}

void FWacomBackpackWorkspaceInteractionModel::CommitReleasedCards(TConstArrayView<FGuid> ReleasedInstanceIds)
{
	if (!IsCarrying() || ReleasedInstanceIds.IsEmpty())
	{
		return;
	}
	TSet<FGuid> Released;
	for (const FGuid InstanceId : ReleasedInstanceIds)
	{
		Released.Add(InstanceId);
	}
	const int32 PreviousCurrentIndex = Carry.CurrentIndex;
	Carry.RemainingInstanceIds.RemoveAll([&Released](FGuid Id) { return Released.Contains(Id); });
	Selection.OrderedSelectedInstanceIds.RemoveAll([&Released](FGuid Id) { return Released.Contains(Id); });
	if (Carry.RemainingInstanceIds.IsEmpty())
	{
		Carry = FWacomBackpackWorkspaceCarryState();
		Selection = FWacomBackpackWorkspaceSelectionState();
		bMouseCaptured = false;
		Mode = EWacomBackpackWorkspaceInteractionMode::Idle;
		return;
	}
	Carry.DefaultIndex = Carry.RemainingInstanceIds.Num() - 1;
	Carry.CurrentIndex = FMath::Clamp(PreviousCurrentIndex, 0, Carry.DefaultIndex);
	Carry.bMouseCaptured = true;
	bMouseCaptured = true;
	Selection.OrderedSelectedInstanceIds = Carry.RemainingInstanceIds;
	Selection.AnchorInstanceId = Carry.RemainingInstanceIds[Carry.CurrentIndex];
	Selection.SourceZone = Carry.SourceZone;
	Selection.bHasSourceZone = true;
}

void FWacomBackpackWorkspaceInteractionModel::UpdateCarrySourceStorageRevision(uint64 SourceStorageRevision)
{
	if (IsCarrying())
	{
		Carry.SourceStorageRevision = SourceStorageRevision;
	}
}

void FWacomBackpackWorkspaceInteractionModel::SetCarryInputSuspended(bool bSuspended)
{
	if (!IsCarrying())
	{
		bMouseCaptured = false;
		Mode = EWacomBackpackWorkspaceInteractionMode::Idle;
		return;
	}
	Carry.bMouseCaptured = !bSuspended;
	bMouseCaptured = !bSuspended;
	Mode = bSuspended
		? EWacomBackpackWorkspaceInteractionMode::Suspended
		: EWacomBackpackWorkspaceInteractionMode::Carry;
}

void FWacomBackpackWorkspaceInteractionModel::CancelTransientState()
{
	Selection = FWacomBackpackWorkspaceSelectionState();
	Carry = FWacomBackpackWorkspaceCarryState();
	PileMove = FWacomBackpackWorkspacePileMoveState();
	MarqueeStartSelection.Reset();
	bMouseCaptured = false;
	Mode = EWacomBackpackWorkspaceInteractionMode::Idle;
}

void FWacomBackpackWorkspaceInteractionModel::RestoreCarry(
	const FWacomBackpackWorkspaceCarryState& CarrySnapshot)
{
	Carry = CarrySnapshot;
	if (Carry.RemainingInstanceIds.IsEmpty())
	{
		CancelTransientState();
		return;
	}
	Carry.DefaultIndex = Carry.RemainingInstanceIds.Num() - 1;
	Carry.CurrentIndex = FMath::Clamp(Carry.CurrentIndex, 0, Carry.DefaultIndex);
	Carry.bMouseCaptured = true;
	bMouseCaptured = true;
	Selection = FWacomBackpackWorkspaceSelectionState();
	Selection.OrderedSelectedInstanceIds = Carry.RemainingInstanceIds;
	Selection.AnchorInstanceId = Carry.RemainingInstanceIds[Carry.CurrentIndex];
	Selection.SourceZone = Carry.SourceZone;
	Selection.bHasSourceZone = true;
	Mode = EWacomBackpackWorkspaceInteractionMode::Carry;
}
