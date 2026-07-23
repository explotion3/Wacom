// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartOutlineProxyGeometry.h"

namespace
{
	struct FAffineMapping2D
	{
		FVector2D InputOrigin = FVector2D::ZeroVector;
		FVector2D InputAxisOne = FVector2D::ZeroVector;
		FVector2D InputAxisTwo = FVector2D::ZeroVector;
		FVector2D OutputOrigin = FVector2D::ZeroVector;
		FVector2D OutputAxisOne = FVector2D::ZeroVector;
		FVector2D OutputAxisTwo = FVector2D::ZeroVector;
		double Determinant = 0.0;

		bool Initialize(
			const FVector2D& InputZero,
			const FVector2D& InputOne,
			const FVector2D& InputTwo,
			const FVector2D& OutputZero,
			const FVector2D& OutputOne,
			const FVector2D& OutputTwo)
		{
			InputOrigin = InputZero;
			InputAxisOne = InputOne - InputZero;
			InputAxisTwo = InputTwo - InputZero;
			OutputOrigin = OutputZero;
			OutputAxisOne = OutputOne - OutputZero;
			OutputAxisTwo = OutputTwo - OutputZero;
			Determinant = InputAxisOne.X * InputAxisTwo.Y
				- InputAxisOne.Y * InputAxisTwo.X;
			return FMath::Abs(Determinant) > UE_DOUBLE_SMALL_NUMBER;
		}

		FVector2D Map(const FVector2D& Input) const
		{
			const FVector2D Delta = Input - InputOrigin;
			const double AlongOne = (
				Delta.X * InputAxisTwo.Y - Delta.Y * InputAxisTwo.X) / Determinant;
			const double AlongTwo = (
				InputAxisOne.X * Delta.Y - InputAxisOne.Y * Delta.X) / Determinant;
			return OutputOrigin + AlongOne * OutputAxisOne + AlongTwo * OutputAxisTwo;
		}
	};

	bool IsFiniteVector(const FVector2D& Value)
	{
		return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y);
	}
}

bool FWacomBattleEnemyPartOutlineProxyFrame::IsValid() const
{
	return IsFiniteVector(LocalCenter)
		&& IsFiniteVector(SourceSizeUnrealUnits)
		&& IsFiniteVector(PaddedSizeUnrealUnits)
		&& IsFiniteVector(SourcePixelSize)
		&& IsFiniteVector(AtlasUVOrigin)
		&& IsFiniteVector(AtlasUVAxisX)
		&& IsFiniteVector(AtlasUVAxisY)
		&& IsFiniteVector(CanvasToSourceScale)
		&& SourceSizeUnrealUnits.X > UE_SMALL_NUMBER
		&& SourceSizeUnrealUnits.Y > UE_SMALL_NUMBER
		&& PaddedSizeUnrealUnits.X >= SourceSizeUnrealUnits.X
		&& PaddedSizeUnrealUnits.Y >= SourceSizeUnrealUnits.Y
		&& SourcePixelSize.X > UE_SMALL_NUMBER
		&& SourcePixelSize.Y > UE_SMALL_NUMBER
		&& CanvasToSourceScale.X >= 1.0
		&& CanvasToSourceScale.Y >= 1.0;
}

