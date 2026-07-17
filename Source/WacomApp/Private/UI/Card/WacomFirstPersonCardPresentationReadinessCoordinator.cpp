// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardPresentationReadinessCoordinator.h"

void FWacomFirstPersonCardPresentationReadinessCoordinator::Begin(
	EWacomFirstPersonCardPresentationReadinessChannel Channel,
	uint32 Generation,
	FName EffectName,
	bool bBlocksPresentationPhase,
	bool bAlreadyReady)
{
	FWacomFirstPersonCardPresentationReadinessChannelState& State = Resolve(Channel);
	State.EffectName = EffectName;
	State.bBlocksPresentationPhase = bBlocksPresentationPhase;
	State.Gate.Begin(Generation, bAlreadyReady);
}

EWacomFirstPersonCardPresentationReadinessPollResult
FWacomFirstPersonCardPresentationReadinessCoordinator::Poll(
	EWacomFirstPersonCardPresentationReadinessChannel Channel,
	float DeltaTime,
	bool bMaterialReady,
	bool bPaintReady)
{
	return Resolve(Channel).Gate.Poll(DeltaTime, bMaterialReady, bPaintReady);
}

void FWacomFirstPersonCardPresentationReadinessCoordinator::Reset(
	EWacomFirstPersonCardPresentationReadinessChannel Channel)
{
	FWacomFirstPersonCardPresentationReadinessChannelState& State = Resolve(Channel);
	State.Gate.Reset();
	State.EffectName = NAME_None;
	State.bBlocksPresentationPhase = true;
}

void FWacomFirstPersonCardPresentationReadinessCoordinator::ResetAll()
{
	Reset(EWacomFirstPersonCardPresentationReadinessChannel::Surface);
	Reset(EWacomFirstPersonCardPresentationReadinessChannel::CostDigit);
	Reset(EWacomFirstPersonCardPresentationReadinessChannel::EffectBadge);
}

bool FWacomFirstPersonCardPresentationReadinessCoordinator::IsActive(
	EWacomFirstPersonCardPresentationReadinessChannel Channel) const
{
	return Resolve(Channel).Gate.IsActive();
}

bool FWacomFirstPersonCardPresentationReadinessCoordinator::IsPending(
	EWacomFirstPersonCardPresentationReadinessChannel Channel) const
{
	return Resolve(Channel).Gate.IsPending();
}

bool FWacomFirstPersonCardPresentationReadinessCoordinator::IsAnyPending() const
{
	return Surface.Gate.IsPending()
		|| CostDigit.Gate.IsPending()
		|| EffectBadge.Gate.IsPending();
}

bool FWacomFirstPersonCardPresentationReadinessCoordinator::IsAnyBlockingPending() const
{
	return (Surface.Gate.IsPending() && Surface.bBlocksPresentationPhase)
		|| (CostDigit.Gate.IsPending() && CostDigit.bBlocksPresentationPhase)
		|| (EffectBadge.Gate.IsPending() && EffectBadge.bBlocksPresentationPhase);
}

bool FWacomFirstPersonCardPresentationReadinessCoordinator::IsOwnedBy(
	EWacomFirstPersonCardPresentationReadinessChannel Channel,
	FName EffectName) const
{
	const FWacomFirstPersonCardPresentationReadinessChannelState& State = Resolve(Channel);
	return State.Gate.IsActive() && State.EffectName == EffectName;
}

uint32 FWacomFirstPersonCardPresentationReadinessCoordinator::GetGeneration(
	EWacomFirstPersonCardPresentationReadinessChannel Channel) const
{
	return Resolve(Channel).Gate.GetGeneration();
}

FName FWacomFirstPersonCardPresentationReadinessCoordinator::GetEffectName(
	EWacomFirstPersonCardPresentationReadinessChannel Channel) const
{
	return Resolve(Channel).EffectName;
}

EWacomFirstPersonCardPresentationReadinessState
FWacomFirstPersonCardPresentationReadinessCoordinator::GetState(
	EWacomFirstPersonCardPresentationReadinessChannel Channel) const
{
	return Resolve(Channel).Gate.GetState();
}

FWacomFirstPersonCardPresentationReadinessChannelState&
FWacomFirstPersonCardPresentationReadinessCoordinator::Resolve(
	EWacomFirstPersonCardPresentationReadinessChannel Channel)
{
	switch (Channel)
	{
	case EWacomFirstPersonCardPresentationReadinessChannel::CostDigit:
		return CostDigit;
	case EWacomFirstPersonCardPresentationReadinessChannel::EffectBadge:
		return EffectBadge;
	case EWacomFirstPersonCardPresentationReadinessChannel::Surface:
	default:
		return Surface;
	}
}

const FWacomFirstPersonCardPresentationReadinessChannelState&
FWacomFirstPersonCardPresentationReadinessCoordinator::Resolve(
	EWacomFirstPersonCardPresentationReadinessChannel Channel) const
{
	return const_cast<FWacomFirstPersonCardPresentationReadinessCoordinator*>(this)->Resolve(Channel);
}
