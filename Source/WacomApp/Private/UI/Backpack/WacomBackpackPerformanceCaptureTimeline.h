// Copyright Wacom. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "CoreTypes.h"

enum class EWacomBackpackPerformanceCapturePhase : uint8
{
	Warmup,
	Closed,
	Opening,
	Idle,
	Interaction,
	Finalizing,
	Complete,
};

struct FWacomBackpackPerformanceCaptureTimeline
{
	static constexpr double WarmupSeconds = 10.0;
	static constexpr double ClosedSeconds = 60.0;
	static constexpr double OpeningStabilizationSeconds = 3.0;
	static constexpr double IdleSeconds = 60.0;
	static constexpr double InteractionSeconds = 60.0;
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

	static EWacomBackpackPerformanceCapturePhase NextPhase(
		EWacomBackpackPerformanceCapturePhase Phase)
	{
		switch (Phase)
		{
		case EWacomBackpackPerformanceCapturePhase::Warmup:
			return EWacomBackpackPerformanceCapturePhase::Closed;
		case EWacomBackpackPerformanceCapturePhase::Closed:
			return EWacomBackpackPerformanceCapturePhase::Opening;
		case EWacomBackpackPerformanceCapturePhase::Opening:
			return EWacomBackpackPerformanceCapturePhase::Idle;
		case EWacomBackpackPerformanceCapturePhase::Idle:
			return EWacomBackpackPerformanceCapturePhase::Interaction;
		case EWacomBackpackPerformanceCapturePhase::Interaction:
			return EWacomBackpackPerformanceCapturePhase::Finalizing;
		case EWacomBackpackPerformanceCapturePhase::Finalizing:
			return EWacomBackpackPerformanceCapturePhase::Complete;
		case EWacomBackpackPerformanceCapturePhase::Complete:
		default:
			return EWacomBackpackPerformanceCapturePhase::Complete;
		}
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

	static bool IsMeasuredPhase(EWacomBackpackPerformanceCapturePhase Phase)
	{
		return Phase != EWacomBackpackPerformanceCapturePhase::Opening
			&& Phase != EWacomBackpackPerformanceCapturePhase::Finalizing
			&& Phase != EWacomBackpackPerformanceCapturePhase::Complete;
	}
};

#endif
