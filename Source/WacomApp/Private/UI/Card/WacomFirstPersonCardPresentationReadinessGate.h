// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

enum class EWacomFirstPersonCardPresentationReadinessState : uint8
{
	Inactive,
	WaitingForMaterial,
	WaitingForPaint,
	Ready,
	Failed
};

enum class EWacomFirstPersonCardPresentationReadinessPollResult : uint8
{
	Inactive,
	Waiting,
	BecameReady,
	Ready,
	Failed
};

/**
 * App-private readiness state for a transient first-person card material.
 *
 * The gate intentionally owns no UObject or widget reference. The slot supplies
 * the current material/paint facts each tick, which keeps teardown deterministic
 * and gives automation a small timing seam independent from Slate internals.
 */
class FWacomFirstPersonCardPresentationReadinessGate
{
public:
	static constexpr float DefaultTimeoutSeconds = 0.75f;

	void Begin(uint32 InGeneration, bool bAlreadyReady = false)
	{
		Generation = InGeneration;
		ElapsedSeconds = 0.0f;
		bReadyEdgePending = bAlreadyReady;
		State = InGeneration == 0
			? EWacomFirstPersonCardPresentationReadinessState::Failed
			: (bAlreadyReady
				? EWacomFirstPersonCardPresentationReadinessState::Ready
				: EWacomFirstPersonCardPresentationReadinessState::WaitingForMaterial);
	}
	EWacomFirstPersonCardPresentationReadinessPollResult Poll(
		float DeltaTime,
		bool bMaterialReady,
		bool bPaintReady)
	{
		if (State == EWacomFirstPersonCardPresentationReadinessState::Inactive)
		{
			return EWacomFirstPersonCardPresentationReadinessPollResult::Inactive;
		}
		if (State == EWacomFirstPersonCardPresentationReadinessState::Failed)
		{
			return EWacomFirstPersonCardPresentationReadinessPollResult::Failed;
		}
		if (State == EWacomFirstPersonCardPresentationReadinessState::Ready)
		{
			if (bReadyEdgePending)
			{
				bReadyEdgePending = false;
				return EWacomFirstPersonCardPresentationReadinessPollResult::BecameReady;
			}
			return EWacomFirstPersonCardPresentationReadinessPollResult::Ready;
		}

		ElapsedSeconds += FMath::Max(0.0f, DeltaTime);
		if (!bMaterialReady)
		{
			State = EWacomFirstPersonCardPresentationReadinessState::WaitingForMaterial;
		}
		else if (!bPaintReady)
		{
			State = EWacomFirstPersonCardPresentationReadinessState::WaitingForPaint;
		}
		else
		{
			State = EWacomFirstPersonCardPresentationReadinessState::Ready;
			bReadyEdgePending = false;
			return EWacomFirstPersonCardPresentationReadinessPollResult::BecameReady;
		}

		if (ElapsedSeconds >= DefaultTimeoutSeconds)
		{
			State = EWacomFirstPersonCardPresentationReadinessState::Failed;
			return EWacomFirstPersonCardPresentationReadinessPollResult::Failed;
		}
		return EWacomFirstPersonCardPresentationReadinessPollResult::Waiting;
	}
	void Reset()
	{
		State = EWacomFirstPersonCardPresentationReadinessState::Inactive;
		Generation = 0;
		ElapsedSeconds = 0.0f;
		bReadyEdgePending = false;
	}

	bool IsActive() const
	{
		return State != EWacomFirstPersonCardPresentationReadinessState::Inactive;
	}
	bool IsPending() const
	{
		return State == EWacomFirstPersonCardPresentationReadinessState::WaitingForMaterial
			|| State == EWacomFirstPersonCardPresentationReadinessState::WaitingForPaint;
	}
	bool IsReady() const
	{
		return State == EWacomFirstPersonCardPresentationReadinessState::Ready;
	}
	bool HasFailed() const
	{
		return State == EWacomFirstPersonCardPresentationReadinessState::Failed;
	}
	uint32 GetGeneration() const { return Generation; }
	float GetElapsedSeconds() const { return ElapsedSeconds; }
	EWacomFirstPersonCardPresentationReadinessState GetState() const { return State; }

private:
	EWacomFirstPersonCardPresentationReadinessState State =
		EWacomFirstPersonCardPresentationReadinessState::Inactive;
	uint32 Generation = 0;
	float ElapsedSeconds = 0.0f;
	bool bReadyEdgePending = false;
};
