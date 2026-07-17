// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomFirstPersonCardPresentationReadinessGate.h"

enum class EWacomFirstPersonCardPresentationReadinessChannel : uint8
{
	Surface,
	CostDigit,
	EffectBadge
};

struct FWacomFirstPersonCardPresentationReadinessChannelState
{
	FWacomFirstPersonCardPresentationReadinessGate Gate;
	FName EffectName;
	bool bBlocksPresentationPhase = true;
};

/** Owns all transient material readiness generations for one card slot. */
class WACOMAPP_API FWacomFirstPersonCardPresentationReadinessCoordinator
{
public:
	void Begin(
		EWacomFirstPersonCardPresentationReadinessChannel Channel,
		uint32 Generation,
		FName EffectName,
		bool bBlocksPresentationPhase,
		bool bAlreadyReady);
	EWacomFirstPersonCardPresentationReadinessPollResult Poll(
		EWacomFirstPersonCardPresentationReadinessChannel Channel,
		float DeltaTime,
		bool bMaterialReady,
		bool bPaintReady);
	void Reset(EWacomFirstPersonCardPresentationReadinessChannel Channel);
	void ResetAll();

	bool IsActive(EWacomFirstPersonCardPresentationReadinessChannel Channel) const;
	bool IsPending(EWacomFirstPersonCardPresentationReadinessChannel Channel) const;
	bool IsAnyPending() const;
	bool IsAnyBlockingPending() const;
	bool IsOwnedBy(
		EWacomFirstPersonCardPresentationReadinessChannel Channel,
		FName EffectName) const;
	uint32 GetGeneration(EWacomFirstPersonCardPresentationReadinessChannel Channel) const;
	FName GetEffectName(EWacomFirstPersonCardPresentationReadinessChannel Channel) const;
	EWacomFirstPersonCardPresentationReadinessState GetState(
		EWacomFirstPersonCardPresentationReadinessChannel Channel) const;

private:
	FWacomFirstPersonCardPresentationReadinessChannelState& Resolve(
		EWacomFirstPersonCardPresentationReadinessChannel Channel);
	const FWacomFirstPersonCardPresentationReadinessChannelState& Resolve(
		EWacomFirstPersonCardPresentationReadinessChannel Channel) const;

	FWacomFirstPersonCardPresentationReadinessChannelState Surface;
	FWacomFirstPersonCardPresentationReadinessChannelState CostDigit;
	FWacomFirstPersonCardPresentationReadinessChannelState EffectBadge;
};
