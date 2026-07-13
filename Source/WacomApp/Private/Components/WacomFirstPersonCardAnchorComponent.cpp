// Copyright Wacom. All Rights Reserved.

#include "Components/WacomFirstPersonCardAnchorComponent.h"

#include "Actors/WacomRunTunnelSegmentActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "Components/WacomFirstPersonViewStageBlendComponent.h"
#include "Components/WacomRunTunnelMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomFirstPersonCardAnchorRuntimeState.h"
#include "UI/Card/WacomFirstPersonCardLayerDelegateRouter.h"
#include "UI/Card/WacomFirstPersonCardLayerOwner.h"
#include "UI/Card/WacomFirstPersonCardHandTargetImpactStyle.h"
#include "UI/Card/WacomFirstPersonCardPlayedDissolveStyle.h"
#include "UI/Card/WacomFirstPersonCardPileTransferStyle.h"
#include "UI/Card/WacomFirstPersonCardUseEffectStyle.h"
#include "UI/Card/WacomFirstPersonCardSelectionStyle.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#if WITH_AUTOMATION_TESTS
#include "Cards/CardDefinition.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#endif
#include "UI/Card/WacomFirstPersonCardViewWidget.h"
#include "UI/Card/WacomFirstPersonCardSlotLayoutBuilder.h"

namespace
{
	const FName NoOwnerReason(TEXT("NoOwner"));
	const FName NoPlayerControllerReason(TEXT("NoPlayerController"));
	const FName NoCameraManagerReason(TEXT("NoCameraManager"));
	const FName CameraFallbackReason(TEXT("CameraFallback"));

	bool IsCameraStageAnchorMode(EWacomFirstPersonCardAnchorMode Mode)
	{
		return Mode == EWacomFirstPersonCardAnchorMode::RunTunnel
			|| Mode == EWacomFirstPersonCardAnchorMode::BattleCamera
			|| Mode == EWacomFirstPersonCardAnchorMode::ViewStageBlend;
	}

	bool UsesCameraStageFollowSpeed(
		EWacomFirstPersonCardAnchorMode PreviousMode,
		EWacomFirstPersonCardAnchorMode NextMode)
	{
		if (PreviousMode == EWacomFirstPersonCardAnchorMode::ViewStageBlend
			|| NextMode == EWacomFirstPersonCardAnchorMode::ViewStageBlend)
		{
			return true;
		}

		return PreviousMode != NextMode
			&& IsCameraStageAnchorMode(PreviousMode)
			&& IsCameraStageAnchorMode(NextMode);
	}

	void BuildProjectionConfigFromAnchor(
		const UWacomFirstPersonCardAnchorComponent& Anchor,
		FWacomFirstPersonCardResolvedLayoutConfig& Config)
	{
		Config.ProjectionMode = Anchor.ProjectionMode;
		Config.ViewportClampMode = Anchor.ViewportClampMode;
		Config.LookInfluenceYaw = Anchor.LookInfluenceYaw;
		Config.LookInfluencePitch = Anchor.LookInfluencePitch;
		Config.FanYawDegrees = Anchor.FanYawDegrees;
		Config.ProjectionPadding = Anchor.ProjectionPadding;
		Config.SoftClampOffscreenAllowancePixels = Anchor.SoftClampOffscreenAllowancePixels;
		Config.SoftClampBlendRangePixels = Anchor.SoftClampBlendRangePixels;
		Config.bEnableCardLayerPixelSnapping = Anchor.bEnableCardLayerPixelSnapping;
		Config.CardLayerPixelSnapGrid = Anchor.CardLayerPixelSnapGrid;
		Config.bClampCardLayerRenderAngle = Anchor.bClampCardLayerRenderAngle;
		Config.MaxCardLayerRenderAngleDegrees = Anchor.MaxCardLayerRenderAngleDegrees;
	}

	void BuildHandShapeConfigFromAnchor(
		const UWacomFirstPersonCardAnchorComponent& Anchor,
		FWacomFirstPersonCardResolvedLayoutConfig& Config)
	{
		Config.AuthoredCardSpacingPixels = Anchor.AuthoredCardSpacingPixels;
		Config.AuthoredMaxHandWidthPixels = Anchor.AuthoredMaxHandWidthPixels;
		Config.AuthoredHandScreenOffset = Anchor.AuthoredHandScreenOffset;
		Config.AuthoredCenterLiftPixels = Anchor.AuthoredCenterLiftPixels;
		Config.AuthoredDropCurveExponent = Anchor.AuthoredDropCurveExponent;
		Config.AuthoredFanCurveExponent = Anchor.AuthoredFanCurveExponent;
		Config.bAuthoredCenterCardsDrawOnTop = Anchor.bAuthoredCenterCardsDrawOnTop;
		Config.HandCardRenderScale = Anchor.HandCardRenderScale;
		Config.HandMaxEdgeDropPixels = Anchor.HandMaxEdgeDropPixels;
		Config.bScaleEdgeDropByHandCount = Anchor.bScaleEdgeDropByHandCount;
		Config.ShortHandEdgeDropPixels = Anchor.ShortHandEdgeDropPixels;
		Config.EdgeDropScaleMinCardCount = Anchor.EdgeDropScaleMinCardCount;
		Config.EdgeDropScaleMaxCardCount = Anchor.EdgeDropScaleMaxCardCount;
		Config.DisabledRenderOpacity = Anchor.DisabledRenderOpacity;
	}

	void BuildMotionConfigFromAnchor(
		const UWacomFirstPersonCardAnchorComponent& Anchor,
		FWacomFirstPersonCardResolvedLayoutConfig& Config)
	{
		Config.bEnableAnchorScreenSmoothing = Anchor.bEnableAnchorScreenSmoothing;
		Config.AnchorScreenSmoothingSpeed = Anchor.AnchorScreenSmoothingSpeed;
		Config.AnchorScreenSmoothingResetDistancePixels = Anchor.AnchorScreenSmoothingResetDistancePixels;
		Config.bEnableCardSlotMotion = Anchor.bEnableCardSlotMotion;
		Config.CardSlotMotionSpeed = Anchor.CardSlotMotionSpeed;
		Config.CardSlotOpacitySpeed = Anchor.CardSlotOpacitySpeed;
		Config.CardSlotMotionEasePower = Anchor.CardSlotMotionEasePower;
		Config.bOverrideHoverMotionProfile = Anchor.bOverrideHoverMotionProfile;
		Config.HoverMotionSpeed = Anchor.HoverMotionSpeed;
		Config.HoverOpacitySpeed = Anchor.HoverOpacitySpeed;
		Config.HoverMotionEasePower = Anchor.HoverMotionEasePower;
		Config.bOverrideDragTargetFocusMotionProfile = Anchor.bOverrideDragTargetFocusMotionProfile;
		Config.DragTargetFocusMotionSpeed = Anchor.DragTargetFocusMotionSpeed;
		Config.DragTargetFocusOpacitySpeed = Anchor.DragTargetFocusOpacitySpeed;
		Config.DragTargetFocusMotionEasePower = Anchor.DragTargetFocusMotionEasePower;
		Config.bOverrideEnterExitMotionProfile = Anchor.bOverrideEnterExitMotionProfile;
		Config.EnterMotionSpeed = Anchor.EnterMotionSpeed;
		Config.EnterOpacitySpeed = Anchor.EnterOpacitySpeed;
		Config.EnterMotionEasePower = Anchor.EnterMotionEasePower;
		Config.ExitMotionSpeed = Anchor.ExitMotionSpeed;
		Config.ExitOpacitySpeed = Anchor.ExitOpacitySpeed;
		Config.ExitMotionEasePower = Anchor.ExitMotionEasePower;
		Config.CardSlotEnterOffsetPixels = Anchor.CardSlotEnterOffsetPixels;
		Config.CardSlotEnterOpacity = Anchor.CardSlotEnterOpacity;
		Config.CardSlotExitOffsetPixels = Anchor.CardSlotExitOffsetPixels;
		Config.CardSlotExitDuration = Anchor.CardSlotExitDuration;
		Config.CardSlotMotionResetDistancePixels = Anchor.CardSlotMotionResetDistancePixels;
		Config.bEnableEventAwareCardTransitions = Anchor.bEnableEventAwareCardTransitions;
		Config.bEnableReadableTransitionOrigins = Anchor.bEnableReadableTransitionOrigins;
		Config.DrawnCardEnterOffsetPixels = Anchor.DrawnCardEnterOffsetPixels;
		Config.DrawnCardEnterOriginMode = Anchor.DrawnCardEnterOriginMode;
		Config.DrawnCardEnterViewportAnchor = Anchor.DrawnCardEnterViewportAnchor;
		Config.DrawnCardEnterScaleMultiplier = Anchor.DrawnCardEnterScaleMultiplier;
		Config.DrawnCardEnterAngleOffsetDegrees = Anchor.DrawnCardEnterAngleOffsetDegrees;
		Config.DrawnCardEnterDurationSeconds = Anchor.DrawnCardEnterDurationSeconds;
		Config.DrawnCardEnterStaggerSeconds = Anchor.DrawnCardEnterStaggerSeconds;
		Config.DrawnCardEnterArcLiftPixels = Anchor.DrawnCardEnterArcLiftPixels;
		Config.DrawnCardEnterEasePower = Anchor.DrawnCardEnterEasePower;
		Config.bBlockInteractionDuringDrawnCardEnter = Anchor.bBlockInteractionDuringDrawnCardEnter;
		Config.GainedCardEnterOffsetPixels = Anchor.GainedCardEnterOffsetPixels;
		Config.GainedCardEnterOriginMode = Anchor.GainedCardEnterOriginMode;
		Config.GainedCardEnterViewportAnchor = Anchor.GainedCardEnterViewportAnchor;
		Config.GainedCardEnterScaleMultiplier = Anchor.GainedCardEnterScaleMultiplier;
		Config.GainedCardEnterAngleOffsetDegrees = Anchor.GainedCardEnterAngleOffsetDegrees;
		Config.GainedCardEnterDurationSeconds = Anchor.GainedCardEnterDurationSeconds;
		Config.GainedCardEnterStaggerSeconds = Anchor.GainedCardEnterStaggerSeconds;
		Config.GainedCardEnterArcLiftPixels = Anchor.GainedCardEnterArcLiftPixels;
		Config.GainedCardEnterEasePower = Anchor.GainedCardEnterEasePower;
		Config.bBlockInteractionDuringGainedCardEnter = Anchor.bBlockInteractionDuringGainedCardEnter;
		Config.HandAnchorCardEnterOffsetPixels = Anchor.HandAnchorCardEnterOffsetPixels;
		Config.HandAnchorCardEnterOriginMode = Anchor.HandAnchorCardEnterOriginMode;
		Config.HandAnchorCardEnterViewportAnchor = Anchor.HandAnchorCardEnterViewportAnchor;
		Config.HandAnchorCardEnterScaleMultiplier = Anchor.HandAnchorCardEnterScaleMultiplier;
		Config.HandAnchorCardEnterAngleOffsetDegrees = Anchor.HandAnchorCardEnterAngleOffsetDegrees;
		Config.HandAnchorCardEnterDurationSeconds = Anchor.HandAnchorCardEnterDurationSeconds;
		Config.HandAnchorCardEnterStaggerSeconds = Anchor.HandAnchorCardEnterStaggerSeconds;
		Config.HandAnchorCardEnterArcLiftPixels = Anchor.HandAnchorCardEnterArcLiftPixels;
		Config.HandAnchorCardEnterEasePower = Anchor.HandAnchorCardEnterEasePower;
		Config.bBlockInteractionDuringHandAnchorCardEnter = Anchor.bBlockInteractionDuringHandAnchorCardEnter;
		Config.bEnableCardEnterSounds = Anchor.bEnableCardEnterSounds;
		Config.DrawnCardEnterSound = Anchor.DrawnCardEnterSound;
		Config.GainedCardEnterSound = Anchor.GainedCardEnterSound;
		Config.RunHandCardEnterSound = Anchor.RunHandCardEnterSound;
		Config.HandAnchorCardEnterSound = Anchor.HandAnchorCardEnterSound;
		Config.CardEnterSoundVolumeMultiplier = Anchor.CardEnterSoundVolumeMultiplier;
		Config.CardEnterSoundPitchMultiplier = Anchor.CardEnterSoundPitchMultiplier;
		Config.PlayedCardExitOffsetPixels = Anchor.PlayedCardExitOffsetPixels;
		Config.PlayedCardExitOriginMode = Anchor.PlayedCardExitOriginMode;
		Config.PlayedCardExitViewportAnchor = Anchor.PlayedCardExitViewportAnchor;
		Config.PlayedCardExitScaleMultiplier = Anchor.PlayedCardExitScaleMultiplier;
		Config.PlayedCardExitAngleOffsetDegrees = Anchor.PlayedCardExitAngleOffsetDegrees;
		Config.DiscardedCardExitOffsetPixels = Anchor.DiscardedCardExitOffsetPixels;
		Config.DiscardedCardExitOriginMode = Anchor.DiscardedCardExitOriginMode;
		Config.DiscardedCardExitViewportAnchor = Anchor.DiscardedCardExitViewportAnchor;
		Config.DiscardedCardExitScaleMultiplier = Anchor.DiscardedCardExitScaleMultiplier;
		Config.DiscardedCardExitAngleOffsetDegrees = Anchor.DiscardedCardExitAngleOffsetDegrees;
		Config.DiscardedCardExitStaggerSeconds = Anchor.DiscardedCardExitStaggerSeconds;
	}