bool WacomBattleEnemyPartOutlineProxyGeometry::BuildFrame(
	TConstArrayView<FVector4> BakedRenderData,
	const FVector2D& AtlasStartUV,
	const FVector2D& AtlasSizeUV,
	const FVector2D& TextureSizePixels,
	float UnrealUnitsPerPixel,
	float PaddingSourcePixels,
	FWacomBattleEnemyPartOutlineProxyFrame& OutFrame)
{
	OutFrame = FWacomBattleEnemyPartOutlineProxyFrame();
	if (BakedRenderData.Num() < 3
		|| !IsFiniteVector(AtlasStartUV)
		|| !IsFiniteVector(AtlasSizeUV)
		|| !IsFiniteVector(TextureSizePixels)
		|| AtlasSizeUV.X <= UE_SMALL_NUMBER
		|| AtlasSizeUV.Y <= UE_SMALL_NUMBER
		|| TextureSizePixels.X <= UE_SMALL_NUMBER
		|| TextureSizePixels.Y <= UE_SMALL_NUMBER
		|| !FMath::IsFinite(UnrealUnitsPerPixel)
		|| UnrealUnitsPerPixel <= UE_SMALL_NUMBER
		|| !FMath::IsFinite(PaddingSourcePixels)
		|| PaddingSourcePixels < 0.0f)
	{
		return false;
	}

	int32 FirstIndex = INDEX_NONE;
	int32 SecondIndex = INDEX_NONE;
	int32 ThirdIndex = INDEX_NONE;
	FAffineMapping2D AtlasToLocal;
	for (int32 IndexOne = 1; IndexOne < BakedRenderData.Num() - 1
		&& FirstIndex == INDEX_NONE; ++IndexOne)
	{
		for (int32 IndexTwo = IndexOne + 1; IndexTwo < BakedRenderData.Num(); ++IndexTwo)
		{
			const FVector4& VertexZero = BakedRenderData[0];
			const FVector4& VertexOne = BakedRenderData[IndexOne];
			const FVector4& VertexTwo = BakedRenderData[IndexTwo];
			if (AtlasToLocal.Initialize(
				FVector2D(VertexZero.Z, VertexZero.W),
				FVector2D(VertexOne.Z, VertexOne.W),
				FVector2D(VertexTwo.Z, VertexTwo.W),
				FVector2D(VertexZero.X, VertexZero.Y),
				FVector2D(VertexOne.X, VertexOne.Y),
				FVector2D(VertexTwo.X, VertexTwo.Y)))
			{
				FirstIndex = 0;
				SecondIndex = IndexOne;
				ThirdIndex = IndexTwo;
				break;
			}
		}
	}
	if (FirstIndex == INDEX_NONE)
	{
		return false;
	}

	const FVector2D AtlasCorners[] =
	{
		AtlasStartUV,
		AtlasStartUV + FVector2D(AtlasSizeUV.X, 0.0),
		AtlasStartUV + FVector2D(0.0, AtlasSizeUV.Y),
		AtlasStartUV + AtlasSizeUV,
	};
	FVector2D LocalMinimum = AtlasToLocal.Map(AtlasCorners[0]);
	FVector2D LocalMaximum = LocalMinimum;
	for (int32 CornerIndex = 1; CornerIndex < UE_ARRAY_COUNT(AtlasCorners); ++CornerIndex)
	{
		const FVector2D LocalCorner = AtlasToLocal.Map(AtlasCorners[CornerIndex]);
		LocalMinimum.X = FMath::Min(LocalMinimum.X, LocalCorner.X);
		LocalMinimum.Y = FMath::Min(LocalMinimum.Y, LocalCorner.Y);
		LocalMaximum.X = FMath::Max(LocalMaximum.X, LocalCorner.X);
		LocalMaximum.Y = FMath::Max(LocalMaximum.Y, LocalCorner.Y);
	}

	const FVector4& VertexZero = BakedRenderData[FirstIndex];
	const FVector4& VertexOne = BakedRenderData[SecondIndex];
	const FVector4& VertexTwo = BakedRenderData[ThirdIndex];
	FAffineMapping2D LocalToAtlas;
	if (!LocalToAtlas.Initialize(
		FVector2D(VertexZero.X, VertexZero.Y),
		FVector2D(VertexOne.X, VertexOne.Y),
		FVector2D(VertexTwo.X, VertexTwo.Y),
		FVector2D(VertexZero.Z, VertexZero.W),
		FVector2D(VertexOne.Z, VertexOne.W),
		FVector2D(VertexTwo.Z, VertexTwo.W)))
	{
		return false;
	}

	OutFrame.LocalCenter = (LocalMinimum + LocalMaximum) * 0.5;
	OutFrame.SourceSizeUnrealUnits = LocalMaximum - LocalMinimum;
	OutFrame.SourcePixelSize = OutFrame.SourceSizeUnrealUnits / UnrealUnitsPerPixel;
	const FVector2D PaddingUnrealUnits(PaddingSourcePixels * UnrealUnitsPerPixel);
	OutFrame.PaddedSizeUnrealUnits = OutFrame.SourceSizeUnrealUnits
		+ PaddingUnrealUnits * 2.0;
	OutFrame.AtlasUVOrigin = LocalToAtlas.Map(LocalMinimum);
	OutFrame.AtlasUVAxisX = LocalToAtlas.Map(
		LocalMinimum + FVector2D(OutFrame.SourceSizeUnrealUnits.X, 0.0))
		- OutFrame.AtlasUVOrigin;
	OutFrame.AtlasUVAxisY = LocalToAtlas.Map(
		LocalMinimum + FVector2D(0.0, OutFrame.SourceSizeUnrealUnits.Y))
		- OutFrame.AtlasUVOrigin;
	OutFrame.CanvasToSourceScale = FVector2D(
		OutFrame.PaddedSizeUnrealUnits.X / OutFrame.SourceSizeUnrealUnits.X,
		OutFrame.PaddedSizeUnrealUnits.Y / OutFrame.SourceSizeUnrealUnits.Y);
	return OutFrame.IsValid();
}

FVector2D WacomBattleEnemyPartOutlineProxyGeometry::MapProxyUVToSourceLocal(
	const FWacomBattleEnemyPartOutlineProxyFrame& Frame,
	const FVector2D& ProxyUV)
{
	return (ProxyUV - FVector2D(0.5)) * Frame.CanvasToSourceScale
		+ FVector2D(0.5);
}

FRotator WacomBattleEnemyPartOutlineProxyGeometry::ResolvePlaneToSpriteRotation()
{
	// Basic Plane UV.V follows local +Y. Negative roll maps that axis onto
	// Paper2D local +Z; positive roll mirrors the silhouette vertically.
	return FRotator(0.0, 0.0, -90.0);
}
