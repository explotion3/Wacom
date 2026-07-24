// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceRuntimeHost.h"

#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "UI/Backpack/WacomBackpackCardWidgetTransfer.h"
#include "UI/Backpack/WacomBackpackWorkspaceFrameScheduler.h"
#include "UI/Backpack/WacomBackpackWorkspaceGestureController.h"
#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"
#include "UI/Backpack/WacomBackpackWorkspaceRuntime.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomBackpackZonePileWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"

namespace
{
FVector2D RotateBackpackRuntimeHostVector(
	const FVector2D Vector,
	const float AngleDegrees)
{
	const float Radians = FMath::DegreesToRadians(AngleDegrees);
	const float CosAngle = FMath::Cos(Radians);
	const float SinAngle = FMath::Sin(Radians);
	return FVector2D(
		Vector.X * CosAngle - Vector.Y * SinAngle,
		Vector.X * SinAngle + Vector.Y * CosAngle);
}
}

bool FWacomBackpackWorkspaceRuntimeHost::IsValid() const
{
	return ::IsValid(&Adapter) && Adapter.Runtime.IsValid();
}

bool FWacomBackpackWorkspaceRuntimeHost::HasInteractionModel() const
{
	return Adapter.InteractionModel.IsValid();
}

FWacomBackpackWorkspaceInteractionModel*
FWacomBackpackWorkspaceRuntimeHost::GetInteractionModel()
{
	return Adapter.InteractionModel.Get();
}

const FWacomBackpackWorkspaceInteractionModel*
FWacomBackpackWorkspaceRuntimeHost::GetInteractionModel() const
{
	return Adapter.InteractionModel.Get();
}

uint64 FWacomBackpackWorkspaceRuntimeHost::GetCurrentStorageRevision() const
{
	return Adapter.CurrentStorageRevision;
}

FWacomBackpackWorkspaceFrameScheduler&
FWacomBackpackWorkspaceRuntimeHost::GetFrameScheduler()
{
	return Adapter.GetRuntime().FrameScheduler;
}

const UWacomBackpackWorkspaceStyle&
FWacomBackpackWorkspaceRuntimeHost::GetStyle() const
{
	return Adapter.InteractionStyle.IsValid()
		? *Adapter.InteractionStyle.Get()
		: *GetDefault<UWacomBackpackWorkspaceStyle>();
}

FVector2D FWacomBackpackWorkspaceRuntimeHost::ToLocalPointer(
	const FPointerEvent& Event) const
{
	return Adapter.GetCachedGeometry().AbsoluteToLocal(
		Event.GetScreenSpacePosition());
}

void FWacomBackpackWorkspaceRuntimeHost::EnsureFrameSchedulerRunning()
{
	Adapter.EnsureFrameSchedulerRunning();
}

void FWacomBackpackWorkspaceRuntimeHost::FlushPresentation()
{
	if (IsValid())
	{
		Adapter.GetRuntime().Presentation.Flush(*this);
	}
}

void FWacomBackpackWorkspaceRuntimeHost::RefreshFrameWork()
{
	if (IsValid())
	{
		Adapter.GetRuntime().Presentation.RefreshFrameWork(*this);
	}
}

void FWacomBackpackWorkspaceRuntimeHost::RecordPresentationFlush(
	const FWacomBackpackWorkspacePresentationRequest& Request)
{
#if WITH_AUTOMATION_TESTS
	FWacomBackpackWorkspacePresentationController::FAutomationMetrics& Metrics =
		Adapter.GetRuntime().Presentation.AutomationMetrics;
	++Metrics.PresentationFlushCount;
	Metrics.bLastPresentationAppliedAllCards = Request.bAllCards;
	Metrics.LastPresentationAppliedInstanceIds =
		Request.CardInstanceIds.Array();
#else
	(void)Request;
#endif
}

void FWacomBackpackWorkspaceRuntimeHost::ApplyNavigationTargets()
{
	Adapter.ReconcileNavigationTargets();
#if WITH_AUTOMATION_TESTS
	++Adapter.GetRuntime().Presentation.AutomationMetrics
		.NavigationTargetsApplyCount;
#endif
}

void FWacomBackpackWorkspaceRuntimeHost::ApplyCarryTopology()
{
	Adapter.SyncCarryLayer();
#if WITH_AUTOMATION_TESTS
	++Adapter.GetRuntime().Presentation.AutomationMetrics
		.CarryTopologyApplyCount;
#endif
}

bool FWacomBackpackWorkspaceRuntimeHost::IsCarryStripDirty() const
{
	return Adapter.GetRuntime().Presentation.bCarryStripLayoutDirty;
}

void FWacomBackpackWorkspaceRuntimeHost::ApplyCarryStrip()
{
	Adapter.RebuildCarryStripLayout();
#if WITH_AUTOMATION_TESTS
	++Adapter.GetRuntime().Presentation.AutomationMetrics
		.CarryStripApplyCount;
#endif
}

void FWacomBackpackWorkspaceRuntimeHost::ApplyStaticCards(
	const FWacomBackpackWorkspacePresentationRequest& Request)
{
	const UWacomBackpackWorkspaceStyle& Style = GetStyle();
	Adapter.ForEachPresentationCard(
		Request,
		[&Adapter = Adapter, &Style](UWacomDeckCardWidget& Card)
		{
			Adapter.ApplyStaticCardPresentation(Card, Style);
		});
#if WITH_AUTOMATION_TESTS
	++Adapter.GetRuntime().Presentation.AutomationMetrics
		.StaticCardStageApplyCount;
#endif
}

void FWacomBackpackWorkspaceRuntimeHost::ApplyCardSemantics(
	const FWacomBackpackWorkspacePresentationRequest& Request)
{
	Adapter.ApplyCardSemanticsPresentation(Request);
#if WITH_AUTOMATION_TESTS
	++Adapter.GetRuntime().Presentation.AutomationMetrics
		.CardSemanticsStageApplyCount;
#endif
}

