// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceMotionCoordinator.h"

#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Card/WacomCardMotionKernel.h"

namespace
{
	constexpr float LocalPosePositionTolerance = 0.05f;
	constexpr float LocalPoseAngleTolerance = 0.01f;

	FWacomFirstPersonCardDepthConfig BuildDepthConfig(
		const FWacomBackpackActiveCardDepthStyle& Style,
		bool bSimplifiedMotion)
	{
		FWacomFirstPersonCardDepthConfig Config;
		Config.bEnableFake3D = !bSimplifiedMotion;
		Config.HoverMaxTiltDegrees = Style.HoverMaximumTiltDegrees;
		Config.DragMaxTiltDegrees = Style.CarryMaximumTiltDegrees;
		Config.PerspectiveStrength = Style.PerspectiveStrength;
		Config.ResponseSpeed = Style.ResponseSpeed;
		Config.ReturnSpeed = Style.ReturnSpeed;
		Config.DragVelocityFilterSpeed = Style.VelocityFilterSpeed;
		Config.DragVelocityForMaxTiltPixelsPerSecond =
			Style.CarryVelocityForMaximumTiltPixelsPerSecond;
		Config.bEnableContactShadow = !bSimplifiedMotion;
		Config.ContactShadowTiltOffsetPixels = Style.ContactShadowTiltOffsetPixels;
		Config.ContactShadowOpacityMultiplier = Style.ContactShadowOpacityMultiplier;
		Config.bEnableSurfaceParallax = !bSimplifiedMotion;
		Config.AttachmentParallaxDepthPixels = Style.SurfaceParallaxDepthPixels;
		Config.bReduceSurfaceParallaxMotion = bSimplifiedMotion;
		return Config;
	}
}

FWacomBackpackWorkspaceCardVisualState
FWacomBackpackWorkspaceMotionCoordinator::BuildVisualState(
	const UWacomBackpackWorkspaceStyle& Style,
	bool bSelected,
	bool bCurrent,
	bool bReadOnly,
	bool bValidTarget,
	bool bRejectedTarget)
{
	FWacomBackpackWorkspaceCardVisualState State;
	State.FeedbackMaterial = Style.CardFeedbackMaterial;
	State.Opacity = bReadOnly ? 0.72f : 1.0f;
	if (bRejectedTarget)
	{
		State.Tint = Style.RejectedTargetColor;
		State.FeedbackOpacity = Style.CardStateOverlayOpacity;
	}
	else if (bValidTarget)
	{
		State.Tint = Style.ValidTargetColor;
		State.FeedbackOpacity = Style.CardStateOverlayOpacity;
	}
	else if (bSelected)
	{
		State.Tint = Style.SelectionColor;
		State.FeedbackOpacity = Style.CardStateOverlayOpacity;
	}
	return State;
}

bool FWacomBackpackWorkspaceMotionCoordinator::IsCardMoving(
	const UWacomDeckCardWidget& Card) const
{
	return LocalPoseMotions.Contains(
		TWeakObjectPtr<UWacomDeckCardWidget>(
			const_cast<UWacomDeckCardWidget*>(&Card)));
}

void FWacomBackpackWorkspaceMotionCoordinator::ReconcileActiveCard(
	UWacomDeckCardWidget* DesiredCard,
	bool bDesiredCarrying,
	const FGeometry& WorkspaceGeometry,
	FVector2D InPointerLocal,
	const UWacomBackpackWorkspaceStyle& Style,
	bool bSimplifiedMotion)
{
	if (ActiveCard.Get() != DesiredCard)
	{
		DisableActiveCard();
		ActiveCard = DesiredCard;
		ActiveDepthMotion.Reset();
	}
	bActiveCardCarrying = bDesiredCarrying;
	bSimplified = bSimplifiedMotion;
	UpdatePointer(WorkspaceGeometry, InPointerLocal, bDesiredCarrying);
	UpdateActiveDepth(0.0f, WorkspaceGeometry, Style);
}

void FWacomBackpackWorkspaceMotionCoordinator::UpdatePointer(
	const FGeometry& WorkspaceGeometry,
	FVector2D InPointerLocal,
	bool bCarrying)
{
	bDepthPointerChanged = !bHasPointer || !PointerLocal.Equals(InPointerLocal, 0.01f);
	PointerLocal = InPointerLocal;
	bHasPointer = true;
	bActiveCardCarrying = bCarrying;
}

