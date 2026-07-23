// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"

struct FWacomBattleEnemyPartInteractionQueryCandidate
{
	FHitResult Hit;
	FString StableIdentity;
	float ScreenDistanceSquared = TNumericLimits<float>::Max();
	float TraceDepth = TNumericLimits<float>::Max();
	bool bInCurrentRegistry = false;
};
/** Pure deterministic policy shared by runtime query code and focused automation specs. */
struct FWacomBattleEnemyPartInteractionQueryPolicy
{
	static bool IsEligible(
		const FWacomBattleEnemyPartInteractionQueryCandidate& Candidate,
		float OccluderDepth)
	{
		return Candidate.bInCurrentRegistry
			&& Candidate.TraceDepth >= 0.0f
			&& Candidate.TraceDepth <= OccluderDepth + UE_KINDA_SMALL_NUMBER;
	}

	static bool IsPreferred(
		const FWacomBattleEnemyPartInteractionQueryCandidate& Left,
		const FWacomBattleEnemyPartInteractionQueryCandidate& Right)
	{
		if (!FMath::IsNearlyEqual(
			Left.ScreenDistanceSquared,
			Right.ScreenDistanceSquared))
		{
			return Left.ScreenDistanceSquared < Right.ScreenDistanceSquared;
		}
		if (!FMath::IsNearlyEqual(Left.TraceDepth, Right.TraceDepth))
		{
			return Left.TraceDepth < Right.TraceDepth;
		}
		return Left.StableIdentity < Right.StableIdentity;
	}
};
