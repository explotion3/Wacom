// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartPresentationBounds.h"

namespace
{
	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z)
			&& !Value.ContainsNaN();
	}

	float ProjectHalfExtent(
		const FVector& PlaneAxis,
		const FVector& HalfAxisX,
		const FVector& HalfAxisY,
		const FVector& HalfAxisZ)
	{
		const FVector UnitAxis = PlaneAxis.GetSafeNormal();
		if (UnitAxis.IsNearlyZero())
		{
			return 0.0f;
		}
		return FMath::Abs(FVector::DotProduct(UnitAxis, HalfAxisX))
			+ FMath::Abs(FVector::DotProduct(UnitAxis, HalfAxisY))
			+ FMath::Abs(FVector::DotProduct(UnitAxis, HalfAxisZ));
	}
}

FWacomBattleEnemyPartPresentationBounds
FWacomBattleEnemyPartPresentationBounds::FromLocalBounds(
	const FBoxSphereBounds& LocalBounds,
	const FTransform& WorldTransform,
	FName Source)
{
	FWacomBattleEnemyPartPresentationBounds Result;
	const FVector LocalExtent = LocalBounds.BoxExtent.GetAbs();
	if (!IsFiniteVector(LocalBounds.Origin)
		|| !IsFiniteVector(LocalExtent)
		|| LocalExtent.GetMax() <= UE_SMALL_NUMBER)
	{
		return Result;
	}

	Result.WorldCenter = WorldTransform.TransformPosition(LocalBounds.Origin);
	Result.WorldHalfAxisX = WorldTransform.TransformVector(
		FVector(LocalExtent.X, 0.0f, 0.0f));
	Result.WorldHalfAxisY = WorldTransform.TransformVector(
		FVector(0.0f, LocalExtent.Y, 0.0f));
	Result.WorldHalfAxisZ = WorldTransform.TransformVector(
		FVector(0.0f, 0.0f, LocalExtent.Z));
	if (!IsFiniteVector(Result.WorldCenter)
		|| !IsFiniteVector(Result.WorldHalfAxisX)
		|| !IsFiniteVector(Result.WorldHalfAxisY)
		|| !IsFiniteVector(Result.WorldHalfAxisZ)
		|| FMath::Max3(
			Result.WorldHalfAxisX.SizeSquared(),
			Result.WorldHalfAxisY.SizeSquared(),
			Result.WorldHalfAxisZ.SizeSquared()) <= FMath::Square(UE_SMALL_NUMBER))
	{
		return FWacomBattleEnemyPartPresentationBounds();
	}

	Result.Source = Source;
	Result.bValid = true;
	return Result;
}

FVector2D FWacomBattleEnemyPartPresentationBounds::ProjectSizeCentimeters(
	const FVector& PlaneRight,
	const FVector& PlaneUp) const
{
	if (!bValid)
	{
		return FVector2D::ZeroVector;
	}
	const float Width = 2.0f * ProjectHalfExtent(
		PlaneRight,
		WorldHalfAxisX,
		WorldHalfAxisY,
		WorldHalfAxisZ);
	const float Height = 2.0f * ProjectHalfExtent(
		PlaneUp,
		WorldHalfAxisX,
		WorldHalfAxisY,
		WorldHalfAxisZ);
	return FMath::IsFinite(Width) && FMath::IsFinite(Height)
		? FVector2D(Width, Height)
		: FVector2D::ZeroVector;
}

float FWacomBattleEnemyPartPresentationBounds::ProjectShorterAxisCentimeters(
	const FVector& PlaneRight,
	const FVector& PlaneUp) const
{
	const FVector2D Size = ProjectSizeCentimeters(PlaneRight, PlaneUp);
	return FMath::Min(Size.X, Size.Y);
}

float FWacomBattleEnemyPartPresentationBounds::ProjectDiameterCentimeters(
	const FVector& PlaneRight,
	const FVector& PlaneUp) const
{
	const FVector2D Size = ProjectSizeCentimeters(PlaneRight, PlaneUp);
	return FMath::Max(Size.X, Size.Y);
}