	void BuildInteractionConfigFromAnchor(
		const UWacomFirstPersonCardAnchorComponent& Anchor,
		FWacomFirstPersonCardResolvedLayoutConfig& Config)
	{
		Config.PendingTargetingLiftPixels = Anchor.PendingTargetingLiftPixels;
		Config.PendingTargetingScale = Anchor.PendingTargetingScale;
		Config.PendingTargetingZOrderBoost = Anchor.PendingTargetingZOrderBoost;
		Config.bPendingTargetingStraightenAngle = Anchor.bPendingTargetingStraightenAngle;
		Config.PendingTargetingAngleBlend = Anchor.PendingTargetingAngleBlend;
		Config.bEnableTargetSelectHandDeemphasis = Anchor.bEnableTargetSelectHandDeemphasis;
		Config.TargetSelectNonPendingOpacityMultiplier = Anchor.TargetSelectNonPendingOpacityMultiplier;
		Config.HoverLiftPixels = Anchor.HoverLiftPixels;
		Config.HoverScale = Anchor.HoverScale;
		Config.HoverZOrderBoost = Anchor.HoverZOrderBoost;
		Config.HoverHitHysteresisPixels = Anchor.HoverHitHysteresisPixels;
		Config.CardDepth.bEnableFake3D = Anchor.bEnableCardFake3D;
		Config.CardDepth.HoverMaxTiltDegrees = Anchor.HoverCardFake3DMaxTiltDegrees;
		Config.CardDepth.DragMaxTiltDegrees = Anchor.DragCardFake3DMaxTiltDegrees;
		Config.CardDepth.PressedTiltMultiplier = Anchor.PressedCardFake3DTiltMultiplier;
		Config.CardDepth.PerspectiveStrength = Anchor.CardFake3DPerspectiveStrength;
		Config.CardDepth.ResponseSpeed = Anchor.CardFake3DResponseSpeed;
		Config.CardDepth.ReturnSpeed = Anchor.CardFake3DReturnSpeed;
		Config.CardDepth.DragVelocityFilterSpeed = Anchor.CardFake3DDragVelocityFilterSpeed;
		Config.CardDepth.DragVelocityForMaxTiltPixelsPerSecond =
			Anchor.CardFake3DVelocityForMaxTiltPixelsPerSecond;
		Config.CardDepth.bEnableContactShadow = Anchor.bEnableCardContactShadow;
		Config.CardDepth.HoverContactShadowLift = Anchor.CardHoverContactShadowLift;
		Config.CardDepth.DragContactShadowLift = Anchor.CardDragContactShadowLift;
		Config.CardUseEffect.bEnabled = Anchor.bEnableCardUseEffect;
		Config.CardUseEffect.bReducedMotion = Anchor.bReduceCardUseEffectMotion;
		Config.CardUseEffect.Style = Anchor.CardUseEffectStyle
			? Anchor.CardUseEffectStyle->Style
			: FWacomFirstPersonCardUseEffectStyleData();
		if (Anchor.CardUseEffectDurationOverrideSeconds >= 0.0f)
		{
			Config.CardUseEffect.Style.DurationSeconds =
				Anchor.CardUseEffectDurationOverrideSeconds;
		}
		Config.PlayedDissolve.bEnabled = Anchor.bEnableCardPlayedDissolve;
		Config.PlayedDissolve.bReducedMotion = Anchor.bReduceCardPlayedDissolveMotion;
		Config.PlayedDissolve.Style = Anchor.CardPlayedDissolveStyle
			? Anchor.CardPlayedDissolveStyle->Style
			: FWacomFirstPersonCardPlayedDissolveStyleData();
		if (Anchor.CardPlayedDissolveDurationOverrideSeconds >= 0.0f)
		{
			Config.PlayedDissolve.Style.DurationSeconds =
				Anchor.CardPlayedDissolveDurationOverrideSeconds;
		}
		Config.HandTargetImpact.bEnabled = Anchor.bEnableCardHandTargetImpact;
		Config.HandTargetImpact.bReducedMotion = Anchor.bReduceCardHandTargetImpactMotion;
		Config.HandTargetImpact.Style = Anchor.CardHandTargetImpactStyle
			? Anchor.CardHandTargetImpactStyle->Style
			: FWacomFirstPersonCardHandTargetImpactStyleData();
		if (Anchor.CardHandTargetImpactPreviewPeriodOverrideSeconds >= 0.0f)
		{
			Config.HandTargetImpact.Style.PreviewPeriodSeconds =
				Anchor.CardHandTargetImpactPreviewPeriodOverrideSeconds;
		}
		if (Anchor.CardHandTargetImpactCommitDurationOverrideSeconds >= 0.0f)
		{
			Config.HandTargetImpact.Style.CommitDurationSeconds =
				Anchor.CardHandTargetImpactCommitDurationOverrideSeconds;
		}
		Config.PileTransfer.bEnabled = Anchor.bEnableCardPileTransfer;
		Config.PileTransfer.bDiscardToPileEnabled = Anchor.bEnableCardDiscardGlyphTransfer;
		Config.PileTransfer.bReducedMotion = Anchor.bReduceCardPileTransferMotion;
		Config.PileTransfer.Style = Anchor.CardPileTransferStyle
			? Anchor.CardPileTransferStyle->Style
			: FWacomFirstPersonCardPileTransferStyleData();
		Config.Selection.bEnabled = Anchor.bEnableCardSelectionEffect;
		Config.Selection.bReducedMotion = Anchor.bReduceCardSelectionMotion;
		Config.Selection.Style = Anchor.CardSelectionStyle
			? Anchor.CardSelectionStyle->Style
			: FWacomFirstPersonCardSelectionStyleData();
		if (Anchor.CardSelectionEnterDurationOverrideSeconds >= 0.0f)
		{
			Config.Selection.Style.EnterDurationSeconds =
				Anchor.CardSelectionEnterDurationOverrideSeconds;
		}
		if (Anchor.CardSelectionExitDurationOverrideSeconds >= 0.0f)
		{
			Config.Selection.Style.ExitDurationSeconds =
				Anchor.CardSelectionExitDurationOverrideSeconds;
		}
		Config.bEnableCardInteractionFeedback = Anchor.bEnableCardInteractionFeedback;
		Config.PlayableHoverFeedbackColor = Anchor.PlayableHoverFeedbackColor;
		Config.PlayableHoverFeedbackOpacity = Anchor.PlayableHoverFeedbackOpacity;
		Config.PressedFeedbackScale = Anchor.PressedFeedbackScale;
		Config.PressedFeedbackColor = Anchor.PressedFeedbackColor;
		Config.PressedFeedbackOpacity = Anchor.PressedFeedbackOpacity;
		Config.bEnableDragPickupFeedback = Anchor.bEnableCardDragPickupFeedback;
		Config.DragPickupDurationSeconds = Anchor.CardDragPickupDurationSeconds;
		Config.DragPickupRiseSeconds = Anchor.CardDragPickupRiseSeconds;
		Config.DragPickupLiftPixels = Anchor.CardDragPickupLiftPixels;
		Config.DragPickupScaleMultiplier = Anchor.CardDragPickupScaleMultiplier;
		Config.bReduceDragPickupMotion = Anchor.bReduceCardDragPickupMotion;
		Config.DragPickupSound = Anchor.CardDragPickupSound;
		Config.DragPickupSoundVolumeMultiplier = Anchor.CardDragPickupSoundVolumeMultiplier;
		Config.DragPickupSoundPitchMultiplier = Anchor.CardDragPickupSoundPitchMultiplier;
		Config.DragPickupSoundPitchVariation = Anchor.CardDragPickupSoundPitchVariation;
		Config.ConfirmFeedbackDuration = Anchor.ConfirmFeedbackDuration;
		Config.ConfirmFeedbackOpacity = Anchor.ConfirmFeedbackOpacity;
		Config.DenyFeedbackDuration = Anchor.DenyFeedbackDuration;
		Config.DenyFeedbackShakePixels = Anchor.DenyFeedbackShakePixels;
		Config.DenyFeedbackColor = Anchor.DenyFeedbackColor;
		Config.DenyFeedbackOpacity = Anchor.DenyFeedbackOpacity;
		Config.InteractionFeedbackMaterial = Anchor.InteractionFeedbackMaterial;
		Config.InteractionFeedbackEdgeWidth = Anchor.InteractionFeedbackEdgeWidth;
		Config.InteractionFeedbackEdgeSoftness = Anchor.InteractionFeedbackEdgeSoftness;
		Config.InteractionFeedbackVignetteStrength = Anchor.InteractionFeedbackVignetteStrength;
		Config.InteractionFeedbackVignetteRadius = Anchor.InteractionFeedbackVignetteRadius;
		Config.InteractionFeedbackVignetteSoftness = Anchor.InteractionFeedbackVignetteSoftness;
		Config.bEnablePlayCommitFeedback = Anchor.bEnablePlayCommitFeedback;
		Config.PlayCommitFeedbackDuration = Anchor.PlayCommitFeedbackDuration;
		Config.PlayCommitFeedbackOpacity = Anchor.PlayCommitFeedbackOpacity;
		Config.PlayCommitFeedbackColor = Anchor.PlayCommitFeedbackColor;
		Config.PlayCommitFeedbackScale = Anchor.PlayCommitFeedbackScale;
		Config.bEnableRetainedFeedback = Anchor.bEnableRetainedFeedback;
		Config.RetainedFeedbackDuration = Anchor.RetainedFeedbackDuration;
		Config.RetainedFeedbackStaggerSeconds = Anchor.RetainedFeedbackStaggerSeconds;
		Config.RetainedFeedbackLiftPixels = Anchor.RetainedFeedbackLiftPixels;
		Config.RetainedFeedbackScale = Anchor.RetainedFeedbackScale;
		Config.RetainedFeedbackZOrderBoost = Anchor.RetainedFeedbackZOrderBoost;
		Config.DragCardTargetFocusLiftPixels = Anchor.DragCardTargetFocusLiftPixels;
		Config.DragCardTargetFocusScale = Anchor.DragCardTargetFocusScale;
		Config.DragCardTargetFocusZOrderBoost = Anchor.DragCardTargetFocusZOrderBoost;
	}

	FWacomFirstPersonCardResolvedLayoutConfig BuildResolvedLayoutConfigFromComponent(
		const UWacomFirstPersonCardAnchorComponent& Anchor)
	{
		FWacomFirstPersonCardResolvedLayoutConfig Config;
		BuildProjectionConfigFromAnchor(Anchor, Config);
		BuildHandShapeConfigFromAnchor(Anchor, Config);
		BuildMotionConfigFromAnchor(Anchor, Config);
		BuildInteractionConfigFromAnchor(Anchor, Config);
		return Config;
	}

	FWacomFirstPersonCardResolvedLayoutConfig ResolveLayoutConfig(
		const UWacomFirstPersonCardAnchorComponent& Anchor)
	{
		return BuildResolvedLayoutConfigFromComponent(Anchor);
	}

	FWacomFirstPersonCardSlotFeedbackConfig BuildSlotFeedbackConfig(
		const FWacomFirstPersonCardResolvedLayoutConfig& Config)
	{
		FWacomFirstPersonCardSlotFeedbackConfig FeedbackConfig;
		FeedbackConfig.bEnabled = Config.bEnableCardInteractionFeedback;
		FeedbackConfig.PlayableHoverColor = Config.PlayableHoverFeedbackColor;
		FeedbackConfig.PlayableHoverOpacity = Config.PlayableHoverFeedbackOpacity;
		FeedbackConfig.PressedScale = Config.PressedFeedbackScale;
		FeedbackConfig.PressedColor = Config.PressedFeedbackColor;
		FeedbackConfig.PressedOpacity = Config.PressedFeedbackOpacity;
		FeedbackConfig.bEnableDragPickupFeedback = Config.bEnableDragPickupFeedback;
		FeedbackConfig.DragPickupDurationSeconds = Config.DragPickupDurationSeconds;
		FeedbackConfig.DragPickupRiseSeconds = Config.DragPickupRiseSeconds;
		FeedbackConfig.DragPickupLiftPixels = Config.DragPickupLiftPixels;
		FeedbackConfig.DragPickupScaleMultiplier = Config.DragPickupScaleMultiplier;
		FeedbackConfig.bReduceDragPickupMotion = Config.bReduceDragPickupMotion;
		FeedbackConfig.DragPickupSound = Config.DragPickupSound;
		FeedbackConfig.DragPickupSoundVolumeMultiplier = Config.DragPickupSoundVolumeMultiplier;
		FeedbackConfig.DragPickupSoundPitchMultiplier = Config.DragPickupSoundPitchMultiplier;
		FeedbackConfig.DragPickupSoundPitchVariation = Config.DragPickupSoundPitchVariation;
		FeedbackConfig.ConfirmDuration = Config.ConfirmFeedbackDuration;
		FeedbackConfig.ConfirmOpacity = Config.ConfirmFeedbackOpacity;
		FeedbackConfig.DenyDuration = Config.DenyFeedbackDuration;
		FeedbackConfig.DenyShakePixels = Config.DenyFeedbackShakePixels;
		FeedbackConfig.DenyColor = Config.DenyFeedbackColor;
		FeedbackConfig.DenyOpacity = Config.DenyFeedbackOpacity;
		FeedbackConfig.InteractionFeedbackMaterial = Config.InteractionFeedbackMaterial;
		FeedbackConfig.InteractionFeedbackEdgeWidth = Config.InteractionFeedbackEdgeWidth;
		FeedbackConfig.InteractionFeedbackEdgeSoftness = Config.InteractionFeedbackEdgeSoftness;
		FeedbackConfig.InteractionFeedbackVignetteStrength = Config.InteractionFeedbackVignetteStrength;
		FeedbackConfig.InteractionFeedbackVignetteRadius = Config.InteractionFeedbackVignetteRadius;
		FeedbackConfig.InteractionFeedbackVignetteSoftness = Config.InteractionFeedbackVignetteSoftness;
		FeedbackConfig.bEnablePlayCommitFeedback = Config.bEnablePlayCommitFeedback;
		FeedbackConfig.PlayCommitDuration = Config.PlayCommitFeedbackDuration;
		FeedbackConfig.PlayCommitOpacity = Config.PlayCommitFeedbackOpacity;
		FeedbackConfig.PlayCommitColor = Config.PlayCommitFeedbackColor;
		FeedbackConfig.PlayCommitScale = Config.PlayCommitFeedbackScale;
		FeedbackConfig.bEnableRetainedFeedback = Config.bEnableRetainedFeedback;
		FeedbackConfig.RetainedFeedbackDuration = Config.RetainedFeedbackDuration;
		FeedbackConfig.RetainedFeedbackStaggerSeconds = Config.RetainedFeedbackStaggerSeconds;
		FeedbackConfig.RetainedFeedbackLiftPixels = Config.RetainedFeedbackLiftPixels;
		FeedbackConfig.RetainedFeedbackScale = Config.RetainedFeedbackScale;
		FeedbackConfig.RetainedFeedbackZOrderBoost = Config.RetainedFeedbackZOrderBoost;
		return FeedbackConfig;
	}

	FWacomFirstPersonCardSlotVisualConfig BuildSlotVisualConfig(
		const FWacomFirstPersonCardResolvedLayoutConfig& Config)
	{
		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.HoverLiftPixels = Config.HoverLiftPixels;
		VisualConfig.HoverScale = Config.HoverScale;
		VisualConfig.HoverZOrderBoost = Config.HoverZOrderBoost;
		VisualConfig.PendingTargetingLiftPixels = Config.PendingTargetingLiftPixels;
		VisualConfig.PendingTargetingScale = Config.PendingTargetingScale;
		VisualConfig.PendingTargetingZOrderBoost = Config.PendingTargetingZOrderBoost;
		VisualConfig.bPendingTargetingStraightenAngle = Config.bPendingTargetingStraightenAngle;
		VisualConfig.PendingTargetingAngleBlend = Config.PendingTargetingAngleBlend;
		VisualConfig.bEnableTargetSelectHandDeemphasis = Config.bEnableTargetSelectHandDeemphasis;
		VisualConfig.TargetSelectNonPendingOpacityMultiplier = Config.TargetSelectNonPendingOpacityMultiplier;
		VisualConfig.DragCardTargetFocusLiftPixels = Config.DragCardTargetFocusLiftPixels;
		VisualConfig.DragCardTargetFocusScale = Config.DragCardTargetFocusScale;
		VisualConfig.DragCardTargetFocusZOrderBoost = Config.DragCardTargetFocusZOrderBoost;
		VisualConfig.CardDepth = Config.CardDepth;
		VisualConfig.Selection = Config.Selection;
		VisualConfig.CardUseEffect = Config.CardUseEffect;
		VisualConfig.PlayedDissolve = Config.PlayedDissolve;
		VisualConfig.HandTargetImpact = Config.HandTargetImpact;
		return VisualConfig;
	}