void FWacomBackpackWorkspaceMotionCoordinator::SetLocalPoseTarget(
	UWacomDeckCardWidget& Card,
	FVector2D Translation,
	float AngleDegrees,
	float DurationSeconds,
	bool bSimplifiedMotion)
{
	if (bSimplifiedMotion || DurationSeconds <= 0.0f)
	{
		SnapLocalPose(Card, Translation, AngleDegrees);
		return;
	}
	if (const FLocalPoseMotion* Existing = LocalPoseMotions.Find(&Card))
	{
		if (Existing->TargetTranslation.Equals(Translation, LocalPosePositionTolerance)
			&& FMath::Abs(FMath::FindDeltaAngleDegrees(
				Existing->TargetAngleDegrees,
				AngleDegrees)) <= LocalPoseAngleTolerance
			&& !Existing->bSettlement)
		{
			return;
		}
	}
	if (Card.GetBackpackLocalMotionTranslation().Equals(Translation, LocalPosePositionTolerance)
		&& FMath::Abs(FMath::FindDeltaAngleDegrees(
			Card.GetBackpackLocalMotionAngle(), AngleDegrees)) <= LocalPoseAngleTolerance)
	{
		LocalPoseMotions.Remove(&Card);
		return;
	}

	FLocalPoseMotion& Motion = LocalPoseMotions.FindOrAdd(&Card);
	Motion.StartTranslation = Card.GetBackpackLocalMotionTranslation();
	Motion.TargetTranslation = Translation;
	Motion.StartAngleDegrees = Card.GetBackpackLocalMotionAngle();
	Motion.TargetAngleDegrees = AngleDegrees;
	Motion.ElapsedSeconds = 0.0f;
	Motion.DurationSeconds = DurationSeconds;
	Motion.bSettlement = false;
}

void FWacomBackpackWorkspaceMotionCoordinator::SnapLocalPose(
	UWacomDeckCardWidget& Card,
	FVector2D Translation,
	float AngleDegrees)
{
	LocalPoseMotions.Remove(&Card);
	Card.ApplyBackpackLocalMotionPose(Translation, AngleDegrees);
}

void FWacomBackpackWorkspaceMotionCoordinator::StopLocalPoseMotionPreservingCurrent(
	UWacomDeckCardWidget& Card)
{
	LocalPoseMotions.Remove(&Card);
	PickupCards.Remove(&Card);
	CompletedSettlements.Remove(&Card);
}

void FWacomBackpackWorkspaceMotionCoordinator::BeginCarryPickup(
	TConstArrayView<TWeakObjectPtr<UWacomDeckCardWidget>> Cards,
	float LiftPixels,
	float DurationSeconds,
	bool bSimplifiedMotion)
{
	PickupCards.Reset();
	PickupElapsedSeconds = 0.0f;
	PickupDurationSeconds = FMath::Max(0.0f, DurationSeconds);
	PickupLiftPixels = FMath::Max(0.0f, LiftPixels);
	LastPickupOffsetPixels = 0.0f;
	if (bSimplifiedMotion || PickupDurationSeconds <= 0.0f || PickupLiftPixels <= 0.0f)
	{
		return;
	}
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& Card : Cards)
	{
		if (Card.IsValid())
		{
			PickupCards.Add(Card);
		}
	}
}

void FWacomBackpackWorkspaceMotionCoordinator::BeginSettlement(
	UWacomDeckCardWidget& Card,
	FVector2D StartLocalTranslation,
	float StartLocalAngleDegrees,
	float DurationSeconds,
	bool bSimplifiedMotion)
{
	Card.ApplyBackpackLocalMotionPose(StartLocalTranslation, StartLocalAngleDegrees);
	if (bSimplifiedMotion || DurationSeconds <= 0.0f)
	{
		CompletedSettlements.Add(&Card);
		Card.ResetBackpackLocalMotionPose();
		return;
	}
	FLocalPoseMotion& Motion = LocalPoseMotions.FindOrAdd(&Card);
	Motion.StartTranslation = StartLocalTranslation;
	Motion.TargetTranslation = FVector2D::ZeroVector;
	Motion.StartAngleDegrees = StartLocalAngleDegrees;
	Motion.TargetAngleDegrees = 0.0f;
	Motion.ElapsedSeconds = 0.0f;
	Motion.DurationSeconds = DurationSeconds;
	Motion.bSettlement = true;
}

