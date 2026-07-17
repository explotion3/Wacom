// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

enum class EWacomFirstPersonCardSurfaceEffectOwner : uint8
{
	Base,
	RetainSeal,
	GainReveal,
	DrawReveal,
	HandTargetImpact,
	CardUseReform,
	Departure
};

struct FWacomFirstPersonCardSurfaceEffectClaims
{
	bool bDeparture = false;
	bool bCardUseReform = false;
	bool bHandTargetImpact = false;
	bool bDrawReveal = false;
	bool bGainReveal = false;
	bool bRetainSeal = false;
};

/** Resolves the single Retainer surface owner without relying on call order. */
class WACOMAPP_API FWacomFirstPersonCardSurfaceEffectArbiter
{
public:
	EWacomFirstPersonCardSurfaceEffectOwner Resolve(
		const FWacomFirstPersonCardSurfaceEffectClaims& Claims);
	EWacomFirstPersonCardSurfaceEffectOwner GetOwner() const { return Owner; }
	void Reset() { Owner = EWacomFirstPersonCardSurfaceEffectOwner::Base; }

private:
	EWacomFirstPersonCardSurfaceEffectOwner Owner =
		EWacomFirstPersonCardSurfaceEffectOwner::Base;
};
