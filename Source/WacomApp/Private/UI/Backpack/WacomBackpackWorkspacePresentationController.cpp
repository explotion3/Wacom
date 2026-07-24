// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspacePresentationController.h"

#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "UI/Backpack/WacomBackpackWorkspaceFrameScheduler.h"
#include "UI/Backpack/WacomBackpackWorkspaceRuntimeHost.h"

namespace
{
constexpr float BackpackLayoutGeometryTolerance = 0.5f;
constexpr int32 BackpackRequiredStableLayoutSamples = 2;
}

void FWacomBackpackWorkspacePresentationController::RequestRefresh(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	const EWacomBackpackWorkspacePresentationDirty Reasons,
	const TConstArrayView<FGuid> CardInstanceIds,
	const bool bAllCards,
	const bool bFlushImmediately)
{
	if (!Host.IsValid())
	{
		return;
	}
	Host.GetFrameScheduler().RequestPresentation(
		Reasons,
		CardInstanceIds,
		bAllCards);
	if (bFlushImmediately)
	{
		Flush(Host);
	}
	else
	{
		Host.EnsureFrameSchedulerRunning();
	}
}

void FWacomBackpackWorkspacePresentationController::Flush(
	FWacomBackpackWorkspaceRuntimeHost& Host)
{
	if (!Host.IsValid())
	{
		return;
	}
	FWacomBackpackWorkspacePresentationRequest Request =
		Host.GetFrameScheduler().ConsumePresentation();
	if (Request.IsEmpty())
	{
		return;
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(Wacom_Backpack_PresentationPipelineFlush);
	Host.RecordPresentationFlush(Request);
	if (!Host.HasInteractionModel())
	{
		return;
	}

	if (Request.Has(
		EWacomBackpackWorkspacePresentationDirty::NavigationTargets))
	{
		Host.ApplyNavigationTargets();
	}
	if (Request.Has(
		EWacomBackpackWorkspacePresentationDirty::CarryTopology))
	{
		Host.ApplyCarryTopology();
	}
	if (Request.Has(EWacomBackpackWorkspacePresentationDirty::CarryStrip)
		&& Host.IsCarryStripDirty())
	{
		Host.ApplyCarryStrip();
	}
	if (Request.Has(EWacomBackpackWorkspacePresentationDirty::StaticCards))
	{
		Host.ApplyStaticCards(Request);
	}
	if (Request.Has(
		EWacomBackpackWorkspacePresentationDirty::CardSemantics))
	{
		Host.ApplyCardSemantics(Request);
	}
	if (Request.Has(
		EWacomBackpackWorkspacePresentationDirty::MotionTarget))
	{
		Host.ApplyMotionTarget();
	}
	if (Request.Has(
		EWacomBackpackWorkspacePresentationDirty::
			NavigationPresentation))
	{
		Host.ApplyNavigationPresentation(Request);
	}
	if (Request.Has(
		EWacomBackpackWorkspacePresentationDirty::Accessibility))
	{
		Host.ApplyAccessibility(Request);
	}
	if (Request.Has(EWacomBackpackWorkspacePresentationDirty::Paint))
	{
		Host.ApplyPaintInvalidation();
	}
	Host.CollapseCompatibilityMarquee();
}

void FWacomBackpackWorkspacePresentationController::WakeFrame(
	FWacomBackpackWorkspaceRuntimeHost& Host)
{
	if (!Host.IsValid())
	{
		return;
	}
	RefreshFrameWork(Host);
	Host.EnsureFrameSchedulerRunning();
}

void FWacomBackpackWorkspacePresentationController::RefreshFrameWork(
	FWacomBackpackWorkspaceRuntimeHost& Host)
{
	if (!Host.IsValid())
	{
		return;
	}
	FWacomBackpackWorkspaceFrameScheduler& Scheduler =
		Host.GetFrameScheduler();
	Scheduler.SetWork(
		EWacomBackpackWorkspaceFrameWork::CarryTracking,
		Host.IsCarrying() && !Host.IsCarryInputSuspended());
	Scheduler.SetWork(
		EWacomBackpackWorkspaceFrameWork::PileTracking,
		Host.IsPileMoving());
	Scheduler.SetWork(
		EWacomBackpackWorkspaceFrameWork::HoverExpandDelay,
		Host.IsHoverExpandDelayActive());
	Scheduler.SetWork(
		EWacomBackpackWorkspaceFrameWork::BaseLayoutTransition,
		Host.HasBaseLayoutTransitions());
	Scheduler.SetWork(
		EWacomBackpackWorkspaceFrameWork::Motion,
		Host.MotionWantsTick());
	Scheduler.SetWork(
		EWacomBackpackWorkspaceFrameWork::Settlement,
		Host.HasActiveSettlements());
	Scheduler.SetWork(
		EWacomBackpackWorkspaceFrameWork::FocusExitDelay,
		Host.IsFocusExitPending());
	Scheduler.SetWork(
		EWacomBackpackWorkspaceFrameWork::PileCollapse,
		Host.IsPileCollapsePending());
	Scheduler.SetWork(
		EWacomBackpackWorkspaceFrameWork::SaleDeparture,
		Host.CanAdvanceSaleDeparture());

	if (!Scheduler.IsDeferredCardFaceRenderPending())
	{
		return;
	}
	if (Host.CanWakeDeferredCardFaceRender())
	{
		Scheduler.ResumeDeferredCardFaceRender();
	}
	else
	{
		Scheduler.SuspendDeferredCardFaceRender();
	}
}

EActiveTimerReturnType
FWacomBackpackWorkspacePresentationController::TickFrame(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	const uint64 TimerGeneration,
	const float DeltaSeconds)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Wacom_Backpack_FrameSchedulerTick);
	if (!Host.IsValid())
	{
		return EActiveTimerReturnType::Stop;
	}
	FWacomBackpackWorkspaceFrameScheduler& Scheduler =
		Host.GetFrameScheduler();
	if (!Scheduler.IsTimerCurrent(TimerGeneration))
	{
		return EActiveTimerReturnType::Stop;
	}

	Scheduler.BeginFrame();
	Host.RecordFrameTick();
	Host.BeginFramePhaseRecording();

	// 帧内新请求保持 pending；本帧只在固定起点消费一次。
	Host.RecordFramePhase(TEXT("Presentation"));
	Flush(Host);

	Host.RecordFramePhase(TEXT("Geometry"));
	if (Scheduler.HasWork(
		EWacomBackpackWorkspaceFrameWork::GeometryStabilization))
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(
			Wacom_Backpack_FrameGeometryStabilization);
		FVector2D AcceptedLayoutSize;
		if (Scheduler.PushGeometrySample(
				Host.GetLocalGeometrySize(),
				BackpackLayoutGeometryTolerance,
				BackpackRequiredStableLayoutSamples,
				AcceptedLayoutSize))
		{
			Host.AcceptStableLayoutGeometry(AcceptedLayoutSize);
		}
	}

	const bool bCarrying =
		Host.IsCarrying() && !Host.IsCarryInputSuspended();
	const bool bPileMoving = Host.IsPileMoving();
	Host.RecordFramePhase(TEXT("PointerTracking"));
	FVector2D LatestPointer;
	if (bCarrying && Host.TryGetCursorLocalPosition(LatestPointer))
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(
			Wacom_Backpack_FrameCarryTracking);
		Host.UpdateCarryAnchor(LatestPointer);
	}
	if (bCarrying)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(
			Wacom_Backpack_FrameCarryVisualAnchor);
		Host.ApplyCarryVisualAnchor(DeltaSeconds);
	}
	if (bPileMoving && Host.TryGetCursorLocalPosition(LatestPointer))
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(
			Wacom_Backpack_FramePileTracking);
		Host.QueueAndFlushPilePointer(LatestPointer);
	}
	else if (!bPileMoving)
	{
		Host.ClearQueuedPilePointer();
	}

	if (Host.IsHoverExpandDelayActive())
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(
			Wacom_Backpack_FrameHoverExpandDelay);
		Host.AdvanceHoverExpandDelay(DeltaSeconds);
	}

	Host.RecordFramePhase(TEXT("LayoutMotion"));
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(
			Wacom_Backpack_FrameLayoutAndMotion);
		Host.AdvanceLayoutAndMotion(DeltaSeconds);
	}
	if (Scheduler.HasWork(
		EWacomBackpackWorkspaceFrameWork::SaleDeparture))
	{
		Host.RecordFramePhase(TEXT("SaleDeparture"));
		TRACE_CPUPROFILER_EVENT_SCOPE(
			Wacom_Backpack_FrameSaleDeparture);
		if (Host.AdvanceSaleDeparture(DeltaSeconds))
		{
			RequestRefresh(
				Host,
				EWacomBackpackWorkspacePresentationDirty::MotionTarget,
				{},
				false,
				false);
		}
	}
	Host.RecordFramePhase(TEXT("FocusSettlement"));
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(
			Wacom_Backpack_FrameFocusAndSettlement);
		Host.AdvanceFocusAndSettlement(DeltaSeconds);
	}

	Host.RecordFramePhase(TEXT("PileCollapse"));
	Host.CompletePileCollapseIfReady();
	if (Scheduler.IsDeferredCardFaceRenderReady())
	{
		Host.RecordFramePhase(TEXT("DeferredCardFaceRender"));
		TRACE_CPUPROFILER_EVENT_SCOPE(
			Wacom_Backpack_FrameDeferredCardFaceRender);
		Host.ExecuteDeferredCardFaceRender();
	}

	RefreshFrameWork(Host);
	if (!Scheduler.WantsFrame())
	{
		Scheduler.MarkTimerStopped(TimerGeneration);
		return EActiveTimerReturnType::Stop;
	}
	return EActiveTimerReturnType::Continue;
}