	FWacomFirstPersonCardSlotMotionConfig BuildSlotMotionConfig(
		const FWacomFirstPersonCardResolvedLayoutConfig& Config)
	{
		const auto MakeProfile = [](
			float MotionSpeed,
			float OpacitySpeed,
			float EasePower)
		{
			FWacomFirstPersonCardMotionProfile Profile;
			Profile.MotionSpeed = MotionSpeed;
			Profile.OpacitySpeed = OpacitySpeed;
			Profile.EasePower = EasePower;
			return Profile;
		};

		FWacomFirstPersonCardSlotMotionConfig MotionConfig;
		MotionConfig.bEnabled = Config.bEnableCardSlotMotion;
		MotionConfig.MotionSpeed = Config.CardSlotMotionSpeed;
		MotionConfig.OpacitySpeed = Config.CardSlotOpacitySpeed;
		MotionConfig.EasePower = Config.CardSlotMotionEasePower;
		const FWacomFirstPersonCardMotionProfile DefaultProfile = MakeProfile(
			Config.CardSlotMotionSpeed,
			Config.CardSlotOpacitySpeed,
			Config.CardSlotMotionEasePower);
		MotionConfig.LayoutMotionProfile = DefaultProfile;
		MotionConfig.HoverMotionProfile = Config.bOverrideHoverMotionProfile
			? MakeProfile(
				Config.HoverMotionSpeed,
				Config.HoverOpacitySpeed,
				Config.HoverMotionEasePower)
			: DefaultProfile;
		MotionConfig.PendingMotionProfile = DefaultProfile;
		MotionConfig.DragTargetFocusMotionProfile = Config.bOverrideDragTargetFocusMotionProfile
			? MakeProfile(Config.DragTargetFocusMotionSpeed, Config.DragTargetFocusOpacitySpeed, Config.DragTargetFocusMotionEasePower)
			: DefaultProfile;
		MotionConfig.EnterMotionProfile = Config.bOverrideEnterExitMotionProfile
			? MakeProfile(
				Config.EnterMotionSpeed,
				Config.EnterOpacitySpeed,
				Config.EnterMotionEasePower)
			: DefaultProfile;
		MotionConfig.ExitMotionProfile = Config.bOverrideEnterExitMotionProfile
			? MakeProfile(
				Config.ExitMotionSpeed,
				Config.ExitOpacitySpeed,
				Config.ExitMotionEasePower)
			: DefaultProfile;
		MotionConfig.EnterOffsetPixels = Config.CardSlotEnterOffsetPixels;
		MotionConfig.EnterOpacity = Config.CardSlotEnterOpacity;
		MotionConfig.ExitOffsetPixels = Config.CardSlotExitOffsetPixels;
		MotionConfig.ExitDuration = Config.CardSlotExitDuration;
		MotionConfig.ResetDistancePixels = Config.CardSlotMotionResetDistancePixels;
		MotionConfig.bEnableEventAwareTransitions = Config.bEnableEventAwareCardTransitions;
		MotionConfig.bEnableReadableTransitionOrigins = Config.bEnableReadableTransitionOrigins;
		MotionConfig.DrawnEnterOffsetPixels = Config.DrawnCardEnterOffsetPixels;
		MotionConfig.DrawnEnterOriginMode = Config.DrawnCardEnterOriginMode;
		MotionConfig.DrawnEnterViewportAnchor = Config.DrawnCardEnterViewportAnchor;
		MotionConfig.DrawnEnterScaleMultiplier = Config.DrawnCardEnterScaleMultiplier;
		MotionConfig.DrawnEnterAngleOffsetDegrees = Config.DrawnCardEnterAngleOffsetDegrees;
		MotionConfig.DrawnEnterDurationSeconds = Config.DrawnCardEnterDurationSeconds;
		MotionConfig.DrawnEnterStaggerSeconds = Config.DrawnCardEnterStaggerSeconds;
		MotionConfig.DrawnEnterArcLiftPixels = Config.DrawnCardEnterArcLiftPixels;
		MotionConfig.DrawnEnterEasePower = Config.DrawnCardEnterEasePower;
		MotionConfig.bBlockInteractionDuringDrawnEnter = Config.bBlockInteractionDuringDrawnCardEnter;
		MotionConfig.GainedEnterOffsetPixels = Config.GainedCardEnterOffsetPixels;
		MotionConfig.GainedEnterOriginMode = Config.GainedCardEnterOriginMode;
		MotionConfig.GainedEnterViewportAnchor = Config.GainedCardEnterViewportAnchor;
		MotionConfig.GainedEnterScaleMultiplier = Config.GainedCardEnterScaleMultiplier;
		MotionConfig.GainedEnterAngleOffsetDegrees = Config.GainedCardEnterAngleOffsetDegrees;
		MotionConfig.GainedEnterDurationSeconds = Config.GainedCardEnterDurationSeconds;
		MotionConfig.GainedEnterStaggerSeconds = Config.GainedCardEnterStaggerSeconds;
		MotionConfig.GainedEnterArcLiftPixels = Config.GainedCardEnterArcLiftPixels;
		MotionConfig.GainedEnterEasePower = Config.GainedCardEnterEasePower;
		MotionConfig.bBlockInteractionDuringGainedEnter = Config.bBlockInteractionDuringGainedCardEnter;
		MotionConfig.HandAnchorEnterOffsetPixels = Config.HandAnchorCardEnterOffsetPixels;
		MotionConfig.HandAnchorEnterOriginMode = Config.HandAnchorCardEnterOriginMode;
		MotionConfig.HandAnchorEnterViewportAnchor = Config.HandAnchorCardEnterViewportAnchor;
		MotionConfig.HandAnchorEnterScaleMultiplier = Config.HandAnchorCardEnterScaleMultiplier;
		MotionConfig.HandAnchorEnterAngleOffsetDegrees = Config.HandAnchorCardEnterAngleOffsetDegrees;
		MotionConfig.HandAnchorEnterDurationSeconds = Config.HandAnchorCardEnterDurationSeconds;
		MotionConfig.HandAnchorEnterStaggerSeconds = Config.HandAnchorCardEnterStaggerSeconds;
		MotionConfig.HandAnchorEnterArcLiftPixels = Config.HandAnchorCardEnterArcLiftPixels;
		MotionConfig.HandAnchorEnterEasePower = Config.HandAnchorCardEnterEasePower;
		MotionConfig.bBlockInteractionDuringHandAnchorEnter = Config.bBlockInteractionDuringHandAnchorCardEnter;
		MotionConfig.bEnableEnterSounds = Config.bEnableCardEnterSounds;
		MotionConfig.DrawnEnterSound = Config.DrawnCardEnterSound;
		MotionConfig.GainedEnterSound = Config.GainedCardEnterSound;
		MotionConfig.RunHandEnterSound = Config.RunHandCardEnterSound;
		MotionConfig.HandAnchorEnterSound = Config.HandAnchorCardEnterSound;
		MotionConfig.EnterSoundVolumeMultiplier = Config.CardEnterSoundVolumeMultiplier;
		MotionConfig.EnterSoundPitchMultiplier = Config.CardEnterSoundPitchMultiplier;
		MotionConfig.PlayedExitOffsetPixels = Config.PlayedCardExitOffsetPixels;
		MotionConfig.PlayedExitOriginMode = Config.PlayedCardExitOriginMode;
		MotionConfig.PlayedExitViewportAnchor = Config.PlayedCardExitViewportAnchor;
		MotionConfig.PlayedExitScaleMultiplier = Config.PlayedCardExitScaleMultiplier;
		MotionConfig.PlayedExitAngleOffsetDegrees = Config.PlayedCardExitAngleOffsetDegrees;
		MotionConfig.DiscardedExitOffsetPixels = Config.DiscardedCardExitOffsetPixels;
		MotionConfig.DiscardedExitOriginMode = Config.DiscardedCardExitOriginMode;
		MotionConfig.DiscardedExitViewportAnchor = Config.DiscardedCardExitViewportAnchor;
		MotionConfig.DiscardedExitScaleMultiplier = Config.DiscardedCardExitScaleMultiplier;
		MotionConfig.DiscardedExitAngleOffsetDegrees = Config.DiscardedCardExitAngleOffsetDegrees;
		MotionConfig.DiscardedExitStaggerSeconds = Config.DiscardedCardExitStaggerSeconds;
		return MotionConfig;
	}

	FWacomFirstPersonCardDragConfig BuildCardDragConfig(
		const UWacomFirstPersonCardAnchorComponent& Anchor,
		const FWacomFirstPersonCardResolvedLayoutConfig& Config)
	{
		FWacomFirstPersonCardDragConfig DragConfig;
		DragConfig.bEnableFirstPersonCardDragCommit = Anchor.bEnableFirstPersonCardDragCommit;
		DragConfig.CardInspectHoldDelaySeconds = Anchor.CardInspectHoldDelaySeconds;
		DragConfig.CardDragStartThresholdPixels = Anchor.CardDragStartThresholdPixels;
		DragConfig.CardInspectScrubHandPaddingPixels = Anchor.CardInspectScrubHandPaddingPixels;
		DragConfig.HoverHitHysteresisPixels = Config.HoverHitHysteresisPixels;
		DragConfig.NoTargetCardDragOutCommitDistancePixels =
			Anchor.NoTargetCardDragOutCommitDistancePixels;
		DragConfig.NoTargetCardDragOutDirection = Anchor.NoTargetCardDragOutDirection;
		DragConfig.CardInspectScreenPosition = Anchor.CardInspectScreenPosition;
		DragConfig.CardInspectScale = Anchor.CardInspectScale;
		DragConfig.bShowDetailDuringCardInspect = Anchor.bShowDetailDuringCardInspect;
		DragConfig.bEnableAimArrow = Anchor.bEnableAimArrow;
		DragConfig.bLogCardDragDiagnostics = Anchor.bLogCardDragDiagnostics;
		DragConfig.SelectedSourceLiftPixels = Config.PendingTargetingLiftPixels;
		DragConfig.SelectedSourceScale = Config.PendingTargetingScale;
		DragConfig.SelectedSourceZOrderBoost = Config.PendingTargetingZOrderBoost;
		DragConfig.bSelectedSourceStraightenAngle = Config.bPendingTargetingStraightenAngle;
		DragConfig.SelectedSourceAngleBlend = Config.PendingTargetingAngleBlend;
		return DragConfig;
	}

