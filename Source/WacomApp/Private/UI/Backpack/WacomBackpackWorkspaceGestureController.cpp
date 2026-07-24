// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceGestureController.h"

#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"
#include "UI/Backpack/WacomBackpackWorkspaceRuntimeHost.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackZonePileWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"

namespace
{
TArray<FGuid> BuildChangedGestureInstanceIds(
	const TConstArrayView<FGuid> Before,
	const TConstArrayView<FGuid> After)
{
	TArray<FGuid> Changed;
	for (const FGuid InstanceId : Before)
	{
		if (!After.Contains(InstanceId))
		{
			Changed.Add(InstanceId);
		}
	}
	for (const FGuid InstanceId : After)
	{
		if (!Before.Contains(InstanceId))
		{
			Changed.AddUnique(InstanceId);
		}
	}
	return Changed;
}
}

EWacomBackpackWorkspaceInputReply
FWacomBackpackWorkspaceGestureController::HandleCardPointerDown(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	UWacomDeckCardWidget* CardWidget,
	const FPointerEvent& Event,
	const bool bAllowPileHeaderReroute)
{
	if (!Host.IsValid())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	Host.RelinquishSemanticNavigationForPointerInput();
	Host.SyncExpandedPileLensInputLock(Event);
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	if (!Model || !CardWidget || Host.IsCarryInputSuspended())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}

	const FKey Button = Event.GetEffectingButton();
	if (Model->IsCarrying()
		&& (Button == EKeys::LeftMouseButton
			|| Button == EKeys::RightMouseButton))
	{
		Model->NotifyReleaseGestureStarted();
		return EWacomBackpackWorkspaceInputReply::CaptureAndFocus;
	}
	if (Button == EKeys::RightMouseButton)
	{
		return CardWidget->RequestBattleEnabledToggle()
			? EWacomBackpackWorkspaceInputReply::Handled
			: EWacomBackpackWorkspaceInputReply::Unhandled;
	}

	const FVector2D PointerLocal = Host.ToLocalPointer(Event);
	if (Button == EKeys::LeftMouseButton
		&& bAllowPileHeaderReroute
		&& TryBeginPileHeaderPress(
			Host,
			PointerLocal,
			Event,
			Event.IsControlDown()))
	{
		return EWacomBackpackWorkspaceInputReply::CaptureAndFocus;
	}
	if (Button != EKeys::LeftMouseButton || !CardWidget->IsMoveEnabled())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}

	if (!Event.IsControlDown()
		&& Model->BeginCarry(
			CardWidget->GetCardInstanceId(),
			PointerLocal,
			Host.GetCurrentStorageRevision()))
	{
		ClearCardPress();
		Host.NotifyCarryStarted(
			PointerLocal,
			Model->GetCarry().RemainingInstanceIds);
		return ResolveHandledPointerReply(Host);
	}

	if (Event.IsControlDown())
	{
		Host.BeginSelectionVisualFreeze(
			FWacomBackpackZoneKey::Make(
				CardWidget->GetWorkspaceDisplayZone(),
				CardWidget->GetWorkspaceDisplayOwnerInstanceId()));
	}
	Host.ClearExpandedPileFocus(true);
	Model->SetCardPressActive(true);
	BeginCardPress(
		CardWidget->GetCardInstanceId(),
		PointerLocal,
		Event.GetScreenSpacePosition(),
		Event.IsControlDown());
	return EWacomBackpackWorkspaceInputReply::CaptureAndFocus;
}

EWacomBackpackWorkspaceInputReply
FWacomBackpackWorkspaceGestureController::
	TryHandleExpandedPileVisualPointerDown(
		FWacomBackpackWorkspaceRuntimeHost& Host,
		const FVector2D PointerLocal,
		const FPointerEvent& Event)
{
	UWacomDeckCardWidget* VisualHitCard =
		Host.ResolveExpandedPileVisualCard(PointerLocal);
	return VisualHitCard
		? HandleCardPointerDown(Host, VisualHitCard, Event, false)
		: EWacomBackpackWorkspaceInputReply::Unhandled;
}

