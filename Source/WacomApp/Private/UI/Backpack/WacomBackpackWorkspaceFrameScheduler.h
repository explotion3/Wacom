// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

enum class EWacomBackpackWorkspacePresentationDirty : uint16
{
	None = 0,
	NavigationTargets = 1 << 0,
	CarryTopology = 1 << 1,
	CarryStrip = 1 << 2,
	StaticCards = 1 << 3,
	CardSemantics = 1 << 4,
	MotionTarget = 1 << 5,
	NavigationPresentation = 1 << 6,
	Accessibility = 1 << 7,
	Paint = 1 << 8,
	All = NavigationTargets
		| CarryTopology
		| CarryStrip
		| StaticCards
		| CardSemantics
		| MotionTarget
		| NavigationPresentation
		| Accessibility
		| Paint,
};
ENUM_CLASS_FLAGS(EWacomBackpackWorkspacePresentationDirty);

enum class EWacomBackpackWorkspaceFrameWork : uint16
{
	None = 0,
	GeometryStabilization = 1 << 0,
	CarryTracking = 1 << 1,
	PileTracking = 1 << 2,
	HoverExpandDelay = 1 << 3,
	BaseLayoutTransition = 1 << 4,
	Motion = 1 << 5,
	Settlement = 1 << 6,
	FocusExitDelay = 1 << 7,
	PileCollapse = 1 << 8,
	DeferredCardFaceRender = 1 << 9,
	SaleDeparture = 1 << 10,
};
ENUM_CLASS_FLAGS(EWacomBackpackWorkspaceFrameWork);

struct WACOMAPP_API FWacomBackpackWorkspacePresentationRequest
{
	EWacomBackpackWorkspacePresentationDirty Dirty =
		EWacomBackpackWorkspacePresentationDirty::None;
	TSet<FGuid> CardInstanceIds;
	bool bAllCards = false;

	bool IsEmpty() const
	{
		return Dirty == EWacomBackpackWorkspacePresentationDirty::None;
	}

	bool Has(EWacomBackpackWorkspacePresentationDirty Reasons) const
	{
		return EnumHasAnyFlags(Dirty, Reasons);
	}

	bool IncludesCard(FGuid InstanceId) const
	{
		return bAllCards || CardInstanceIds.Contains(InstanceId);
	}

	void Merge(
		EWacomBackpackWorkspacePresentationDirty Reasons,
		TConstArrayView<FGuid> InCardInstanceIds = {},
		bool bInAllCards = false);

	void Reset()
	{
		*this = FWacomBackpackWorkspacePresentationRequest();
	}
};

/**
 * Workspace 的 App-private 帧调度状态。
 *
 * 它不持有 UWidget，也不注册 Slate callback；Widget adapter 只负责把本对象的
 * pending request/work 映射到唯一 ActiveTimer。
 */
class WACOMAPP_API FWacomBackpackWorkspaceFrameScheduler
{
public:
	void RequestPresentation(
		EWacomBackpackWorkspacePresentationDirty Reasons,
		TConstArrayView<FGuid> CardInstanceIds = {},
		bool bAllCards = false);

	const FWacomBackpackWorkspacePresentationRequest& PeekPresentation() const
	{
		return PendingPresentation;
	}

	FWacomBackpackWorkspacePresentationRequest ConsumePresentation();

	void SetWork(EWacomBackpackWorkspaceFrameWork Work, bool bActive);
	bool HasWork(EWacomBackpackWorkspaceFrameWork Work) const;
	EWacomBackpackWorkspaceFrameWork GetActiveWork() const { return ActiveWork; }

	bool WantsFrame() const
	{
		return !PendingPresentation.IsEmpty()
			|| ActiveWork != EWacomBackpackWorkspaceFrameWork::None;
	}

	uint64 MarkTimerRegistered();
	void MarkTimerStopped(uint64 TimerGeneration);
	void InvalidateTimer();
	bool IsTimerCurrent(uint64 TimerGeneration) const;
	bool IsTimerRegistered() const { return bTimerRegistered; }
	uint64 GetTimerGeneration() const { return Generation; }

	uint64 BeginFrame();
	uint64 GetFrameSerial() const { return FrameSerial; }

	bool RequestGeometryStabilization();
	bool PushGeometrySample(
		FVector2D LayoutSize,
		float Tolerance,
		int32 RequiredStableSamples,
		FVector2D& OutStableLayoutSize);
	void CancelGeometryStabilization();

	void RequestDeferredCardFaceRender(bool bWakeFrame = true);
	void SuspendDeferredCardFaceRender();
	void ResumeDeferredCardFaceRender();
	bool IsDeferredCardFaceRenderPending() const
	{
		return bDeferredCardFaceRenderPending;
	}
	bool IsDeferredCardFaceRenderReady() const;
	void CompleteDeferredCardFaceRender();

	void Reset();

private:
	FWacomBackpackWorkspacePresentationRequest PendingPresentation;
	EWacomBackpackWorkspaceFrameWork ActiveWork =
		EWacomBackpackWorkspaceFrameWork::None;

	bool bTimerRegistered = false;
	uint64 Generation = 0;
	uint64 FrameSerial = 0;

	FVector2D PendingLayoutSize = FVector2D::ZeroVector;
	int32 StableLayoutSampleCount = 0;

	bool bDeferredCardFaceRenderPending = false;
	uint64 DeferredCardFaceRenderReadyFrame = MAX_uint64;
};
