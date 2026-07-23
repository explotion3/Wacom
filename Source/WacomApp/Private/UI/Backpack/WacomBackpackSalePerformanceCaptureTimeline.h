// Copyright Wacom. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "WacomBackpackPerformanceCaptureTimeline.h"

/**
 * 出售表现专用短采样时间线。
 *
 * 规则事务和材质离场在 Interaction 区域开始后自动触发；Opening 与
 * Finalizing 继续保持在测量区域之外，避免几何初始化和 Trace.Stop 污染结果。
 */
struct FWacomBackpackSalePerformanceCaptureTimeline
{
	static constexpr double WarmupSeconds = 5.0;
	static constexpr double ClosedSeconds = 5.0;
	static constexpr double OpeningStabilizationSeconds = 3.0;
	static constexpr double IdleSeconds = 3.0;
	static constexpr double InteractionSeconds = 5.0;
	static constexpr double FinalizingSeconds = 0.25;

	static constexpr double ClosedStartSeconds = WarmupSeconds;
	static constexpr double OpeningStartSeconds = ClosedStartSeconds + ClosedSeconds;
	static constexpr double IdleStartSeconds = OpeningStartSeconds + OpeningStabilizationSeconds;
	static constexpr double InteractionStartSeconds = IdleStartSeconds + IdleSeconds;
	static constexpr double FinalizingStartSeconds = InteractionStartSeconds + InteractionSeconds;
	static constexpr double CompleteStartSeconds = FinalizingStartSeconds + FinalizingSeconds;

	static EWacomBackpackPerformanceCapturePhase ResolvePhase(double ElapsedSeconds)
	{
		if (ElapsedSeconds < ClosedStartSeconds)
		{
			return EWacomBackpackPerformanceCapturePhase::Warmup;
		}
		if (ElapsedSeconds < OpeningStartSeconds)
		{
			return EWacomBackpackPerformanceCapturePhase::Closed;
		}
		if (ElapsedSeconds < IdleStartSeconds)
		{
			return EWacomBackpackPerformanceCapturePhase::Opening;
		}
		if (ElapsedSeconds < InteractionStartSeconds)
		{
			return EWacomBackpackPerformanceCapturePhase::Idle;
		}
		if (ElapsedSeconds < FinalizingStartSeconds)
		{
			return EWacomBackpackPerformanceCapturePhase::Interaction;
		}
		if (ElapsedSeconds < CompleteStartSeconds)
		{
			return EWacomBackpackPerformanceCapturePhase::Finalizing;
		}
		return EWacomBackpackPerformanceCapturePhase::Complete;
	}

	static double GetPhaseEndSeconds(EWacomBackpackPerformanceCapturePhase Phase)
	{
		switch (Phase)
		{
		case EWacomBackpackPerformanceCapturePhase::Warmup:
			return ClosedStartSeconds;
		case EWacomBackpackPerformanceCapturePhase::Closed:
			return OpeningStartSeconds;
		case EWacomBackpackPerformanceCapturePhase::Opening:
			return IdleStartSeconds;
		case EWacomBackpackPerformanceCapturePhase::Idle:
			return InteractionStartSeconds;
		case EWacomBackpackPerformanceCapturePhase::Interaction:
			return FinalizingStartSeconds;
		case EWacomBackpackPerformanceCapturePhase::Finalizing:
		case EWacomBackpackPerformanceCapturePhase::Complete:
		default:
			return CompleteStartSeconds;
		}
	}
};

#endif