EWacomBackpackWorkspaceInputReply
FWacomBackpackWorkspaceGestureController::HandleCardPointerMove(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	const FPointerEvent& Event)
{
	if (!Host.IsValid())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	Host.RelinquishSemanticNavigationForPointerInput();
	Host.SyncExpandedPileLensInputLock(Event);
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	if (!Model || Host.IsCarryInputSuspended())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}

	const FVector2D PointerLocal = Host.ToLocalPointer(Event);
	if (Model->IsMarqueeActive())
	{
		Model->UpdateMarquee(PointerLocal);
		Host.InvalidatePaint();
		return ResolveHandledPointerReply(Host);
	}
	if (Model->IsCarrying())
	{
		Host.QueueCarryPointer(PointerLocal);
		Host.BroadcastInteractionChanged();
		return ResolveHandledPointerReply(Host);
	}
	if (TryBeginCarryFromPendingPress(Host, PointerLocal, Event))
	{
		return ResolveHandledPointerReply(Host);
	}
	Host.UpdateExpandedPileFocus(PointerLocal);
	if (Host.HasPresentationFocusedCard())
	{
		Host.UpdateMotionPointer(PointerLocal);
	}
	return ResolveHandledPointerReply(Host);
}

EWacomBackpackWorkspaceInputReply
FWacomBackpackWorkspaceGestureController::HandleCardPointerUp(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	const FPointerEvent& Event)
{
	if (!Host.IsValid())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	Host.RelinquishSemanticNavigationForPointerInput();
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	if (!Model || Host.IsCarryInputSuspended())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}

	const FKey Button = Event.GetEffectingButton();
	if (Model->IsMarqueeActive() && Button == EKeys::LeftMouseButton)
	{
		const TArray<FGuid> Previous =
			Model->GetSelection().OrderedSelectedInstanceIds;
		Model->UpdateMarquee(Host.ToLocalPointer(Event));
		Model->CompleteMarquee();
		Host.UpdateSelectionVisualFreezeLifetime();
		Host.ClearExpandedPileFocus(true);
		Host.NotifySelectionChanged(BuildChangedGestureInstanceIds(
			Previous,
			Model->GetSelection().OrderedSelectedInstanceIds));
		return EWacomBackpackWorkspaceInputReply::ReleaseCapture;
	}
	if (Model->IsCarrying())
	{
		Host.SyncCarryPointerForRelease(Host.ToLocalPointer(Event));
		Host.BroadcastPointerRelease(Button == EKeys::RightMouseButton);
		return ResolveHandledPointerReply(Host);
	}
	if (CardPress.bActive && Button == EKeys::LeftMouseButton)
	{
		const TArray<FGuid> Previous =
			Model->GetSelection().OrderedSelectedInstanceIds;
		Model->ClickCard(CardPress.InstanceId, CardPress.bControlDown);
		Model->SetCardPressActive(false);
		CardPress.Reset();
		Host.UpdateSelectionVisualFreezeLifetime();
		Host.NotifySelectionChanged(BuildChangedGestureInstanceIds(
			Previous,
			Model->GetSelection().OrderedSelectedInstanceIds));
		return EWacomBackpackWorkspaceInputReply::ReleaseCapture;
	}
	return EWacomBackpackWorkspaceInputReply::Unhandled;
}