void FWacomBackpackWorkspaceRuntimeHost::ApplyMotionTarget()
{
	Adapter.ReconcileMotionTarget();
#if WITH_AUTOMATION_TESTS
	++Adapter.GetRuntime().Presentation.AutomationMetrics
		.MotionTargetApplyCount;
#endif
}

void FWacomBackpackWorkspaceRuntimeHost::ApplyNavigationPresentation(
	const FWacomBackpackWorkspacePresentationRequest& Request)
{
	Adapter.RefreshNavigationPresentation(Request);
#if WITH_AUTOMATION_TESTS
	++Adapter.GetRuntime().Presentation.AutomationMetrics
		.NavigationPresentationApplyCount;
#endif
}

void FWacomBackpackWorkspaceRuntimeHost::ApplyAccessibility(
	const FWacomBackpackWorkspacePresentationRequest& Request)
{
	Adapter.RefreshCardAccessibilityPresentation(Request);
#if WITH_AUTOMATION_TESTS
	++Adapter.GetRuntime().Presentation.AutomationMetrics
		.AccessibilityApplyCount;
#endif
}

void FWacomBackpackWorkspaceRuntimeHost::ApplyPaintInvalidation()
{
	Adapter.Invalidate(EInvalidateWidgetReason::Paint);
#if WITH_AUTOMATION_TESTS
	++Adapter.GetRuntime().Presentation.AutomationMetrics
		.PaintInvalidationApplyCount;
#endif
}

