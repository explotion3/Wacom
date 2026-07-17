// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardPresentationReadinessCoordinator.h"
#include "WacomFirstPersonCardSurfaceEffectArbiter.h"

class FWacomFirstPersonCardDataRewritePlayback;
class FWacomFirstPersonCardDrawRevealPlayback;
class FWacomFirstPersonCardEffectBadgeFeedbackPlayback;
class FWacomFirstPersonCardGainRevealPlayback;
class FWacomFirstPersonCardHandTargetImpactPlayback;
class FWacomFirstPersonCardRetainSealPlayback;
class FWacomFirstPersonCardSurfaceDeparturePlayback;
class FWacomFirstPersonCardTransitionPlayback;
class FWacomFirstPersonCardUseReformPlayback;

/** Transient orchestration state owned by the semantic presentation controller. */
struct FWacomFirstPersonCardSlotPresentationRuntimeState
{
	bool bPlaybackFrozenForReadiness = false;
	bool bSuppressRetainSealSurfaceForReadinessFailure = false;

	int32 PendingDataRewriteFieldMask = 0;
	EWacomFirstPersonCardDataRewriteTone PendingDataRewriteTone =
		EWacomFirstPersonCardDataRewriteTone::Neutral;
	int32 PendingDataRewriteSeed = 0;
	int32 PendingDataRewriteSequenceIndex = 0;
	int32 PendingDataRewriteSequenceCount = 1;
	bool bPendingDataRewriteHandoff = false;
	bool bDataRewriteBlocksPresentationPhase = false;
	bool bEffectBadgeFeedbackBlocksPresentationPhase = false;

	FWacomFirstPersonCardLayerSlotView DeferredHandTargetExitSlotView;
	TOptional<FWacomFirstPersonCardTransitionMotionProfile> DeferredHandTargetExitProfile;
	EWacomFirstPersonCardSlotTransitionKind DeferredHandTargetExitTransitionKind =
		EWacomFirstPersonCardSlotTransitionKind::Default;
	bool bHandTargetImpactDeparturePending = false;
	bool bHandTargetImpactDepartureOwnedByPileTransfer = false;
	bool bHandTargetImpactDepartureGateReleased = false;

	bool bUsesSurfaceDepartureExit = false;
	EWacomFirstPersonCardSlotTransitionKind SurfaceDepartureTransitionKind =
		EWacomFirstPersonCardSlotTransitionKind::Default;
	FVector2D EnterTransitionStartWidgetPosition = FVector2D::ZeroVector;

	void Reset();
};

struct FWacomFirstPersonCardSlotPresentationActivityInput
{
	bool bSlotExiting = false;
};

struct FWacomFirstPersonCardSlotPresentationActivityView
{
	bool bNeedsTick = false;
	bool bBlocksPresentation = false;
};

/**
 * Owns every semantic playback for one slot. Algorithms remain specialised;
 * this object centralises lifetime, readiness, surface ownership and reset.
 */
class WACOMAPP_API FWacomFirstPersonCardSlotPresentationController
{
public:
	FWacomFirstPersonCardSlotPresentationController();
	~FWacomFirstPersonCardSlotPresentationController();

	void ResetOwnedState();
	FWacomFirstPersonCardSlotPresentationActivityView BuildActivityView(
		const FWacomFirstPersonCardSlotPresentationActivityInput& Input) const;

	TUniquePtr<FWacomFirstPersonCardTransitionPlayback> TransitionPlayback;
	TUniquePtr<FWacomFirstPersonCardSurfaceDeparturePlayback> SurfaceDeparturePlayback;
	TUniquePtr<FWacomFirstPersonCardUseReformPlayback> CardUseReformPlayback;
	TUniquePtr<FWacomFirstPersonCardHandTargetImpactPlayback> HandTargetImpactPlayback;
	TUniquePtr<FWacomFirstPersonCardDataRewritePlayback> DataRewritePlayback;
	TUniquePtr<FWacomFirstPersonCardEffectBadgeFeedbackPlayback> EffectBadgeFeedbackPlayback;
	TUniquePtr<FWacomFirstPersonCardDrawRevealPlayback> DrawRevealPlayback;
	TUniquePtr<FWacomFirstPersonCardGainRevealPlayback> GainRevealPlayback;
	TUniquePtr<FWacomFirstPersonCardRetainSealPlayback> RetainSealPlayback;

	FWacomFirstPersonCardPresentationReadinessCoordinator Readiness;
	FWacomFirstPersonCardSurfaceEffectArbiter SurfaceArbiter;
	FWacomFirstPersonCardSlotPresentationRuntimeState State;
};