EWacomBackpackWorkspaceInputReply
FWacomBackpackWorkspaceGestureController::HandlePilePointerDown(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	UWacomBackpackZonePileWidget* PileWidget,
	const FPointerEvent& Event)
{
	if (!Host.IsValid())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	Host.RelinquishSemanticNavigationForPointerInput();
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	if (!Model || !PileWidget || Host.IsCarryInputSuspended())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	if (Model->IsCarrying())
	{
		Model->NotifyReleaseGestureStarted();
		return EWacomBackpackWorkspaceInputReply::CaptureAndFocus;
	}
	if (Event.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}

	const FVector2D PointerLocal = Host.ToLocalPointer(Event);
	if (!PileWidget->WasLastPointerDownOnDragHandle()
		&& Host.DoesPileMatchExpandedFocus(*PileWidget))
	{
		const EWacomBackpackWorkspaceInputReply CardReply =
			TryHandleExpandedPileVisualPointerDown(
				Host,
				PointerLocal,
				Event);
		if (IsWacomBackpackInputHandled(CardReply))
		{
			return CardReply;
		}
	}
	BeginPendingPilePress(
		Host,
		*PileWidget,
		PointerLocal,
		Event,
		Event.IsControlDown(),
		PileWidget->WasLastPointerDownOnDragHandle());
	return EWacomBackpackWorkspaceInputReply::CaptureAndFocus;
}

void FWacomBackpackWorkspaceGestureController::BeginPendingPilePress(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	UWacomBackpackZonePileWidget& Pile,
	const FVector2D PointerLocal,
	const FPointerEvent& Event,
	const bool bControlDown,
	const bool bOnDragHandle)
{
	Host.SetExpandedPileLensInputLocked(false, false);
	const FSlateRect HeaderRect = Pile.GetResolvedHeaderRect();
	BeginPilePress(
		Pile,
		PointerLocal,
		Event.GetScreenSpacePosition(),
		FVector2D(HeaderRect.Left, HeaderRect.Top),
		bControlDown,
		bOnDragHandle);
}

bool FWacomBackpackWorkspaceGestureController::TryBeginPileHeaderPress(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	const FVector2D PointerLocal,
	const FPointerEvent& Event,
	const bool bControlDown)
{
	UWacomBackpackZonePileWidget* HeaderPile =
		Host.FindPileHeaderAt(PointerLocal);
	if (!HeaderPile)
	{
		return false;
	}
	Host.ClearExpandedPileFocus(true);
	BeginPendingPilePress(
		Host,
		*HeaderPile,
		PointerLocal,
		Event,
		bControlDown,
		true);
	return true;
}

bool FWacomBackpackWorkspaceGestureController::
	TryBeginCarryFromPendingPress(
		FWacomBackpackWorkspaceRuntimeHost& Host,
		const FVector2D PointerLocal,
		const FPointerEvent& Event)
{
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	if (!Model || !CardPress.bActive || !HasCardDragThreshold(Event))
	{
		return false;
	}
	const FGuid InstanceId = CardPress.InstanceId;
	if (!Model->IsSelected(InstanceId))
	{
		Model->ClickCard(InstanceId, false);
	}
	const bool bStarted = Model->BeginCarry(
		InstanceId,
		PointerLocal,
		Host.GetCurrentStorageRevision());
	CardPress.Reset();
	Model->SetCardPressActive(false);
	if (!bStarted)
	{
		return false;
	}
	Host.NotifyCarryStarted(
		PointerLocal,
		Model->GetCarry().RemainingInstanceIds);
	return true;
}

bool FWacomBackpackWorkspaceGestureController::TryBeginPileMove(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	const FVector2D PointerLocal,
	const FPointerEvent& Event)
{
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	UWacomBackpackZonePileWidget* Pile = PilePress.Pile.Get();
	if (!Model || !PilePress.bActive || !Pile
		|| !Pile->GetPileView().bMovable
		|| !PilePress.bOnDragHandle
		|| !HasPileDragThreshold(Event))
	{
		return false;
	}
	const FWacomBackpackZoneKey Zone = FWacomBackpackZoneKey::Make(
		Pile->GetPileView().Zone,
		Pile->GetPileView().OwnerInstanceId);
	Host.EndSelectionVisualFreeze(false);
	Host.ClearExpandedPileFocus(true);
	PileMoveSnapshot =
		Host.CapturePileMoveVisualSnapshot(*Pile, Zone);
	if (!Model->BeginPileMove(
		Zone,
		PilePress.LocalPosition,
		PilePress.PileStartPosition))
	{
		PileMoveSnapshot.Reset();
		return false;
	}
	PilePress.Reset();
	Host.QueuePilePointer(PointerLocal);
	Host.FlushPilePointer();
	Host.EnsureFrameSchedulerRunning();
	Host.BroadcastInteractionChanged();
	return true;
}

