// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardSlotPresentationController.h"

#include "UI/Card/WacomFirstPersonCardDataRewritePlayback.h"
#include "UI/Card/WacomFirstPersonCardDrawRevealPlayback.h"
#include "UI/Card/WacomFirstPersonCardEffectBadgeFeedbackPlayback.h"
#include "UI/Card/WacomFirstPersonCardGainRevealPlayback.h"
#include "UI/Card/WacomFirstPersonCardHandTargetImpactPlayback.h"
#include "UI/Card/WacomFirstPersonCardRetainSealPlayback.h"
#include "UI/Card/WacomFirstPersonCardSurfaceDeparturePlayback.h"
#include "UI/Card/WacomFirstPersonCardTransitionPlayback.h"
#include "UI/Card/WacomFirstPersonCardUseReformPlayback.h"

void FWacomFirstPersonCardSlotPresentationRuntimeState::Reset()
{
	*this = FWacomFirstPersonCardSlotPresentationRuntimeState();
}

FWacomFirstPersonCardSlotPresentationController::FWacomFirstPersonCardSlotPresentationController()
	: TransitionPlayback(MakeUnique<FWacomFirstPersonCardTransitionPlayback>())
{
}

FWacomFirstPersonCardSlotPresentationController::~FWacomFirstPersonCardSlotPresentationController() = default;

void FWacomFirstPersonCardSlotPresentationController::ResetOwnedState()
{
	Readiness.ResetAll();
	SurfaceArbiter.Reset();
	State.Reset();
	if (TransitionPlayback)
	{
		TransitionPlayback->Reset();
	}
	if (SurfaceDeparturePlayback)
	{
		SurfaceDeparturePlayback->Reset();
	}
	if (CardUseReformPlayback)
	{
		CardUseReformPlayback->Reset();
	}
	if (HandTargetImpactPlayback)
	{
		HandTargetImpactPlayback->Reset();
	}
	if (DataRewritePlayback)
	{
		DataRewritePlayback->Reset();
	}
	if (EffectBadgeFeedbackPlayback)
	{
		EffectBadgeFeedbackPlayback->Reset();
	}
	if (DrawRevealPlayback)
	{
		DrawRevealPlayback->Reset();
	}
	if (GainRevealPlayback)
	{
		GainRevealPlayback->Reset();
	}
	if (RetainSealPlayback)
	{
		RetainSealPlayback->Reset();
	}
}

FWacomFirstPersonCardSlotPresentationActivityView
FWacomFirstPersonCardSlotPresentationController::BuildActivityView(
	const FWacomFirstPersonCardSlotPresentationActivityInput& Input) const
{
	const bool bEnterActive = TransitionPlayback && TransitionPlayback->IsEnterActive();
	const bool bCardUseReformBlocking =
		CardUseReformPlayback && CardUseReformPlayback->IsBlockingStage();
	const bool bHandTargetActive =
		HandTargetImpactPlayback && HandTargetImpactPlayback->IsActive();
	const bool bHandTargetCommitActive =
		HandTargetImpactPlayback && HandTargetImpactPlayback->IsCommitActive();
	const bool bDataRewriteActive = DataRewritePlayback && DataRewritePlayback->IsActive();
	const bool bEffectBadgeActive =
		EffectBadgeFeedbackPlayback && EffectBadgeFeedbackPlayback->IsActive();
	const bool bDrawRevealActive = DrawRevealPlayback && DrawRevealPlayback->IsActive();
	const bool bGainRevealActive = GainRevealPlayback && GainRevealPlayback->IsActive();
	const bool bRetainBlocking =
		RetainSealPlayback && RetainSealPlayback->IsBlockingPresentation();

	FWacomFirstPersonCardSlotPresentationActivityView View;
	View.bNeedsTick = Readiness.IsAnyPending()
		|| bEnterActive
		|| Input.bSlotExiting
		|| bCardUseReformBlocking
		|| bHandTargetActive
		|| bDataRewriteActive
		|| bEffectBadgeActive
		|| State.bPendingDataRewriteHandoff
		|| State.bHandTargetImpactDeparturePending
		|| bDrawRevealActive
		|| bGainRevealActive
		|| bRetainBlocking;
	View.bBlocksPresentation = Readiness.IsAnyBlockingPending()
		|| bEnterActive
		|| Input.bSlotExiting
		|| bCardUseReformBlocking
		|| bHandTargetCommitActive
		|| State.bHandTargetImpactDeparturePending
		|| bRetainBlocking
		|| (State.bDataRewriteBlocksPresentationPhase
			&& (bDataRewriteActive || State.bPendingDataRewriteHandoff))
		|| (State.bEffectBadgeFeedbackBlocksPresentationPhase && bEffectBadgeActive);
	return View;
}
