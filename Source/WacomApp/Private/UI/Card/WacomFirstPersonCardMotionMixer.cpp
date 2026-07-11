// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardMotionMixer.h"

FWacomFirstPersonCardLayerSlotView FWacomFirstPersonCardMotionMixer::ComposePresentationSlotView(
	const FWacomFirstPersonCardLayerSlotView& BaseSlotView,
	const FWacomFirstPersonCardSlotVisualState& State,
	const FWacomFirstPersonCardSlotVisualConfig& VisualConfig)
{
	FWacomFirstPersonCardLayerSlotView Presentation = BaseSlotView;
	if (!BaseSlotView.bProjected)
	{
		return Presentation;
	}

	if (State.bPendingSource)
	{
		Presentation.ScreenPosition.Y -= VisualConfig.PendingTargetingLiftPixels;
		Presentation.WidgetPosition = Presentation.ScreenPosition;
		Presentation.SnappedWidgetPosition = Presentation.ScreenPosition;
		Presentation.RenderScale =
			FMath::Max(0.01f, Presentation.RenderScale * VisualConfig.PendingTargetingScale);
		if (VisualConfig.bPendingTargetingStraightenAngle)
		{
			Presentation.RenderAngleDegrees = FMath::Lerp(
				Presentation.RenderAngleDegrees,
				0.0f,
				VisualConfig.PendingTargetingAngleBlend);
		}
		Presentation.ZOrder += VisualConfig.PendingTargetingZOrderBoost;
	}
	else if (State.bTargetSelectDeemphasized)
	{
		Presentation.RenderOpacity = FMath::Clamp(
			Presentation.RenderOpacity * VisualConfig.TargetSelectNonPendingOpacityMultiplier,
			0.0f,
			1.0f);
	}

	if (State.bHovered)
	{
		Presentation.ScreenPosition.Y -= VisualConfig.HoverLiftPixels;
		Presentation.WidgetPosition = Presentation.ScreenPosition;
		Presentation.SnappedWidgetPosition = Presentation.ScreenPosition;
		Presentation.RenderScale = FMath::Max(0.01f, Presentation.RenderScale * VisualConfig.HoverScale);
		Presentation.ZOrder += VisualConfig.HoverZOrderBoost;
	}

	return Presentation;
}

EWacomFirstPersonCardMotionIntent FWacomFirstPersonCardMotionMixer::ResolveMotionIntentForPresentationChange(
	const FWacomFirstPersonCardSlotVisualState& PreviousState,
	const FWacomFirstPersonCardSlotVisualState& NewState,
	const FWacomFirstPersonCardLayerSlotView& PreviousPresentationSlotView,
	const FWacomFirstPersonCardLayerSlotView& NewPresentationSlotView,
	EWacomFirstPersonCardMotionIntent PreferredIntent)
{
	EWacomFirstPersonCardMotionIntent Intent = PreferredIntent;
	const auto ChooseHigherPriorityIntent = [&Intent](EWacomFirstPersonCardMotionIntent Candidate)
	{
		if (GetMotionIntentPriority(Candidate) > GetMotionIntentPriority(Intent))
		{
			Intent = Candidate;
		}
	};

	if (PreviousState.bPendingSource != NewState.bPendingSource
		|| PreviousState.bTargetSelectDeemphasized != NewState.bTargetSelectDeemphasized)
	{
		ChooseHigherPriorityIntent(EWacomFirstPersonCardMotionIntent::Pending);
	}
	if (PreviousState.bHovered != NewState.bHovered)
	{
		ChooseHigherPriorityIntent(EWacomFirstPersonCardMotionIntent::Hover);
	}

	const bool bPresentationChanged =
		FVector2D::Distance(PreviousPresentationSlotView.ScreenPosition, NewPresentationSlotView.ScreenPosition) > 0.1f
		|| FMath::Abs(PreviousPresentationSlotView.RenderAngleDegrees - NewPresentationSlotView.RenderAngleDegrees) > 0.05f
		|| FMath::Abs(PreviousPresentationSlotView.RenderScale - NewPresentationSlotView.RenderScale) > 0.001f
		|| FMath::Abs(PreviousPresentationSlotView.RenderOpacity - NewPresentationSlotView.RenderOpacity) > 0.01f
		|| PreviousPresentationSlotView.ZOrder != NewPresentationSlotView.ZOrder;
	return bPresentationChanged ? Intent : PreferredIntent;
}

const FWacomFirstPersonCardMotionProfile& FWacomFirstPersonCardMotionMixer::GetMotionProfileForIntent(
	const FWacomFirstPersonCardSlotMotionConfig& MotionConfig,
	EWacomFirstPersonCardMotionIntent Intent)
{
	switch (Intent)
	{
	case EWacomFirstPersonCardMotionIntent::Hover:
		return MotionConfig.HoverMotionProfile;
	case EWacomFirstPersonCardMotionIntent::Pending:
		return MotionConfig.PendingMotionProfile;
	case EWacomFirstPersonCardMotionIntent::Enter:
		return MotionConfig.EnterMotionProfile;
	case EWacomFirstPersonCardMotionIntent::Exit:
		return MotionConfig.ExitMotionProfile;
	case EWacomFirstPersonCardMotionIntent::Layout:
	default:
		return MotionConfig.LayoutMotionProfile;
	}
}