void FWacomBackpackWorkspaceRuntimeHost::CollapseCompatibilityMarquee()
{
	if (Adapter.SelectionMarquee)
	{
		Adapter.SelectionMarquee->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void FWacomBackpackWorkspaceRuntimeHost::RecordFrameTick()
{
#if WITH_AUTOMATION_TESTS
	++Adapter.GetRuntime().Presentation.AutomationMetrics
		.FrameSchedulerTickCount;
#endif
}

void FWacomBackpackWorkspaceRuntimeHost::BeginFramePhaseRecording()
{
#if WITH_AUTOMATION_TESTS
	Adapter.GetRuntime().Presentation.AutomationMetrics
		.LastFramePhaseOrder.Reset();
#endif
}

void FWacomBackpackWorkspaceRuntimeHost::RecordFramePhase(
	const FName Phase)
{
#if WITH_AUTOMATION_TESTS
	Adapter.GetRuntime().Presentation.AutomationMetrics
		.LastFramePhaseOrder.Add(Phase);
#else
	(void)Phase;
#endif
}

FVector2D FWacomBackpackWorkspaceRuntimeHost::GetLocalGeometrySize() const
{
	return Adapter.GetCachedGeometry().GetLocalSize();
}

FVector2D FWacomBackpackWorkspaceRuntimeHost::GetLayoutSpaceSize() const
{
	return Adapter.GetLayoutSpaceSize();
}

bool FWacomBackpackWorkspaceRuntimeHost::AcceptStableLayoutGeometry(
	const FVector2D LayoutSize)
{
	return Adapter.AcceptStableLayoutGeometry(LayoutSize);
}

bool FWacomBackpackWorkspaceRuntimeHost::IsCarrying() const
{
	return Adapter.InteractionModel
		&& Adapter.InteractionModel->IsCarrying();
}

bool FWacomBackpackWorkspaceRuntimeHost::IsPileMoving() const
{
	return Adapter.InteractionModel
		&& Adapter.InteractionModel->IsPileMoving();
}

bool FWacomBackpackWorkspaceRuntimeHost::IsCarryInputSuspended() const
{
	return Adapter.GetRuntime().Presentation.IsCarryInputSuspended();
}

void FWacomBackpackWorkspaceRuntimeHost::
	RelinquishSemanticNavigationForPointerInput()
{
	FWacomBackpackWorkspaceNavigationController& Navigation =
		Adapter.GetRuntime().Navigation;
	const bool bHadSemanticFocus =
		Navigation.IsSemanticFocusActive();
	Navigation.NotifyPointerInput();
	if (bHadSemanticFocus)
	{
		Adapter.OnBrowseFocusChangedNative.Broadcast(nullptr);
	}
}

void FWacomBackpackWorkspaceRuntimeHost::SyncExpandedPileLensInputLock(
	const FPointerEvent& Event)
{
	Adapter.SyncExpandedPileLensInputLockFromPointerEvent(Event);
}

void FWacomBackpackWorkspaceRuntimeHost::SetExpandedPileLensInputLocked(
	const bool bLocked,
	const bool bResumeImmediately)
{
	Adapter.SetExpandedPileLensInputLocked(bLocked, bResumeImmediately);
}

UWacomDeckCardWidget*
FWacomBackpackWorkspaceRuntimeHost::ResolveExpandedPileVisualCard(
	const FVector2D PointerLocal) const
{
	const FWacomBackpackWorkspacePresentationController& Presentation =
		Adapter.GetRuntime().Presentation;
	const FSlateRect& Header = Presentation.ExpandedPileFocus.HeaderRect;
	if (PointerLocal.X >= Header.Left && PointerLocal.X <= Header.Right
		&& PointerLocal.Y >= Header.Top && PointerLocal.Y <= Header.Bottom)
	{
		return nullptr;
	}
	const bool bStationary = PointerLocal.Equals(
		Presentation.ExpandedPileFocus.PointerLocal,
		0.5f);
	const int32 HitIndex = Adapter.ResolveExpandedPileVisualHitIndex(
		PointerLocal,
		bStationary
			? UWacomBackpackWorkspaceWidget::EExpandedPileHitResolveMode::
				StationaryRetention
			: UWacomBackpackWorkspaceWidget::EExpandedPileHitResolveMode::
				PointerAcquisition);
	return Presentation.ExpandedPileFocus.Cards.IsValidIndex(HitIndex)
		? Presentation.ExpandedPileFocus.Cards[HitIndex].Card.Get()
		: nullptr;
}

UWacomBackpackZonePileWidget*
FWacomBackpackWorkspaceRuntimeHost::FindPileHeaderAt(
	const FVector2D PointerLocal) const
{
	return Adapter.FindPileHeaderAt(PointerLocal);
}

bool FWacomBackpackWorkspaceRuntimeHost::DoesPileMatchExpandedFocus(
	const UWacomBackpackZonePileWidget& Pile) const
{
	const FWacomBackpackZonePileView& View = Pile.GetPileView();
	const FWacomBackpackWorkspacePresentationController& Presentation =
		Adapter.GetRuntime().Presentation;
	return View.bExpanded
		&& View.Zone == Presentation.ExpandedPileFocus.Zone
		&& (View.Zone != EZoneKind::SpecialZone
			|| View.OwnerInstanceId
				== Presentation.ExpandedPileFocus.OwnerInstanceId);
}

FWacomBackpackZoneKey
FWacomBackpackWorkspaceRuntimeHost::ResolveMarqueeSource(
	const FVector2D PointerLocal) const
{
	const FWacomBackpackWorkspacePresentationController& Presentation =
		Adapter.GetRuntime().Presentation;
	const FSlateRect& Bounds = Presentation.ExpandedContentBounds;
	if (Presentation.bHasExpandedContentBounds
		&& PointerLocal.X >= Bounds.Left
		&& PointerLocal.X <= Bounds.Right
		&& PointerLocal.Y >= Bounds.Top
		&& PointerLocal.Y <= Bounds.Bottom)
	{
		return FWacomBackpackZoneKey::Make(
			Presentation.ExpandedContentZone,
			Presentation.ExpandedContentOwnerInstanceId);
	}
	return FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
}

void FWacomBackpackWorkspaceRuntimeHost::
	ReconcileExpandedPileFocusForMarqueeSource(
		const FWacomBackpackZoneKey& SourceZone)
{
	const FWacomBackpackWorkspacePresentationController& Presentation =
		Adapter.GetRuntime().Presentation;
	const FWacomBackpackZoneKey FocusZone =
		FWacomBackpackZoneKey::Make(
			Presentation.ExpandedPileFocus.Zone,
			Presentation.ExpandedPileFocus.OwnerInstanceId);
	if (!(SourceZone == FocusZone)
		|| Presentation.ExpandedPileFocus.FocusIndex == INDEX_NONE)
	{
		Adapter.ClearExpandedPileFocus(true);
	}
}

void FWacomBackpackWorkspaceRuntimeHost::ClearExpandedPileFocus(
	const bool bAnimateReturn)
{
	Adapter.ClearExpandedPileFocus(bAnimateReturn);
}

void FWacomBackpackWorkspaceRuntimeHost::UpdateExpandedPileFocus(
	const FVector2D PointerLocal)
{
	Adapter.UpdateExpandedPileFocus(PointerLocal);
}

void FWacomBackpackWorkspaceRuntimeHost::BeginExpandedPileFocusExit()
{
	Adapter.BeginExpandedPileFocusExit();
}

bool FWacomBackpackWorkspaceRuntimeHost::HasPresentationFocusedCard() const
{
	return Adapter.GetPresentationFocusedCard() != nullptr;
}

void FWacomBackpackWorkspaceRuntimeHost::UpdateMotionPointer(
	const FVector2D PointerLocal)
{
	Adapter.GetRuntime().Motion.UpdatePointer(
		Adapter.GetCachedGeometry(),
		PointerLocal,
		false);
	Adapter.WakeFrameScheduler();
}

void FWacomBackpackWorkspaceRuntimeHost::BeginSelectionVisualFreeze(
	const FWacomBackpackZoneKey& SourceZone)
{
	Adapter.BeginSelectionVisualFreeze(SourceZone);
}

void FWacomBackpackWorkspaceRuntimeHost::EndSelectionVisualFreeze(
	const bool bAnimateReturn)
{
	Adapter.EndSelectionVisualFreeze(bAnimateReturn);
}

void FWacomBackpackWorkspaceRuntimeHost::
	UpdateSelectionVisualFreezeLifetime()
{
	Adapter.UpdateSelectionVisualFreezeLifetime();
}

void FWacomBackpackWorkspaceRuntimeHost::NotifyCarryStarted(
	const FVector2D PointerLocal,
	const TConstArrayView<FGuid> InstanceIds)
{
	SetExpandedPileLensInputLocked(false, false);
	Adapter.EndSelectionVisualFreeze(false);
	Adapter.GetRuntime().Presentation
		.bCarryCurrentExplicitlySelectedByWheel = false;
	Adapter.ClearExpandedPileFocus(true);
	Adapter.UpdateCarryAnchor(PointerLocal);
	Adapter.GetRuntime().Presentation.bCarryStripLayoutDirty = true;
	Adapter.BeginCarryPickupFeedback();
	Adapter.WakeFrameScheduler();
	Adapter.RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::NavigationTargets
			| EWacomBackpackWorkspacePresentationDirty::CarryTopology
			| EWacomBackpackWorkspacePresentationDirty::CarryStrip
			| EWacomBackpackWorkspacePresentationDirty::StaticCards
			| EWacomBackpackWorkspacePresentationDirty::CardSemantics
			| EWacomBackpackWorkspacePresentationDirty::MotionTarget
			| EWacomBackpackWorkspacePresentationDirty::
				NavigationPresentation
			| EWacomBackpackWorkspacePresentationDirty::Accessibility
			| EWacomBackpackWorkspacePresentationDirty::Paint,
		InstanceIds);
	BroadcastInteractionChanged();
}

void FWacomBackpackWorkspaceRuntimeHost::NotifySelectionChanged(
	const TConstArrayView<FGuid> ChangedInstanceIds,
	const bool bBroadcast)
{
	Adapter.RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::StaticCards
			| EWacomBackpackWorkspacePresentationDirty::CardSemantics
			| EWacomBackpackWorkspacePresentationDirty::MotionTarget
			| EWacomBackpackWorkspacePresentationDirty::Accessibility
			| EWacomBackpackWorkspacePresentationDirty::Paint,
		ChangedInstanceIds);
	if (bBroadcast)
	{
		BroadcastInteractionChanged();
	}
}

