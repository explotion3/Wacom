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
#include "UI/Card/WacomFirstPersonCardAnchorDebugWidget.h"
#include "UI/Card/WacomFirstPersonCardLayoutPreset.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"

namespace
{
	const FName NoOwnerReason(TEXT("NoOwner"));
	const FName NoPlayerControllerReason(TEXT("NoPlayerController"));
	const FName NoCameraManagerReason(TEXT("NoCameraManager"));
	const FName CameraFallbackReason(TEXT("CameraFallback"));

	struct FWacomFirstPersonCardResolvedLayoutConfig
	{
		EWacomFirstPersonCardProjectionMode ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;
		EWacomFirstPersonCardLayoutMode CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;
		EWacomFirstPersonCardViewportClampMode ViewportClampMode = EWacomFirstPersonCardViewportClampMode::SoftClampToViewport;
		float FanYawDegrees = 3.0f;
		float AuthoredCardSpacingPixels = 120.0f;
		float AuthoredMaxHandWidthPixels = 720.0f;
		FVector2D AuthoredHandScreenOffset = FVector2D::ZeroVector;
		float AuthoredCenterLiftPixels = 0.0f;
		float AuthoredDropCurveExponent = 2.0f;
		float AuthoredFanCurveExponent = 1.0f;
		bool bAuthoredCenterCardsDrawOnTop = true;
		float ProjectionPadding = 24.0f;
		float SoftClampOffscreenAllowancePixels = 260.0f;
		float SoftClampBlendRangePixels = 240.0f;
		bool bEnableCardLayerPixelSnapping = true;
		float CardLayerPixelSnapGrid = 1.0f;
		bool bClampCardLayerRenderAngle = true;
		float MaxCardLayerRenderAngleDegrees = 4.0f;
		float StaticCardRenderScale = 0.55f;
		float StaticCardEdgeDropPixels = 72.0f;
		bool bEnableAnchorScreenSmoothing = true;
		float AnchorScreenSmoothingSpeed = 18.0f;
		float AnchorScreenSmoothingResetDistancePixels = 320.0f;
		bool bEnableCardSlotMotion = true;
		float CardSlotMotionSpeed = 26.0f;
		float CardSlotOpacitySpeed = 18.0f;
		FVector2D CardSlotEnterOffsetPixels = FVector2D(0.0f, 48.0f);
		float CardSlotEnterOpacity = 0.0f;
		FVector2D CardSlotExitOffsetPixels = FVector2D(0.0f, 36.0f);
		float CardSlotExitDuration = 0.16f;
		float CardSlotMotionResetDistancePixels = 420.0f;
		bool bEnableEventAwareCardTransitions = true;
		bool bEnableReadableTransitionOrigins = true;
		FVector2D DrawnCardEnterOffsetPixels = FVector2D(0.0f, 96.0f);
		EWacomFirstPersonCardTransitionOriginMode DrawnCardEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;
		FVector2D DrawnCardEnterViewportAnchor = FVector2D(0.5f, 1.0f);
		float DrawnCardEnterScaleMultiplier = 0.96f;
		float DrawnCardEnterAngleOffsetDegrees = 0.0f;
		FVector2D GainedCardEnterOffsetPixels = FVector2D(0.0f, -120.0f);
		EWacomFirstPersonCardTransitionOriginMode GainedCardEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;
		FVector2D GainedCardEnterViewportAnchor = FVector2D(0.5f, 0.0f);
		float GainedCardEnterScaleMultiplier = 0.96f;
		float GainedCardEnterAngleOffsetDegrees = 0.0f;
		FVector2D PlayedCardExitOffsetPixels = FVector2D(0.0f, -120.0f);
		EWacomFirstPersonCardTransitionOriginMode PlayedCardExitOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
		FVector2D PlayedCardExitViewportAnchor = FVector2D(0.5f, 0.0f);
		float PlayedCardExitScaleMultiplier = 0.96f;
		float PlayedCardExitAngleOffsetDegrees = 0.0f;
		FVector2D DiscardedCardExitOffsetPixels = FVector2D(0.0f, 120.0f);
		EWacomFirstPersonCardTransitionOriginMode DiscardedCardExitOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
		FVector2D DiscardedCardExitViewportAnchor = FVector2D(0.5f, 1.0f);
		float DiscardedCardExitScaleMultiplier = 0.96f;
		float DiscardedCardExitAngleOffsetDegrees = 0.0f;
		float PendingTargetingLiftPixels = 36.0f;
		float PendingTargetingScale = 1.08f;
		int32 PendingTargetingZOrderBoost = 1200;
		bool bPendingTargetingStraightenAngle = true;
		float PendingTargetingAngleBlend = 0.75f;
		bool bEnableTargetSelectHandDeemphasis = true;
		float TargetSelectNonPendingOpacityMultiplier = 0.88f;
		float HandAnchorScale = 0.96f;
		float DisabledRenderOpacity = 0.78f;
		float HoverLiftPixels = 28.0f;
		float HoverScale = 1.06f;
		int32 HoverZOrderBoost = 500;
		bool bEnableCardInteractionFeedback = true;
		FLinearColor PlayableHoverFeedbackColor = FLinearColor(1.0f, 0.92f, 0.45f, 1.0f);
		float PlayableHoverFeedbackOpacity = 0.06f;
		float PressedFeedbackScale = 0.985f;
		FLinearColor PressedFeedbackColor = FLinearColor::White;
		float PressedFeedbackOpacity = 0.10f;
		float ConfirmFeedbackDuration = 0.08f;
		float ConfirmFeedbackOpacity = 0.12f;
		float DenyFeedbackDuration = 0.18f;
		float DenyFeedbackShakePixels = 8.0f;
		FLinearColor DenyFeedbackColor = FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);
		float DenyFeedbackOpacity = 0.18f;
		bool bEnablePlayCommitFeedback = true;
		float PlayCommitFeedbackDuration = 0.12f;
		float PlayCommitFeedbackOpacity = 0.16f;
		FLinearColor PlayCommitFeedbackColor = FLinearColor(0.75f, 1.0f, 0.55f, 1.0f);
		float PlayCommitFeedbackScale = 1.015f;
		bool bUsingPreset = false;
		bool bPresetFallback = true;
		FString PresetName = TEXT("None");
	};

	FWacomFirstPersonCardResolvedLayoutConfig BuildResolvedLayoutConfigFromComponent(
		const UWacomFirstPersonCardAnchorComponent& Anchor)
	{
		FWacomFirstPersonCardResolvedLayoutConfig Config;
		Config.ProjectionMode = Anchor.ProjectionMode;
		Config.CardLayoutMode = Anchor.CardLayoutMode;
		Config.ViewportClampMode = Anchor.ViewportClampMode;
		Config.FanYawDegrees = Anchor.FanYawDegrees;
		Config.AuthoredCardSpacingPixels = Anchor.AuthoredCardSpacingPixels;
		Config.AuthoredMaxHandWidthPixels = Anchor.AuthoredMaxHandWidthPixels;
		Config.AuthoredHandScreenOffset = Anchor.AuthoredHandScreenOffset;
		Config.AuthoredCenterLiftPixels = Anchor.AuthoredCenterLiftPixels;
		Config.AuthoredDropCurveExponent = Anchor.AuthoredDropCurveExponent;
		Config.AuthoredFanCurveExponent = Anchor.AuthoredFanCurveExponent;
		Config.bAuthoredCenterCardsDrawOnTop = Anchor.bAuthoredCenterCardsDrawOnTop;
		Config.ProjectionPadding = Anchor.ProjectionPadding;
		Config.SoftClampOffscreenAllowancePixels = Anchor.SoftClampOffscreenAllowancePixels;
		Config.SoftClampBlendRangePixels = Anchor.SoftClampBlendRangePixels;
		Config.bEnableCardLayerPixelSnapping = Anchor.bEnableCardLayerPixelSnapping;
		Config.CardLayerPixelSnapGrid = Anchor.CardLayerPixelSnapGrid;
		Config.bClampCardLayerRenderAngle = Anchor.bClampCardLayerRenderAngle;
		Config.MaxCardLayerRenderAngleDegrees = Anchor.MaxCardLayerRenderAngleDegrees;
		Config.StaticCardRenderScale = Anchor.StaticCardRenderScale;
		Config.StaticCardEdgeDropPixels = Anchor.StaticCardEdgeDropPixels;
		Config.bEnableAnchorScreenSmoothing = Anchor.bEnableAnchorScreenSmoothing;
		Config.AnchorScreenSmoothingSpeed = Anchor.AnchorScreenSmoothingSpeed;
		Config.AnchorScreenSmoothingResetDistancePixels = Anchor.AnchorScreenSmoothingResetDistancePixels;
		Config.bEnableCardSlotMotion = Anchor.bEnableCardSlotMotion;
		Config.CardSlotMotionSpeed = Anchor.CardSlotMotionSpeed;
		Config.CardSlotOpacitySpeed = Anchor.CardSlotOpacitySpeed;
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
		Config.PendingTargetingLiftPixels = Anchor.PendingTargetingLiftPixels;
		Config.PendingTargetingScale = Anchor.PendingTargetingScale;
		Config.PendingTargetingZOrderBoost = Anchor.PendingTargetingZOrderBoost;
		Config.bPendingTargetingStraightenAngle = Anchor.bPendingTargetingStraightenAngle;
		Config.PendingTargetingAngleBlend = Anchor.PendingTargetingAngleBlend;
		Config.bEnableTargetSelectHandDeemphasis = Anchor.bEnableTargetSelectHandDeemphasis;
		Config.TargetSelectNonPendingOpacityMultiplier = Anchor.TargetSelectNonPendingOpacityMultiplier;
		Config.HandAnchorScale = Anchor.HandAnchorScale;
		Config.DisabledRenderOpacity = Anchor.DisabledRenderOpacity;
		Config.HoverLiftPixels = Anchor.HoverLiftPixels;
		Config.HoverScale = Anchor.HoverScale;
		Config.HoverZOrderBoost = Anchor.HoverZOrderBoost;
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
		Config.bEnablePlayCommitFeedback = Anchor.bEnablePlayCommitFeedback;
		Config.PlayCommitFeedbackDuration = Anchor.PlayCommitFeedbackDuration;
		Config.PlayCommitFeedbackOpacity = Anchor.PlayCommitFeedbackOpacity;
		Config.PlayCommitFeedbackColor = Anchor.PlayCommitFeedbackColor;
		Config.PlayCommitFeedbackScale = Anchor.PlayCommitFeedbackScale;
		return Config;
	}

	void ApplyPresetToResolvedLayoutConfig(
		const UWacomFirstPersonCardLayoutPreset& Preset,
		FWacomFirstPersonCardResolvedLayoutConfig& Config);

	FWacomFirstPersonCardResolvedLayoutConfig ResolveLayoutConfig(
		const UWacomFirstPersonCardAnchorComponent& Anchor)
	{
		FWacomFirstPersonCardResolvedLayoutConfig Config = BuildResolvedLayoutConfigFromComponent(Anchor);
		Config.bUsingPreset = Anchor.bUseFirstPersonCardLayoutPreset && Anchor.FirstPersonCardLayoutPreset != nullptr;
		Config.bPresetFallback = !Config.bUsingPreset;
		Config.PresetName = Anchor.FirstPersonCardLayoutPreset
			? Anchor.FirstPersonCardLayoutPreset->GetName()
			: TEXT("None");
		if (Config.bUsingPreset)
		{
			ApplyPresetToResolvedLayoutConfig(*Anchor.FirstPersonCardLayoutPreset, Config);
		}
		else if (Anchor.bUseFirstPersonCardLayoutPreset)
		{
			Config.PresetName = TEXT("Missing");
		}
		return Config;
	}

	void ApplyPresetToResolvedLayoutConfig(
		const UWacomFirstPersonCardLayoutPreset& Preset,
		FWacomFirstPersonCardResolvedLayoutConfig& Config)
	{
		Config.ProjectionMode = Preset.ProjectionMode;
		Config.CardLayoutMode = Preset.CardLayoutMode;
		Config.ViewportClampMode = Preset.ViewportClampMode;
		Config.FanYawDegrees = Preset.FanYawDegrees;
		Config.AuthoredCardSpacingPixels = Preset.AuthoredCardSpacingPixels;
		Config.AuthoredMaxHandWidthPixels = Preset.AuthoredMaxHandWidthPixels;
		Config.AuthoredHandScreenOffset = Preset.AuthoredHandScreenOffset;
		Config.AuthoredCenterLiftPixels = Preset.AuthoredCenterLiftPixels;
		Config.AuthoredDropCurveExponent = Preset.AuthoredDropCurveExponent;
		Config.AuthoredFanCurveExponent = Preset.AuthoredFanCurveExponent;
		Config.bAuthoredCenterCardsDrawOnTop = Preset.bAuthoredCenterCardsDrawOnTop;
		Config.ProjectionPadding = Preset.ProjectionPadding;
		Config.SoftClampOffscreenAllowancePixels = Preset.SoftClampOffscreenAllowancePixels;
		Config.SoftClampBlendRangePixels = Preset.SoftClampBlendRangePixels;
		Config.bEnableCardLayerPixelSnapping = Preset.bEnableCardLayerPixelSnapping;
		Config.CardLayerPixelSnapGrid = Preset.CardLayerPixelSnapGrid;
		Config.bClampCardLayerRenderAngle = Preset.bClampCardLayerRenderAngle;
		Config.MaxCardLayerRenderAngleDegrees = Preset.MaxCardLayerRenderAngleDegrees;
		Config.StaticCardRenderScale = Preset.StaticCardRenderScale;
		Config.StaticCardEdgeDropPixels = Preset.StaticCardEdgeDropPixels;
		Config.bEnableAnchorScreenSmoothing = Preset.bEnableAnchorScreenSmoothing;
		Config.AnchorScreenSmoothingSpeed = Preset.AnchorScreenSmoothingSpeed;
		Config.AnchorScreenSmoothingResetDistancePixels = Preset.AnchorScreenSmoothingResetDistancePixels;
		Config.bEnableCardSlotMotion = Preset.bEnableCardSlotMotion;
		Config.CardSlotMotionSpeed = Preset.CardSlotMotionSpeed;
		Config.CardSlotOpacitySpeed = Preset.CardSlotOpacitySpeed;
		Config.CardSlotEnterOffsetPixels = Preset.CardSlotEnterOffsetPixels;
		Config.CardSlotEnterOpacity = Preset.CardSlotEnterOpacity;
		Config.CardSlotExitOffsetPixels = Preset.CardSlotExitOffsetPixels;
		Config.CardSlotExitDuration = Preset.CardSlotExitDuration;
		Config.CardSlotMotionResetDistancePixels = Preset.CardSlotMotionResetDistancePixels;
		Config.bEnableEventAwareCardTransitions = Preset.bEnableEventAwareCardTransitions;
		Config.bEnableReadableTransitionOrigins = Preset.bEnableReadableTransitionOrigins;
		Config.DrawnCardEnterOffsetPixels = Preset.DrawnCardEnterOffsetPixels;
		Config.DrawnCardEnterOriginMode = Preset.DrawnCardEnterOriginMode;
		Config.DrawnCardEnterViewportAnchor = Preset.DrawnCardEnterViewportAnchor;
		Config.DrawnCardEnterScaleMultiplier = Preset.DrawnCardEnterScaleMultiplier;
		Config.DrawnCardEnterAngleOffsetDegrees = Preset.DrawnCardEnterAngleOffsetDegrees;
		Config.GainedCardEnterOffsetPixels = Preset.GainedCardEnterOffsetPixels;
		Config.GainedCardEnterOriginMode = Preset.GainedCardEnterOriginMode;
		Config.GainedCardEnterViewportAnchor = Preset.GainedCardEnterViewportAnchor;
		Config.GainedCardEnterScaleMultiplier = Preset.GainedCardEnterScaleMultiplier;
		Config.GainedCardEnterAngleOffsetDegrees = Preset.GainedCardEnterAngleOffsetDegrees;
		Config.PlayedCardExitOffsetPixels = Preset.PlayedCardExitOffsetPixels;
		Config.PlayedCardExitOriginMode = Preset.PlayedCardExitOriginMode;
		Config.PlayedCardExitViewportAnchor = Preset.PlayedCardExitViewportAnchor;
		Config.PlayedCardExitScaleMultiplier = Preset.PlayedCardExitScaleMultiplier;
		Config.PlayedCardExitAngleOffsetDegrees = Preset.PlayedCardExitAngleOffsetDegrees;
		Config.DiscardedCardExitOffsetPixels = Preset.DiscardedCardExitOffsetPixels;
		Config.DiscardedCardExitOriginMode = Preset.DiscardedCardExitOriginMode;
		Config.DiscardedCardExitViewportAnchor = Preset.DiscardedCardExitViewportAnchor;
		Config.DiscardedCardExitScaleMultiplier = Preset.DiscardedCardExitScaleMultiplier;
		Config.DiscardedCardExitAngleOffsetDegrees = Preset.DiscardedCardExitAngleOffsetDegrees;
		Config.PendingTargetingLiftPixels = Preset.PendingTargetingLiftPixels;
		Config.PendingTargetingScale = Preset.PendingTargetingScale;
		Config.PendingTargetingZOrderBoost = Preset.PendingTargetingZOrderBoost;
		Config.bPendingTargetingStraightenAngle = Preset.bPendingTargetingStraightenAngle;
		Config.PendingTargetingAngleBlend = Preset.PendingTargetingAngleBlend;
		Config.bEnableTargetSelectHandDeemphasis = Preset.bEnableTargetSelectHandDeemphasis;
		Config.TargetSelectNonPendingOpacityMultiplier = Preset.TargetSelectNonPendingOpacityMultiplier;
		Config.HandAnchorScale = Preset.HandAnchorScale;
		Config.DisabledRenderOpacity = Preset.DisabledRenderOpacity;
		Config.HoverLiftPixels = Preset.HoverLiftPixels;
		Config.HoverScale = Preset.HoverScale;
		Config.HoverZOrderBoost = Preset.HoverZOrderBoost;
		Config.bEnableCardInteractionFeedback = Preset.bEnableCardInteractionFeedback;
		Config.PlayableHoverFeedbackColor = Preset.PlayableHoverFeedbackColor;
		Config.PlayableHoverFeedbackOpacity = Preset.PlayableHoverFeedbackOpacity;
		Config.PressedFeedbackScale = Preset.PressedFeedbackScale;
		Config.PressedFeedbackColor = Preset.PressedFeedbackColor;
		Config.PressedFeedbackOpacity = Preset.PressedFeedbackOpacity;
		Config.ConfirmFeedbackDuration = Preset.ConfirmFeedbackDuration;
		Config.ConfirmFeedbackOpacity = Preset.ConfirmFeedbackOpacity;
		Config.DenyFeedbackDuration = Preset.DenyFeedbackDuration;
		Config.DenyFeedbackShakePixels = Preset.DenyFeedbackShakePixels;
		Config.DenyFeedbackColor = Preset.DenyFeedbackColor;
		Config.DenyFeedbackOpacity = Preset.DenyFeedbackOpacity;
		Config.bUsingPreset = true;
		Config.bPresetFallback = false;
		Config.PresetName = Preset.GetName();
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
		FeedbackConfig.bEnablePlayCommitFeedback = Config.bEnablePlayCommitFeedback;
		FeedbackConfig.PlayCommitDuration = Config.PlayCommitFeedbackDuration;
		FeedbackConfig.PlayCommitOpacity = Config.PlayCommitFeedbackOpacity;
		FeedbackConfig.PlayCommitColor = Config.PlayCommitFeedbackColor;
		FeedbackConfig.PlayCommitScale = Config.PlayCommitFeedbackScale;
		return FeedbackConfig;
	}

	FWacomFirstPersonCardSlotMotionConfig BuildSlotMotionConfig(
		const FWacomFirstPersonCardResolvedLayoutConfig& Config)
	{
		FWacomFirstPersonCardSlotMotionConfig MotionConfig;
		MotionConfig.bEnabled = Config.bEnableCardSlotMotion;
		MotionConfig.MotionSpeed = Config.CardSlotMotionSpeed;
		MotionConfig.OpacitySpeed = Config.CardSlotOpacitySpeed;
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

		AddInt(static_cast<int32>(Config.ProjectionMode));
		AddInt(static_cast<int32>(Config.CardLayoutMode));
		AddInt(static_cast<int32>(Config.ViewportClampMode));
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
		AddFloat(Config.StaticCardRenderScale);
		AddFloat(Config.StaticCardEdgeDropPixels);
		AddBool(Config.bEnableAnchorScreenSmoothing);
		AddFloat(Config.AnchorScreenSmoothingSpeed);
		AddFloat(Config.AnchorScreenSmoothingResetDistancePixels);
		AddBool(Config.bEnableCardSlotMotion);
		AddFloat(Config.CardSlotMotionSpeed);
		AddFloat(Config.CardSlotOpacitySpeed);
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
		AddFloat(Config.HandAnchorScale);
		AddFloat(Config.DisabledRenderOpacity);
		AddFloat(Config.HoverLiftPixels);
		AddFloat(Config.HoverScale);
		AddInt(Config.HoverZOrderBoost);
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
		AddBool(Config.bEnablePlayCommitFeedback);
		AddFloat(Config.PlayCommitFeedbackDuration);
		AddFloat(Config.PlayCommitFeedbackOpacity);
		AddColor(Config.PlayCommitFeedbackColor);
		AddFloat(Config.PlayCommitFeedbackScale);
		Combine(GetTypeHash(Config.PresetName));
		return Hash;
	}

	FVector2D ApplyViewportClampToWidgetPositionForConfig(
		FVector2D UnclampedPosition,
		FVector2D WidgetViewportSize,
		const FWacomFirstPersonCardResolvedLayoutConfig& Config,
		bool& bOutClamped,
		bool& bOutOutsideViewport,
		float& OutOffscreenDistancePixels)
	{
		bOutClamped = false;
		bOutOutsideViewport = false;
		OutOffscreenDistancePixels = 0.0f;

		const float Padding = FMath::Max(0.0f, Config.ProjectionPadding);
		const FVector2D SafeMin(Padding, Padding);
		const FVector2D SafeMax(
			FMath::Max(Padding, WidgetViewportSize.X - Padding),
			FMath::Max(Padding, WidgetViewportSize.Y - Padding));
		const FVector2D NearestSafePoint(
			FMath::Clamp(UnclampedPosition.X, SafeMin.X, SafeMax.X),
			FMath::Clamp(UnclampedPosition.Y, SafeMin.Y, SafeMax.Y));
		OutOffscreenDistancePixels = FVector2D::Distance(UnclampedPosition, NearestSafePoint);
		bOutOutsideViewport = OutOffscreenDistancePixels > KINDA_SMALL_NUMBER;

		if (Config.ViewportClampMode == EWacomFirstPersonCardViewportClampMode::AllowOffscreen)
		{
			return UnclampedPosition;
		}

		if (Config.ViewportClampMode == EWacomFirstPersonCardViewportClampMode::HardClampToViewport)
		{
			bOutClamped = bOutOutsideViewport;
			return NearestSafePoint;
		}

		const float Allowance = FMath::Max(0.0f, Config.SoftClampOffscreenAllowancePixels);
		const FVector2D SoftMin = SafeMin - FVector2D(Allowance, Allowance);
		const FVector2D SoftMax = SafeMax + FVector2D(Allowance, Allowance);
		const FVector2D NearestSoftPoint(
			FMath::Clamp(UnclampedPosition.X, SoftMin.X, SoftMax.X),
			FMath::Clamp(UnclampedPosition.Y, SoftMin.Y, SoftMax.Y));
		const float SoftOvershootDistance = FVector2D::Distance(UnclampedPosition, NearestSoftPoint);
		if (SoftOvershootDistance <= KINDA_SMALL_NUMBER)
		{
			return UnclampedPosition;
		}

		const float BlendRange = FMath::Max(0.0f, Config.SoftClampBlendRangePixels);
		const float Alpha = BlendRange <= KINDA_SMALL_NUMBER
			? 1.0f
			: FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(SoftOvershootDistance / BlendRange, 0.0f, 1.0f));
		const FVector2D ClampedPosition = FMath::Lerp(UnclampedPosition, NearestSoftPoint, Alpha);
		bOutClamped = !UnclampedPosition.Equals(ClampedPosition, KINDA_SMALL_NUMBER);
		return ClampedPosition;
	}

	FVector2D SnapCardLayerPositionForConfig(
		FVector2D Position,
		const FWacomFirstPersonCardResolvedLayoutConfig& Config,
		bool& bOutPixelSnapped)
	{
		bOutPixelSnapped = false;
		if (!Config.bEnableCardLayerPixelSnapping)
		{
			return Position;
		}

		const float Grid = FMath::Max(0.01f, Config.CardLayerPixelSnapGrid);
		const FVector2D SnappedPosition(
			FMath::RoundToFloat(Position.X / Grid) * Grid,
			FMath::RoundToFloat(Position.Y / Grid) * Grid);
		bOutPixelSnapped = !SnappedPosition.Equals(Position, KINDA_SMALL_NUMBER);
		return SnappedPosition;
	}

	float ClampCardLayerRenderAngleForConfig(
		float AngleDegrees,
		const FWacomFirstPersonCardResolvedLayoutConfig& Config)
	{
		if (!Config.bClampCardLayerRenderAngle)
		{
			return AngleDegrees;
		}

		const float MaxAbsAngle = FMath::Max(0.0f, Config.MaxCardLayerRenderAngleDegrees);
		return FMath::Clamp(AngleDegrees, -MaxAbsAngle, MaxAbsAngle);
	}
}

