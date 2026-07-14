// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"

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

void FWacomBackpackWorkspaceInteractionModel::ReplaceSelection(
	TConstArrayView<FGuid> InstanceIds)
{
	Selection.OrderedSelectedInstanceIds.Reset();
	for (const FGuid InstanceId : InstanceIds)
	{
		if (IsMovable(InstanceId) && !Selection.OrderedSelectedInstanceIds.Contains(InstanceId))
		{
			Selection.OrderedSelectedInstanceIds.Add(InstanceId);
		}
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
			return !IsMovable(InstanceId);
		});
	if (!Selection.OrderedSelectedInstanceIds.Contains(Selection.AnchorInstanceId))
	{
		Selection.AnchorInstanceId = Selection.OrderedSelectedInstanceIds.IsEmpty()
			? FGuid()
			: Selection.OrderedSelectedInstanceIds.Last();
	}
}

void FWacomBackpackWorkspaceInteractionModel::ReconcileCards(
	const FWacomBackpackZoneKey& InActiveZone,
	TConstArrayView<FWacomBackpackWorkspaceCardHitRecord> Cards)
{
	const bool bZoneChanged = !(ActiveZone == InActiveZone);
	if (bZoneChanged)
	{
		CancelTransientState();
	}
	ActiveZone = InActiveZone;
	AvailableCards = TArray<FWacomBackpackWorkspaceCardHitRecord>(Cards);
	NormalizeSelection();

	if (IsCarrying())
	{
		const bool bCarryInvalid = !(Carry.SourceZone == ActiveZone)
			|| Carry.RemainingInstanceIds.ContainsByPredicate(
				[this](FGuid InstanceId) { return !IsMovable(InstanceId); });
		if (bCarryInvalid)
		{
			CancelTransientState();
		}
	}
}

void FWacomBackpackWorkspaceInteractionModel::ClickCard(FGuid InstanceId, bool bControlDown)
{
	if (IsCarrying() || !IsMovable(InstanceId))
	{
		return;
	}
	if (!bControlDown)
	{
		ReplaceSelection(MakeArrayView(&InstanceId, 1));
		return;
	}
	if (Selection.OrderedSelectedInstanceIds.Remove(InstanceId) == 0)
	{
		Selection.OrderedSelectedInstanceIds.Add(InstanceId);
	}
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
	if (IsCarrying())
	{
		return;
	}
	Selection.MarqueeStart = Start;
	Selection.MarqueeCurrent = Start;
	Selection.MarqueeMode = bControlDown
		? EWacomBackpackSelectionMode::Toggle
		: EWacomBackpackSelectionMode::Replace;
	Selection.bMarqueeActive = true;
	MarqueeStartSelection = Selection.OrderedSelectedInstanceIds;
	bMouseCaptured = true;
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
		if (Card.bMovable
			&& Card.CardCenter.X >= Minimum.X && Card.CardCenter.X <= Maximum.X
			&& Card.CardCenter.Y >= Minimum.Y && Card.CardCenter.Y <= Maximum.Y)
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
}

void FWacomBackpackWorkspaceInteractionModel::SelectAllMovable()
{
	if (IsCarrying())
	{
		return;
	}
	TArray<FGuid> MovableIds;
	for (const FWacomBackpackWorkspaceCardHitRecord& Card : AvailableCards)
	{
		if (Card.bMovable)
		{
			MovableIds.Add(Card.InstanceId);
		}
	}
	ReplaceSelection(MovableIds);
}

bool FWacomBackpackWorkspaceInteractionModel::BeginCarry(
	FGuid DraggedInstanceId,
	FVector2D PointerPosition,
	uint64 SourceStorageRevision)
{
	if (IsCarrying() || !IsMovable(DraggedInstanceId))
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
	Carry.SourceZone = ActiveZone;
	Carry.SourceStorageRevision = SourceStorageRevision;
	bMouseCaptured = true;
	Selection.bMarqueeActive = false;
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
		Carry.CurrentIndex + Direction,
		0,
		Carry.RemainingInstanceIds.Num() - 1);
}

void FWacomBackpackWorkspaceInteractionModel::NotifyReleaseGestureStarted()
{
	if (IsCarrying())
	{
		Carry.bInitialReleaseGuardArmed = false;
	}
}

FWacomBackpackWorkspaceReleaseIntent FWacomBackpackWorkspaceInteractionModel::BuildReleaseIntent(
	bool bReleaseAll)
{
	FWacomBackpackWorkspaceReleaseIntent Intent;
	Intent.bReleaseAll = bReleaseAll;
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

void FWacomBackpackWorkspaceInteractionModel::CommitReleasedCards(
	TConstArrayView<FGuid> ReleasedInstanceIds)
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
	Carry.RemainingInstanceIds.RemoveAll(
		[&Released](FGuid InstanceId) { return Released.Contains(InstanceId); });
	Selection.OrderedSelectedInstanceIds.RemoveAll(
		[&Released](FGuid InstanceId) { return Released.Contains(InstanceId); });
	if (Carry.RemainingInstanceIds.IsEmpty())
	{
		Carry = FWacomBackpackWorkspaceCarryState();
		Selection = FWacomBackpackWorkspaceSelectionState();
		bMouseCaptured = false;
		return;
	}
	Carry.DefaultIndex = Carry.RemainingInstanceIds.Num() - 1;
	Carry.CurrentIndex = FMath::Clamp(PreviousCurrentIndex, 0, Carry.DefaultIndex);
	Carry.bMouseCaptured = true;
	bMouseCaptured = true;
	Selection.OrderedSelectedInstanceIds = Carry.RemainingInstanceIds;
	Selection.AnchorInstanceId = Carry.RemainingInstanceIds[Carry.CurrentIndex];
}

void FWacomBackpackWorkspaceInteractionModel::UpdateCarrySourceStorageRevision(
	uint64 SourceStorageRevision)
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
		return;
	}
	Carry.bMouseCaptured = !bSuspended;
	bMouseCaptured = !bSuspended;
}

void FWacomBackpackWorkspaceInteractionModel::CancelTransientState()
{
	Selection = FWacomBackpackWorkspaceSelectionState();
	Carry = FWacomBackpackWorkspaceCarryState();
	MarqueeStartSelection.Reset();
	bMouseCaptured = false;
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
}