void FWacomBackpackWorkspaceRuntimeHost::InvalidatePaint()
{
	Adapter.Invalidate(EInvalidateWidgetReason::Paint);
}

void FWacomBackpackWorkspaceRuntimeHost::QueueCarryPointer(
	const FVector2D PointerLocal)
{
	Adapter.QueueCarryPointer(PointerLocal);
}

void FWacomBackpackWorkspaceRuntimeHost::SyncCarryPointerForRelease(
	const FVector2D PointerLocal)
{
	Adapter.SyncCarryPointerForRelease(PointerLocal);
}

void FWacomBackpackWorkspaceRuntimeHost::BroadcastPointerRelease(
	const bool bReleaseAll)
{
	Adapter.BroadcastPointerRelease(bReleaseAll);
}

void FWacomBackpackWorkspaceRuntimeHost::BroadcastInteractionChanged()
{
	Adapter.OnInteractionChangedNative.Broadcast();
}

void FWacomBackpackWorkspaceRuntimeHost::BroadcastPileExpansion(
	UWacomBackpackZonePileWidget& Pile)
{
	Adapter.OnPileExpansionRequestedNative.Broadcast(
		Pile.GetPileView().Zone,
		Pile.GetPileView().OwnerInstanceId,
		false);
}

FWacomBackpackPileMoveVisualSnapshot
FWacomBackpackWorkspaceRuntimeHost::CapturePileMoveVisualSnapshot(
	UWacomBackpackZonePileWidget& Pile,
	const FWacomBackpackZoneKey& Zone) const
{
	FWacomBackpackPileMoveVisualSnapshot Snapshot;
	if (const UCanvasPanelSlot* PileCanvasSlot =
		Cast<UCanvasPanelSlot>(Pile.Slot))
	{
		Snapshot.Pile = &Pile;
		Snapshot.Zone = Zone.Zone;
		Snapshot.OwnerInstanceId = Zone.OwnerInstanceId;
		Snapshot.CanvasPosition = PileCanvasSlot->GetPosition();
		Snapshot.ZOrder = PileCanvasSlot->GetZOrder();
		Snapshot.bValid = true;
	}
	return Snapshot;
}

void FWacomBackpackWorkspaceRuntimeHost::RestorePileMoveVisualSnapshot(
	const FWacomBackpackPileMoveVisualSnapshot& Snapshot)
{
	if (!Snapshot.bValid)
	{
		return;
	}
	if (UWacomBackpackZonePileWidget* Pile = Snapshot.Pile.Get())
	{
		if (UCanvasPanelSlot* PileCanvasSlot =
			Cast<UCanvasPanelSlot>(Pile->Slot))
		{
			PileCanvasSlot->SetPosition(Snapshot.CanvasPosition);
			PileCanvasSlot->SetZOrder(Snapshot.ZOrder);
		}
		Pile->SetRenderTranslation(FVector2D::ZeroVector);
	}
}

void FWacomBackpackWorkspaceRuntimeHost::QueuePilePointer(
	const FVector2D PointerLocal)
{
	Adapter.QueuePilePointer(PointerLocal);
}

void FWacomBackpackWorkspaceRuntimeHost::FlushPilePointer()
{
	Adapter.FlushQueuedPilePointer();
}

void FWacomBackpackWorkspaceRuntimeHost::ApplyActivePileMove()
{
	Adapter.ApplyActivePileMove();
}

TArray<FSlateRect>
FWacomBackpackWorkspaceRuntimeHost::CollectOccupiedPileHeaders(
	const FWacomBackpackZoneKey& ExcludedZone) const
{
	TArray<FSlateRect> Headers;
	for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile :
		Adapter.GetRegisteredPileWidgets())
	{
		const UWacomBackpackZonePileWidget* Pile = WeakPile.Get();
		if (!Pile || Pile->GetPileView().HasSameIdentity(
			ExcludedZone.Zone,
			ExcludedZone.OwnerInstanceId))
		{
			continue;
		}
		Headers.Add(Pile->GetResolvedHeaderRect());
	}
	return Headers;
}

void FWacomBackpackWorkspaceRuntimeHost::CommitPileMoveVisual(
	const FWacomBackpackWorkspacePileMoveState& Completed,
	const FVector2D SnappedTopLeft)
{
	const FVector2D FinalDelta = SnappedTopLeft - Completed.PileStart;
	Adapter.CommitPileMoveCardLayouts(Completed.Zone, FinalDelta);
	for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile :
		Adapter.GetRegisteredPileWidgets())
	{
		UWacomBackpackZonePileWidget* Pile = WeakPile.Get();
		if (!Pile || !Pile->GetPileView().HasSameIdentity(
			Completed.Zone.Zone,
			Completed.Zone.OwnerInstanceId))
		{
			continue;
		}
		const FSlateRect Frame = Pile->GetResolvedFrameRect();
		const FSlateRect Header = Pile->GetResolvedHeaderRect();
		const FVector2D FrameOffset(
			Frame.Left - Header.Left,
			Frame.Top - Header.Top);
		if (UCanvasPanelSlot* PileCanvasSlot =
			Cast<UCanvasPanelSlot>(Pile->Slot))
		{
			PileCanvasSlot->SetPosition(SnappedTopLeft + FrameOffset);
			Pile->SetRenderTranslation(FVector2D::ZeroVector);
		}
		break;
	}
}