	uint32 BuildResolvedLayoutConfigHash(
		const FWacomFirstPersonCardResolvedLayoutConfig& Config)
	{
		uint32 Hash = 0;
		const auto Combine = [&Hash](uint32 Value)
		{
			Hash = HashCombineFast(Hash, Value);
		};
		const auto AddBool = [&Combine](bool bValue)
		{
			Combine(bValue ? 1u : 0u);
		};
		const auto AddInt = [&Combine](int32 Value)
		{
			Combine(GetTypeHash(Value));
		};
		const auto AddFloat = [&Combine](float Value)
		{
			Combine(GetTypeHash(FMath::RoundToInt(Value * 1000.0f)));
		};
		const auto AddVector = [&AddFloat](const FVector2D& Value)
		{
			AddFloat(Value.X);
			AddFloat(Value.Y);
		};
		const auto AddColor = [&AddFloat](const FLinearColor& Value)
		{
			AddFloat(Value.R);
			AddFloat(Value.G);
			AddFloat(Value.B);
			AddFloat(Value.A);
		};
		const auto AddSoftObjectPath = [&Combine](const FSoftObjectPath& Value)
		{
			Combine(GetTypeHash(Value.ToString()));
		};

		AddInt(static_cast<int32>(Config.ProjectionMode));
		AddInt(static_cast<int32>(Config.ViewportClampMode));
		AddFloat(Config.LookInfluenceYaw);
		AddFloat(Config.LookInfluencePitch);
		AddFloat(Config.FanYawDegrees);
		AddFloat(Config.AuthoredCardSpacingPixels);
		AddFloat(Config.AuthoredMaxHandWidthPixels);
		AddVector(Config.AuthoredHandScreenOffset);
		AddFloat(Config.AuthoredCenterLiftPixels);
		AddFloat(Config.AuthoredDropCurveExponent);
		AddFloat(Config.AuthoredFanCurveExponent);
		AddBool(Config.bAuthoredCenterCardsDrawOnTop);
		AddFloat(Config.ProjectionPadding);
		AddFloat(Config.SoftClampOffscreenAllowancePixels);
		AddFloat(Config.SoftClampBlendRangePixels);
		AddBool(Config.bEnableCardLayerPixelSnapping);
		AddFloat(Config.CardLayerPixelSnapGrid);
		AddBool(Config.bClampCardLayerRenderAngle);
		AddFloat(Config.MaxCardLayerRenderAngleDegrees);
		AddFloat(Config.HandCardRenderScale);
		AddFloat(Config.HandMaxEdgeDropPixels);
		AddBool(Config.bScaleEdgeDropByHandCount);
		AddFloat(Config.ShortHandEdgeDropPixels);
		AddInt(Config.EdgeDropScaleMinCardCount);
		AddInt(Config.EdgeDropScaleMaxCardCount);
		AddBool(Config.bEnableAnchorScreenSmoothing);
		AddFloat(Config.AnchorScreenSmoothingSpeed);
		AddFloat(Config.AnchorScreenSmoothingResetDistancePixels);
		AddBool(Config.bEnableCardSlotMotion);
		AddFloat(Config.CardSlotMotionSpeed);
		AddFloat(Config.CardSlotOpacitySpeed);
		AddFloat(Config.CardSlotMotionEasePower);
		AddBool(Config.bOverrideHoverMotionProfile);
		AddFloat(Config.HoverMotionSpeed);
		AddFloat(Config.HoverOpacitySpeed);
		AddFloat(Config.HoverMotionEasePower);
		AddBool(Config.bOverrideDragTargetFocusMotionProfile);
		AddFloat(Config.DragTargetFocusMotionSpeed);
		AddFloat(Config.DragTargetFocusOpacitySpeed);
		AddFloat(Config.DragTargetFocusMotionEasePower);
		AddBool(Config.bOverrideEnterExitMotionProfile);
		AddFloat(Config.EnterMotionSpeed);
		AddFloat(Config.EnterOpacitySpeed);
		AddFloat(Config.EnterMotionEasePower);
		AddFloat(Config.ExitMotionSpeed);
		AddFloat(Config.ExitOpacitySpeed);
		AddFloat(Config.ExitMotionEasePower);
		AddVector(Config.CardSlotEnterOffsetPixels);
		AddFloat(Config.CardSlotEnterOpacity);
		AddVector(Config.CardSlotExitOffsetPixels);
		AddFloat(Config.CardSlotExitDuration);
		AddFloat(Config.CardSlotMotionResetDistancePixels);
		AddBool(Config.bEnableEventAwareCardTransitions);
		AddBool(Config.bEnableReadableTransitionOrigins);
		AddVector(Config.DrawnCardEnterOffsetPixels);
		AddInt(static_cast<int32>(Config.DrawnCardEnterOriginMode));
		AddVector(Config.DrawnCardEnterViewportAnchor);
		AddFloat(Config.DrawnCardEnterScaleMultiplier);
		AddFloat(Config.DrawnCardEnterAngleOffsetDegrees);
		AddFloat(Config.DrawnCardEnterDurationSeconds);
		AddFloat(Config.DrawnCardEnterStaggerSeconds);
		AddFloat(Config.DrawnCardEnterArcLiftPixels);
		AddFloat(Config.DrawnCardEnterEasePower);
		AddBool(Config.bBlockInteractionDuringDrawnCardEnter);
		AddVector(Config.GainedCardEnterOffsetPixels);
		AddInt(static_cast<int32>(Config.GainedCardEnterOriginMode));
		AddVector(Config.GainedCardEnterViewportAnchor);
		AddFloat(Config.GainedCardEnterScaleMultiplier);
		AddFloat(Config.GainedCardEnterAngleOffsetDegrees);
		AddFloat(Config.GainedCardEnterDurationSeconds);
		AddFloat(Config.GainedCardEnterStaggerSeconds);
		AddFloat(Config.GainedCardEnterArcLiftPixels);
		AddFloat(Config.GainedCardEnterEasePower);
		AddBool(Config.bBlockInteractionDuringGainedCardEnter);
		AddVector(Config.HandAnchorCardEnterOffsetPixels);
		AddInt(static_cast<int32>(Config.HandAnchorCardEnterOriginMode));
		AddVector(Config.HandAnchorCardEnterViewportAnchor);
		AddFloat(Config.HandAnchorCardEnterScaleMultiplier);
		AddFloat(Config.HandAnchorCardEnterAngleOffsetDegrees);
		AddFloat(Config.HandAnchorCardEnterDurationSeconds);
		AddFloat(Config.HandAnchorCardEnterStaggerSeconds);
		AddFloat(Config.HandAnchorCardEnterArcLiftPixels);
		AddFloat(Config.HandAnchorCardEnterEasePower);
		AddBool(Config.bBlockInteractionDuringHandAnchorCardEnter);
		AddBool(Config.bEnableCardEnterSounds);
		AddSoftObjectPath(Config.DrawnCardEnterSound.ToSoftObjectPath());
		AddSoftObjectPath(Config.GainedCardEnterSound.ToSoftObjectPath());
		AddSoftObjectPath(Config.RunHandCardEnterSound.ToSoftObjectPath());
		AddSoftObjectPath(Config.HandAnchorCardEnterSound.ToSoftObjectPath());
		AddFloat(Config.CardEnterSoundVolumeMultiplier);
		AddFloat(Config.CardEnterSoundPitchMultiplier);
		AddVector(Config.PlayedCardExitOffsetPixels);
		AddInt(static_cast<int32>(Config.PlayedCardExitOriginMode));
		AddVector(Config.PlayedCardExitViewportAnchor);
		AddFloat(Config.PlayedCardExitScaleMultiplier);
		AddFloat(Config.PlayedCardExitAngleOffsetDegrees);
		AddVector(Config.DiscardedCardExitOffsetPixels);
		AddInt(static_cast<int32>(Config.DiscardedCardExitOriginMode));
		AddVector(Config.DiscardedCardExitViewportAnchor);
		AddFloat(Config.DiscardedCardExitScaleMultiplier);
		AddFloat(Config.DiscardedCardExitAngleOffsetDegrees);
		AddFloat(Config.DiscardedCardExitStaggerSeconds);
		AddFloat(Config.PendingTargetingLiftPixels);
		AddFloat(Config.PendingTargetingScale);
		AddInt(Config.PendingTargetingZOrderBoost);
		AddBool(Config.bPendingTargetingStraightenAngle);
		AddFloat(Config.PendingTargetingAngleBlend);
		AddBool(Config.bEnableTargetSelectHandDeemphasis);
		AddFloat(Config.TargetSelectNonPendingOpacityMultiplier);
		AddFloat(Config.DisabledRenderOpacity);
		AddFloat(Config.HoverLiftPixels);
		AddFloat(Config.HoverScale);
		AddInt(Config.HoverZOrderBoost);
		AddFloat(Config.HoverHitHysteresisPixels);
		AddBool(Config.CardDepth.bEnableFake3D);
		AddFloat(Config.CardDepth.HoverMaxTiltDegrees);
		AddFloat(Config.CardDepth.DragMaxTiltDegrees);
		AddFloat(Config.CardDepth.PressedTiltMultiplier);
		AddFloat(Config.CardDepth.PerspectiveStrength);
		AddFloat(Config.CardDepth.ResponseSpeed);
		AddFloat(Config.CardDepth.ReturnSpeed);
		AddFloat(Config.CardDepth.DragVelocityFilterSpeed);
		AddFloat(Config.CardDepth.DragVelocityForMaxTiltPixelsPerSecond);
		AddBool(Config.CardDepth.bEnableContactShadow);
		AddFloat(Config.CardDepth.HoverContactShadowLift);
		AddFloat(Config.CardDepth.DragContactShadowLift);
		AddBool(Config.CardUseEffect.bEnabled);
		AddBool(Config.CardUseEffect.bReducedMotion);
		Combine(GetTypeHash(Config.CardUseEffect.Style.SurfaceEffectMaterialInstance.Get()));
		Combine(GetTypeHash(static_cast<uint8>(Config.CardUseEffect.Style.EffectKind)));
		AddFloat(Config.CardUseEffect.Style.DurationSeconds);
		AddFloat(Config.CardUseEffect.Style.ConfirmHoldSeconds);
		AddFloat(Config.CardUseEffect.Style.EdgeFlipImpactSeconds);
		AddFloat(Config.CardUseEffect.Style.EdgeFlipLiftPixels);
		AddFloat(Config.CardUseEffect.Style.EdgeFlipScaleMultiplier);
		AddFloat(Config.CardUseEffect.Style.EdgeFlipMinimumHorizontalScale);
		AddFloat(Config.CardUseEffect.Style.EdgeFlipReformOutSeconds);
		AddFloat(Config.CardUseEffect.Style.EdgeFlipReformHiddenHoldSeconds);
		AddFloat(Config.CardUseEffect.Style.EdgeFlipReformInSeconds);
		AddFloat(Config.CardUseEffect.Style.EdgeFlipReformSettleSeconds);
		AddFloat(Config.CardUseEffect.Style.ReformDissolveOutSeconds);
		AddFloat(Config.CardUseEffect.Style.ReformHiddenHoldSeconds);
		AddFloat(Config.CardUseEffect.Style.ReformBuildInSeconds);
		Combine(GetTypeHash(Config.CardUseEffect.Style.StartSound.Get()));
		AddFloat(Config.CardUseEffect.Style.StartSoundVolumeMultiplier);
		AddFloat(Config.CardUseEffect.Style.StartSoundPitchMultiplier);
		AddFloat(Config.CardUseEffect.Style.StartSoundPitchVariation);
		AddBool(Config.PlayedDissolve.bEnabled);
		AddBool(Config.PlayedDissolve.bReducedMotion);
		AddInt(static_cast<int32>(Config.PlayedDissolve.Style.EffectKind));
		Combine(GetTypeHash(Config.PlayedDissolve.Style.SurfaceEffectMaterial.Get()));
		Combine(GetTypeHash(Config.PlayedDissolve.Style.NoiseTexture.Get()));
		AddFloat(Config.PlayedDissolve.Style.DurationSeconds);
		AddFloat(Config.PlayedDissolve.Style.ConfirmHoldSeconds);
		AddFloat(Config.PlayedDissolve.Style.GridColumns);
		AddFloat(Config.PlayedDissolve.Style.DirectionAngleDegrees);
		AddFloat(Config.PlayedDissolve.Style.Jitter);
		AddColor(Config.PlayedDissolve.Style.EdgeColor);
		AddColor(Config.PlayedDissolve.Style.EdgeAccentColor);
		AddFloat(Config.PlayedDissolve.Style.EdgeWidth);
		AddFloat(Config.PlayedDissolve.Style.EdgeIntensity);
		AddFloat(Config.PlayedDissolve.Style.AshDensity);
		AddFloat(Config.PlayedDissolve.Style.AshTrailWidth);
		AddFloat(Config.PlayedDissolve.Style.AshLiftPixels);
		AddFloat(Config.PlayedDissolve.Style.AshDriftPixels);
		AddInt(Config.PlayedDissolve.Style.OrderedDither.BayerMatrixSize);
		AddFloat(Config.PlayedDissolve.Style.OrderedDither.BandWidth);
		AddFloat(Config.PlayedDissolve.Style.OrderedDither.ResidueDensity);
		AddFloat(Config.PlayedDissolve.Style.OrderedDither.ResidueTrailWidth);
		AddFloat(Config.PlayedDissolve.Style.OrderedDither.ResidueTravelPixels);
		AddFloat(Config.PlayedDissolve.Style.OrderedDither.ResidueMainDirectionRatio);
		AddFloat(Config.PlayedDissolve.Style.OrderedDither.ResidueDirectionSpreadDegrees);
		AddFloat(Config.PlayedDissolve.Style.OrderedDither.ResidueScatterStrength);
		AddFloat(Config.PlayedDissolve.Style.ShadowFadeFraction);
		Combine(GetTypeHash(Config.PlayedDissolve.Style.StartSound.Get()));
		AddFloat(Config.PlayedDissolve.Style.StartSoundVolumeMultiplier);
		AddFloat(Config.PlayedDissolve.Style.StartSoundPitchMultiplier);
		AddFloat(Config.PlayedDissolve.Style.StartSoundPitchVariation);
		AddBool(Config.HandTargetImpact.bEnabled);
		AddBool(Config.HandTargetImpact.bReducedMotion);
		Combine(GetTypeHash(Config.HandTargetImpact.Style.SurfaceEffectMaterialInstance.Get()));
		AddFloat(Config.HandTargetImpact.Style.PreviewFadeInSeconds);
		AddFloat(Config.HandTargetImpact.Style.PreviewPeriodSeconds);
		AddFloat(Config.HandTargetImpact.Style.CommitDelaySeconds);
		AddFloat(Config.HandTargetImpact.Style.DepartureGateSeconds);
		AddFloat(Config.HandTargetImpact.Style.ReboundPeakSeconds);
		AddFloat(Config.HandTargetImpact.Style.CommitDurationSeconds);
		AddFloat(Config.HandTargetImpact.Style.CompressionScale);
		AddFloat(Config.HandTargetImpact.Style.CompressionTranslationPixels);
		AddFloat(Config.HandTargetImpact.Style.ReboundScale);
		AddFloat(Config.HandTargetImpact.Style.ReboundLiftPixels);
		AddInt(Config.HandTargetImpact.Style.ZOrderBoost);
		Combine(GetTypeHash(Config.HandTargetImpact.Style.ImpactSound.Get()));
		AddFloat(Config.HandTargetImpact.Style.ImpactSoundVolumeMultiplier);
		AddFloat(Config.HandTargetImpact.Style.ImpactSoundPitchMultiplier);
		AddFloat(Config.HandTargetImpact.Style.ImpactSoundPitchVariation);
		AddBool(Config.PileTransfer.bEnabled);
		AddBool(Config.PileTransfer.bDiscardToPileEnabled);
		AddBool(Config.PileTransfer.bReducedMotion);
		Combine(GetTypeHash(Config.PileTransfer.Style.GlyphMaterialInstance.Get()));
		AddFloat(Config.PileTransfer.Style.GlyphSize.X);
		AddFloat(Config.PileTransfer.Style.GlyphSize.Y);
		AddFloat(Config.PileTransfer.Style.StartChargeSeconds);
		AddFloat(Config.PileTransfer.Style.FlightSeconds);
		AddInt(Config.PileTransfer.Style.LaneCount);
		AddFloat(Config.PileTransfer.Style.BaseStaggerSeconds);
		AddFloat(Config.PileTransfer.Style.SettleSeconds);
		AddFloat(Config.PileTransfer.Style.ArcHeightRatio);
		AddFloat(Config.PileTransfer.Style.MinArcHeightPixels);
		AddFloat(Config.PileTransfer.Style.MaxArcHeightPixels);
		AddFloat(Config.PileTransfer.Style.DiscardCollapseSeconds);
		AddFloat(Config.PileTransfer.Style.DiscardGlyphRevealStartSeconds);
		AddFloat(Config.PileTransfer.Style.DiscardFlightSeconds);
		AddFloat(Config.PileTransfer.Style.DiscardStaggerSeconds);
		AddFloat(Config.PileTransfer.Style.DiscardImpactSeconds);
		AddFloat(Config.PileTransfer.Style.DiscardImpactScale);
		AddFloat(Config.PileTransfer.Style.ReshuffleImpactSeconds);
		AddFloat(Config.PileTransfer.Style.ReshuffleImpactScale);
		AddFloat(Config.PileTransfer.Style.ReshuffleFinalImpactStrengthMultiplier);
		AddBool(Config.PileTransfer.Style.bEnableTrail);
		AddFloat(Config.PileTransfer.Style.TrailSampleIntervalSeconds);
		AddInt(Config.PileTransfer.Style.HighDetailTrailSegmentsPerGlyph);
		AddInt(Config.PileTransfer.Style.MediumDetailTrailSegmentsPerGlyph);
		AddInt(Config.PileTransfer.Style.LowDetailTrailSegmentsPerGlyph);
		AddFloat(Config.PileTransfer.Style.TrailHeadWidthPixels);
		AddFloat(Config.PileTransfer.Style.TrailTailWidthPixels);
		AddFloat(Config.PileTransfer.Style.TrailHeadOpacity);
		AddFloat(Config.PileTransfer.Style.TrailTailOpacity);
		AddInt(Config.PileTransfer.Style.MaxTrailQuadCount);
		AddFloat(Config.PileTransfer.Style.MoteLifetimeSeconds);
		AddFloat(Config.PileTransfer.Style.MoteMinSizePixels);
		AddFloat(Config.PileTransfer.Style.MoteMaxSizePixels);
		AddFloat(Config.PileTransfer.Style.MoteBackwardDistancePixels);
		AddFloat(Config.PileTransfer.Style.MoteLateralDistancePixels);
		AddInt(Config.PileTransfer.Style.HighDetailMaxActiveGlyphs);
		AddInt(Config.PileTransfer.Style.MediumDetailMaxActiveGlyphs);
		AddInt(Config.PileTransfer.Style.HighDetailMoteSlotsPerGlyph);
		AddInt(Config.PileTransfer.Style.MediumDetailMoteSlotsPerGlyph);
		AddInt(Config.PileTransfer.Style.LowDetailMoteSlotsPerGlyph);
		AddInt(Config.PileTransfer.Style.MaxMoteQuadCount);
		AddFloat(Config.PileTransfer.Style.SafeViewportPaddingPixels);
		AddFloat(Config.PileTransfer.Style.ReducedMotionDurationSeconds);
		Combine(GetTypeHash(Config.PileTransfer.Style.StartSound.Get()));
		Combine(GetTypeHash(Config.PileTransfer.Style.TravelSound.Get()));
		Combine(GetTypeHash(Config.PileTransfer.Style.CompleteSound.Get()));
		AddFloat(Config.PileTransfer.Style.SoundVolumeMultiplier);
		AddFloat(Config.PileTransfer.Style.SoundPitchMultiplier);
		AddBool(Config.Selection.bEnabled);
		AddBool(Config.Selection.bReducedMotion);
		AddColor(Config.Selection.Style.PrimaryColor);
		AddColor(Config.Selection.Style.SecondaryColor);
		AddColor(Config.Selection.Style.AccentColor);
		AddFloat(Config.Selection.Style.EnterDurationSeconds);
		AddFloat(Config.Selection.Style.ExitDurationSeconds);
		AddFloat(Config.Selection.Style.SustainPeriodSeconds);
		AddFloat(Config.Selection.Style.SustainIntensity);
		AddFloat(Config.Selection.Style.GridColumns);
		AddFloat(Config.Selection.Style.SweepAngleDegrees);
		AddFloat(Config.Selection.Style.SweepWidth);
		AddFloat(Config.Selection.Style.SweepIntensity);
		AddFloat(Config.Selection.Style.InnerEdgePixels);
		AddFloat(Config.Selection.Style.OuterEdgePixels);
		AddFloat(Config.Selection.Style.GlintDensity);
		AddFloat(Config.Selection.Style.GlintSpeed);
		Combine(GetTypeHash(Config.Selection.Style.PixelClusterMask.Get()));
		AddBool(Config.bEnableCardInteractionFeedback);
		AddColor(Config.PlayableHoverFeedbackColor);
		AddFloat(Config.PlayableHoverFeedbackOpacity);
		AddFloat(Config.PressedFeedbackScale);
		AddColor(Config.PressedFeedbackColor);
		AddFloat(Config.PressedFeedbackOpacity);
		AddBool(Config.bEnableDragPickupFeedback);
		AddFloat(Config.DragPickupDurationSeconds);
		AddFloat(Config.DragPickupRiseSeconds);
		AddFloat(Config.DragPickupLiftPixels);
		AddFloat(Config.DragPickupScaleMultiplier);
		AddBool(Config.bReduceDragPickupMotion);
		Combine(GetTypeHash(Config.DragPickupSound.Get()));
		AddFloat(Config.DragPickupSoundVolumeMultiplier);
		AddFloat(Config.DragPickupSoundPitchMultiplier);
		AddFloat(Config.DragPickupSoundPitchVariation);
		AddFloat(Config.ConfirmFeedbackDuration);
		AddFloat(Config.ConfirmFeedbackOpacity);
		AddColor(Config.DenyFeedbackColor);
		AddFloat(Config.DenyFeedbackOpacity);
		AddFloat(Config.DenyFeedbackDuration);
		AddFloat(Config.DenyFeedbackShakePixels);
		AddSoftObjectPath(Config.InteractionFeedbackMaterial.ToSoftObjectPath());
		AddFloat(Config.InteractionFeedbackEdgeWidth);
		AddFloat(Config.InteractionFeedbackEdgeSoftness);
		AddFloat(Config.InteractionFeedbackVignetteStrength);
		AddFloat(Config.InteractionFeedbackVignetteRadius);
		AddFloat(Config.InteractionFeedbackVignetteSoftness);
		AddBool(Config.bEnablePlayCommitFeedback);
		AddFloat(Config.PlayCommitFeedbackDuration);
		AddFloat(Config.PlayCommitFeedbackOpacity);
		AddColor(Config.PlayCommitFeedbackColor);
		AddFloat(Config.PlayCommitFeedbackScale);
		AddBool(Config.bEnableRetainedFeedback);
		AddFloat(Config.RetainedFeedbackDuration);
		AddFloat(Config.RetainedFeedbackStaggerSeconds);
		AddFloat(Config.RetainedFeedbackLiftPixels);
		AddFloat(Config.RetainedFeedbackScale);
		AddInt(Config.RetainedFeedbackZOrderBoost);
		AddFloat(Config.DragCardTargetFocusLiftPixels);
		AddFloat(Config.DragCardTargetFocusScale);
		AddInt(Config.DragCardTargetFocusZOrderBoost);
		return Hash;
	}

}