bool FWacomBackpackWorkspaceGestureController::
	TryBeginMarqueeFromPendingPilePress(
		FWacomBackpackWorkspaceRuntimeHost& Host,
		const FVector2D PointerLocal,
		const FPointerEvent& Event)
{
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	UWacomBackpackZonePileWidget* Pile = PilePress.Pile.Get();
	if (!Model || !PilePress.bActive || !Pile
		|| PilePress.bOnDragHandle
		|| !HasPileDragThreshold(Event))
	{
		return false;
	}
	const FWacomBackpackZoneKey SourceZone =
		FWacomBackpackZoneKey::Make(
			Pile->GetPileView().Zone,
			Pile->GetPileView().OwnerInstanceId);
	const TArray<FGuid> Previous =
		Model->GetSelection().OrderedSelectedInstanceIds;
	Host.BeginSelectionVisualFreeze(SourceZone);
	Model->BeginMarquee(
		SourceZone,
		PilePress.LocalPosition,
		PilePress.bControlDown);
	Model->UpdateMarquee(PointerLocal);
	PilePress.Reset();
	Host.NotifySelectionChanged(
		BuildChangedGestureInstanceIds(
			Previous,
			Model->GetSelection().OrderedSelectedInstanceIds),
		false);
	return Model->IsMarqueeActive();
}

bool FWacomBackpackWorkspaceGestureController::
	TryBeginMarqueeFromPendingBlankPress(
		FWacomBackpackWorkspaceRuntimeHost& Host,
		const FVector2D PointerLocal,
		const FPointerEvent& Event)
{
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	if (!Model || !MarqueePress.bActive
		|| !HasMarqueeDragThreshold(Event))
	{
		return false;
	}
	const TArray<FGuid> Previous =
		Model->GetSelection().OrderedSelectedInstanceIds;
	Host.BeginSelectionVisualFreeze(MarqueePress.SourceZone);
	Model->BeginMarquee(
		MarqueePress.SourceZone,
		MarqueePress.LocalPosition,
		MarqueePress.bControlDown);
	Model->UpdateMarquee(PointerLocal);
	Host.ReconcileExpandedPileFocusForMarqueeSource(
		MarqueePress.SourceZone);
	MarqueePress.Reset();
	Host.NotifySelectionChanged(
		BuildChangedGestureInstanceIds(
			Previous,
			Model->GetSelection().OrderedSelectedInstanceIds),
		false);
	return Model->IsMarqueeActive();
}

EWacomBackpackWorkspaceInputReply
FWacomBackpackWorkspaceGestureController::HandleWorkspacePointerDown(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	const FPointerEvent& Event)
{
	if (!Host.IsValid())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	Host.RelinquishSemanticNavigationForPointerInput();
	Host.SyncExpandedPileLensInputLock(Event);
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	if (!Model || Host.IsCarryInputSuspended())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	const FKey Button = Event.GetEffectingButton();
	const FVector2D PointerLocal = Host.ToLocalPointer(Event);
	if (!Model->IsCarrying()
		&& (Button == EKeys::LeftMouseButton
			|| Button == EKeys::RightMouseButton))
	{
		const EWacomBackpackWorkspaceInputReply CardReply =
			TryHandleExpandedPileVisualPointerDown(
				Host,
				PointerLocal,
				Event);
		if (IsWacomBackpackInputHandled(CardReply))
		{
			return CardReply;
		}
	}
	if (Button == EKeys::LeftMouseButton)
	{
		if (Model->IsCarrying())
		{
			Model->NotifyReleaseGestureStarted();
			return EWacomBackpackWorkspaceInputReply::CaptureAndFocus;
		}
		BeginMarqueePress(
			Host.ResolveMarqueeSource(PointerLocal),
			PointerLocal,
			Event.GetScreenSpacePosition(),
			Event.IsControlDown());
		return EWacomBackpackWorkspaceInputReply::CaptureAndFocus;
	}
	if (Model->IsCarrying() && Button == EKeys::RightMouseButton)
	{
		Model->NotifyReleaseGestureStarted();
		return EWacomBackpackWorkspaceInputReply::CaptureAndFocus;
	}
	return EWacomBackpackWorkspaceInputReply::Unhandled;
}