void FWacomBackpackWorkspaceRuntimeHost::BroadcastPileMoveCommitted(
	const FWacomBackpackWorkspacePileMoveState& Completed,
	const FVector2D SnappedTopLeft)
{
	const FVector2D WorkspaceSize = Adapter.GetLayoutSpaceSize();
	Adapter.OnPileMoveCommittedNative.Broadcast(
		Completed.Zone.Zone,
		Completed.Zone.OwnerInstanceId,
		FVector2D(
			WorkspaceSize.X > 1.0f
				? SnappedTopLeft.X / WorkspaceSize.X
				: 0.0f,
			WorkspaceSize.Y > 1.0f
				? SnappedTopLeft.Y / WorkspaceSize.Y
				: 0.0f));
}

void FWacomBackpackWorkspaceRuntimeHost::RememberPreviousCarryCurrentCard(
	const FGuid InstanceId)
{
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard :
		Adapter.GetBoundCardWidgets())
	{
		UWacomDeckCardWidget* Card = WeakCard.Get();
		if (Card && Adapter.IsInCarryVisualLayer(Card)
			&& Card->GetCardInstanceId() == InstanceId)
		{
			Adapter.GetRuntime().Presentation.PreviousCarryCurrentCard =
				Card;
			return;
		}
	}
}

void FWacomBackpackWorkspaceRuntimeHost::NotifyCarryCurrentChanged(
	const TConstArrayView<FGuid> ChangedInstanceIds,
	const bool bIncludeCarryTopology,
	const bool bCurrentChanged)
{
	if (bCurrentChanged)
	{
		Adapter.GetRuntime().Presentation
			.bCarryCurrentExplicitlySelectedByWheel = true;
	}
	Adapter.GetRuntime().Presentation.bCarryStripLayoutDirty = true;
	EWacomBackpackWorkspacePresentationDirty Reasons =
		EWacomBackpackWorkspacePresentationDirty::CarryStrip
		| EWacomBackpackWorkspacePresentationDirty::StaticCards
		| EWacomBackpackWorkspacePresentationDirty::CardSemantics
		| EWacomBackpackWorkspacePresentationDirty::MotionTarget
		| EWacomBackpackWorkspacePresentationDirty::Accessibility
		| EWacomBackpackWorkspacePresentationDirty::Paint;
	if (bIncludeCarryTopology)
	{
		Reasons |=
			EWacomBackpackWorkspacePresentationDirty::CarryTopology;
	}
	Adapter.RequestPresentationRefresh(Reasons, ChangedInstanceIds);
	Adapter.WakeFrameScheduler();
	BroadcastInteractionChanged();
}

void FWacomBackpackWorkspaceRuntimeHost::
	ReconcileNavigationTargetsForInput()
{
	Adapter.ReconcileNavigationTargets();
}

void FWacomBackpackWorkspaceRuntimeHost::NotifyNavigationMoved(
	const TConstArrayView<FGuid> ChangedInstanceIds)
{
	Adapter.RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::StaticCards
			| EWacomBackpackWorkspacePresentationDirty::MotionTarget
			| EWacomBackpackWorkspacePresentationDirty::
				NavigationPresentation
			| EWacomBackpackWorkspacePresentationDirty::Accessibility
			| EWacomBackpackWorkspacePresentationDirty::Paint,
		ChangedInstanceIds);
	BroadcastInteractionChanged();
}

void FWacomBackpackWorkspaceRuntimeHost::BroadcastRelease(
	const bool bReleaseAll,
	const EWacomBackpackWorkspaceReleaseTargetKind TargetKind,
	const FWacomBackpackZoneKey& TargetZone)
{
	Adapter.BroadcastRelease(bReleaseAll, TargetKind, TargetZone);
}

void FWacomBackpackWorkspaceRuntimeHost::BroadcastPileExpansion(
	const FWacomBackpackZoneKey& Zone)
{
	Adapter.OnPileExpansionRequestedNative.Broadcast(
		Zone.Zone,
		Zone.OwnerInstanceId,
		false);
}

UWacomDeckCardWidget*
FWacomBackpackWorkspaceRuntimeHost::FindBoundCard(
	const FGuid InstanceId) const
{
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard :
		Adapter.GetBoundCardWidgets())
	{
		UWacomDeckCardWidget* Card = WeakCard.Get();
		if (Card && Card->GetCardInstanceId() == InstanceId)
		{
			return Card;
		}
	}
	return nullptr;
}

void FWacomBackpackWorkspaceRuntimeHost::
	BroadcastControlsHelpRequested()
{
	Adapter.OnControlsHelpRequestedNative.Broadcast();
}

bool FWacomBackpackWorkspaceRuntimeHost::
	IsExpandedPileLensInputLocked() const
{
	return Adapter.GetRuntime().Presentation
		.bExpandedPileLensInputLocked;
}

bool FWacomBackpackWorkspaceRuntimeHost::
	HasCancelableInteraction() const
{
	return (Adapter.InteractionModel
			&& (Adapter.InteractionModel->IsCarrying()
				|| Adapter.InteractionModel->IsMarqueeActive()
				|| Adapter.InteractionModel->IsPileMoving()))
		|| Adapter.GetRuntime().Gesture.HasAnyPendingPress();
}

