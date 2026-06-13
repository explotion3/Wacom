// Copyright Wacom. All Rights Reserved.

#include "Components/WacomFirstPersonCardAnchorComponent.h"

#include "Actors/WacomRunTunnelSegmentActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "Components/WacomRunTunnelMovementComponent.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Cards/CardDefinition.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomFirstPersonCardAnchorRuntimeState.h"
#include "UI/Card/WacomFirstPersonCardAnchorDebugWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerDelegateRouter.h"
#include "UI/Card/WacomFirstPersonCardLayerOwner.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/Card/WacomFirstPersonCardSlotLayoutBuilder.h"

namespace
{
	const FName NoOwnerReason(TEXT("NoOwner"));
	const FName NoPlayerControllerReason(TEXT("NoPlayerController"));
	const FName NoCameraManagerReason(TEXT("NoCameraManager"));
	const FName CameraFallbackReason(TEXT("CameraFallback"));

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
		Config.bKeepAuthoredCardBodyBottomInViewport = Anchor.bKeepAuthoredCardBodyBottomInViewport;
		Config.AuthoredCardBodyBottomViewportPaddingPixels = Anchor.AuthoredCardBodyBottomViewportPaddingPixels;
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
		Config.GainedCardEnterOffsetPixels = Anchor.GainedCardEnterOffsetPixels;
		Config.GainedCardEnterOriginMode = Anchor.GainedCardEnterOriginMode;
		Config.GainedCardEnterViewportAnchor = Anchor.GainedCardEnterViewportAnchor;
		Config.GainedCardEnterScaleMultiplier = Anchor.GainedCardEnterScaleMultiplier;
		Config.GainedCardEnterAngleOffsetDegrees = Anchor.GainedCardEnterAngleOffsetDegrees;
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
		Config.bEnableCardInteractionFeedback = Anchor.bEnableCardInteractionFeedback;
		Config.PlayableHoverFeedbackColor = Anchor.PlayableHoverFeedbackColor;
		Config.PlayableHoverFeedbackOpacity = Anchor.PlayableHoverFeedbackOpacity;
		Config.PressedFeedbackScale = Anchor.PressedFeedbackScale;
		Config.PressedFeedbackColor = Anchor.PressedFeedbackColor;
		Config.PressedFeedbackOpacity = Anchor.PressedFeedbackOpacity;
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
		Config.bAllowCameraLookDuringCardDrag = Anchor.bAllowCameraLookDuringCardDrag;
		Config.CardDragCameraLookScale = Anchor.CardDragCameraLookScale;
		Config.CardDragCameraLookInterpSpeedOverride = Anchor.CardDragCameraLookInterpSpeedOverride;
		Config.bAllowCameraLookDuringCardPointer = Anchor.bAllowCameraLookDuringCardPointer;
		Config.CardPointerCameraLookScale = Anchor.CardPointerCameraLookScale;
		Config.CardPointerCameraLookInterpSpeedOverride = Anchor.CardPointerCameraLookInterpSpeedOverride;
	}

	void BuildDragFeedbackConfigFromAnchor(
		const UWacomFirstPersonCardAnchorComponent& Anchor,
		FWacomFirstPersonCardResolvedLayoutConfig& Config)
	{
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
		BuildDragFeedbackConfigFromAnchor(Anchor, Config);
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
			? MakeProfile(
				Config.DragTargetFocusMotionSpeed,
				Config.DragTargetFocusOpacitySpeed,
				Config.DragTargetFocusMotionEasePower)
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
		MotionConfig.GainedEnterOffsetPixels = Config.GainedCardEnterOffsetPixels;
		MotionConfig.GainedEnterOriginMode = Config.GainedCardEnterOriginMode;
		MotionConfig.GainedEnterViewportAnchor = Config.GainedCardEnterViewportAnchor;
		MotionConfig.GainedEnterScaleMultiplier = Config.GainedCardEnterScaleMultiplier;
		MotionConfig.GainedEnterAngleOffsetDegrees = Config.GainedCardEnterAngleOffsetDegrees;
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
		DragConfig.NoTargetCardDragOutCommitDistancePixels = Anchor.NoTargetCardDragOutCommitDistancePixels;
		DragConfig.NoTargetCardDragOutDirection = Anchor.NoTargetCardDragOutDirection;
		DragConfig.CardInspectScreenPosition = Anchor.CardInspectScreenPosition;
		DragConfig.CardInspectScale = Anchor.CardInspectScale;
		DragConfig.bShowDetailDuringCardInspect = Anchor.bShowDetailDuringCardInspect;
		DragConfig.bEnableAimArrow = Anchor.bEnableAimArrow;
		DragConfig.bLogCardDragDiagnostics = Anchor.bLogCardDragDiagnostics;
		DragConfig.bAllowCameraLookDuringCardDrag = Anchor.bAllowCameraLookDuringCardDrag;
		DragConfig.CardDragCameraLookScale = Anchor.CardDragCameraLookScale;
		DragConfig.CardDragCameraLookInterpSpeedOverride = Anchor.CardDragCameraLookInterpSpeedOverride;
		DragConfig.bAllowCameraLookDuringCardPointer = Config.bAllowCameraLookDuringCardPointer;
		DragConfig.CardPointerCameraLookScale = Config.CardPointerCameraLookScale;
		DragConfig.CardPointerCameraLookInterpSpeedOverride = Config.CardPointerCameraLookInterpSpeedOverride;
		DragConfig.bEnableDragTargetFeedback = Anchor.bEnableDragTargetFeedback;
		DragConfig.DragValidTargetColor = Anchor.DragValidTargetColor;
		DragConfig.DragInvalidTargetColor = Anchor.DragInvalidTargetColor;
		DragConfig.DragCardProbeTargetColor = Anchor.DragCardProbeTargetColor;
		DragConfig.DragTargetFeedbackOpacity = Anchor.DragTargetFeedbackOpacity;
		DragConfig.bSnapAimArrowToValidWorldTarget = Anchor.bSnapAimArrowToValidWorldTarget;
		DragConfig.DragAimArrowSnapBlend = Anchor.DragAimArrowSnapBlend;
		DragConfig.DragCommitReadyScale = Anchor.DragCommitReadyScale;
		DragConfig.DragCardTargetProbeScale = Anchor.DragCardTargetProbeScale;
		DragConfig.DragCardTargetFocusLiftPixels = Config.DragCardTargetFocusLiftPixels;
		DragConfig.DragCardTargetFocusScale = Config.DragCardTargetFocusScale;
		DragConfig.DragCardTargetFocusZOrderBoost = Config.DragCardTargetFocusZOrderBoost;
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
		AddBool(Config.bKeepAuthoredCardBodyBottomInViewport);
		AddFloat(Config.AuthoredCardBodyBottomViewportPaddingPixels);
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
		AddVector(Config.GainedCardEnterOffsetPixels);
		AddInt(static_cast<int32>(Config.GainedCardEnterOriginMode));
		AddVector(Config.GainedCardEnterViewportAnchor);
		AddFloat(Config.GainedCardEnterScaleMultiplier);
		AddFloat(Config.GainedCardEnterAngleOffsetDegrees);
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
		AddFloat(Config.DragCardTargetFocusLiftPixels);
		AddFloat(Config.DragCardTargetFocusScale);
		AddInt(Config.DragCardTargetFocusZOrderBoost);
		AddBool(Config.bEnableCardInteractionFeedback);
		AddColor(Config.PlayableHoverFeedbackColor);
		AddFloat(Config.PlayableHoverFeedbackOpacity);
		AddFloat(Config.PressedFeedbackScale);
		AddColor(Config.PressedFeedbackColor);
		AddFloat(Config.PressedFeedbackOpacity);
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
		AddBool(Config.bAllowCameraLookDuringCardDrag);
		AddFloat(Config.CardDragCameraLookScale);
		AddFloat(Config.CardDragCameraLookInterpSpeedOverride);
		AddBool(Config.bAllowCameraLookDuringCardPointer);
		AddFloat(Config.CardPointerCameraLookScale);
		AddFloat(Config.CardPointerCameraLookInterpSpeedOverride);
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
	ConfigureTickPrerequisites();
	SetComponentTickEnabled(true);
}

void UWacomFirstPersonCardAnchorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetBattleHandInteractionEnabled(false);
	ResetAnchorScreenSmoothing();
	RemoveCardLayer();
	RemoveDebugWidget();
	Super::EndPlay(EndPlayReason);
}

void UWacomFirstPersonCardAnchorComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshAnchor(DeltaTime);
	UpdateDebugWidget();
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

	if (!bHasInitializedAnchor || FollowInterpSpeed <= 0.0f || DeltaTime <= 0.0f)
	{
		CurrentAnchorTransform = TargetAnchorTransform;
	}
	else
	{
		const FVector SmoothedLocation = FMath::VInterpTo(
			CurrentAnchorTransform.GetLocation(),
			TargetAnchorTransform.GetLocation(),
			DeltaTime,
			FollowInterpSpeed);
		const FRotator SmoothedRotation = FMath::RInterpTo(
			CurrentAnchorTransform.Rotator(),
			TargetAnchorTransform.Rotator(),
			DeltaTime,
			FollowInterpSpeed);
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

FWacomFirstPersonCardAnchorDebugView UWacomFirstPersonCardAnchorComponent::GetFirstPersonCardAnchorDebugView(
	int32 NumDebugCards) const
{
	RefreshResolvedCardLayoutRuntimeState();
	const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
	FWacomFirstPersonCardAnchorDebugView View;
	View.bHasValidAnchor = bHasValidAnchor;
	View.Mode = CurrentMode;
	View.AnchorTransform = CurrentAnchorTransform;
	View.ProjectionMode = Config.ProjectionMode;
	View.ViewportClampMode = Config.ViewportClampMode;
	View.LookOffsetUsed = CurrentLookOffsetUsed;
	View.RawCursorLookOffset = CurrentRawCursorLookOffset;
	View.AppliedAnchorLookOffset = CurrentLookOffsetUsed;
	View.bLookResponsiveProjection = Config.ProjectionMode == EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	View.LookInfluenceYaw = Config.LookInfluenceYaw;
	View.LookInfluencePitch = Config.LookInfluencePitch;
	View.LastFallbackReason = LastFallbackReason;
	View.bBodyLockedLayout = Config.ProjectionMode == EWacomFirstPersonCardProjectionMode::BodyLocked;
	View.bCurrentCameraProjection = true;
	View.bLookOffsetAppliedToLayout = bCurrentLookOffsetAppliedToLayout;
	View.bAnchorScreenSmoothed = bLastAnchorScreenSmoothed;
	if (CardLayerWidget)
	{
		View.LayerMotionDebugView = CardLayerWidget->GetSlotMotionDebugView();
	}

	if (!bHasValidAnchor)
	{
		return View;
	}

	const int32 ClampedCount = FMath::Clamp(NumDebugCards, 0, 32);
	View.ProjectedPoints.Reserve(ClampedCount);
	TArray<FWacomFirstPersonCardLayerEntry> DebugEntries;
	DebugEntries.SetNum(ClampedCount);
	const TArray<FWacomFirstPersonCardLayerSlotView> DebugSlots = BuildCardSlotViewsFromEntries(DebugEntries);
	for (const FWacomFirstPersonCardLayerSlotView& Slot : DebugSlots)
	{
		FWacomFirstPersonCardProjectedPoint Point;
		Point.Index = Slot.Index;
		Point.WorldLocation = CurrentAnchorTransform.GetLocation();
		Point.RawScreenPosition = Slot.RawScreenPosition;
		Point.WidgetPosition = Slot.WidgetPosition;
		Point.UnclampedWidgetPosition = Slot.UnclampedWidgetPosition;
		Point.SnappedWidgetPosition = Slot.SnappedWidgetPosition;
		Point.ScreenPosition = Slot.ScreenPosition;
		Point.ProjectionMode = Slot.ProjectionMode;
		Point.ViewportClampMode = Slot.ViewportClampMode;
		Point.AnchorWidgetPosition = Slot.AnchorWidgetPosition;
		Point.UnsmoothedAnchorWidgetPosition = Slot.UnsmoothedAnchorWidgetPosition;
		Point.SmoothedAnchorWidgetPosition = Slot.SmoothedAnchorWidgetPosition;
		Point.AuthoredLayoutOffset = Slot.AuthoredLayoutOffset;
		Point.NormalizedHandOffset = Slot.NormalizedHandOffset;
		Point.ViewportScale = Slot.ViewportScale;
		Point.OffscreenDistancePixels = Slot.OffscreenDistancePixels;
		Point.AnchorScreenSmoothingDistancePixels = Slot.AnchorScreenSmoothingDistancePixels;
		Point.bProjected = Slot.bProjected;
		Point.bClamped = Slot.bClamped;
		Point.bOutsideViewport = Slot.bOutsideViewport;
		Point.bPixelSnapped = Slot.bPixelSnapped;
		Point.bAnchorScreenSmoothed = Slot.bAnchorScreenSmoothed;
		Point.bBodyLockedLayout = Slot.bBodyLockedLayout;
		Point.bCurrentCameraProjection = Slot.bCurrentCameraProjection;
		Point.bLookOffsetAppliedToLayout = Slot.bLookOffsetAppliedToLayout;
		View.ProjectedPoints.Add(Point);
	}
	View.bAnchorScreenSmoothed = bLastAnchorScreenSmoothed;
	return View;
}

TArray<FWacomFirstPersonCardLayerSlotView> UWacomFirstPersonCardAnchorComponent::BuildPreviewCardSlotViews() const
{
	return BuildCardSlotViewsFromEntries(BuildPreviewCardLayerEntries());
}

void UWacomFirstPersonCardAnchorComponent::SetRuntimeCardLayerEntries(
	FName SourceId,
	const TArray<FWacomFirstPersonCardLayerEntry>& Entries)
{
	const bool bRuntimeSourceChanged = RuntimeState && RuntimeState->SetEntries(SourceId, Entries);
	if (bRuntimeSourceChanged)
	{
		RefreshResolvedCardLayoutRuntimeState();
	}
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

void UWacomFirstPersonCardAnchorComponent::SetRuntimeCardLayerData(
	FName SourceId,
	const TArray<FWacomCardViewData>& Cards)
{
	SetRuntimeCardLayerEntries(SourceId, BuildCardLayerEntriesFromData(Cards));
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

TArray<FWacomFirstPersonCardLayerEntry> UWacomFirstPersonCardAnchorComponent::BuildPreviewCardLayerEntries() const
{
	TArray<FWacomCardViewData> CardData;
	const int32 DesiredCount = PreviewCardDefinitions.Num() > 0
		? PreviewCardDefinitions.Num()
		: PreviewCardCountFallback;
	const int32 ClampedCount = FMath::Clamp(DesiredCount, 0, 32);
	CardData.Reserve(ClampedCount);
	for (int32 Index = 0; Index < ClampedCount; ++Index)
	{
		CardData.Add(BuildPreviewCardViewData(Index));
	}
	return BuildCardLayerEntriesFromData(CardData);
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

	FVector2D ViewportSize = FVector2D::ZeroVector;
	GetViewportSizeForAnchor(ViewportSize);

	FWacomFirstPersonCardSlotLayoutBuildInput BuildInput;
	BuildInput.CardEntries = &CardEntries;
	BuildInput.Config = &Config;
	BuildInput.AnchorPoint = AnchorPoint;
	BuildInput.WidgetViewportSize = ViewportSize;
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
		: BuildPreviewCardSlotViews();
}

void UWacomFirstPersonCardAnchorComponent::SetBattleHandInteractionEnabled(bool bEnabled)
{
	if (bEnableBattleHandInteraction == bEnabled)
	{
		return;
	}

	bEnableBattleHandInteraction = bEnabled;
	if (!bEnableBattleHandInteraction)
	{
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
	if (CardLayerWidget)
	{
		CardLayerWidget->SetCardLayerInteractionEnabled(bEnableBattleHandInteraction);
	}
}

#if WITH_AUTOMATION_TESTS
FWacomFirstPersonCardAnchorAutomationTestView UWacomFirstPersonCardAnchorComponent::GetAutomationTestViewForTest() const
{
	RefreshResolvedCardLayoutRuntimeState();
	FWacomFirstPersonCardAnchorAutomationTestView View;
	View.CardLayerWidget = CardLayerWidget;
	View.CardLayerConfigApplyCount = CardLayerOwner ? CardLayerOwner->GetConfigApplyCountForTest() : 0;
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

FString UWacomFirstPersonCardAnchorComponent::GetDebugSummary() const
{
	RefreshResolvedCardLayoutRuntimeState();
	const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
	const FString LayerMotionSummary = CardLayerWidget
		? CardLayerWidget->GetSlotMotionDebugSummary()
		: TEXT("SlotMotion Inactive");
	const FString DragTargetSummary = CardLayerWidget
		? CardLayerWidget->GetDragTargetDebugSummary()
		: TEXT("DragTarget Inactive");
	return FString::Printf(
		TEXT("FirstPersonCardAnchor Mode=%s ProjectionMode=%s ViewportClampMode=%s BodyLockedLayout=%s CurrentCameraProjection=true LookResponsiveProjection=%s LookUsedForLayout=%s LookInfluenceYaw=%.3f LookInfluencePitch=%.3f Valid=%s Anchor=%s RawCursorLookOffset=%s AppliedAnchorLookOffset=%s LookOffset=%s Fallback=%s PixelSnap=%s SnapGrid=%.2f AngleClamp=%s MaxAngle=%.2f ViewportScale=%.2f SoftAllowance=%.2f SoftBlend=%.2f AnchorScreenSmoothing=%s AnchorScreenSmoothingSpeed=%.2f AnchorScreenSmoothingReset=%.2f AnchorScreenSmoothed=%s AnchorScreenSmoothingDistance=%.2f DragCommit=%s HoldDelay=%.2f DragThreshold=%.2f DragCameraLook=%s DragCameraLookScale=%.2f DragCameraLookInterpOverride=%.2f PointerCameraLook=%s PointerCameraLookScale=%.2f PointerCameraLookInterpOverride=%.2f DragTargetFeedback=%s DragAimSnap=%s DragAimSnapBlend=%.2f %s %s"),
		*AnchorModeToString(CurrentMode),
		*ProjectionModeToString(Config.ProjectionMode),
		*ViewportClampModeToString(Config.ViewportClampMode),
		Config.ProjectionMode == EWacomFirstPersonCardProjectionMode::BodyLocked ? TEXT("true") : TEXT("false"),
		Config.ProjectionMode == EWacomFirstPersonCardProjectionMode::LegacyWorldProjected ? TEXT("true") : TEXT("false"),
		bCurrentLookOffsetAppliedToLayout ? TEXT("true") : TEXT("false"),
		Config.LookInfluenceYaw,
		Config.LookInfluencePitch,
		bHasValidAnchor ? TEXT("true") : TEXT("false"),
		*CurrentAnchorTransform.ToHumanReadableString(),
		*CurrentRawCursorLookOffset.ToString(),
		*CurrentLookOffsetUsed.ToString(),
		*CurrentLookOffsetUsed.ToString(),
		*LastFallbackReason.ToString(),
		Config.bEnableCardLayerPixelSnapping ? TEXT("true") : TEXT("false"),
		Config.CardLayerPixelSnapGrid,
		Config.bClampCardLayerRenderAngle ? TEXT("true") : TEXT("false"),
		Config.MaxCardLayerRenderAngleDegrees,
		GetViewportScaleForAnchor(),
		Config.SoftClampOffscreenAllowancePixels,
		Config.SoftClampBlendRangePixels,
		Config.bEnableAnchorScreenSmoothing ? TEXT("true") : TEXT("false"),
		Config.AnchorScreenSmoothingSpeed,
		Config.AnchorScreenSmoothingResetDistancePixels,
		bLastAnchorScreenSmoothed ? TEXT("true") : TEXT("false"),
		LastAnchorScreenSmoothingDistancePixels,
		bEnableFirstPersonCardDragCommit ? TEXT("true") : TEXT("false"),
		CardInspectHoldDelaySeconds,
		CardDragStartThresholdPixels,
		bAllowCameraLookDuringCardDrag ? TEXT("true") : TEXT("false"),
		CardDragCameraLookScale,
		CardDragCameraLookInterpSpeedOverride,
		Config.bAllowCameraLookDuringCardPointer ? TEXT("true") : TEXT("false"),
		Config.CardPointerCameraLookScale,
		Config.CardPointerCameraLookInterpSpeedOverride,
		bEnableDragTargetFeedback ? TEXT("true") : TEXT("false"),
		bSnapAimArrowToValidWorldTarget ? TEXT("true") : TEXT("false"),
		DragAimArrowSnapBlend,
		*LayerMotionSummary,
		*DragTargetSummary);
}

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

			FTransform CameraTransform = FTransform::Identity;
			if (!ResolveCameraTransformForAnchor(CameraTransform))
			{
				OutMode = EWacomFirstPersonCardAnchorMode::Invalid;
				OutFallbackReason = NoCameraManagerReason;
				return false;
			}

			OutBaseTransform = FTransform(
				BattleCamera->GetBaseBattleRotation(),
				CameraTransform.GetLocation(),
				FVector::OneVector);
			OutMode = EWacomFirstPersonCardAnchorMode::BattleCamera;
			OutFallbackReason = NAME_None;
			return true;
		}
	}

	if (const UWacomRunTunnelMovementComponent* RunTunnel = Character->GetRunTunnelMovementComponent())
	{
		if (RunTunnel->IsRunTunnelActive())
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

FWacomCardViewData UWacomFirstPersonCardAnchorComponent::BuildPreviewCardViewData(int32 CardIndex) const
{
	if (PreviewCardDefinitions.IsValidIndex(CardIndex))
	{
		const TSoftObjectPtr<UCardDefinition>& CardPtr = PreviewCardDefinitions[CardIndex];
		if (const UCardDefinition* Card = CardPtr.LoadSynchronous())
		{
			return UWacomCardPresentationBuilder::BuildCardViewData(Card);
		}
	}

	FWacomCardViewData Data;
	Data.Name = FText::Format(
		NSLOCTEXT("Wacom.FirstPersonCardLayer", "DevelopmentPreviewPlaceholderName", "Anchor Card {0}"),
		FText::AsNumber(CardIndex + 1));
	Data.TypeText = NSLOCTEXT("Wacom.FirstPersonCardLayer", "DevelopmentPreviewPlaceholderType", "Development Preview");
	Data.Description = NSLOCTEXT(
		"Wacom.FirstPersonCardLayer",
		"DevelopmentPreviewPlaceholderDescription",
		"HUD card view driven by the first-person anchor.");
	Data.Cost = CardIndex;
	Data.bShowCost = true;
	Data.Value = CardIndex + 1;
	Data.bShowValue = true;
	return Data;
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
	}
}

void UWacomFirstPersonCardAnchorComponent::RefreshResolvedCardLayoutRuntimeState() const
{
	const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
	const uint32 ConfigHash = BuildResolvedLayoutConfigHash(Config);
	const bool bConfigChanged =
		!bHasResolvedCardLayoutConfigHash
		|| LastResolvedCardLayoutConfigHash != ConfigHash;
	if (!bConfigChanged)
	{
		return;
	}

	bHasResolvedCardLayoutConfigHash = true;
	LastResolvedCardLayoutConfigHash = ConfigHash;
	InvalidateResolvedCardLayoutRuntimeState();
}

void UWacomFirstPersonCardAnchorComponent::InvalidateResolvedCardLayoutRuntimeState() const
{
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

void UWacomFirstPersonCardAnchorComponent::UpdateDebugWidget()
{
	if (!bDrawDebugProjection)
	{
		RemoveDebugWidget();
		return;
	}

	APlayerController* PC = GetOwnerPlayerController();
	if (!PC || !PC->IsLocalController())
	{
		RemoveDebugWidget();
		return;
	}

	if (!DebugWidget)
	{
		DebugWidget = CreateWidget<UWacomFirstPersonCardAnchorDebugWidget>(
			PC,
			UWacomFirstPersonCardAnchorDebugWidget::StaticClass());
		if (DebugWidget)
		{
			DebugWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			DebugWidget->AddToViewport(DebugWidgetZOrder);
		}
	}

	if (DebugWidget)
	{
		DebugWidget->SetDebugView(GetFirstPersonCardAnchorDebugView(5));
	}
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
	if (!bDrawPreviewCardLayer && !HasRuntimeCardLayerData())
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
	const FWacomFirstPersonCardResolvedLayoutConfig ResolvedConfig = ResolveLayoutConfig(*this);

	FWacomFirstPersonCardLayerOwnerConfig OwnerConfig;
	OwnerConfig.LayerWidgetClass = CardLayerWidgetClass;
	OwnerConfig.CardViewClass = FirstPersonCardViewClass;
	OwnerConfig.ZOrder = CardLayerZOrder;
	OwnerConfig.ConfigHash = BuildResolvedLayoutConfigHash(ResolvedConfig);
	OwnerConfig.SlotMotionConfig = BuildSlotMotionConfig(ResolvedConfig);
	OwnerConfig.SlotVisualConfig = BuildSlotVisualConfig(ResolvedConfig);
	OwnerConfig.SlotFeedbackConfig = BuildSlotFeedbackConfig(ResolvedConfig);
	OwnerConfig.CardDragConfig = BuildCardDragConfig(*this, ResolvedConfig);
	OwnerConfig.bLogSlotMotionDiagnostics = bLogCardLayerMotionDiagnostics;
	OwnerConfig.bInteractionEnabled = bEnableBattleHandInteraction;

	FWacomFirstPersonCardLayerOwnerUpdateInput UpdateInput;
	UpdateInput.PlayerController = PC;
	UpdateInput.Config = OwnerConfig;
	UpdateInput.Slots = BuildActiveCardLayerSlotViews();
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
		return RuntimeState && RuntimeState->HasTransitionHintsForCurrentSource()
			? RuntimeState->ConsumeTransitionHintsForCurrentSource()
			: TArray<FWacomFirstPersonCardLayerTransitionHint>();
	};

	CardLayerOwner->Update(UpdateInput, CardLayerWidget);
}

void UWacomFirstPersonCardAnchorComponent::RemoveDebugWidget()
{
	if (DebugWidget)
	{
		DebugWidget->RemoveFromParent();
		DebugWidget = nullptr;
	}
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