EWacomBackpackWorkspaceInputReply
FWacomBackpackWorkspaceGestureController::HandleWorkspacePointerMove(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	const FPointerEvent& Event)
{
	if (!Host.IsValid())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	Host.RelinquishSemanticNavigationForPointerInput();
	Host.SyncExpandedPileLensInputLock(Event);
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	if (!Model || Host.IsCarryInputSuspended())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	const FVector2D PointerLocal = Host.ToLocalPointer(Event);
	if (Model->IsCarrying())
	{
		Host.QueueCarryPointer(PointerLocal);
		Host.BroadcastInteractionChanged();
		return ResolveHandledPointerReply(Host);
	}
	if (Model->IsPileMoving())
	{
		Host.QueuePilePointer(PointerLocal);
		return ResolveHandledPointerReply(Host);
	}
	if (TryBeginPileMove(Host, PointerLocal, Event)
		|| TryBeginMarqueeFromPendingPilePress(
			Host,
			PointerLocal,
			Event)
		|| TryBeginMarqueeFromPendingBlankPress(
			Host,
			PointerLocal,
			Event)
		|| TryBeginCarryFromPendingPress(
			Host,
			PointerLocal,
			Event))
	{
		return ResolveHandledPointerReply(Host);
	}
	if (Model->IsMarqueeActive())
	{
		Model->UpdateMarquee(PointerLocal);
		Host.InvalidatePaint();
		return ResolveHandledPointerReply(Host);
	}
	Host.UpdateExpandedPileFocus(PointerLocal);
	return EWacomBackpackWorkspaceInputReply::Unhandled;
}