void FWacomBackpackWorkspaceRuntimeHost::CancelInteraction(
	const bool bAnimateCarryReturn)
{
	if (!IsValid())
	{
		return;
	}
	FWacomBackpackWorkspaceRuntime& Runtime = Adapter.GetRuntime();
	FWacomBackpackWorkspaceInteractionModel* Model =
		Adapter.InteractionModel.Get();
	if (bAnimateCarryReturn && Model && Model->IsCarrying()
		&& !Runtime.Presentation.IsSimplifiedMotion())
	{
		if (!Adapter.SettlementLayer)
		{
			Adapter.EnsureFallbackTree();
		}
		const UWacomBackpackWorkspaceStyle& Style = GetStyle();
		const TArray<FGuid> ReturningIds =
			Model->GetCarry().RemainingInstanceIds;
		Adapter.CaptureReleasedVisualPoses(ReturningIds);
		for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard :
			Adapter.GetBoundCardWidgets())
		{
			UWacomDeckCardWidget* Card = WeakCard.Get();
			if (!Card
				|| !ReturningIds.Contains(Card->GetCardInstanceId()))
			{
				continue;
			}
			const FWacomBackpackWorkspaceCardLayout* Target =
				Adapter.GetVisualState().BaseLayouts().Find(Card);
			const FWacomBackpackWorkspaceCardVisualPose* Start =
				Adapter.GetVisualState().FindReleasedVisualPose(
					Card->GetCardInstanceId());
			if (!Target || !Start || !Adapter.SettlementLayer)
			{
				continue;
			}
			Wacom::Backpack::ReparentCardPreservingSlate(
				*Adapter.SettlementLayer,
				*Card);
			Adapter.ApplyCardLayout(
				*Card,
				Target->Center,
				Target->Size,
				Target->AngleDegrees,
				Target->ZOrder);
			Adapter.GetVisualState().SetSettlementTarget(
				*Card,
				*Target);
			Runtime.Motion.BeginSettlement(
				*Card,
				RotateBackpackRuntimeHostVector(
					Start->Center - Target->Center,
					-Target->AngleDegrees),
				FMath::FindDeltaAngleDegrees(
					Target->AngleDegrees,
					Start->AngleDegrees),
				Style.CancelReturnSeconds,
				false);
		}
		Runtime.Gesture.CancelPending(*this);
		Model->CancelTransientState();
		Adapter.StopFrameScheduler();
		Runtime.Presentation.bHasQueuedCarryPointer = false;
		Runtime.Presentation.bHasQueuedPilePointer = false;
		Runtime.Presentation.bCarryStripLayoutDirty = false;
		Adapter.GetVisualState().ResetReleasedHandoffs();
		Runtime.Presentation.bCarryVisualAnchorInitialized = false;
		if (Adapter.CarryRoot)
		{
			Adapter.CarryRoot->SetRenderTranslation(
				FVector2D::ZeroVector);
		}
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ReleaseAllPointerCapture(0);
		}
		Adapter.RequestPresentationRefresh(
			EWacomBackpackWorkspacePresentationDirty::All,
			{},
			true);
		Adapter.WakeFrameScheduler();
		return;
	}

	Adapter.SetExpandedPileLensInputLocked(false, false);
	Adapter.SetPileDropFeedback(
		EZoneKind::Backpack,
		FGuid(),
		FWacomBackpackDropFeedbackView());
	Adapter.CancelHoverExpandTimer();
	Adapter.ClearExpandedPileFocus(false);
	Adapter.EndSelectionVisualFreeze(false);
	Runtime.Gesture.CancelPending(*this);
	Runtime.Presentation.SetCarryInputSuspended(false);
	Runtime.Presentation.bPileCollapseAnimationPending = false;
	if (Model)
	{
		Model->CancelTransientState();
	}
	Adapter.StopFrameScheduler();
	Runtime.Presentation.bHasQueuedCarryPointer = false;
	Runtime.Presentation.bHasQueuedPilePointer = false;
	Runtime.Presentation.bCarryStripLayoutDirty = false;
	Runtime.Presentation.bCarryCurrentExplicitlySelectedByWheel = false;
	Runtime.Presentation.PreviousCarryCurrentCard.Reset();
	Adapter.GetVisualState().ResetTransientMotion();
	Runtime.Motion.Reset();
	if (Adapter.SettlementLayer && Adapter.StaticCardLayer)
	{
		TArray<UWacomDeckCardWidget*> SettlingCards;
		for (int32 Index = 0;
			Index < Adapter.SettlementLayer->GetChildrenCount();
			++Index)
		{
			if (UWacomDeckCardWidget* Card =
				Cast<UWacomDeckCardWidget>(
					Adapter.SettlementLayer->GetChildAt(Index)))
			{
				if (!Adapter.IsSaleDepartureCard(Card))
				{
					SettlingCards.Add(Card);
				}
			}
		}
		for (UWacomDeckCardWidget* Card : SettlingCards)
		{
			Wacom::Backpack::ReparentCardPreservingSlate(
				*Adapter.StaticCardLayer,
				*Card);
			if (const FWacomBackpackWorkspaceCardLayout* Base =
				Adapter.GetVisualState().BaseLayouts().Find(Card))
			{
				Adapter.ApplyCardLayout(
					*Card,
					Base->Center,
					Base->Size,
					Base->AngleDegrees,
					Base->ZOrder);
			}
		}
	}
	Adapter.RestoreStaticCardParents();
	Runtime.Presentation.bCarryVisualAnchorInitialized = false;
	Runtime.Presentation.CarryAnchorLocal = FVector2D::ZeroVector;
	Runtime.Presentation.CarryVisualAnchorLocal =
		FVector2D::ZeroVector;
	Adapter.CancelHoverExpandTimer();
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().ReleaseAllPointerCapture(0);
	}
	Adapter.RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::All,
		{},
		true);
	Adapter.WakeFrameScheduler();
}

bool FWacomBackpackWorkspaceRuntimeHost::HasExpandedContent() const
{
	return Adapter.GetRuntime().Presentation
		.bHasExpandedContentBounds;
}

void FWacomBackpackWorkspaceRuntimeHost::
	BroadcastCollapseExpandedPileRequested()
{
	Adapter.OnCollapseExpandedPileRequestedNative.Broadcast();
}

bool FWacomBackpackWorkspaceRuntimeHost::TryGetCursorLocalPosition(
	FVector2D& OutPointerLocal) const
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}
	OutPointerLocal = Adapter.GetCachedGeometry().AbsoluteToLocal(
		FSlateApplication::Get().GetCursorPos());
	return true;
}

void FWacomBackpackWorkspaceRuntimeHost::UpdateCarryAnchor(
	const FVector2D PointerLocal)
{
	Adapter.UpdateCarryAnchor(PointerLocal);
}

