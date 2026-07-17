// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardSurfaceEffectArbiter.h"

EWacomFirstPersonCardSurfaceEffectOwner FWacomFirstPersonCardSurfaceEffectArbiter::Resolve(
	const FWacomFirstPersonCardSurfaceEffectClaims& Claims)
{
	if (Claims.bDeparture)
	{
		Owner = EWacomFirstPersonCardSurfaceEffectOwner::Departure;
	}
	else if (Claims.bCardUseReform)
	{
		Owner = EWacomFirstPersonCardSurfaceEffectOwner::CardUseReform;
	}
	else if (Claims.bHandTargetImpact)
	{
		Owner = EWacomFirstPersonCardSurfaceEffectOwner::HandTargetImpact;
	}
	else if (Claims.bDrawReveal)
	{
		Owner = EWacomFirstPersonCardSurfaceEffectOwner::DrawReveal;
	}
	else if (Claims.bGainReveal)
	{
		Owner = EWacomFirstPersonCardSurfaceEffectOwner::GainReveal;
	}
	else if (Claims.bRetainSeal)
	{
		Owner = EWacomFirstPersonCardSurfaceEffectOwner::RetainSeal;
	}
	else
	{
		Owner = EWacomFirstPersonCardSurfaceEffectOwner::Base;
	}
	return Owner;
}