EWacomBackpackWorkspaceInputReply
FWacomBackpackWorkspaceGestureController::HandleWorkspacePointerUp(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	const FPointerEvent& Event)
{
	if (!Host.IsValid())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	Host.RelinquishSemanticNavigationForPointerInput();
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	if (!Model || Host.IsCarryInputSuspended())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	const FKey Button = Event.GetEffectingButton();
	const FVector2D PointerLocal = Host.ToLocalPointer(Event);
	if (Model->IsCarrying())
	{
		if (Button == EKeys::LeftMouseButton
			|| Button == EKeys::RightMouseButton)
		{
			Host.SyncCarryPointerForRelease(PointerLocal);
			Host.BroadcastPointerRelease(
				Button == EKeys::RightMouseButton);
			return ResolveHandledPointerReply(Host);
		}
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	if (Model->IsPileMoving() && Button == EKeys::LeftMouseButton)
	{
		Host.QueuePilePointer(PointerLocal);
		Host.FlushPilePointer();
		const FWacomBackpackWorkspacePileMoveState Completed =
			Model->CompletePileMove();
		const UWacomBackpackWorkspaceStyle& Style = Host.GetStyle();
		const FVector2D WorkspaceSize = Host.GetLayoutSpaceSize();
		const FVector2D HeaderSize(
			FMath::Max(260.0f, Style.PileCollapsedSize.X),
			48.0f);
		const FVector2D Snapped =
			FWacomBackpackWorkspaceLayoutSolver::
				ResolvePileHeaderOverlap(
					Completed.CurrentPosition,
					WorkspaceSize,
					HeaderSize,
					HeaderSize,
					Style.PileSnapGridPixels,
					Style.PileEdgeMarginPixels,
					Host.CollectOccupiedPileHeaders(Completed.Zone));
		Host.CommitPileMoveVisual(Completed, Snapped);
		PileMoveSnapshot.Reset();
		Host.BroadcastPileMoveCommitted(Completed, Snapped);
		PilePress.Reset();
		Host.ClearQueuedPilePointer();
		return EWacomBackpackWorkspaceInputReply::ReleaseCapture;
	}
	if (PilePress.bActive && Button == EKeys::LeftMouseButton)
	{
		Host.ClearExpandedPileFocus(true);
		Model->ClickBlank();
		Host.UpdateSelectionVisualFreezeLifetime();
		if (UWacomBackpackZonePileWidget* Pile = PilePress.Pile.Get())
		{
			Host.BroadcastPileExpansion(*Pile);
		}
		PilePress.Reset();
		return EWacomBackpackWorkspaceInputReply::ReleaseCapture;
	}
	if (Model->IsMarqueeActive() && Button == EKeys::LeftMouseButton)
	{
		const TArray<FGuid> Previous =
			Model->GetSelection().OrderedSelectedInstanceIds;
		Model->UpdateMarquee(PointerLocal);
		Model->CompleteMarquee();
		Host.UpdateSelectionVisualFreezeLifetime();
		Host.ClearExpandedPileFocus(true);
		Host.NotifySelectionChanged(BuildChangedGestureInstanceIds(
			Previous,
			Model->GetSelection().OrderedSelectedInstanceIds));
		return EWacomBackpackWorkspaceInputReply::ReleaseCapture;
	}
	if (MarqueePress.bActive && Button == EKeys::LeftMouseButton)
	{
		const TArray<FGuid> Previous =
			Model->GetSelection().OrderedSelectedInstanceIds;
		Model->ClickBlank();
		MarqueePress.Reset();
		Host.UpdateSelectionVisualFreezeLifetime();
		Host.ClearExpandedPileFocus(true);
		Host.NotifySelectionChanged(BuildChangedGestureInstanceIds(
			Previous,
			Model->GetSelection().OrderedSelectedInstanceIds));
		return EWacomBackpackWorkspaceInputReply::ReleaseCapture;
	}
	return EWacomBackpackWorkspaceInputReply::Unhandled;
}

EWacomBackpackWorkspaceInputReply
FWacomBackpackWorkspaceGestureController::HandleMouseWheel(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	const FPointerEvent& Event)
{
	if (!Host.IsValid())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	if (Host.IsCarryInputSuspended() || !Model || !Model->IsCarrying())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	const FWacomBackpackWorkspaceCarryState& PreviousCarry =
		Model->GetCarry();
	const int32 PreviousIndex = PreviousCarry.CurrentIndex;
	TArray<FGuid> ChangedInstanceIds;
	if (PreviousCarry.RemainingInstanceIds.IsValidIndex(
		PreviousCarry.CurrentIndex))
	{
		const FGuid PreviousId =
			PreviousCarry.RemainingInstanceIds[
				PreviousCarry.CurrentIndex];
		ChangedInstanceIds.Add(PreviousId);
		Host.RememberPreviousCarryCurrentCard(PreviousId);
	}
	Model->StepCurrentByWheel(Event.GetWheelDelta());
	const FWacomBackpackWorkspaceCarryState& CurrentCarry =
		Model->GetCarry();
	if (CurrentCarry.RemainingInstanceIds.IsValidIndex(
		CurrentCarry.CurrentIndex))
	{
		ChangedInstanceIds.AddUnique(
			CurrentCarry.RemainingInstanceIds[
				CurrentCarry.CurrentIndex]);
	}
	Host.NotifyCarryCurrentChanged(
		ChangedInstanceIds,
		true,
		CurrentCarry.CurrentIndex != PreviousIndex);
	return EWacomBackpackWorkspaceInputReply::Handled;
}

void FWacomBackpackWorkspaceGestureController::HandleMouseLeave(
	FWacomBackpackWorkspaceRuntimeHost& Host)
{
	if (Host.IsValid())
	{
		Host.BeginExpandedPileFocusExit();
	}
}

void FWacomBackpackWorkspaceGestureController::CancelPending(
	FWacomBackpackWorkspaceRuntimeHost& Host)
{
	if (PileMoveSnapshot.bValid && Host.IsValid())
	{
		Host.RestorePileMoveVisualSnapshot(PileMoveSnapshot);
	}
	Reset();
}

EWacomBackpackWorkspaceInputReply
FWacomBackpackWorkspaceGestureController::ResolveHandledPointerReply(
	const FWacomBackpackWorkspaceRuntimeHost& Host) const
{
	if (Host.IsCarryInputSuspended())
	{
		return EWacomBackpackWorkspaceInputReply::ReleaseCapture;
	}
	const FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	if ((Model && (Model->IsCarrying()
			|| Model->IsMarqueeActive()
			|| Model->IsPileMoving()))
		|| HasAnyPendingPress())
	{
		return EWacomBackpackWorkspaceInputReply::CaptureAndFocus;
	}
	return EWacomBackpackWorkspaceInputReply::ReleaseCapture;
}

void FWacomBackpackWorkspaceGestureController::BeginCardPress(
	FGuid InstanceId,
	FVector2D LocalPosition,
	FVector2D ScreenPosition,
	bool bControlDown)
{
	CardPress.InstanceId = InstanceId;
	CardPress.LocalPosition = LocalPosition;
	CardPress.ScreenPosition = ScreenPosition;
	CardPress.bControlDown = bControlDown;
	CardPress.bActive = true;
}

void FWacomBackpackWorkspaceGestureController::BeginPilePress(
	UWacomBackpackZonePileWidget& Pile,
	FVector2D LocalPosition,
	FVector2D ScreenPosition,
	FVector2D PileStartPosition,
	bool bControlDown,
	bool bOnDragHandle)
{
	PilePress.Pile = &Pile;
	PilePress.LocalPosition = LocalPosition;
	PilePress.ScreenPosition = ScreenPosition;
	PilePress.PileStartPosition = PileStartPosition;
	PilePress.bControlDown = bControlDown;
	PilePress.bOnDragHandle = bOnDragHandle;
	PilePress.bActive = true;
}

void FWacomBackpackWorkspaceGestureController::BeginMarqueePress(
	const FWacomBackpackZoneKey& SourceZone,
	FVector2D LocalPosition,
	FVector2D ScreenPosition,
	bool bControlDown)
{
	MarqueePress.SourceZone = SourceZone;
	MarqueePress.LocalPosition = LocalPosition;
	MarqueePress.ScreenPosition = ScreenPosition;
	MarqueePress.bControlDown = bControlDown;
	MarqueePress.bActive = true;
}

bool FWacomBackpackWorkspaceGestureController::HasCardDragThreshold(
	const FPointerEvent& Event) const
{
	return CardPress.bActive && HasDragThreshold(Event, CardPress.ScreenPosition);
}

bool FWacomBackpackWorkspaceGestureController::HasPileDragThreshold(
	const FPointerEvent& Event) const
{
	return PilePress.bActive && HasDragThreshold(Event, PilePress.ScreenPosition);
}

bool FWacomBackpackWorkspaceGestureController::HasMarqueeDragThreshold(
	const FPointerEvent& Event) const
{
	return MarqueePress.bActive && HasDragThreshold(Event, MarqueePress.ScreenPosition);
}

void FWacomBackpackWorkspaceGestureController::ResetPendingPresses()
{
	CardPress.Reset();
	PilePress.Reset();
	MarqueePress.Reset();
}

void FWacomBackpackWorkspaceGestureController::Reset()
{
	ResetPendingPresses();
	PileMoveSnapshot.Reset();
}

bool FWacomBackpackWorkspaceGestureController::HasDragThreshold(
	const FPointerEvent& Event,
	FVector2D ScreenOrigin)
{
	return FSlateApplication::IsInitialized()
		&& FSlateApplication::Get().HasTraveledFarEnoughToTriggerDrag(Event, ScreenOrigin);
}