void FWacomBackpackWorkspaceMotionCoordinator::Tick(
	float DeltaTime,
	const FGeometry& WorkspaceGeometry,
	const UWacomBackpackWorkspaceStyle& Style,
	bool bSimplifiedMotion)
{
	bSimplified = bSimplifiedMotion;
	const float SafeDeltaTime = FMath::Clamp(DeltaTime, 0.0f, 0.1f);
	if (bSimplifiedMotion)
	{
		for (auto It = LocalPoseMotions.CreateIterator(); It; ++It)
		{
			if (UWacomDeckCardWidget* Card = It.Key().Get())
			{
				Card->ApplyBackpackLocalMotionPose(
					It.Value().TargetTranslation,
					It.Value().TargetAngleDegrees);
				if (It.Value().bSettlement)
				{
					CompletedSettlements.Add(Card);
				}
			}
			It.RemoveCurrent();
		}
		for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : PickupCards)
		{
			if (UWacomDeckCardWidget* Card = WeakCard.Get())
			{
				FVector2D Translation = Card->GetBackpackLocalMotionTranslation();
				Translation.Y += LastPickupOffsetPixels;
				Card->ApplyBackpackLocalMotionPose(
					Translation,
					Card->GetBackpackLocalMotionAngle());
			}
		}
		PickupCards.Reset();
		LastPickupOffsetPixels = 0.0f;
		UpdateActiveDepth(0.0f, WorkspaceGeometry, Style);
		return;
	}
	for (auto It = LocalPoseMotions.CreateIterator(); It; ++It)
	{
		UWacomDeckCardWidget* Card = It.Key().Get();
		if (!Card)
		{
			It.RemoveCurrent();
			continue;
		}
		FLocalPoseMotion& Motion = It.Value();
		Motion.ElapsedSeconds += SafeDeltaTime;
		const float LinearAlpha = Motion.DurationSeconds > 0.0f
			? FMath::Clamp(Motion.ElapsedSeconds / Motion.DurationSeconds, 0.0f, 1.0f)
			: 1.0f;
		const float Alpha = FWacomCardMotionKernel::ComputeEaseOutAlpha(LinearAlpha);
		Card->ApplyBackpackLocalMotionPose(
			FMath::Lerp(Motion.StartTranslation, Motion.TargetTranslation, Alpha),
			FWacomCardMotionKernel::LerpAngleShortest(
				Motion.StartAngleDegrees,
				Motion.TargetAngleDegrees,
				Alpha));
		if (LinearAlpha >= 1.0f)
		{
			if (Motion.bSettlement)
			{
				CompletedSettlements.Add(Card);
			}
			It.RemoveCurrent();
		}
	}

	if (!PickupCards.IsEmpty())
	{
		PickupElapsedSeconds += SafeDeltaTime;
		const float LinearAlpha = PickupDurationSeconds > 0.0f
			? FMath::Clamp(PickupElapsedSeconds / PickupDurationSeconds, 0.0f, 1.0f)
			: 1.0f;
		const float Offset = FMath::Sin(LinearAlpha * UE_PI) * PickupLiftPixels;
		for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : PickupCards)
		{
			if (UWacomDeckCardWidget* Card = WeakCard.Get())
			{
				FVector2D Translation = Card->GetBackpackLocalMotionTranslation();
				Translation.Y += LastPickupOffsetPixels - Offset;
				Card->ApplyBackpackLocalMotionPose(
					Translation,
					Card->GetBackpackLocalMotionAngle());
			}
		}
		LastPickupOffsetPixels = Offset;
		if (LinearAlpha >= 1.0f)
		{
			PickupCards.Reset();
			LastPickupOffsetPixels = 0.0f;
		}
	}

	UpdateActiveDepth(SafeDeltaTime, WorkspaceGeometry, Style);
}

bool FWacomBackpackWorkspaceMotionCoordinator::WantsTick() const
{
	return !LocalPoseMotions.IsEmpty()
		|| !PickupCards.IsEmpty()
		|| (ActiveCard.IsValid() && (bDepthPointerChanged || ActiveDepthMotion.IsInMotion()));
}

