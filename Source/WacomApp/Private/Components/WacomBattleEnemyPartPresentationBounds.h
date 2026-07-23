// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Immutable world-space presentation bounds for one scene enemy part.
 *
 * The three half axes retain the authored visual orientation after transform,
 * so camera-plane projection does not inflate through a world AABB.
 */
struct WACOMAPP_API FWacomBattleEnemyPartPresentationBounds
{
	static FWacomBattleEnemyPartPresentationBounds FromLocalBounds(
		const FBoxSphereBounds& LocalBounds,
		const FTransform& WorldTransform,
		FName Source);

	bool IsValid() const { return bValid; }
	const FVector& GetWorldCenter() const { return WorldCenter; }
	const FVector& GetWorldHalfAxisX() const { return WorldHalfAxisX; }
	const FVector& GetWorldHalfAxisY() const { return WorldHalfAxisY; }
	const FVector& GetWorldHalfAxisZ() const { return WorldHalfAxisZ; }
	FName GetSource() const { return Source; }

	FVector2D ProjectSizeCentimeters(
		const FVector& PlaneRight,
		const FVector& PlaneUp) const;
	float ProjectShorterAxisCentimeters(
		const FVector& PlaneRight,
		const FVector& PlaneUp) const;
	float ProjectDiameterCentimeters(
		const FVector& PlaneRight,
		const FVector& PlaneUp) const;

private:
	FVector WorldCenter = FVector::ZeroVector;
	FVector WorldHalfAxisX = FVector::ZeroVector;
	FVector WorldHalfAxisY = FVector::ZeroVector;
	FVector WorldHalfAxisZ = FVector::ZeroVector;
	FName Source = NAME_None;
	bool bValid = false;
};