void FWacomFirstPersonCardAnchorRuntimeStateDeleter::operator()(
	FWacomFirstPersonCardAnchorRuntimeState* State) const
{
	delete State;
}

void FWacomFirstPersonCardLayerOwnerDeleter::operator()(
	FWacomFirstPersonCardLayerOwner* Owner) const
{
	delete Owner;
}

void FWacomFirstPersonCardLayerDelegateRouterDeleter::operator()(
	FWacomFirstPersonCardLayerDelegateRouter* Router) const
{
	delete Router;
}

UWacomFirstPersonCardAnchorComponent::UWacomFirstPersonCardAnchorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	RuntimeState.Reset(new FWacomFirstPersonCardAnchorRuntimeState());
	CardLayerOwner.Reset(new FWacomFirstPersonCardLayerOwner());
	CardLayerDelegateRouter.Reset(new FWacomFirstPersonCardLayerDelegateRouter());
	ConfigureCardLayerDelegateRouter();
}

UWacomFirstPersonCardAnchorComponent::~UWacomFirstPersonCardAnchorComponent() = default;

void UWacomFirstPersonCardAnchorComponent::BeginPlay()
{
	Super::BeginPlay();
	bFirstPersonCardLayerInteractionEnabled = bEnableBattleHandInteraction;
	ConfigureTickPrerequisites();
	SetComponentTickEnabled(true);
}

void UWacomFirstPersonCardAnchorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetFirstPersonCardLayerInteractionEnabled(false);
	ResetAnchorScreenSmoothing();
	RemoveCardLayer();
	Super::EndPlay(EndPlayReason);
}

void UWacomFirstPersonCardAnchorComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshAnchor(DeltaTime);
	UpdateCardLayer();
}

void UWacomFirstPersonCardAnchorComponent::RefreshAnchor(float DeltaTime)
{
	RefreshResolvedCardLayoutRuntimeState();
	const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
	FTransform BaseTransform = FTransform::Identity;
	EWacomFirstPersonCardAnchorMode ResolvedMode = EWacomFirstPersonCardAnchorMode::Invalid;
	FName ResolvedFallbackReason = NAME_None;
	if (!ResolveBaseAnchor(BaseTransform, ResolvedMode, ResolvedFallbackReason))
	{
		bHasValidAnchor = false;
		CurrentMode = ResolvedMode;
		CurrentLookOffsetUsed = FRotator::ZeroRotator;
		CurrentRawCursorLookOffset = FRotator::ZeroRotator;
		bCurrentLookOffsetAppliedToLayout = false;
		LastFallbackReason = ResolvedFallbackReason;
		ResetAnchorScreenSmoothing();
		return;
	}

	FRotator LookOffset = FRotator::ZeroRotator;
	FRotator RawCursorLookOffset = FRotator::ZeroRotator;
	if (const AWacomPlayerCharacter* Character = GetOwnerCharacter())
	{
		if (const UWacomCursorLookDriverComponent* CursorLook = Character->GetCursorLookDriverComponent())
		{
			RawCursorLookOffset = CursorLook->GetCurrentLookOffset();
		}
	}
	if (Config.ProjectionMode == EWacomFirstPersonCardProjectionMode::LegacyWorldProjected)
	{
		LookOffset.Pitch = RawCursorLookOffset.Pitch * Config.LookInfluencePitch;
		LookOffset.Yaw = RawCursorLookOffset.Yaw * Config.LookInfluenceYaw;
	}

	FRotator AnchorRotation = BaseTransform.Rotator();
	AnchorRotation.Roll = 0.0f;
	AnchorRotation.Pitch += LookOffset.Pitch;
	AnchorRotation.Yaw += LookOffset.Yaw;
	AnchorRotation.Roll = 0.0f;

	const FRotationMatrix AnchorRotationMatrix(AnchorRotation);
	const FVector AnchorLocation =
		BaseTransform.GetLocation()
		+ AnchorRotationMatrix.GetScaledAxis(EAxis::X) * FMath::Max(0.0f, DistanceFromView)
		+ AnchorRotationMatrix.GetScaledAxis(EAxis::Y) * HorizontalOffset
		+ AnchorRotationMatrix.GetScaledAxis(EAxis::Z) * VerticalOffset;
	const FTransform TargetAnchorTransform(AnchorRotation, AnchorLocation, FVector::OneVector);
	const float EffectiveFollowInterpSpeed = UsesCameraStageFollowSpeed(CurrentMode, ResolvedMode)
		? CameraStageFollowInterpSpeed
		: FollowInterpSpeed;

	if (!bHasInitializedAnchor || EffectiveFollowInterpSpeed <= 0.0f || DeltaTime <= 0.0f)
	{
		CurrentAnchorTransform = TargetAnchorTransform;
	}
	else
	{
		const FVector SmoothedLocation = FMath::VInterpTo(
			CurrentAnchorTransform.GetLocation(),
			TargetAnchorTransform.GetLocation(),
			DeltaTime,
			EffectiveFollowInterpSpeed);
		const FRotator SmoothedRotation = FMath::RInterpTo(
			CurrentAnchorTransform.Rotator(),
			TargetAnchorTransform.Rotator(),
			DeltaTime,
			EffectiveFollowInterpSpeed);
		CurrentAnchorTransform = FTransform(SmoothedRotation, SmoothedLocation, FVector::OneVector);
	}

	bHasInitializedAnchor = true;
	bHasValidAnchor = true;
	CurrentMode = ResolvedMode;
	CurrentLookOffsetUsed = LookOffset;
	CurrentRawCursorLookOffset = RawCursorLookOffset;
	bCurrentLookOffsetAppliedToLayout = !LookOffset.IsNearlyZero();
	LastFallbackReason = ResolvedFallbackReason;
}

void UWacomFirstPersonCardAnchorComponent::RefreshCardLayerNow(float DeltaTime)
{
	RefreshAnchor(DeltaTime);
	UpdateCardLayer();
}

FTransform UWacomFirstPersonCardAnchorComponent::ComputeCardTransform(int32 NumCards, int32 CardIndex) const
{
	RefreshResolvedCardLayoutRuntimeState();
	const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
	if (!bHasValidAnchor || NumCards <= 0 || CardIndex < 0 || CardIndex >= NumCards)
	{
		return CurrentAnchorTransform;
	}

	const float CenterOffset =
		static_cast<float>(CardIndex) - (static_cast<float>(NumCards - 1) * 0.5f);
	const FRotator AnchorRotation = CurrentAnchorTransform.Rotator();
	const FRotationMatrix AnchorRotationMatrix(AnchorRotation);
	const FVector CardLocation =
		CurrentAnchorTransform.GetLocation()
		+ AnchorRotationMatrix.GetScaledAxis(EAxis::Y) * (CenterOffset * FMath::Max(0.0f, CardSpacing));

	FRotator CardRotation = (AnchorRotation + FRotator(0.0f, 180.0f, 0.0f)).GetNormalized();
	CardRotation.Yaw += CenterOffset * Config.FanYawDegrees;
	return FTransform(CardRotation, CardLocation, FVector::OneVector);
}

bool UWacomFirstPersonCardAnchorComponent::ProjectCardTransformToScreen(
	const FTransform& CardTransform,
	FWacomFirstPersonCardProjectedPoint& OutProjectedPoint,
	int32 PointIndex) const
{
	RefreshResolvedCardLayoutRuntimeState();
	const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
	OutProjectedPoint = FWacomFirstPersonCardProjectedPoint();
	OutProjectedPoint.Index = PointIndex;
	OutProjectedPoint.WorldLocation = CardTransform.GetLocation();
	OutProjectedPoint.ProjectionMode = Config.ProjectionMode;
	OutProjectedPoint.bBodyLockedLayout = Config.ProjectionMode == EWacomFirstPersonCardProjectionMode::BodyLocked;
	OutProjectedPoint.bCurrentCameraProjection = true;
	OutProjectedPoint.bLookOffsetAppliedToLayout = bCurrentLookOffsetAppliedToLayout;

	FVector2D WidgetPosition = FVector2D::ZeroVector;
	FVector2D RawScreenPosition = FVector2D::ZeroVector;
	const bool bProjectionSucceeded = ProjectWorldLocationToWidgetPositionForAnchor(
		CardTransform.GetLocation(),
		WidgetPosition,
		RawScreenPosition);
	if (!bProjectionSucceeded)
	{
		return false;
	}

	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (!GetViewportSizeForAnchor(ViewportSize)
		|| ViewportSize.X <= 0.0f
		|| ViewportSize.Y <= 0.0f)
	{
		return false;
	}

	const float ViewportScale = FMath::Max(0.01f, GetViewportScaleForAnchor());
	const FVector2D WidgetViewportSize = ViewportSize / ViewportScale;
	const FVector2D UnclampedPosition = WidgetPosition;
	bool bClamped = false;
	bool bOutsideViewport = false;
	float OffscreenDistancePixels = 0.0f;
	WidgetPosition = FWacomFirstPersonCardSlotLayoutBuilder::ApplyViewportClampToWidgetPosition(
		UnclampedPosition,
		WidgetViewportSize,
		Config,
		bClamped,
		bOutsideViewport,
		OffscreenDistancePixels);

	bool bPixelSnapped = false;
	const FVector2D SnappedPosition =
		FWacomFirstPersonCardSlotLayoutBuilder::SnapPosition(WidgetPosition, Config, bPixelSnapped);

	OutProjectedPoint.RawScreenPosition = RawScreenPosition;
	OutProjectedPoint.WidgetPosition = WidgetPosition;
	OutProjectedPoint.UnclampedWidgetPosition = UnclampedPosition;
	OutProjectedPoint.SnappedWidgetPosition = SnappedPosition;
	OutProjectedPoint.ScreenPosition = SnappedPosition;
	OutProjectedPoint.ViewportClampMode = Config.ViewportClampMode;
	OutProjectedPoint.ViewportScale = ViewportScale;
	OutProjectedPoint.OffscreenDistancePixels = OffscreenDistancePixels;
	OutProjectedPoint.UnsmoothedAnchorWidgetPosition = WidgetPosition;
	OutProjectedPoint.SmoothedAnchorWidgetPosition = WidgetPosition;
	OutProjectedPoint.bProjected = true;
	OutProjectedPoint.bClamped = bClamped;
	OutProjectedPoint.bOutsideViewport = bOutsideViewport;
	OutProjectedPoint.bPixelSnapped = bPixelSnapped;
	return true;
}

bool UWacomFirstPersonCardAnchorComponent::ResolveCameraTransformForAnchor(FTransform& OutCameraTransform) const
{
	const APlayerController* PC = GetOwnerPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}

	OutCameraTransform = FTransform(
		PC->PlayerCameraManager->GetCameraRotation(),
		PC->PlayerCameraManager->GetCameraLocation(),
		FVector::OneVector);
	return true;
}

bool UWacomFirstPersonCardAnchorComponent::ProjectWorldLocationForAnchor(
	const FVector& WorldLocation,
	FVector2D& OutScreenPosition) const
{
	APlayerController* PC = GetOwnerPlayerController();
	return PC && PC->ProjectWorldLocationToScreen(WorldLocation, OutScreenPosition, false);
}

bool UWacomFirstPersonCardAnchorComponent::ProjectWorldLocationToWidgetPositionForAnchor(
	const FVector& WorldLocation,
	FVector2D& OutWidgetPosition,
	FVector2D& OutRawScreenPosition) const
{
	APlayerController* PC = GetOwnerPlayerController();
	if (!PC)
	{
		return false;
	}

	if (!ProjectWorldLocationForAnchor(WorldLocation, OutRawScreenPosition))
	{
		return false;
	}

	return UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		PC,
		WorldLocation,
		OutWidgetPosition,
		false);
}

