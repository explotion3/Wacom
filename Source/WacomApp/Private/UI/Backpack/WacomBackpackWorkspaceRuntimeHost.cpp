// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceRuntimeHost.h"

#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "UI/Backpack/WacomBackpackCardWidgetTransfer.h"
#include "UI/Backpack/WacomBackpackWorkspaceFrameScheduler.h"
#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "UI/Backpack/WacomBackpackWorkspaceRuntime.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"

bool FWacomBackpackWorkspaceRuntimeHost::IsValid() const
{
	return ::IsValid(&Adapter) && Adapter.Runtime.IsValid();
}

bool FWacomBackpackWorkspaceRuntimeHost::HasInteractionModel() const
{
	return Adapter.InteractionModel.IsValid();
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