void FWacomBackpackWorkspaceMotionCoordinator::ConsumeCompletedSettlements(
	TArray<TWeakObjectPtr<UWacomDeckCardWidget>>& OutCards)
{
	OutCards.Append(CompletedSettlements);
	CompletedSettlements.Reset();
}

void FWacomBackpackWorkspaceMotionCoordinator::Reset()
{
	for (const TPair<TWeakObjectPtr<UWacomDeckCardWidget>, FLocalPoseMotion>& Pair : LocalPoseMotions)
	{
		if (UWacomDeckCardWidget* Card = Pair.Key.Get())
		{
			Card->ResetBackpackLocalMotionPose();
		}
	}
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : PickupCards)
	{
		if (UWacomDeckCardWidget* Card = WeakCard.Get())
		{
			Card->ResetBackpackLocalMotionPose();
		}
	}
	DisableActiveCard();
	LocalPoseMotions.Reset();
	PickupCards.Reset();
	CompletedSettlements.Reset();
	ActiveCard.Reset();
	ActiveDepthMotion.Reset();
	bHasPointer = false;
	bDepthPointerChanged = false;
	bActiveCardCarrying = false;
	LastPickupOffsetPixels = 0.0f;
}

void FWacomBackpackWorkspaceMotionCoordinator::DisableActiveCard()
{
	if (UWacomDeckCardWidget* Previous = ActiveCard.Get())
	{
		Previous->ApplyBackpackDepthPresentation(false, FWacomFirstPersonCardDepthView());
	}
}

void FWacomBackpackWorkspaceMotionCoordinator::UpdateActiveDepth(
	float DeltaTime,
	const FGeometry& WorkspaceGeometry,
	const UWacomBackpackWorkspaceStyle& Style)
{
	UWacomDeckCardWidget* Card = ActiveCard.Get();
	if (!Card)
	{
		return;
	}
	if (bSimplified)
	{
		Card->ApplyBackpackDepthPresentation(false, FWacomFirstPersonCardDepthView());
		ActiveDepthMotion.Reset();
		bDepthPointerChanged = false;
		return;
	}

	const FGeometry& CardGeometry = Card->GetCachedGeometry();
	FWacomFirstPersonCardDepthMotionInput Input;
	Input.bProjected = true;
	Input.bHovered = !bActiveCardCarrying;
	Input.bDragging = bActiveCardCarrying;
	Input.bHasPointerPosition = bHasPointer;
	Input.bPointerPositionChanged = bDepthPointerChanged;
	Input.PointerPosition = PointerLocal;
	Input.CardCenter = WorkspaceGeometry.AbsoluteToLocal(
		CardGeometry.LocalToAbsolute(CardGeometry.GetLocalSize() * 0.5f));
	const float CardBaseAngleDegrees = Card->GetRenderTransformAngle();
	const float CardBaseAngleRadians = FMath::DegreesToRadians(CardBaseAngleDegrees);
	const FVector2D LocalMotionTranslation = Card->GetBackpackLocalMotionTranslation();
	Input.CardCenter += FVector2D(
		LocalMotionTranslation.X * FMath::Cos(CardBaseAngleRadians)
			- LocalMotionTranslation.Y * FMath::Sin(CardBaseAngleRadians),
		LocalMotionTranslation.X * FMath::Sin(CardBaseAngleRadians)
			+ LocalMotionTranslation.Y * FMath::Cos(CardBaseAngleRadians));
	Input.CardBodySize = CardGeometry.GetLocalSize();
	Input.CardRenderScale = 1.0f;
	Input.CardRenderAngleDegrees = CardBaseAngleDegrees
		+ Card->GetBackpackLocalMotionAngle();
	const FWacomFirstPersonCardDepthConfig Config = BuildDepthConfig(
		Style.ActiveCardDepthMotion,
		false);
	const FWacomFirstPersonCardDepthView& Depth = ActiveDepthMotion.Update(Config, Input, DeltaTime);
	Card->ApplyBackpackDepthPresentation(true, Depth);
	LastDepthPointerLocal = PointerLocal;
	bDepthPointerChanged = false;
}