bool UWacomFirstPersonCardAnchorComponent::GetViewportSizeForAnchor(FVector2D& OutViewportSize) const
{
	const APlayerController* PC = GetOwnerPlayerController();
	if (!PC)
	{
		return false;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
	OutViewportSize = FVector2D(ViewportSizeX, ViewportSizeY);
	return ViewportSizeX > 0 && ViewportSizeY > 0;
}

float UWacomFirstPersonCardAnchorComponent::GetViewportScaleForAnchor() const
{
	APlayerController* PC = GetOwnerPlayerController();
	if (!PC)
	{
		return 1.0f;
	}

	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(PC);
	return ViewportScale > 0.0f ? ViewportScale : 1.0f;
}

float UWacomFirstPersonCardAnchorComponent::GetAnchorSmoothingDeltaTimeForAnchor() const
{
	const UWorld* World = GetWorld();
	return World ? FMath::Max(0.0f, World->GetDeltaSeconds()) : 0.0f;
}

bool UWacomFirstPersonCardAnchorComponent::CanCreateCardLayerForAnchor(
	APlayerController* PlayerController) const
{
	return PlayerController && PlayerController->IsLocalController();
}

UWacomFirstPersonCardLayerWidget* UWacomFirstPersonCardAnchorComponent::CreateCardLayerWidgetForAnchor(
	APlayerController* PlayerController,
	TSubclassOf<UWacomFirstPersonCardLayerWidget> LayerClass) const
{
	if (!PlayerController || !LayerClass)
	{
		return nullptr;
	}

	return CreateWidget<UWacomFirstPersonCardLayerWidget>(PlayerController, LayerClass);
}

void UWacomFirstPersonCardAnchorComponent::AddCardLayerWidgetToViewportForAnchor(
	UWacomFirstPersonCardLayerWidget* LayerWidget,
	int32 ZOrder) const
{
	if (LayerWidget)
	{
		LayerWidget->AddToViewport(ZOrder);
	}
}

void UWacomFirstPersonCardAnchorComponent::CommitRuntimeCardLayerFrame(
	const FWacomFirstPersonCardLayerPresentationFrame& Frame)
{
	ApplyRuntimeCardLayerSourceLifecycleFrame(
		FWacomFirstPersonCardLayerSourceLifecycleFrame::FromPresentationFrame(Frame));
}

void UWacomFirstPersonCardAnchorComponent::ApplyRuntimeCardLayerSourceLifecycleFrame(
	const FWacomFirstPersonCardLayerSourceLifecycleFrame& Frame)
{
	const FName SourceId = Frame.ResolveSourceId();
	if (Frame.bSetTransitionPresentationEnabled
		&& RuntimeState
		&& !SourceId.IsNone())
	{
		RuntimeState->SetTransitionPresentationEnabled(
			SourceId,
			Frame.bTransitionPresentationEnabled);
	}
	if (Frame.bSetPresentationAnchors && RuntimeState && !SourceId.IsNone())
	{
		RuntimeState->SetPresentationAnchors(SourceId, Frame.PresentationAnchors);
	}
	if (Frame.bSetPileTransferHints && RuntimeState && !SourceId.IsNone())
	{
		RuntimeState->SetPileTransferHints(SourceId, Frame.PileTransferHints);
	}

	if (Frame.bCommitPresentationFrame)
	{
		ApplyRuntimeCardLayerPresentationFrame(Frame.PresentationFrame);
	}

	if (Frame.bSetInteractionEnabled)
	{
		SetFirstPersonCardLayerInteractionEnabled(Frame.bInteractionEnabled);
	}

	if (Frame.bCancelActiveDrag && CardLayerWidget)
	{
		CardLayerWidget->CancelCardDragGesture(Frame.bBroadcastDragCancel);
	}

	switch (Frame.ClearMode)
	{
	case EWacomFirstPersonCardLayerSourceClearMode::RuntimeData:
		if (!SourceId.IsNone())
		{
			ClearRuntimeCardLayerData(SourceId);
		}
		break;
	case EWacomFirstPersonCardLayerSourceClearMode::VisualState:
		if (RuntimeState && !SourceId.IsNone())
		{
			RuntimeState->ClearPresentationAnchors(SourceId);
		}
		ClearCardLayerVisualState();
		break;
	case EWacomFirstPersonCardLayerSourceClearMode::None:
	default:
		break;
	}
}

void UWacomFirstPersonCardAnchorComponent::ApplyRuntimeCardLayerPresentationFrame(
	const FWacomFirstPersonCardLayerPresentationFrame& Frame)
{
	if (!RuntimeState || Frame.SourceId.IsNone())
	{
		return;
	}

	const bool bRuntimeSourceChanged =
		RuntimeState->SetEntries(Frame.SourceId, Frame.Entries);
	switch (Frame.CommitMode)
	{
	case EWacomFirstPersonCardLayerFrameCommitMode::PresentationFrame:
		RuntimeState->SetPresentationFrameHints(Frame.SourceId, Frame.TransitionHints);
		RuntimeState->SetPresentationFrameFeedbackHints(Frame.SourceId, Frame.FeedbackHints);
		break;
	case EWacomFirstPersonCardLayerFrameCommitMode::Suppressed:
	{
		const TArray<FWacomFirstPersonCardLayerTransitionHint> EmptyTransitionHints;
		RuntimeState->SetPresentationFrameHints(Frame.SourceId, EmptyTransitionHints);
		RuntimeState->SetTransitionHints(Frame.SourceId, EmptyTransitionHints);
		const TArray<FWacomFirstPersonCardLayerFeedbackHint> EmptyFeedbackHints;
		RuntimeState->SetPresentationFrameFeedbackHints(Frame.SourceId, EmptyFeedbackHints);
		RuntimeState->SetFeedbackHints(Frame.SourceId, EmptyFeedbackHints);
		RuntimeState->ClearTransientInteraction();
		break;
	}
	case EWacomFirstPersonCardLayerFrameCommitMode::PreviewOverlay:
	case EWacomFirstPersonCardLayerFrameCommitMode::StateRefresh:
	default:
		break;
	}

	if (bRuntimeSourceChanged)
	{
		RefreshResolvedCardLayoutRuntimeState();
	}
}

#if WITH_AUTOMATION_TESTS
void UWacomFirstPersonCardAnchorComponent::SetRuntimeCardLayerEntries(
	FName SourceId,
	const TArray<FWacomFirstPersonCardLayerEntry>& Entries)
{
	FWacomFirstPersonCardLayerPresentationFrame Frame;
	Frame.SourceId = SourceId;
	Frame.Entries = Entries;
	Frame.CommitMode = EWacomFirstPersonCardLayerFrameCommitMode::StateRefresh;
	CommitRuntimeCardLayerFrame(Frame);
}

void UWacomFirstPersonCardAnchorComponent::SetRuntimeCardLayerPresentationFrame(
	const FWacomFirstPersonCardLayerPresentationFrame& Frame)
{
	FWacomFirstPersonCardLayerPresentationFrame MutableFrame = Frame;
	MutableFrame.CommitMode = EWacomFirstPersonCardLayerFrameCommitMode::PresentationFrame;
	CommitRuntimeCardLayerFrame(MutableFrame);
}

void UWacomFirstPersonCardAnchorComponent::SetRuntimeCardLayerPresentationFrame(
	FName SourceId,
	const TArray<FWacomFirstPersonCardLayerEntry>& Entries,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints)
{
	FWacomFirstPersonCardLayerPresentationFrame Frame;
	Frame.SourceId = SourceId;
	Frame.Entries = Entries;
	Frame.TransitionHints = TransitionHints;
	Frame.CommitMode = EWacomFirstPersonCardLayerFrameCommitMode::PresentationFrame;
	CommitRuntimeCardLayerFrame(Frame);
}

void UWacomFirstPersonCardAnchorComponent::SetRuntimeCardLayerTransitionHints(
	FName SourceId,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& Hints)
{
	if (RuntimeState)
	{
		RuntimeState->SetTransitionHints(SourceId, Hints);
	}
}

#endif

void UWacomFirstPersonCardAnchorComponent::SetRuntimeCardLayerTransitionPresentationEnabled(
	FName SourceId,
	bool bEnabled)
{
	if (RuntimeState)
	{
		RuntimeState->SetTransitionPresentationEnabled(SourceId, bEnabled);
	}
}

bool UWacomFirstPersonCardAnchorComponent::HasRuntimeCardLayerPendingPresentationFrame(FName SourceId) const
{
	return RuntimeState
		&& (RuntimeState->HasPresentationFrameHintsForSource(SourceId)
			|| RuntimeState->HasPresentationFrameFeedbackHintsForSource(SourceId));
}

bool UWacomFirstPersonCardAnchorComponent::HasActiveCardLayerPresentationPlayback() const
{
	return CardLayerWidget && CardLayerWidget->HasActivePresentationPlayback();
}

void UWacomFirstPersonCardAnchorComponent::ForceSettleCardLayerPresentationPlayback()
{
	if (RuntimeState)
	{
		RuntimeState->ClearPresentationFrameHints();
		RuntimeState->ClearPileTransferHints();
		RuntimeState->ClearPresentationFrameFeedbackHints();
	}
	if (CardLayerWidget)
	{
		CardLayerWidget->ForceSettlePresentationPlayback();
	}
}

void UWacomFirstPersonCardAnchorComponent::SetRuntimeCardLayerData(
	FName SourceId,
	const TArray<FWacomCardViewData>& Cards)
{
	FWacomFirstPersonCardLayerPresentationFrame Frame;
	Frame.SourceId = SourceId;
	Frame.Entries = BuildCardLayerEntriesFromData(Cards);
	Frame.CommitMode = EWacomFirstPersonCardLayerFrameCommitMode::StateRefresh;
	CommitRuntimeCardLayerFrame(Frame);
}

void UWacomFirstPersonCardAnchorComponent::ClearRuntimeCardLayerData(FName SourceId)
{
	if (!RuntimeState || !RuntimeState->Clear(SourceId))
	{
		return;
	}

	RefreshResolvedCardLayoutRuntimeState();
	ResetAnchorScreenSmoothing();
	if (CardLayerWidget)
	{
		CardLayerWidget->CancelCardDragGesture(true);
		CardLayerWidget->ClearSlotMotionState();
	}
}

void UWacomFirstPersonCardAnchorComponent::ClearCardLayerVisualState()
{
	ResetAnchorScreenSmoothing();
	if (RuntimeState)
	{
		RuntimeState->ClearTransientInteraction();
	}
	if (CardLayerWidget)
	{
		CardLayerWidget->CancelCardDragGesture(true);
		CardLayerWidget->ClearSlotMotionState();
	}
}

bool UWacomFirstPersonCardAnchorComponent::HasRuntimeCardLayerData() const
{
	return RuntimeState && RuntimeState->HasRuntimeData();
}

FName UWacomFirstPersonCardAnchorComponent::GetRuntimeCardLayerSourceId() const
{
	return RuntimeState ? RuntimeState->GetSourceId() : NAME_None;
}

int32 UWacomFirstPersonCardAnchorComponent::GetRuntimeCardLayerCardCount() const
{
	return RuntimeState ? RuntimeState->GetCardCount() : 0;
}

const TArray<FWacomCardViewData>& UWacomFirstPersonCardAnchorComponent::GetRuntimeCardLayerData() const
{
	static const TArray<FWacomCardViewData> EmptyCardData;
	return RuntimeState ? RuntimeState->GetCardData() : EmptyCardData;
}

const TArray<FWacomFirstPersonCardLayerEntry>&
UWacomFirstPersonCardAnchorComponent::GetRuntimeCardLayerEntries() const
{
	static const TArray<FWacomFirstPersonCardLayerEntry> EmptyEntries;
	return RuntimeState ? RuntimeState->GetEntries() : EmptyEntries;
}

FGuid UWacomFirstPersonCardAnchorComponent::GetHoveredCardInstanceId() const
{
	return RuntimeState ? RuntimeState->GetHoveredCardInstanceId() : FGuid();
}

TArray<FWacomFirstPersonCardLayerEntry> UWacomFirstPersonCardAnchorComponent::BuildCardLayerEntriesFromData(
	const TArray<FWacomCardViewData>& CardData)
{
	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	Entries.Reserve(CardData.Num());
	for (const FWacomCardViewData& Data : CardData)
	{
		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardViewData = Data;
		Entry.bIsPlayable = !Data.bDisabled;
		Entries.Add(MoveTemp(Entry));
	}
	return Entries;
}

TArray<FWacomFirstPersonCardLayerSlotView> UWacomFirstPersonCardAnchorComponent::BuildCardSlotViewsFromEntries(
	const TArray<FWacomFirstPersonCardLayerEntry>& CardEntries) const
{
	RefreshResolvedCardLayoutRuntimeState();
	const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
	if (!bHasValidAnchor)
	{
		return {};
	}

	FWacomFirstPersonCardProjectedPoint AnchorPoint;
	const bool bAnchorProjected = ProjectCardTransformToScreen(CurrentAnchorTransform, AnchorPoint, INDEX_NONE);
	if (bAnchorProjected)
	{
		ApplyAnchorScreenSmoothing(AnchorPoint);
	}
	else
	{
		ResetAnchorScreenSmoothing();
	}

	FWacomFirstPersonCardSlotLayoutBuildInput BuildInput;
	BuildInput.CardEntries = &CardEntries;
	BuildInput.Config = &Config;
	BuildInput.AnchorPoint = AnchorPoint;
	BuildInput.HoveredCardInstanceId = GetHoveredCardInstanceId();
	BuildInput.bHasValidAnchor = bHasValidAnchor;
	BuildInput.bAnchorProjected = bAnchorProjected;
	BuildInput.bCurrentLookOffsetAppliedToLayout = bCurrentLookOffsetAppliedToLayout;
	return FWacomFirstPersonCardSlotLayoutBuilder::BuildSlots(BuildInput);
}

TArray<FWacomFirstPersonCardLayerSlotView> UWacomFirstPersonCardAnchorComponent::BuildActiveCardLayerSlotViews() const
{
	return HasRuntimeCardLayerData()
		? BuildCardSlotViewsFromEntries(GetRuntimeCardLayerEntries())
		: TArray<FWacomFirstPersonCardLayerSlotView>();
}

void UWacomFirstPersonCardAnchorComponent::SetFirstPersonCardLayerInteractionEnabled(bool bEnabled)
{
	if (bFirstPersonCardLayerInteractionEnabled == bEnabled)
	{
		return;
	}

	bFirstPersonCardLayerInteractionEnabled = bEnabled;
	if (!bFirstPersonCardLayerInteractionEnabled)
	{
		if (RuntimeState)
		{
			RuntimeState->ClearTransientInteraction();
		}
		if (CardLayerWidget)
		{
			CardLayerWidget->CancelCardDragGesture(true);
		}
	}
	if (CardLayerWidget)
	{
		CardLayerWidget->SetCardLayerInteractionEnabled(bFirstPersonCardLayerInteractionEnabled);
	}
}

#if WITH_AUTOMATION_TESTS
TArray<FWacomFirstPersonCardLayerSlotView>
UWacomFirstPersonCardAnchorComponent::BuildLayoutFixtureCardSlotViews() const
{
	const int32 DesiredCount = LayoutFixtureCardDefinitions.IsEmpty()
		? LayoutFixtureCardCount
		: LayoutFixtureCardDefinitions.Num();
	TArray<FWacomCardViewData> CardData;
	CardData.Reserve(FMath::Clamp(DesiredCount, 0, 32));
	for (int32 Index = 0; Index < FMath::Clamp(DesiredCount, 0, 32); ++Index)
	{
		FWacomCardViewData Data;
		if (LayoutFixtureCardDefinitions.IsValidIndex(Index))
		{
			if (const UCardDefinition* Card = LayoutFixtureCardDefinitions[Index].LoadSynchronous())
			{
				Data = UWacomCardPresentationBuilder::BuildCardViewData(Card);
			}
		}
		if (Data.Name.IsEmpty())
		{
			Data.Name = FText::Format(
				NSLOCTEXT("Wacom.FirstPersonCardLayerTests", "LayoutFixtureName", "Layout Card {0}"),
				FText::AsNumber(Index + 1));
		}
		CardData.Add(MoveTemp(Data));
	}
	return BuildCardSlotViewsFromEntries(BuildCardLayerEntriesFromData(CardData));
}

FWacomFirstPersonCardAnchorAutomationTestView UWacomFirstPersonCardAnchorComponent::GetAutomationTestViewForTest() const
{
	RefreshResolvedCardLayoutRuntimeState();
	FWacomFirstPersonCardAnchorAutomationTestView View;
	View.bHasValidAnchor = bHasValidAnchor;
	View.Mode = CurrentMode;
	View.AnchorTransform = CurrentAnchorTransform;
	View.LookOffsetUsed = CurrentLookOffsetUsed;
	View.RawCursorLookOffset = CurrentRawCursorLookOffset;
	View.AppliedAnchorLookOffset = CurrentLookOffsetUsed;
	View.LookInfluenceYaw = LookInfluenceYaw;
	View.LookInfluencePitch = LookInfluencePitch;
	View.LastFallbackReason = LastFallbackReason;
	View.ProjectionMode = ProjectionMode;
	View.bLookOffsetAppliedToLayout = bCurrentLookOffsetAppliedToLayout;
	View.bLookResponsiveProjection = ProjectionMode == EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	View.CardLayerWidget = CardLayerWidget;
	View.CardLayerConfigApplyCount = CardLayerOwner ? CardLayerOwner->GetConfigApplyCountForTest() : 0;
	if (RuntimeState)
	{
		View.PendingTransitionHintSourceId = RuntimeState->GetTransitionHintSourceId();
		if (RuntimeState->GetPresentationFrameHintSourceId() == RuntimeState->GetSourceId()
			&& RuntimeState->GetPresentationFrameHints().Num() > 0)
		{
			View.PendingTransitionHintSourceId = RuntimeState->GetPresentationFrameHintSourceId();
		}
		View.bHasPendingTransitionHintsForCurrentSource =
			RuntimeState->HasTransitionHintsForCurrentSource()
			|| RuntimeState->HasPresentationFrameHintsForCurrentSource();
		View.bCanConsumePendingTransitionHintsForCurrentSource =
			RuntimeState->CanConsumeTransitionHintsForCurrentSource()
			|| RuntimeState->CanConsumePresentationFrameHintsForCurrentSource();
		View.bTransitionPresentationEnabledForCurrentSource =
			RuntimeState->IsTransitionPresentationEnabled(RuntimeState->GetSourceId());
		View.PresentationAnchorSourceId = RuntimeState->GetPresentationAnchorSourceId();
		View.PresentationAnchors = RuntimeState->GetPresentationAnchors();
		for (const FWacomFirstPersonCardLayerTransitionHint& Hint : RuntimeState->GetTransitionHints())
		{
			View.PendingTransitionHintCardIds.Add(Hint.CardInstanceId);
		}
		for (const FWacomFirstPersonCardLayerTransitionHint& Hint : RuntimeState->GetPresentationFrameHints())
		{
			View.PendingTransitionHintCardIds.Add(Hint.CardInstanceId);
		}
	}
	return View;
}

void UWacomFirstPersonCardAnchorComponent::SetCardLayerWidgetForTest(
	UWacomFirstPersonCardLayerWidget* LayerWidget)
{
	if (CardLayerWidget && CardLayerDelegateRouter)
	{
		CardLayerDelegateRouter->Unbind(CardLayerWidget);
	}

	CardLayerWidget = LayerWidget;
	if (CardLayerWidget && CardLayerDelegateRouter)
	{
		CardLayerDelegateRouter->Bind(CardLayerWidget);
	}
}

void UWacomFirstPersonCardAnchorComponent::SetHoveredCardInstanceIdForTest(const FGuid& CardInstanceId)
{
	if (RuntimeState)
	{
		RuntimeState->SetHoveredCardInstanceId(CardInstanceId);
	}
}
#endif

AWacomPlayerCharacter* UWacomFirstPersonCardAnchorComponent::GetOwnerCharacter() const
{
	return Cast<AWacomPlayerCharacter>(GetOwner());
}

APlayerController* UWacomFirstPersonCardAnchorComponent::GetOwnerPlayerController() const
{
	const AWacomPlayerCharacter* Character = GetOwnerCharacter();
	return Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
}

bool UWacomFirstPersonCardAnchorComponent::ResolveBaseAnchor(
	FTransform& OutBaseTransform,
	EWacomFirstPersonCardAnchorMode& OutMode,
	FName& OutFallbackReason) const
{
	const AWacomPlayerCharacter* Character = GetOwnerCharacter();
	if (!Character)
	{
		OutMode = EWacomFirstPersonCardAnchorMode::Invalid;
		OutFallbackReason = NoOwnerReason;
		return false;
	}

	if (const UWacomBattleCameraLookComponent* BattleCamera = Character->GetBattleCameraLookComponent())
	{
		if (BattleCamera->IsBattleCameraLookActive())
		{
			if (!GetOwnerPlayerController())
			{
				OutMode = EWacomFirstPersonCardAnchorMode::Invalid;
				OutFallbackReason = NoPlayerControllerReason;
				return false;
			}

			FVector CameraLocation = FVector::ZeroVector;
			if (const UCameraComponent* FirstPersonCamera = Character->GetFirstPersonCamera())
			{
				CameraLocation = FirstPersonCamera->GetComponentLocation();
			}
			else
			{
				FTransform CameraTransform = FTransform::Identity;
				if (!ResolveCameraTransformForAnchor(CameraTransform))
				{
					OutMode = EWacomFirstPersonCardAnchorMode::Invalid;
					OutFallbackReason = NoCameraManagerReason;
					return false;
				}
				CameraLocation = CameraTransform.GetLocation();
			}

			OutBaseTransform = FTransform(
				BattleCamera->GetBaseBattleRotation(),
				CameraLocation,
				FVector::OneVector);
			OutMode = EWacomFirstPersonCardAnchorMode::BattleCamera;
			OutFallbackReason = NAME_None;
			return true;
		}
	}

	if (const UWacomFirstPersonViewStageBlendComponent* StageBlend =
		Character->GetFirstPersonViewStageBlendComponent())
	{
		FTransform StageViewTransform = FTransform::Identity;
		if (StageBlend->TryGetCurrentBaseViewTransform(StageViewTransform))
		{
			OutBaseTransform = StageViewTransform;
			OutMode = EWacomFirstPersonCardAnchorMode::ViewStageBlend;
			OutFallbackReason = NAME_None;
			return true;
		}
	}

	if (const UWacomRunTunnelMovementComponent* RunTunnel = Character->GetRunTunnelMovementComponent())
	{
		if (RunTunnel->IsRunTunnelActive() && !RunTunnel->IsRunTunnelSuspended())
		{
			if (const AWacomRunTunnelSegmentActor* Segment = RunTunnel->GetActiveSegment())
			{
				OutBaseTransform = Segment->GetSplineTransformAtDistance(RunTunnel->GetDistanceAlongSpline());
				OutMode = EWacomFirstPersonCardAnchorMode::RunTunnel;
				OutFallbackReason = NAME_None;
				return true;
			}
		}
	}

	if (!GetOwnerPlayerController())
	{
		OutMode = EWacomFirstPersonCardAnchorMode::Invalid;
		OutFallbackReason = NoPlayerControllerReason;
		return false;
	}
	if (!ResolveCameraTransformForAnchor(OutBaseTransform))
	{
		OutMode = EWacomFirstPersonCardAnchorMode::Invalid;
		OutFallbackReason = NoCameraManagerReason;
		return false;
	}

	OutMode = EWacomFirstPersonCardAnchorMode::CameraFallback;
	OutFallbackReason = CameraFallbackReason;
	return true;
}

void UWacomFirstPersonCardAnchorComponent::ConfigureTickPrerequisites()
{
	if (const AWacomPlayerCharacter* Character = GetOwnerCharacter())
	{
		if (UWacomRunTunnelMovementComponent* RunTunnel = Character->GetRunTunnelMovementComponent())
		{
			PrimaryComponentTick.AddPrerequisite(RunTunnel, RunTunnel->PrimaryComponentTick);
		}
		if (UWacomBattleCameraLookComponent* BattleCamera = Character->GetBattleCameraLookComponent())
		{
			PrimaryComponentTick.AddPrerequisite(BattleCamera, BattleCamera->PrimaryComponentTick);
		}
		if (UWacomFirstPersonViewStageBlendComponent* StageBlend =
			Character->GetFirstPersonViewStageBlendComponent())
		{
			PrimaryComponentTick.AddPrerequisite(StageBlend, StageBlend->PrimaryComponentTick);
		}
	}
}

bool UWacomFirstPersonCardAnchorComponent::RefreshResolvedCardLayoutRuntimeState() const
{
	const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
	const uint32 ConfigHash = BuildResolvedLayoutConfigHash(Config);
	const bool bConfigChanged =
		!bHasResolvedCardLayoutConfigHash
		|| LastResolvedCardLayoutConfigHash != ConfigHash;
	if (!bConfigChanged)
	{
		return false;
	}

	bHasResolvedCardLayoutConfigHash = true;
	LastResolvedCardLayoutConfigHash = ConfigHash;
	InvalidateResolvedCardLayoutRuntimeState();
	return true;
}

void UWacomFirstPersonCardAnchorComponent::InvalidateResolvedCardLayoutRuntimeState() const
{
	bHasCachedOwnerConfig = false;
	ResetAnchorScreenSmoothing();
	if (CardLayerWidget)
	{
		CardLayerWidget->ClearSlotMotionState();
	}
}

void UWacomFirstPersonCardAnchorComponent::ResetAnchorScreenSmoothing() const
{
	const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
	bHasSmoothedAnchorWidgetPosition = false;
	bLastAnchorScreenSmoothed = false;
	LastAnchorScreenSmoothingDistancePixels = 0.0f;
	SmoothedAnchorWidgetPosition = FVector2D::ZeroVector;
	LastAnchorScreenSmoothingTargetWidgetPosition = FVector2D::ZeroVector;
	LastAnchorScreenSmoothingFrame = 0;
	SmoothedAnchorProjectionMode = Config.ProjectionMode;
	SmoothedAnchorViewportClampMode = Config.ViewportClampMode;
	SmoothedAnchorMode = CurrentMode;
}

void UWacomFirstPersonCardAnchorComponent::ApplyAnchorScreenSmoothing(
	FWacomFirstPersonCardProjectedPoint& AnchorPoint) const
{
	const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
	AnchorPoint.UnsmoothedAnchorWidgetPosition = AnchorPoint.WidgetPosition;
	AnchorPoint.SmoothedAnchorWidgetPosition = AnchorPoint.WidgetPosition;
	AnchorPoint.AnchorScreenSmoothingDistancePixels = 0.0f;
	AnchorPoint.bAnchorScreenSmoothed = false;
	bLastAnchorScreenSmoothed = false;
	LastAnchorScreenSmoothingDistancePixels = 0.0f;

	if (!AnchorPoint.bProjected
		|| !Config.bEnableAnchorScreenSmoothing
		|| Config.AnchorScreenSmoothingSpeed <= 0.0f)
	{
		SmoothedAnchorWidgetPosition = AnchorPoint.WidgetPosition;
		bHasSmoothedAnchorWidgetPosition = AnchorPoint.bProjected;
		SmoothedAnchorProjectionMode = Config.ProjectionMode;
		SmoothedAnchorViewportClampMode = Config.ViewportClampMode;
		SmoothedAnchorMode = CurrentMode;
		return;
	}

	const bool bModeChanged =
		SmoothedAnchorProjectionMode != Config.ProjectionMode
		|| SmoothedAnchorViewportClampMode != Config.ViewportClampMode
		|| SmoothedAnchorMode != CurrentMode;
	const bool bSameFrameTarget =
		LastAnchorScreenSmoothingFrame == GFrameCounter
		&& LastAnchorScreenSmoothingTargetWidgetPosition.Equals(AnchorPoint.WidgetPosition, KINDA_SMALL_NUMBER);
	const float TargetJumpDistance = bHasSmoothedAnchorWidgetPosition
		? FVector2D::Distance(LastAnchorScreenSmoothingTargetWidgetPosition, AnchorPoint.WidgetPosition)
		: 0.0f;
	const float PreviousDistance = bHasSmoothedAnchorWidgetPosition
		? FVector2D::Distance(SmoothedAnchorWidgetPosition, AnchorPoint.WidgetPosition)
		: 0.0f;
	const float ResetDistance = FMath::Max(0.0f, Config.AnchorScreenSmoothingResetDistancePixels);
	const bool bLargeJump = bHasSmoothedAnchorWidgetPosition
		&& ResetDistance > 0.0f
		&& TargetJumpDistance > ResetDistance;

	if (!bHasSmoothedAnchorWidgetPosition || bModeChanged || bLargeJump)
	{
		SmoothedAnchorWidgetPosition = AnchorPoint.WidgetPosition;
		bHasSmoothedAnchorWidgetPosition = true;
		SmoothedAnchorProjectionMode = Config.ProjectionMode;
		SmoothedAnchorViewportClampMode = Config.ViewportClampMode;
		SmoothedAnchorMode = CurrentMode;
		LastAnchorScreenSmoothingTargetWidgetPosition = AnchorPoint.WidgetPosition;
		LastAnchorScreenSmoothingFrame = GFrameCounter;
		return;
	}

	if (bSameFrameTarget)
	{
		AnchorPoint.WidgetPosition = SmoothedAnchorWidgetPosition;
		AnchorPoint.SmoothedAnchorWidgetPosition = SmoothedAnchorWidgetPosition;
		AnchorPoint.AnchorScreenSmoothingDistancePixels = PreviousDistance;
		AnchorPoint.bAnchorScreenSmoothed = PreviousDistance > KINDA_SMALL_NUMBER;
		bLastAnchorScreenSmoothed = AnchorPoint.bAnchorScreenSmoothed;
		LastAnchorScreenSmoothingDistancePixels = PreviousDistance;
		return;
	}

	const float DeltaTime = GetAnchorSmoothingDeltaTimeForAnchor();
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	SmoothedAnchorWidgetPosition = FMath::Vector2DInterpTo(
		SmoothedAnchorWidgetPosition,
		AnchorPoint.WidgetPosition,
		DeltaTime,
		Config.AnchorScreenSmoothingSpeed);
	const float SmoothedDistance = FVector2D::Distance(SmoothedAnchorWidgetPosition, AnchorPoint.WidgetPosition);
	AnchorPoint.WidgetPosition = SmoothedAnchorWidgetPosition;
	AnchorPoint.SmoothedAnchorWidgetPosition = SmoothedAnchorWidgetPosition;
	AnchorPoint.AnchorScreenSmoothingDistancePixels = SmoothedDistance;
	AnchorPoint.bAnchorScreenSmoothed = SmoothedDistance > KINDA_SMALL_NUMBER;
	bLastAnchorScreenSmoothed = AnchorPoint.bAnchorScreenSmoothed;
	LastAnchorScreenSmoothingDistancePixels = SmoothedDistance;
	SmoothedAnchorProjectionMode = Config.ProjectionMode;
	SmoothedAnchorViewportClampMode = Config.ViewportClampMode;
	SmoothedAnchorMode = CurrentMode;
	LastAnchorScreenSmoothingTargetWidgetPosition = AnchorPoint.UnsmoothedAnchorWidgetPosition;
	LastAnchorScreenSmoothingFrame = GFrameCounter;
}

FString UWacomFirstPersonCardAnchorComponent::ProjectionModeToString(EWacomFirstPersonCardProjectionMode Mode)
{
	switch (Mode)
	{
	case EWacomFirstPersonCardProjectionMode::LegacyWorldProjected:
		return TEXT("LookResponsiveProjected");
	case EWacomFirstPersonCardProjectionMode::BodyLocked:
	default:
		return TEXT("BodyLocked");
	}
}

FString UWacomFirstPersonCardAnchorComponent::ViewportClampModeToString(
	EWacomFirstPersonCardViewportClampMode Mode)
{
	switch (Mode)
	{
	case EWacomFirstPersonCardViewportClampMode::HardClampToViewport:
		return TEXT("HardClampToViewport");
	case EWacomFirstPersonCardViewportClampMode::AllowOffscreen:
		return TEXT("AllowOffscreen");
	case EWacomFirstPersonCardViewportClampMode::SoftClampToViewport:
	default:
		return TEXT("SoftClampToViewport");
	}
}

void UWacomFirstPersonCardAnchorComponent::UpdateCardLayer()
{
	if (!HasRuntimeCardLayerData())
	{
		RemoveCardLayer();
		return;
	}

	APlayerController* PC = GetOwnerPlayerController();
	if (!CanCreateCardLayerForAnchor(PC))
	{
		RemoveCardLayer();
		return;
	}

	if (!CardLayerOwner)
	{
		return;
	}

	RefreshResolvedCardLayoutRuntimeState();

	const bool bLayoutConfigChanged =
		LastResolvedCardLayoutConfigHash != CachedOwnerConfigHash;

	const bool bRuntimeFlagsChanged =
		!bHasCachedOwnerConfig
		|| CachedInteractionEnabled != bFirstPersonCardLayerInteractionEnabled
		|| CachedLogDiagnostics != bLogCardLayerMotionDiagnostics;

	const bool bCardViewClassChanged =
		!bHasCachedOwnerConfig
		|| CachedCardViewClass.Get() != FirstPersonCardViewClass.Get();

	if (bLayoutConfigChanged || bRuntimeFlagsChanged || bCardViewClassChanged)
	{
		const FWacomFirstPersonCardResolvedLayoutConfig ResolvedConfig = ResolveLayoutConfig(*this);

		CachedOwnerConfigHash          = BuildResolvedLayoutConfigHash(ResolvedConfig);
		CachedSlotMotionConfig         = BuildSlotMotionConfig(ResolvedConfig);
		CachedSlotVisualConfig         = BuildSlotVisualConfig(ResolvedConfig);
		CachedSlotFeedbackConfig       = BuildSlotFeedbackConfig(ResolvedConfig);
		CachedCardDragConfig           = BuildCardDragConfig(*this, ResolvedConfig);
		CachedPileTransferConfig       = ResolvedConfig.PileTransfer;
		CachedInteractionEnabled       = bFirstPersonCardLayerInteractionEnabled;
		CachedLogDiagnostics           = bLogCardLayerMotionDiagnostics;
		CachedCardViewClass            = FirstPersonCardViewClass.Get();
		bHasCachedOwnerConfig          = true;
	}

	FWacomFirstPersonCardLayerOwnerConfig OwnerConfig;
	OwnerConfig.LayerWidgetClass          = CardLayerWidgetClass;
	OwnerConfig.CardViewClass             = FirstPersonCardViewClass;
	OwnerConfig.ZOrder                   = CardLayerZOrder;
	OwnerConfig.ConfigHash               = CachedOwnerConfigHash;
	OwnerConfig.SlotMotionConfig          = CachedSlotMotionConfig;
	OwnerConfig.SlotVisualConfig          = CachedSlotVisualConfig;
	OwnerConfig.SlotFeedbackConfig        = CachedSlotFeedbackConfig;
	OwnerConfig.CardDragConfig            = CachedCardDragConfig;
	OwnerConfig.PileTransferConfig        = CachedPileTransferConfig;
	OwnerConfig.bLogSlotMotionDiagnostics  = CachedLogDiagnostics;
	OwnerConfig.bInteractionEnabled       = CachedInteractionEnabled;

	FWacomFirstPersonCardLayerOwnerUpdateInput UpdateInput;
	UpdateInput.PlayerController = PC;
	UpdateInput.Config = OwnerConfig;
	UpdateInput.Slots = BuildActiveCardLayerSlotViews();
	UpdateInput.PresentationAnchors = RuntimeState
		? RuntimeState->GetPresentationAnchorsForCurrentSource()
		: FWacomFirstPersonCardPresentationAnchorSet();
	UpdateInput.CreateLayerWidget = [this](
		APlayerController* PlayerController,
		TSubclassOf<UWacomFirstPersonCardLayerWidget> LayerClass)
	{
		return CreateCardLayerWidgetForAnchor(PlayerController, LayerClass);
	};
	UpdateInput.BindLayerWidget = [this](UWacomFirstPersonCardLayerWidget* LayerWidget)
	{
		if (CardLayerDelegateRouter)
		{
			CardLayerDelegateRouter->Bind(LayerWidget);
		}
	};
	UpdateInput.AddLayerWidgetToViewport = [this](UWacomFirstPersonCardLayerWidget* LayerWidget, int32 ZOrder)
	{
		AddCardLayerWidgetToViewportForAnchor(LayerWidget, ZOrder);
	};
	UpdateInput.ConsumeTransitionHints = [this]()
	{
		if (!RuntimeState)
		{
			return TArray<FWacomFirstPersonCardLayerTransitionHint>();
		}
		if (RuntimeState->CanConsumePresentationFrameHintsForCurrentSource())
		{
			return RuntimeState->ConsumePresentationFrameHintsForCurrentSource();
		}
		return RuntimeState->CanConsumeTransitionHintsForCurrentSource()
			? RuntimeState->ConsumeTransitionHintsForCurrentSource()
			: TArray<FWacomFirstPersonCardLayerTransitionHint>();
	};
	UpdateInput.bCanConsumeTransitionHints =
		RuntimeState
		&& (RuntimeState->CanConsumePresentationFrameHintsForCurrentSource()
			|| RuntimeState->CanConsumeTransitionHintsForCurrentSource());
	UpdateInput.ConsumeFeedbackHints = [this]()
	{
		if (!RuntimeState)
		{
			return TArray<FWacomFirstPersonCardLayerFeedbackHint>();
		}
		if (RuntimeState->CanConsumePresentationFrameFeedbackHintsForCurrentSource())
		{
			return RuntimeState->ConsumePresentationFrameFeedbackHintsForCurrentSource();
		}
		return RuntimeState->CanConsumeFeedbackHintsForCurrentSource()
			? RuntimeState->ConsumeFeedbackHintsForCurrentSource()
			: TArray<FWacomFirstPersonCardLayerFeedbackHint>();
	};
	UpdateInput.bCanConsumeFeedbackHints = RuntimeState
		&& (RuntimeState->CanConsumePresentationFrameFeedbackHintsForCurrentSource()
			|| RuntimeState->CanConsumeFeedbackHintsForCurrentSource());
	UpdateInput.ConsumePileTransferHints = [this]()
	{
		return RuntimeState && RuntimeState->CanConsumePileTransferHintsForCurrentSource()
			? RuntimeState->ConsumePileTransferHintsForCurrentSource()
			: TArray<FWacomFirstPersonCardPileTransferHint>();
	};
	UpdateInput.bCanConsumePileTransferHints = RuntimeState
		&& RuntimeState->CanConsumePileTransferHintsForCurrentSource();

	CardLayerOwner->Update(UpdateInput, CardLayerWidget);
}

void UWacomFirstPersonCardAnchorComponent::RemoveCardLayer()
{
	if (CardLayerOwner)
	{
		CardLayerOwner->Remove(
			CardLayerWidget,
			[this](UWacomFirstPersonCardLayerWidget* LayerWidget)
			{
				if (CardLayerDelegateRouter)
				{
					CardLayerDelegateRouter->Unbind(LayerWidget);
				}
			});
	}
	if (RuntimeState)
	{
		RuntimeState->ClearTransitionHints();
		RuntimeState->ClearPresentationFrameHints();
		RuntimeState->ClearFeedbackHints();
		RuntimeState->ClearPresentationFrameFeedbackHints();
		RuntimeState->ClearTransientInteraction();
	}
}

void UWacomFirstPersonCardAnchorComponent::ConfigureCardLayerDelegateRouter()
{
	if (!CardLayerDelegateRouter)
	{
		return;
	}

	FWacomFirstPersonCardLayerDelegateRouterCallbacks Callbacks;
	Callbacks.CardHovered = [this](
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView)
	{
		OnFirstPersonCardLayerCardHovered.Broadcast(CardInstanceId, SlotView);
	};
	Callbacks.CardUnhovered = [this](
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView)
	{
		OnFirstPersonCardLayerCardUnhovered.Broadcast(CardInstanceId, SlotView);
	};
	Callbacks.HoveredCardLayoutUpdated = [this](
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView)
	{
		OnFirstPersonCardLayerHoveredCardLayoutUpdated.Broadcast(CardInstanceId, SlotView);
	};
	Callbacks.CardTargetHovered = [this](
		const FWacomInteractionTargetHandle& CardTargetHandle,
		const FWacomFirstPersonCardLayerSlotView& SlotView)
	{
		OnFirstPersonCardLayerCardTargetHovered.Broadcast(CardTargetHandle, SlotView);
	};
	Callbacks.CardTargetUnhovered = [this](
		const FWacomInteractionTargetHandle& CardTargetHandle,
		const FWacomFirstPersonCardLayerSlotView& SlotView)
	{
		OnFirstPersonCardLayerCardTargetUnhovered.Broadcast(CardTargetHandle, SlotView);
	};
	Callbacks.HoveredCardTargetUpdated = [this](
		const FWacomInteractionTargetHandle& CardTargetHandle,
		const FWacomFirstPersonCardLayerSlotView& SlotView)
	{
		OnFirstPersonCardLayerHoveredCardTargetUpdated.Broadcast(CardTargetHandle, SlotView);
	};
	Callbacks.DragStarted = [this](const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView)
	{
		OnFirstPersonCardLayerDragStarted.Broadcast(CardInstanceId, DragView);
	};
	Callbacks.DragUpdated = [this](const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView)
	{
		OnFirstPersonCardLayerDragUpdated.Broadcast(CardInstanceId, DragView);
	};
	Callbacks.DragReleased = [this](const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView)
	{
		OnFirstPersonCardLayerDragReleased.Broadcast(CardInstanceId, DragView);
	};
	Callbacks.DragCancelled = [this](const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView)
	{
		OnFirstPersonCardLayerDragCancelled.Broadcast(CardInstanceId, DragView);
	};
	Callbacks.PointerMoved = [this](const FWacomFirstPersonCardPointerView& PointerView)
	{
		OnFirstPersonCardLayerPointerMoved.Broadcast(PointerView);
	};
	Callbacks.PointerLeft = [this]()
	{
		OnFirstPersonCardLayerPointerLeft.Broadcast();
	};
	Callbacks.EnterTransitionStarted = [this](
		const FWacomFirstPersonCardEnterTransitionStartedView& View)
	{
		OnFirstPersonCardLayerEnterTransitionStarted.Broadcast(View);
	};
	Callbacks.PileTransferProgress = [this](const FWacomFirstPersonCardPileTransferProgressView& Progress)
	{
		OnFirstPersonCardLayerPileTransferProgress.Broadcast(Progress);
	};
	Callbacks.GetHoveredCardInstanceId = [this]()
	{
		return RuntimeState ? RuntimeState->GetHoveredCardInstanceId() : FGuid();
	};
	Callbacks.SetHoveredCardInstanceId = [this](const FGuid& CardInstanceId)
	{
		if (RuntimeState)
		{
			RuntimeState->SetHoveredCardInstanceId(CardInstanceId);
		}
	};
	Callbacks.ClearHoveredCardInstanceId = [this]()
	{
		if (RuntimeState)
		{
			RuntimeState->ClearHoveredCardInstanceId();
		}
	};
	Callbacks.GetHoveredCardTargetHandle = [this]()
	{
		return RuntimeState
			? RuntimeState->GetHoveredCardTargetHandle()
			: FWacomInteractionTargetHandle();
	};
	Callbacks.SetHoveredCardTargetHandle = [this](const FWacomInteractionTargetHandle& CardTargetHandle)
	{
		if (RuntimeState)
		{
			RuntimeState->SetHoveredCardTargetHandle(CardTargetHandle);
		}
	};
	Callbacks.ClearHoveredCardTargetHandle = [this]()
	{
		if (RuntimeState)
		{
			RuntimeState->ClearHoveredCardTargetHandle();
		}
	};
	CardLayerDelegateRouter->SetCallbacks(MoveTemp(Callbacks));
}

FString UWacomFirstPersonCardAnchorComponent::AnchorModeToString(EWacomFirstPersonCardAnchorMode Mode)
{
	switch (Mode)
	{
	case EWacomFirstPersonCardAnchorMode::BattleCamera:
		return TEXT("BattleCamera");
	case EWacomFirstPersonCardAnchorMode::ViewStageBlend:
		return TEXT("ViewStageBlend");
	case EWacomFirstPersonCardAnchorMode::RunTunnel:
		return TEXT("RunTunnel");
	case EWacomFirstPersonCardAnchorMode::CameraFallback:
		return TEXT("CameraFallback");
	case EWacomFirstPersonCardAnchorMode::Invalid:
	default:
		return TEXT("Invalid");
	}
}

FWacomInteractionTargetHandle UWacomFirstPersonCardAnchorComponent::BuildCardTargetHandle() const
{
	return RuntimeState ? RuntimeState->GetHoveredCardTargetHandle() : FWacomInteractionTargetHandle();
}

void UWacomFirstPersonCardAnchorComponent::SetFirstPersonCardDragFeedbackTarget(
	const FWacomInteractionTargetHandle& TargetHandle,
	bool bValidTarget,
	EWacomFirstPersonCardDragTargetFeedbackState FeedbackState,
	const TOptional<FVector2D>& FeedbackTargetScreenPosition,
	const FString& ResolvedIntentDebugSummary,
	const TArray<FWacomFirstPersonCardTargetAffordance>& CardTargetAffordances)
{
	if (CardLayerWidget)
	{
		CardLayerWidget->SetCardDragFeedbackTarget(
			TargetHandle,
			bValidTarget,
			FeedbackState,
			FeedbackTargetScreenPosition,
			ResolvedIntentDebugSummary,
			CardTargetAffordances);
	}
}

void UWacomFirstPersonCardAnchorComponent::CancelFirstPersonCardDragGesture(bool bBroadcastCancel)
{
	if (CardLayerWidget)
	{
		CardLayerWidget->CancelCardDragGesture(bBroadcastCancel);
	}
}

bool UWacomFirstPersonCardAnchorComponent::TryStartFirstPersonCardDragGesture(const FGuid& CardInstanceId)
{
	return CardLayerWidget
		&& CardLayerWidget->TryStartCardDragGesture(CardInstanceId);
}

bool UWacomFirstPersonCardAnchorComponent::TryStartFirstPersonCardDragGesture(
	const FGuid& CardInstanceId,
	const TOptional<FVector2D>& InitialPointerWidgetPosition)
{
	return CardLayerWidget
		&& CardLayerWidget->TryStartCardDragGesture(CardInstanceId, InitialPointerWidgetPosition);
}

bool UWacomFirstPersonCardAnchorComponent::UpdateFirstPersonCardDragPointer(
	const FVector2D& WidgetPosition)
{
	return CardLayerWidget
		&& CardLayerWidget->UpdateActiveDragPointerFromWidgetPosition(WidgetPosition);
}

bool UWacomFirstPersonCardAnchorComponent::ReleaseFirstPersonCardDragGesture(
	const FVector2D& WidgetPosition)
{
	return CardLayerWidget
		&& CardLayerWidget->ReleaseActiveDragGestureFromWidgetPosition(WidgetPosition);
}

bool UWacomFirstPersonCardAnchorComponent::ReleaseFirstPersonCardDragGestureAtCurrentPointer()
{
	return CardLayerWidget
		&& CardLayerWidget->ReleaseActiveDragGestureAtCurrentPointer();
}

bool UWacomFirstPersonCardAnchorComponent::IsFirstPersonCardDragGestureActive() const
{
	return CardLayerWidget && CardLayerWidget->IsCardDragGestureActive();
}

bool UWacomFirstPersonCardAnchorComponent::IsFirstPersonCardKeyboardShortcutDragGestureActive() const
{
	return CardLayerWidget
		&& CardLayerWidget->IsKeyboardShortcutCardDragGestureActive();
}