void FWacomBackpackWorkspaceRuntimeHost::ApplyCarryVisualAnchor(
	const float DeltaSeconds)
{
	Adapter.ApplyCarryVisualAnchor(DeltaSeconds);
}

void FWacomBackpackWorkspaceRuntimeHost::QueueAndFlushPilePointer(
	const FVector2D PointerLocal)
{
	Adapter.QueuePilePointer(PointerLocal);
	Adapter.FlushQueuedPilePointer();
}

void FWacomBackpackWorkspaceRuntimeHost::ClearQueuedPilePointer()
{
	Adapter.GetRuntime().Presentation.bHasQueuedPilePointer = false;
}

void FWacomBackpackWorkspaceRuntimeHost::AdvanceHoverExpandDelay(
	const float DeltaSeconds)
{
	FWacomBackpackWorkspacePresentationController& Presentation =
		Adapter.GetRuntime().Presentation;
	if (!Presentation.bHoverExpandTimerActive)
	{
		return;
	}
	if (!Adapter.InteractionModel
		|| !Adapter.InteractionModel->IsCarrying())
	{
		Adapter.CancelHoverExpandTimer();
		return;
	}

	Presentation.HoverExpandElapsedSeconds +=
		FMath::Max(0.0f, DeltaSeconds);
	if (Presentation.HoverExpandElapsedSeconds
		< FMath::Max(0.0f, GetStyle().PileHoverExpandDelaySeconds))
	{
		return;
	}

	const EZoneKind ExpandZone = Presentation.HoverExpandZone;
	const FGuid ExpandOwner = Presentation.HoverExpandOwnerInstanceId;
	Adapter.CancelHoverExpandTimer();
	Adapter.OnPileExpansionRequestedNative.Broadcast(
		ExpandZone,
		ExpandOwner,
		true);
}

void FWacomBackpackWorkspaceRuntimeHost::AdvanceLayoutAndMotion(
	const float DeltaSeconds)
{
	AdvanceBaseCardLayoutTransitions(DeltaSeconds);
	if (Adapter.Runtime)
	{
		Adapter.GetRuntime().Motion.Tick(
			DeltaSeconds,
			Adapter.GetCachedGeometry(),
			GetStyle(),
			Adapter.GetRuntime().Presentation.IsSimplifiedMotion());
	}
}

bool FWacomBackpackWorkspaceRuntimeHost::AdvanceBaseCardLayoutTransitions(
	const float DeltaSeconds)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Wacom_Backpack_BaseLayoutTransitions);
	if (Adapter.GetVisualState().BaseTransitions().IsEmpty())
	{
		return false;
	}
#if WITH_AUTOMATION_TESTS
	++Adapter.GetRuntime().Presentation.AutomationMetrics
		.BaseCardLayoutTransitionTickCount;
#endif
	const UWacomBackpackWorkspaceStyle& Style = GetStyle();
	int32 AppliedCount = 0;
	const bool bStillMoving =
		Adapter.GetVisualState().TickBaseTransitions(
			DeltaSeconds,
			Adapter.GetRuntime().Presentation.IsSimplifiedMotion(),
			[this, &Style](UWacomDeckCardWidget& CardWidget)
			{
				Adapter.ApplyStaticCardPresentation(CardWidget, Style);
			},
			&AppliedCount);
#if WITH_AUTOMATION_TESTS
	Adapter.GetRuntime().Presentation.AutomationMetrics
		.BaseCardLayoutTransitionApplyCount += AppliedCount;
#endif
	return bStillMoving;
}

bool FWacomBackpackWorkspaceRuntimeHost::AdvanceSaleDeparture(
	const float DeltaSeconds)
{
	const bool bHadWork =
		Adapter.GetRuntime().SaleDeparture.HasWork();
	Adapter.GetRuntime().SaleDeparture.Tick(
		DeltaSeconds,
		&Adapter);
	return bHadWork
		&& !Adapter.GetRuntime().SaleDeparture.HasWork();
}

void FWacomBackpackWorkspaceRuntimeHost::AdvanceFocusAndSettlement(
	const float DeltaSeconds)
{
	RefreshExpandedPileVisualHitAtCachedPointer();
	AdvanceExpandedPileFocusExit(DeltaSeconds);
	FinalizeCompletedSettlements();
}

void FWacomBackpackWorkspaceRuntimeHost::
	RefreshExpandedPileVisualHitAtCachedPointer()
{
	FWacomBackpackWorkspacePresentationController& Presentation =
		Adapter.GetRuntime().Presentation;
	const FSlateRect& Header =
		Presentation.ExpandedPileFocus.HeaderRect;
	const FVector2D Pointer =
		Presentation.ExpandedPileFocus.PointerLocal;
	const bool bInsideHeader =
		Pointer.X >= Header.Left && Pointer.X <= Header.Right
		&& Pointer.Y >= Header.Top && Pointer.Y <= Header.Bottom;
	if (!Adapter.IsExpandedPileFocusAllowed() || bInsideHeader)
	{
		return;
	}
	const int32 HitIndex = Adapter.ResolveExpandedPileVisualHitIndex(
		Pointer,
		UWacomBackpackWorkspaceWidget::EExpandedPileHitResolveMode::
			StationaryRetention);
	if (HitIndex != INDEX_NONE)
	{
		Presentation.ExpandedPileFocus.bExitPending = false;
		Presentation.ExpandedPileFocus.ExitDelayRemainingSeconds = 0.0f;
		Adapter.SetExpandedPileFocusIndex(HitIndex);
	}
	else
	{
		Adapter.BeginExpandedPileFocusExit();
	}
}

