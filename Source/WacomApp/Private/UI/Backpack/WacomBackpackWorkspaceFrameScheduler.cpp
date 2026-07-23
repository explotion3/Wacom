// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceFrameScheduler.h"

void FWacomBackpackWorkspacePresentationRequest::Merge(
	EWacomBackpackWorkspacePresentationDirty Reasons,
	TConstArrayView<FGuid> InCardInstanceIds,
	bool bInAllCards)
{
	Dirty |= Reasons;
	if (bAllCards || bInAllCards)
	{
		bAllCards = true;
		CardInstanceIds.Reset();
		return;
	}
	for (const FGuid InstanceId : InCardInstanceIds)
	{
		if (InstanceId.IsValid())
		{
			CardInstanceIds.Add(InstanceId);
		}
	}
}

void FWacomBackpackWorkspaceFrameScheduler::RequestPresentation(
	EWacomBackpackWorkspacePresentationDirty Reasons,
	TConstArrayView<FGuid> CardInstanceIds,
	bool bAllCards)
{
	PendingPresentation.Merge(Reasons, CardInstanceIds, bAllCards);
}

FWacomBackpackWorkspacePresentationRequest
FWacomBackpackWorkspaceFrameScheduler::ConsumePresentation()
{
	FWacomBackpackWorkspacePresentationRequest Result = MoveTemp(PendingPresentation);
	PendingPresentation.Reset();
	return Result;
}

void FWacomBackpackWorkspaceFrameScheduler::SetWork(
	EWacomBackpackWorkspaceFrameWork Work,
	bool bActive)
{
	if (bActive)
	{
		ActiveWork |= Work;
	}
	else
	{
		ActiveWork &= ~Work;
	}
}

bool FWacomBackpackWorkspaceFrameScheduler::HasWork(
	EWacomBackpackWorkspaceFrameWork Work) const
{
	return EnumHasAnyFlags(ActiveWork, Work);
}

uint64 FWacomBackpackWorkspaceFrameScheduler::MarkTimerRegistered()
{
	if (!bTimerRegistered)
	{
		bTimerRegistered = true;
		++Generation;
	}
	return Generation;
}

void FWacomBackpackWorkspaceFrameScheduler::MarkTimerStopped(
	uint64 TimerGeneration)
{
	if (bTimerRegistered && Generation == TimerGeneration)
	{
		bTimerRegistered = false;
	}
}

void FWacomBackpackWorkspaceFrameScheduler::InvalidateTimer()
{
	bTimerRegistered = false;
	++Generation;
}

bool FWacomBackpackWorkspaceFrameScheduler::IsTimerCurrent(
	uint64 TimerGeneration) const
{
	return bTimerRegistered && Generation == TimerGeneration;
}

uint64 FWacomBackpackWorkspaceFrameScheduler::BeginFrame()
{
	return ++FrameSerial;
}

bool FWacomBackpackWorkspaceFrameScheduler::RequestGeometryStabilization()
{
	if (HasWork(EWacomBackpackWorkspaceFrameWork::GeometryStabilization))
	{
		return false;
	}
	PendingLayoutSize = FVector2D::ZeroVector;
	StableLayoutSampleCount = 0;
	SetWork(EWacomBackpackWorkspaceFrameWork::GeometryStabilization, true);
	return true;
}

bool FWacomBackpackWorkspaceFrameScheduler::PushGeometrySample(
	FVector2D LayoutSize,
	float Tolerance,
	int32 RequiredStableSamples,
	FVector2D& OutStableLayoutSize)
{
	if (!HasWork(EWacomBackpackWorkspaceFrameWork::GeometryStabilization)
		|| LayoutSize.X <= 1.0f || LayoutSize.Y <= 1.0f
		|| !FMath::IsFinite(LayoutSize.X) || !FMath::IsFinite(LayoutSize.Y))
	{
		return false;
	}

	if (!LayoutSize.Equals(PendingLayoutSize, FMath::Max(0.0f, Tolerance)))
	{
		PendingLayoutSize = LayoutSize;
		StableLayoutSampleCount = 1;
		return false;
	}

	++StableLayoutSampleCount;
	if (StableLayoutSampleCount < FMath::Max(1, RequiredStableSamples))
	{
		return false;
	}

	OutStableLayoutSize = LayoutSize;
	CancelGeometryStabilization();
	return true;
}

void FWacomBackpackWorkspaceFrameScheduler::CancelGeometryStabilization()
{
	SetWork(EWacomBackpackWorkspaceFrameWork::GeometryStabilization, false);
	PendingLayoutSize = FVector2D::ZeroVector;
	StableLayoutSampleCount = 0;
}

void FWacomBackpackWorkspaceFrameScheduler::RequestDeferredCardFaceRender(
	bool bWakeFrame)
{
	if (!bDeferredCardFaceRenderPending)
	{
		bDeferredCardFaceRenderPending = true;
		DeferredCardFaceRenderReadyFrame = FrameSerial + 1;
	}
	if (bWakeFrame)
	{
		SetWork(EWacomBackpackWorkspaceFrameWork::DeferredCardFaceRender, true);
	}
}

void FWacomBackpackWorkspaceFrameScheduler::SuspendDeferredCardFaceRender()
{
	SetWork(EWacomBackpackWorkspaceFrameWork::DeferredCardFaceRender, false);
}

void FWacomBackpackWorkspaceFrameScheduler::ResumeDeferredCardFaceRender()
{
	if (bDeferredCardFaceRenderPending)
	{
		SetWork(EWacomBackpackWorkspaceFrameWork::DeferredCardFaceRender, true);
	}
}

bool FWacomBackpackWorkspaceFrameScheduler::IsDeferredCardFaceRenderReady() const
{
	return bDeferredCardFaceRenderPending
		&& FrameSerial >= DeferredCardFaceRenderReadyFrame;
}

void FWacomBackpackWorkspaceFrameScheduler::CompleteDeferredCardFaceRender()
{
	bDeferredCardFaceRenderPending = false;
	DeferredCardFaceRenderReadyFrame = MAX_uint64;
	SetWork(EWacomBackpackWorkspaceFrameWork::DeferredCardFaceRender, false);
}

void FWacomBackpackWorkspaceFrameScheduler::Reset()
{
	PendingPresentation.Reset();
	ActiveWork = EWacomBackpackWorkspaceFrameWork::None;
	InvalidateTimer();
	FrameSerial = 0;
	PendingLayoutSize = FVector2D::ZeroVector;
	StableLayoutSampleCount = 0;
	bDeferredCardFaceRenderPending = false;
	DeferredCardFaceRenderReadyFrame = MAX_uint64;
}