float FWacomFirstPersonCardMotionMixer::ComputeMotionAlpha(float Speed, float DeltaTime, float EasePower)
{
	const float LinearAlpha = Speed <= 0.0f ? 1.0f : FMath::Clamp(DeltaTime * Speed, 0.0f, 1.0f);
	return FMath::Pow(LinearAlpha, FMath::Max(0.1f, EasePower));
}

float FWacomFirstPersonCardMotionMixer::ComputeTransitionEaseAlpha(float LinearAlpha, float EasePower)
{
	return FMath::InterpEaseInOut(
		0.0f,
		1.0f,
		FMath::Clamp(LinearAlpha, 0.0f, 1.0f),
		FMath::Max(0.1f, EasePower));
}

FWacomFirstPersonCardLayerSlotView FWacomFirstPersonCardMotionMixer::LerpSlotView(
	const FWacomFirstPersonCardLayerSlotView& From,
	const FWacomFirstPersonCardLayerSlotView& To,
	float MotionAlpha,
	float OpacityAlpha)
{
	FWacomFirstPersonCardLayerSlotView Result = To;
	Result.ScreenPosition = FMath::Lerp(From.ScreenPosition, To.ScreenPosition, MotionAlpha);
	Result.WidgetPosition = Result.ScreenPosition;
	Result.SnappedWidgetPosition = Result.ScreenPosition;
	Result.RenderAngleDegrees = FMath::Lerp(From.RenderAngleDegrees, To.RenderAngleDegrees, MotionAlpha);
	Result.RenderScale = FMath::Lerp(From.RenderScale, To.RenderScale, MotionAlpha);
	Result.RenderOpacity = FMath::Lerp(From.RenderOpacity, To.RenderOpacity, OpacityAlpha);
	Result.bProjected = From.bProjected || To.bProjected;
	return Result;
}

bool FWacomFirstPersonCardMotionMixer::IsNearTarget(
	const FWacomFirstPersonCardLayerSlotView& Current,
	const FWacomFirstPersonCardLayerSlotView& Target)
{
	return FVector2D::Distance(Current.ScreenPosition, Target.ScreenPosition) <= 0.1f
		&& FMath::Abs(Current.RenderAngleDegrees - Target.RenderAngleDegrees) <= 0.05f
		&& FMath::Abs(Current.RenderScale - Target.RenderScale) <= 0.001f
		&& FMath::Abs(Current.RenderOpacity - Target.RenderOpacity) <= 0.01f;
}

FWacomFirstPersonCardLocalFeedbackMixResult FWacomFirstPersonCardMotionMixer::MixLocalFeedback(
	const FWacomFirstPersonCardLocalFeedbackMixInput& Input)
{
	FWacomFirstPersonCardLocalFeedbackMixResult Result;
	if (!Input.SlotView || !Input.FeedbackConfig)
	{
		return Result;
	}
	const FWacomFirstPersonCardLayerSlotView& SlotView = *Input.SlotView;
	const FWacomFirstPersonCardSlotFeedbackConfig& FeedbackConfig = *Input.FeedbackConfig;
	Result.ZOrder = SlotView.ZOrder;

	const bool bDenyActive = FeedbackConfig.bEnabled
		&& Input.DenyFeedbackElapsedSeconds < FeedbackConfig.DenyDuration;
	const float DenyShakeOffset = bDenyActive
		? ComputeDenyShakeOffset(
			Input.DenyFeedbackElapsedSeconds,
			FeedbackConfig.DenyDuration,
			FeedbackConfig.DenyShakePixels)
		: 0.0f;
	const float PressedScale = FeedbackConfig.bEnabled && Input.bPressed
		? FeedbackConfig.PressedScale
		: 1.0f;
	const float CommitScale =
		FeedbackConfig.bEnabled
		&& FeedbackConfig.bEnablePlayCommitFeedback
		&& Input.bCommitFeedbackActive
			? FeedbackConfig.PlayCommitScale
			: 1.0f;
	Result.RenderTransform.Translation = FVector2D(DenyShakeOffset, 0.0f);
	Result.RenderTransform.Scale = FVector2D(FMath::Max(
		0.01f,
		SlotView.RenderScale
			* PressedScale
			* CommitScale));
	Result.RenderTransform.Angle = SlotView.RenderAngleDegrees;
	return Result;
}

int32 FWacomFirstPersonCardMotionMixer::GetMotionIntentPriority(EWacomFirstPersonCardMotionIntent Intent)
{
	switch (Intent)
	{
	case EWacomFirstPersonCardMotionIntent::Exit: return 60;
	case EWacomFirstPersonCardMotionIntent::Enter: return 50;
	case EWacomFirstPersonCardMotionIntent::Pending: return 30;
	case EWacomFirstPersonCardMotionIntent::Hover: return 20;
	case EWacomFirstPersonCardMotionIntent::Layout:
	default: return 10;
	}
}

float FWacomFirstPersonCardMotionMixer::ComputeDenyShakeOffset(
	float ElapsedSeconds,
	float DurationSeconds,
	float ShakePixels)
{
	if (DurationSeconds <= 0.0f || ElapsedSeconds >= DurationSeconds || ShakePixels <= 0.0f)
	{
		return 0.0f;
	}
	const float Progress = FMath::Clamp(ElapsedSeconds / DurationSeconds, 0.0f, 1.0f);
	return FMath::Sin(Progress * PI * 6.0f) * ShakePixels * (1.0f - Progress);
}