void FWacomBackpackWorkspaceRuntimeHost::AdvanceExpandedPileFocusExit(
	const float DeltaSeconds)
{
	FWacomBackpackWorkspacePresentationController& Presentation =
		Adapter.GetRuntime().Presentation;
	if (!Presentation.ExpandedPileFocus.bExitPending)
	{
		return;
	}
	Presentation.ExpandedPileFocus.ExitDelayRemainingSeconds -=
		FMath::Max(0.0f, DeltaSeconds);
	if (Presentation.ExpandedPileFocus.ExitDelayRemainingSeconds <= 0.0f)
	{
		Adapter.ClearExpandedPileFocus(true);
	}
}

void FWacomBackpackWorkspaceRuntimeHost::FinalizeCompletedSettlements()
{
	if (!Adapter.Runtime || !Adapter.StaticCardLayer)
	{
		return;
	}
	TArray<TWeakObjectPtr<UWacomDeckCardWidget>> Completed;
	Adapter.GetRuntime().Motion.ConsumeCompletedSettlements(Completed);
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : Completed)
	{
		UWacomDeckCardWidget* Card = WeakCard.Get();
		FWacomBackpackWorkspaceCardLayout Final;
		if (!Adapter.GetVisualState().TakeSettlementTarget(WeakCard, Final))
		{
			continue;
		}
		Wacom::Backpack::ReparentCardPreservingSlate(
			*Adapter.StaticCardLayer,
			*Card);
		Card->ResetBackpackLocalMotionPose();
		Adapter.ApplyCardLayout(
			*Card,
			Final.Center,
			Final.Size,
			Final.AngleDegrees,
			Final.ZOrder);
	}
	if (!Adapter.GetVisualState().HasActiveSettlements()
		&& !Adapter.GetVisualState().HasReleasedHandoffs()
		&& (!Adapter.InteractionModel
			|| !Adapter.InteractionModel->IsCarrying()))
	{
		if (Adapter.CarryRoot)
		{
			Adapter.CarryRoot->SetRenderTranslation(
				FVector2D::ZeroVector);
		}
		Adapter.GetRuntime().Presentation
			.bCarryVisualAnchorInitialized = false;
	}
}

void FWacomBackpackWorkspaceRuntimeHost::CompletePileCollapseIfReady()
{
	FWacomBackpackWorkspacePresentationController& Presentation =
		Adapter.GetRuntime().Presentation;
	if (!Presentation.bPileCollapseAnimationPending
		|| !Adapter.GetVisualState().BaseTransitions().IsEmpty())
	{
		return;
	}

	Presentation.bPileCollapseAnimationPending = false;
	Presentation.SetCarryInputSuspended(false);
	Adapter.OnPileCollapseAnimationFinishedNative.Broadcast(
		Presentation.CollapsingPileZone,
		Presentation.CollapsingPileOwnerInstanceId);
}

void FWacomBackpackWorkspaceRuntimeHost::ExecuteDeferredCardFaceRender()
{
	FWacomBackpackWorkspaceFrameScheduler& Scheduler =
		Adapter.GetRuntime().FrameScheduler;
	if (!Scheduler.IsDeferredCardFaceRenderPending()
		|| !Adapter.bCardFaceRetainedRenderingEnabled
		|| !Adapter.GetRuntime().Presentation.bHasStableLayoutSize
		|| !Adapter.StaticCardLayer
		|| Adapter.StaticCardLayer->GetVisibility()
			== ESlateVisibility::Hidden
		|| Adapter.StaticCardLayer->GetVisibility()
			== ESlateVisibility::Collapsed)
	{
		Scheduler.SuspendDeferredCardFaceRender();
		return;
	}

	Scheduler.CompleteDeferredCardFaceRender();
	Adapter.RequestBoundCardFaceRenders();
	++Adapter.DeferredCardFaceRenderPassCount;
}

bool FWacomBackpackWorkspaceRuntimeHost::IsHoverExpandDelayActive() const
{
	return Adapter.GetRuntime().Presentation.bHoverExpandTimerActive;
}

bool FWacomBackpackWorkspaceRuntimeHost::HasBaseLayoutTransitions() const
{
	return !Adapter.GetVisualState().BaseTransitions().IsEmpty();
}

bool FWacomBackpackWorkspaceRuntimeHost::MotionWantsTick() const
{
	return Adapter.Runtime
		&& Adapter.GetRuntime().Motion.WantsTick();
}

bool FWacomBackpackWorkspaceRuntimeHost::HasActiveSettlements() const
{
	return Adapter.GetVisualState().HasActiveSettlements();
}

bool FWacomBackpackWorkspaceRuntimeHost::IsFocusExitPending() const
{
	return Adapter.GetRuntime().Presentation.ExpandedPileFocus.bExitPending;
}

bool FWacomBackpackWorkspaceRuntimeHost::IsPileCollapsePending() const
{
	return Adapter.GetRuntime().Presentation.bPileCollapseAnimationPending;
}

bool FWacomBackpackWorkspaceRuntimeHost::CanAdvanceSaleDeparture() const
{
	const ESlateVisibility Visibility = Adapter.SettlementLayer
		? Adapter.SettlementLayer->GetVisibility()
		: ESlateVisibility::Collapsed;
	return Adapter.GetRuntime().SaleDeparture.HasWork()
		&& Adapter.bCardFaceRetainedRenderingEnabled
		&& Adapter.GetRuntime().Presentation.bHasStableLayoutSize
		&& Adapter.SettlementLayer
		&& Visibility != ESlateVisibility::Hidden
		&& Visibility != ESlateVisibility::Collapsed;
}

bool FWacomBackpackWorkspaceRuntimeHost::CanWakeDeferredCardFaceRender() const
{
	const ESlateVisibility Visibility = Adapter.StaticCardLayer
		? Adapter.StaticCardLayer->GetVisibility()
		: ESlateVisibility::Collapsed;
	return Adapter.bCardFaceRetainedRenderingEnabled
		&& Adapter.GetRuntime().Presentation.bHasStableLayoutSize
		&& Adapter.StaticCardLayer
		&& Visibility != ESlateVisibility::Hidden
		&& Visibility != ESlateVisibility::Collapsed;
}