UWacomFirstPersonCardAnchorComponent::UWacomFirstPersonCardAnchorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UWacomFirstPersonCardAnchorComponent::BeginPlay()
{
	Super::BeginPlay();
	ConfigureTickPrerequisites();
	SetComponentTickEnabled(true);
}

void UWacomFirstPersonCardAnchorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetBattleHandInteractionPrototypeEnabled(false);
	ResetAnchorScreenSmoothing();
	RemoveStaticCardLayer();
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
	UpdateStaticCardLayer();
}

void UWacomFirstPersonCardAnchorComponent::RefreshAnchor(float DeltaTime)
{
	RefreshCardLayoutPresetRuntimeState();
	const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
	FTransform BaseTransform = FTransform::Identity;
	EWacomFirstPersonCardAnchorMode ResolvedMode = EWacomFirstPersonCardAnchorMode::Invalid;
	FName ResolvedFallbackReason = NAME_None;
	if (!ResolveBaseAnchor(BaseTransform, ResolvedMode, ResolvedFallbackReason))
	{
		bHasValidAnchor = false;
		CurrentMode = ResolvedMode;
		CurrentLookOffsetUsed = FRotator::ZeroRotator;
		bCurrentLookOffsetAppliedToLayout = false;
		LastFallbackReason = ResolvedFallbackReason;
		ResetAnchorScreenSmoothing();
		return;
	}

	FRotator LookOffset = FRotator::ZeroRotator;
	if (Config.ProjectionMode == EWacomFirstPersonCardProjectionMode::LegacyWorldProjected)
	{
		if (const AWacomPlayerCharacter* Character = GetOwnerCharacter())
		{
			if (const UWacomCursorLookDriverComponent* CursorLook = Character->GetCursorLookDriverComponent())
			{
				const FRotator SharedOffset = CursorLook->GetCurrentLookOffset();
				LookOffset.Pitch = SharedOffset.Pitch * LookInfluencePitch;
				LookOffset.Yaw = SharedOffset.Yaw * LookInfluenceYaw;
			}
		}
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
	bCurrentLookOffsetAppliedToLayout = !LookOffset.IsNearlyZero();
	LastFallbackReason = ResolvedFallbackReason;
}

FTransform UWacomFirstPersonCardAnchorComponent::ComputeCardTransform(int32 NumCards, int32 CardIndex) const
{
	RefreshCardLayoutPresetRuntimeState();
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
	RefreshCardLayoutPresetRuntimeState();
	const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
	OutProjectedPoint = FWacomFirstPersonCardProjectedPoint();
	OutProjectedPoint.Index = PointIndex;
	OutProjectedPoint.WorldLocation = CardTransform.GetLocation();
	OutProjectedPoint.ProjectionMode = Config.ProjectionMode;
	OutProjectedPoint.LayoutMode = Config.CardLayoutMode;
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
	WidgetPosition = ApplyViewportClampToWidgetPositionForConfig(
		UnclampedPosition,
		WidgetViewportSize,
		Config,
		bClamped,
		bOutsideViewport,
		OffscreenDistancePixels);

	bool bPixelSnapped = false;
	const FVector2D SnappedPosition = SnapCardLayerPositionForConfig(WidgetPosition, Config, bPixelSnapped);

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

bool UWacomFirstPersonCardAnchorComponent::CanCreateStaticCardLayerForAnchor(
	APlayerController* PlayerController) const
{
	return PlayerController && PlayerController->IsLocalController();
}

UWacomFirstPersonCardLayerWidget* UWacomFirstPersonCardAnchorComponent::CreateStaticCardLayerWidgetForAnchor(
	APlayerController* PlayerController,
	TSubclassOf<UWacomFirstPersonCardLayerWidget> LayerClass) const
{
	if (!PlayerController || !LayerClass)
	{
		return nullptr;
	}

	return CreateWidget<UWacomFirstPersonCardLayerWidget>(PlayerController, LayerClass);
}

void UWacomFirstPersonCardAnchorComponent::AddStaticCardLayerWidgetToViewportForAnchor(
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
	RefreshCardLayoutPresetRuntimeState();
	const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
	FWacomFirstPersonCardAnchorDebugView View;
	View.bHasValidAnchor = bHasValidAnchor;
	View.Mode = CurrentMode;
	View.AnchorTransform = CurrentAnchorTransform;
	View.ProjectionMode = Config.ProjectionMode;
	View.LayoutMode = Config.CardLayoutMode;
	View.ViewportClampMode = Config.ViewportClampMode;
	View.LookOffsetUsed = CurrentLookOffsetUsed;
	View.LastFallbackReason = LastFallbackReason;
	View.bBodyLockedLayout = Config.ProjectionMode == EWacomFirstPersonCardProjectionMode::BodyLocked;
	View.bCurrentCameraProjection = true;
	View.bLookOffsetAppliedToLayout = bCurrentLookOffsetAppliedToLayout;
	View.bAnchorScreenSmoothed = bLastAnchorScreenSmoothed;
	if (StaticCardLayerWidget)
	{
		View.LayerMotionDebugView = StaticCardLayerWidget->GetSlotMotionDebugView();
	}

	if (!bHasValidAnchor)
	{
		return View;
	}

	const int32 ClampedCount = FMath::Clamp(NumDebugCards, 0, 32);
	View.ProjectedPoints.Reserve(ClampedCount);
	if (Config.CardLayoutMode == EWacomFirstPersonCardLayoutMode::Authored2D)
	{
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
			Point.LayoutMode = Slot.LayoutMode;
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

	for (int32 Index = 0; Index < ClampedCount; ++Index)
	{
		FWacomFirstPersonCardProjectedPoint Point;
		ProjectCardTransformToScreen(ComputeCardTransform(ClampedCount, Index), Point, Index);
		View.ProjectedPoints.Add(Point);
	}
	return View;
}

TArray<FWacomFirstPersonCardLayerSlotView> UWacomFirstPersonCardAnchorComponent::BuildStaticCardSlotViews() const
{
	return BuildCardSlotViewsFromEntries(BuildStaticCardLayerEntries());
}

void UWacomFirstPersonCardAnchorComponent::SetRuntimeCardLayerEntries(
	FName SourceId,
	const TArray<FWacomFirstPersonCardLayerEntry>& Entries)
{
	if (SourceId.IsNone())
	{
		return;
	}

	RuntimeCardLayerSourceId = SourceId;
	RuntimeCardLayerEntries.Reset(Entries.Num());
	RuntimeCardLayerData.Reset(Entries.Num());
	for (FWacomFirstPersonCardLayerEntry Entry : Entries)
	{
		Entry.bIsPlayable = Entry.bIsPlayable && !Entry.CardViewData.bDisabled;
		Entry.CardViewData.bDisabled = !Entry.bIsPlayable;
		RuntimeCardLayerEntries.Add(Entry);
		RuntimeCardLayerData.Add(Entry.CardViewData);
	}
	bHasRuntimeCardLayerData = true;
}

void UWacomFirstPersonCardAnchorComponent::SetRuntimeCardLayerTransitionHints(
	FName SourceId,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& Hints)
{
	if (SourceId.IsNone())
	{
		return;
	}

	RuntimeCardLayerTransitionHintSourceId = SourceId;
	RuntimeCardLayerTransitionHints.Reset(Hints.Num());
	for (const FWacomFirstPersonCardLayerTransitionHint& Hint : Hints)
	{
		if (Hint.CardInstanceId.IsValid()
			&& (Hint.TransitionKind != EWacomFirstPersonCardSlotTransitionKind::Default
				|| Hint.bPlayCommitFeedback))
		{
			RuntimeCardLayerTransitionHints.Add(Hint);
		}
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
	if (SourceId.IsNone() || RuntimeCardLayerSourceId != SourceId)
	{
		return;
	}

	RuntimeCardLayerSourceId = NAME_None;
	RuntimeCardLayerData.Reset();
	RuntimeCardLayerEntries.Reset();
	RuntimeCardLayerTransitionHints.Reset();
	RuntimeCardLayerTransitionHintSourceId = NAME_None;
	HoveredCardInstanceId.Invalidate();
	bHasRuntimeCardLayerData = false;
	ResetAnchorScreenSmoothing();
	if (StaticCardLayerWidget)
	{
		StaticCardLayerWidget->ClearSlotMotionState();
	}
}

TArray<FWacomFirstPersonCardLayerEntry> UWacomFirstPersonCardAnchorComponent::BuildStaticCardLayerEntries() const
{
	TArray<FWacomCardViewData> CardData;
	const int32 DesiredCount = StaticPreviewCardDefinitions.Num() > 0
		? StaticPreviewCardDefinitions.Num()
		: StaticCardCountFallback;
	const int32 ClampedCount = FMath::Clamp(DesiredCount, 0, 32);
	CardData.Reserve(ClampedCount);
	for (int32 Index = 0; Index < ClampedCount; ++Index)
	{
		CardData.Add(BuildStaticCardViewData(Index));
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
	RefreshCardLayoutPresetRuntimeState();
	const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
	TArray<FWacomFirstPersonCardLayerSlotView> Slots;
	if (!bHasValidAnchor)
	{
		return Slots;
	}

	const int32 ClampedCount = FMath::Clamp(CardEntries.Num(), 0, 32);
	Slots.Reserve(ClampedCount);

	bool bHasPendingTargetingCard = false;
	for (int32 EntryIndex = 0; EntryIndex < ClampedCount; ++EntryIndex)
	{
		if (CardEntries[EntryIndex].bIsPendingTargeting)
		{
			bHasPendingTargetingCard = true;
			break;
		}
	}

	FWacomFirstPersonCardProjectedPoint AnchorPoint;
	const bool bUseAuthoredLayout = Config.CardLayoutMode == EWacomFirstPersonCardLayoutMode::Authored2D;
	const bool bAuthoredAnchorProjected = !bUseAuthoredLayout
		|| ProjectCardTransformToScreen(CurrentAnchorTransform, AnchorPoint, INDEX_NONE);
	if (bUseAuthoredLayout && bAuthoredAnchorProjected)
	{
		ApplyAnchorScreenSmoothing(AnchorPoint);
	}
	else if (bUseAuthoredLayout)
	{
		ResetAnchorScreenSmoothing();
	}

	for (int32 Index = 0; Index < ClampedCount; ++Index)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = Index;
		Slot.Entry = CardEntries[Index];
		Slot.Entry.bIsPlayable = Slot.Entry.bIsPlayable && !Slot.Entry.CardViewData.bDisabled;
		Slot.Entry.CardViewData.bDisabled = !Slot.Entry.bIsPlayable;
		Slot.RenderScale = FMath::Max(0.01f, Config.StaticCardRenderScale);
		Slot.RenderOpacity = Slot.Entry.bIsPlayable
			? 1.0f
			: FMath::Clamp(Config.DisabledRenderOpacity, 0.0f, 1.0f);
		Slot.ZOrder = Index;
		Slot.ProjectionMode = Config.ProjectionMode;
		Slot.LayoutMode = Config.CardLayoutMode;
		Slot.ViewportClampMode = Config.ViewportClampMode;
		Slot.bBodyLockedLayout = Config.ProjectionMode == EWacomFirstPersonCardProjectionMode::BodyLocked;
		Slot.bCurrentCameraProjection = true;
		Slot.bLookOffsetAppliedToLayout = bCurrentLookOffsetAppliedToLayout;

		const float CenterOffset =
			static_cast<float>(Index) - (static_cast<float>(ClampedCount - 1) * 0.5f);
		const float MaxAbsCenterOffset = FMath::Max(1.0f, static_cast<float>(ClampedCount - 1) * 0.5f);
		const float NormalizedHandOffset = CenterOffset / MaxAbsCenterOffset;
		const float NormalizedEdgeDistance = FMath::Abs(NormalizedHandOffset);
		const float FanCurveExponent = FMath::Max(0.01f, Config.AuthoredFanCurveExponent);
		const float FanDirection = CenterOffset < 0.0f ? -1.0f : 1.0f;
		const float AuthoredFanMagnitude = FMath::Pow(NormalizedEdgeDistance, FanCurveExponent) * MaxAbsCenterOffset;
		const float FanOffset = bUseAuthoredLayout
			? FanDirection * AuthoredFanMagnitude
			: CenterOffset;
		const bool bIsPendingTargeting = Slot.Entry.bIsPendingTargeting;
		const bool bIsHovered =
			Slot.Entry.CardInstanceId.IsValid()
			&& Slot.Entry.CardInstanceId == HoveredCardInstanceId;
		const bool bAllowHoverTransform = bIsHovered && Slot.Entry.bIsPlayable && !bIsPendingTargeting;
		Slot.NormalizedHandOffset = NormalizedHandOffset;
		Slot.RenderAngleDegrees = ClampCardLayerRenderAngleForConfig(FanOffset * Config.FanYawDegrees, Config);
		if (bUseAuthoredLayout && Config.bAuthoredCenterCardsDrawOnTop)
		{
			Slot.ZOrder = FMath::RoundToInt((1.0f - NormalizedEdgeDistance) * 100.0f);
		}
		if (Slot.Entry.bIsHandAnchor)
		{
			Slot.RenderScale *= FMath::Max(0.01f, Config.HandAnchorScale);
		}
		if (bIsPendingTargeting)
		{
			if (Config.bPendingTargetingStraightenAngle)
			{
				const float AngleBlend = FMath::Clamp(Config.PendingTargetingAngleBlend, 0.0f, 1.0f);
				Slot.RenderAngleDegrees = FMath::Lerp(Slot.RenderAngleDegrees, 0.0f, AngleBlend);
			}
			Slot.RenderScale *= FMath::Max(0.01f, Config.PendingTargetingScale);
			Slot.ZOrder += FMath::Max(0, Config.PendingTargetingZOrderBoost);
		}
		else if (bHasPendingTargetingCard && Config.bEnableTargetSelectHandDeemphasis)
		{
			Slot.RenderOpacity = FMath::Clamp(
				Slot.RenderOpacity * FMath::Clamp(Config.TargetSelectNonPendingOpacityMultiplier, 0.0f, 1.0f),
				0.0f,
				1.0f);
		}
		if (bIsHovered)
		{
			Slot.bIsHovered = true;
		}
		if (bAllowHoverTransform)
		{
			Slot.RenderScale *= FMath::Max(0.01f, Config.HoverScale);
			Slot.ZOrder += FMath::Max(0, Config.HoverZOrderBoost);
		}

		if (bUseAuthoredLayout)
		{
			Slot.RawScreenPosition = AnchorPoint.RawScreenPosition;
			Slot.UnclampedWidgetPosition = AnchorPoint.UnclampedWidgetPosition;
			Slot.ViewportScale = AnchorPoint.ViewportScale;
			Slot.ProjectionMode = AnchorPoint.ProjectionMode;
			Slot.ViewportClampMode = AnchorPoint.ViewportClampMode;
			Slot.AnchorWidgetPosition = AnchorPoint.WidgetPosition;
			Slot.UnsmoothedAnchorWidgetPosition = AnchorPoint.UnsmoothedAnchorWidgetPosition;
			Slot.SmoothedAnchorWidgetPosition = AnchorPoint.SmoothedAnchorWidgetPosition;
			Slot.OffscreenDistancePixels = AnchorPoint.OffscreenDistancePixels;
			Slot.AnchorScreenSmoothingDistancePixels = AnchorPoint.AnchorScreenSmoothingDistancePixels;
			Slot.bBodyLockedLayout = Config.ProjectionMode == EWacomFirstPersonCardProjectionMode::BodyLocked;
			Slot.bCurrentCameraProjection = true;
			Slot.bLookOffsetAppliedToLayout = bCurrentLookOffsetAppliedToLayout;
			Slot.bClamped = AnchorPoint.bClamped;
			Slot.bOutsideViewport = AnchorPoint.bOutsideViewport;
			Slot.bAnchorScreenSmoothed = AnchorPoint.bAnchorScreenSmoothed;
			Slot.bProjected = false;

			if (bAuthoredAnchorProjected)
			{
				const float NaturalHandWidth = FMath::Max(0, ClampedCount - 1) * FMath::Max(0.0f, Config.AuthoredCardSpacingPixels);
				const float MaxHandWidth = FMath::Max(0.0f, Config.AuthoredMaxHandWidthPixels);
				const float WidthScale = (MaxHandWidth > 0.0f && NaturalHandWidth > MaxHandWidth)
					? MaxHandWidth / NaturalHandWidth
					: 1.0f;
				const float XOffset = CenterOffset * FMath::Max(0.0f, Config.AuthoredCardSpacingPixels) * WidthScale;
				const float DropMagnitude =
					FMath::Pow(NormalizedEdgeDistance, FMath::Max(0.01f, Config.AuthoredDropCurveExponent))
					* FMath::Max(0.0f, Config.StaticCardEdgeDropPixels);
				const float CenterLiftMagnitude =
					(1.0f - NormalizedEdgeDistance) * Config.AuthoredCenterLiftPixels;

				FVector2D FinalPosition =
					AnchorPoint.WidgetPosition
					+ Config.AuthoredHandScreenOffset
					+ FVector2D(XOffset, DropMagnitude - CenterLiftMagnitude);
				Slot.AuthoredLayoutOffset = FinalPosition - AnchorPoint.WidgetPosition;
				Slot.bBodyLockedLayout = AnchorPoint.bBodyLockedLayout;
				Slot.bCurrentCameraProjection = AnchorPoint.bCurrentCameraProjection;
				Slot.bLookOffsetAppliedToLayout = AnchorPoint.bLookOffsetAppliedToLayout;
				if (bIsPendingTargeting)
				{
					FinalPosition.Y -= FMath::Max(0.0f, Config.PendingTargetingLiftPixels);
				}
				if (bAllowHoverTransform)
				{
					FinalPosition.Y -= FMath::Max(0.0f, Config.HoverLiftPixels);
				}
				bool bPixelSnapped = false;
				Slot.WidgetPosition = FinalPosition;
				Slot.SnappedWidgetPosition = SnapCardLayerPositionForConfig(FinalPosition, Config, bPixelSnapped);
				Slot.ScreenPosition = Slot.SnappedWidgetPosition;
				Slot.bPixelSnapped = bPixelSnapped;
				Slot.bProjected = AnchorPoint.bProjected;
			}
		}
		else
		{
			FWacomFirstPersonCardProjectedPoint Point;
			if (ProjectCardTransformToScreen(ComputeCardTransform(ClampedCount, Index), Point, Index))
			{
				FVector2D FinalPosition = Point.WidgetPosition;
				Slot.RawScreenPosition = Point.RawScreenPosition;
				Slot.UnclampedWidgetPosition = Point.UnclampedWidgetPosition;
				Slot.ViewportScale = Point.ViewportScale;
				Slot.ProjectionMode = Point.ProjectionMode;
				Slot.LayoutMode = EWacomFirstPersonCardLayoutMode::LegacyProjectedFan2D;
				Slot.ViewportClampMode = Point.ViewportClampMode;
				Slot.AnchorWidgetPosition = Point.WidgetPosition;
				Slot.UnsmoothedAnchorWidgetPosition = Point.WidgetPosition;
				Slot.SmoothedAnchorWidgetPosition = Point.WidgetPosition;
				Slot.OffscreenDistancePixels = Point.OffscreenDistancePixels;
				Slot.AnchorScreenSmoothingDistancePixels = 0.0f;
				Slot.bBodyLockedLayout = Point.bBodyLockedLayout;
				Slot.bCurrentCameraProjection = Point.bCurrentCameraProjection;
				Slot.bLookOffsetAppliedToLayout = Point.bLookOffsetAppliedToLayout;
				FinalPosition.Y += FMath::Square(NormalizedEdgeDistance) * FMath::Max(0.0f, Config.StaticCardEdgeDropPixels);
				if (bIsPendingTargeting)
				{
					FinalPosition.Y -= FMath::Max(0.0f, Config.PendingTargetingLiftPixels);
				}
				if (bAllowHoverTransform)
				{
					FinalPosition.Y -= FMath::Max(0.0f, Config.HoverLiftPixels);
				}
				bool bPixelSnapped = false;
				Slot.WidgetPosition = FinalPosition;
				Slot.SnappedWidgetPosition = SnapCardLayerPositionForConfig(FinalPosition, Config, bPixelSnapped);
				Slot.ScreenPosition = Slot.SnappedWidgetPosition;
				Slot.bPixelSnapped = bPixelSnapped;
				Slot.bProjected = Point.bProjected;
				Slot.bClamped = Point.bClamped;
				Slot.bOutsideViewport = Point.bOutsideViewport;
				Slot.bAnchorScreenSmoothed = false;
			}
		}

		Slots.Add(Slot);
	}
	return Slots;
}

TArray<FWacomFirstPersonCardLayerSlotView> UWacomFirstPersonCardAnchorComponent::BuildActiveCardLayerSlotViews() const
{
	return bHasRuntimeCardLayerData
		? BuildCardSlotViewsFromEntries(RuntimeCardLayerEntries)
		: BuildStaticCardSlotViews();
}

void UWacomFirstPersonCardAnchorComponent::SetBattleHandInteractionPrototypeEnabled(bool bEnabled)
{
	if (bEnableBattleHandInteractionPrototype == bEnabled)
	{
		return;
	}

	bEnableBattleHandInteractionPrototype = bEnabled;
	if (!bEnableBattleHandInteractionPrototype)
	{
		HoveredCardInstanceId.Invalidate();
		if (StaticCardLayerWidget)
		{
			StaticCardLayerWidget->ClearSlotMotionState();
		}
	}
	if (StaticCardLayerWidget)
	{
		StaticCardLayerWidget->SetCardLayerInteractionEnabled(bEnableBattleHandInteractionPrototype);
	}
}

#if WITH_AUTOMATION_TESTS
bool UWacomFirstPersonCardAnchorComponent::IsUsingResolvedCardLayoutPresetForTest() const
{
	RefreshCardLayoutPresetRuntimeState();
	return ResolveLayoutConfig(*this).bUsingPreset;
}
#endif

FString UWacomFirstPersonCardAnchorComponent::GetDebugSummary() const
{
	RefreshCardLayoutPresetRuntimeState();
	const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
	const FString LayerMotionSummary = StaticCardLayerWidget
		? StaticCardLayerWidget->GetSlotMotionDebugSummary()
		: TEXT("SlotMotion Inactive");
	return FString::Printf(
		TEXT("FirstPersonCardAnchor Mode=%s ProjectionMode=%s LayoutMode=%s ViewportClampMode=%s PresetEnabled=%s PresetActive=%s PresetName=%s PresetFallback=%s BodyLockedLayout=%s CurrentCameraProjection=true LookUsedForLayout=%s Valid=%s Anchor=%s LookOffset=%s Fallback=%s PixelSnap=%s SnapGrid=%.2f AngleClamp=%s MaxAngle=%.2f ViewportScale=%.2f SoftAllowance=%.2f SoftBlend=%.2f AnchorScreenSmoothing=%s AnchorScreenSmoothingSpeed=%.2f AnchorScreenSmoothingReset=%.2f AnchorScreenSmoothed=%s AnchorScreenSmoothingDistance=%.2f %s"),
		*AnchorModeToString(CurrentMode),
		*ProjectionModeToString(Config.ProjectionMode),
		*LayoutModeToString(Config.CardLayoutMode),
		*ViewportClampModeToString(Config.ViewportClampMode),
		bUseFirstPersonCardLayoutPreset ? TEXT("true") : TEXT("false"),
		Config.bUsingPreset ? TEXT("true") : TEXT("false"),
		*Config.PresetName,
		Config.bPresetFallback ? TEXT("true") : TEXT("false"),
		Config.ProjectionMode == EWacomFirstPersonCardProjectionMode::BodyLocked ? TEXT("true") : TEXT("false"),
		bCurrentLookOffsetAppliedToLayout ? TEXT("true") : TEXT("false"),
		bHasValidAnchor ? TEXT("true") : TEXT("false"),
		*CurrentAnchorTransform.ToHumanReadableString(),
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
		*LayerMotionSummary);
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

FWacomCardViewData UWacomFirstPersonCardAnchorComponent::BuildStaticCardViewData(int32 CardIndex) const
{
	if (StaticPreviewCardDefinitions.IsValidIndex(CardIndex))
	{
		const TSoftObjectPtr<UCardDefinition>& CardPtr = StaticPreviewCardDefinitions[CardIndex];
		if (const UCardDefinition* Card = CardPtr.LoadSynchronous())
		{
			return UWacomCardPresentationBuilder::BuildCardViewData(Card);
		}
	}

	FWacomCardViewData Data;
	Data.Name = FText::Format(
		NSLOCTEXT("Wacom.FirstPersonCardLayer", "StaticPlaceholderName", "Anchor Card {0}"),
		FText::AsNumber(CardIndex + 1));
	Data.TypeText = NSLOCTEXT("Wacom.FirstPersonCardLayer", "StaticPlaceholderType", "Static Preview");
	Data.Description = NSLOCTEXT(
		"Wacom.FirstPersonCardLayer",
		"StaticPlaceholderDescription",
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

void UWacomFirstPersonCardAnchorComponent::RefreshCardLayoutPresetRuntimeState() const
{
	const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
	const uint32 ConfigHash = BuildResolvedLayoutConfigHash(Config);
	const bool bPresetStateChanged =
		LastResolvedCardLayoutPreset.Get() != FirstPersonCardLayoutPreset
		|| bLastResolvedCardLayoutPresetEnabled != bUseFirstPersonCardLayoutPreset
		|| bLastResolvedCardLayoutPresetFallback != Config.bPresetFallback
		|| !bHasResolvedCardLayoutConfigHash
		|| LastResolvedCardLayoutConfigHash != ConfigHash;
	if (!bPresetStateChanged)
	{
		return;
	}

	LastResolvedCardLayoutPreset = FirstPersonCardLayoutPreset;
	bLastResolvedCardLayoutPresetEnabled = bUseFirstPersonCardLayoutPreset;
	bLastResolvedCardLayoutPresetFallback = Config.bPresetFallback;
	bHasResolvedCardLayoutConfigHash = true;
	LastResolvedCardLayoutConfigHash = ConfigHash;
	InvalidateCardLayoutPresetRuntimeState();

	if (bLogResolvedCardLayoutPreset)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[FirstPersonCardLayoutPreset] Enabled=%s Active=%s Preset=%s Fallback=%s"),
			bUseFirstPersonCardLayoutPreset ? TEXT("true") : TEXT("false"),
			Config.bUsingPreset ? TEXT("true") : TEXT("false"),
			*Config.PresetName,
			Config.bPresetFallback ? TEXT("true") : TEXT("false"));
		bHasLoggedResolvedCardLayoutPreset = true;
	}
	else
	{
		bHasLoggedResolvedCardLayoutPreset = false;
	}
}

void UWacomFirstPersonCardAnchorComponent::InvalidateCardLayoutPresetRuntimeState() const
{
	ResetAnchorScreenSmoothing();
	if (StaticCardLayerWidget)
	{
		StaticCardLayerWidget->ClearSlotMotionState();
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
	SmoothedAnchorLayoutMode = Config.CardLayoutMode;
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
		|| Config.CardLayoutMode != EWacomFirstPersonCardLayoutMode::Authored2D
		|| !Config.bEnableAnchorScreenSmoothing
		|| Config.AnchorScreenSmoothingSpeed <= 0.0f)
	{
		SmoothedAnchorWidgetPosition = AnchorPoint.WidgetPosition;
		bHasSmoothedAnchorWidgetPosition = AnchorPoint.bProjected;
		SmoothedAnchorLayoutMode = Config.CardLayoutMode;
		SmoothedAnchorProjectionMode = Config.ProjectionMode;
		SmoothedAnchorViewportClampMode = Config.ViewportClampMode;
		SmoothedAnchorMode = CurrentMode;
		return;
	}

	const bool bModeChanged =
		SmoothedAnchorLayoutMode != Config.CardLayoutMode
		|| SmoothedAnchorProjectionMode != Config.ProjectionMode
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
		SmoothedAnchorLayoutMode = Config.CardLayoutMode;
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
	SmoothedAnchorLayoutMode = Config.CardLayoutMode;
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
		return TEXT("LegacyWorldProjected");
	case EWacomFirstPersonCardProjectionMode::BodyLocked:
	default:
		return TEXT("BodyLocked");
	}
}

FString UWacomFirstPersonCardAnchorComponent::LayoutModeToString(EWacomFirstPersonCardLayoutMode Mode)
{
	switch (Mode)
	{
	case EWacomFirstPersonCardLayoutMode::LegacyProjectedFan2D:
		return TEXT("LegacyProjectedFan2D");
	case EWacomFirstPersonCardLayoutMode::Authored2D:
	default:
		return TEXT("Authored2D");
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

void UWacomFirstPersonCardAnchorComponent::UpdateStaticCardLayer()
{
	if (!bDrawStaticCardLayer && !bHasRuntimeCardLayerData)
	{
		RemoveStaticCardLayer();
		return;
	}

	APlayerController* PC = GetOwnerPlayerController();
	if (!CanCreateStaticCardLayerForAnchor(PC))
	{
		RemoveStaticCardLayer();
		return;
	}

	if (!StaticCardLayerWidget)
	{
		TSubclassOf<UWacomFirstPersonCardLayerWidget> LayerClass = StaticCardLayerWidgetClass;
		if (!LayerClass)
		{
			LayerClass = UWacomFirstPersonCardLayerWidget::StaticClass();
		}

		StaticCardLayerWidget = CreateStaticCardLayerWidgetForAnchor(PC, LayerClass);
		if (StaticCardLayerWidget)
		{
			RefreshCardLayoutPresetRuntimeState();
			const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
			StaticCardLayerWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			StaticCardLayerWidget->SetCardViewClass(FirstPersonCardViewClass);
			StaticCardLayerWidget->SetSlotMotionConfig(BuildSlotMotionConfig(Config));
			StaticCardLayerWidget->SetSlotFeedbackConfig(BuildSlotFeedbackConfig(Config));
			StaticCardLayerWidget->SetLogSlotMotionDiagnostics(bLogCardLayerMotionDiagnostics);
			StaticCardLayerWidget->SetCardLayerInteractionEnabled(bEnableBattleHandInteractionPrototype);
			BindStaticCardLayerWidget(StaticCardLayerWidget);
			AddStaticCardLayerWidgetToViewportForAnchor(StaticCardLayerWidget, StaticCardLayerZOrder);
		}
	}

	if (StaticCardLayerWidget)
	{
		RefreshCardLayoutPresetRuntimeState();
		const FWacomFirstPersonCardResolvedLayoutConfig Config = ResolveLayoutConfig(*this);
		StaticCardLayerWidget->SetSlotMotionConfig(BuildSlotMotionConfig(Config));
		StaticCardLayerWidget->SetSlotFeedbackConfig(BuildSlotFeedbackConfig(Config));
		StaticCardLayerWidget->SetLogSlotMotionDiagnostics(bLogCardLayerMotionDiagnostics);
		StaticCardLayerWidget->SetCardViewClass(FirstPersonCardViewClass);
		if (RuntimeCardLayerTransitionHintSourceId == RuntimeCardLayerSourceId
			&& RuntimeCardLayerTransitionHints.Num() > 0)
		{
			StaticCardLayerWidget->SetCardTransitionHints(RuntimeCardLayerTransitionHints);
			RuntimeCardLayerTransitionHints.Reset();
			RuntimeCardLayerTransitionHintSourceId = NAME_None;
		}
		StaticCardLayerWidget->SetStaticCardSlots(BuildActiveCardLayerSlotViews());
	}
}

void UWacomFirstPersonCardAnchorComponent::RemoveDebugWidget()
{
	if (DebugWidget)
	{
		DebugWidget->RemoveFromParent();
		DebugWidget = nullptr;
	}
}

void UWacomFirstPersonCardAnchorComponent::RemoveStaticCardLayer()
{
	if (StaticCardLayerWidget)
	{
		StaticCardLayerWidget->ClearSlotMotionState();
		UnbindStaticCardLayerWidget(StaticCardLayerWidget);
		StaticCardLayerWidget->RemoveFromParent();
		StaticCardLayerWidget = nullptr;
	}
	RuntimeCardLayerTransitionHints.Reset();
	RuntimeCardLayerTransitionHintSourceId = NAME_None;
	HoveredCardInstanceId.Invalidate();
}

void UWacomFirstPersonCardAnchorComponent::BindStaticCardLayerWidget(UWacomFirstPersonCardLayerWidget* LayerWidget)
{
	if (!LayerWidget)
	{
		return;
	}

	LayerWidget->OnCardClickedNative.RemoveAll(this);
	LayerWidget->OnCardHoveredNative.RemoveAll(this);
	LayerWidget->OnCardUnhoveredNative.RemoveAll(this);
	LayerWidget->OnHoveredCardSlotUpdatedNative.RemoveAll(this);
	LayerWidget->OnCardClickedNative.AddUObject(this, &UWacomFirstPersonCardAnchorComponent::HandleLayerCardClicked);
	LayerWidget->OnCardHoveredNative.AddUObject(this, &UWacomFirstPersonCardAnchorComponent::HandleLayerCardHovered);
	LayerWidget->OnCardUnhoveredNative.AddUObject(this, &UWacomFirstPersonCardAnchorComponent::HandleLayerCardUnhovered);
	LayerWidget->OnHoveredCardSlotUpdatedNative.AddUObject(
		this,
		&UWacomFirstPersonCardAnchorComponent::HandleLayerHoveredCardSlotUpdated);
}

void UWacomFirstPersonCardAnchorComponent::UnbindStaticCardLayerWidget(UWacomFirstPersonCardLayerWidget* LayerWidget)
{
	if (!LayerWidget)
	{
		return;
	}

	LayerWidget->OnCardClickedNative.RemoveAll(this);
	LayerWidget->OnCardHoveredNative.RemoveAll(this);
	LayerWidget->OnCardUnhoveredNative.RemoveAll(this);
	LayerWidget->OnHoveredCardSlotUpdatedNative.RemoveAll(this);
}

void UWacomFirstPersonCardAnchorComponent::HandleLayerCardClicked(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	OnFirstPersonCardLayerCardClicked.Broadcast(CardInstanceId, SlotView);
}

void UWacomFirstPersonCardAnchorComponent::HandleLayerCardHovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	HoveredCardInstanceId = CardInstanceId;
	OnFirstPersonCardLayerCardHovered.Broadcast(CardInstanceId, SlotView);
}

void UWacomFirstPersonCardAnchorComponent::HandleLayerCardUnhovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (HoveredCardInstanceId == CardInstanceId)
	{
		HoveredCardInstanceId.Invalidate();
	}
	OnFirstPersonCardLayerCardUnhovered.Broadcast(CardInstanceId, SlotView);
}

void UWacomFirstPersonCardAnchorComponent::HandleLayerHoveredCardSlotUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	OnFirstPersonCardLayerHoveredCardLayoutUpdated.Broadcast(CardInstanceId, SlotView);
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
	if (!HoveredCardInstanceId.IsValid())
	{
		return FWacomInteractionTargetHandle();
	}

	return FWacomInteractionTargetHandle::ForCardTarget(HoveredCardInstanceId,
		const_cast<UWacomFirstPersonCardAnchorComponent*>(this));
}
