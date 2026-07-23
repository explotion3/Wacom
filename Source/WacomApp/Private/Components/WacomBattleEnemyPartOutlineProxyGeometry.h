// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Reflection-free frame shared by the runtime outline proxy and its contract tests. */
struct WACOMAPP_API FWacomBattleEnemyPartOutlineProxyFrame
{
	FVector2D LocalCenter = FVector2D::ZeroVector;
	FVector2D SourceSizeUnrealUnits = FVector2D::ZeroVector;
	FVector2D PaddedSizeUnrealUnits = FVector2D::ZeroVector;
	FVector2D SourcePixelSize = FVector2D::ZeroVector;
	FVector2D AtlasUVOrigin = FVector2D::ZeroVector;
	FVector2D AtlasUVAxisX = FVector2D::ZeroVector;
	FVector2D AtlasUVAxisY = FVector2D::ZeroVector;
	FVector2D CanvasToSourceScale = FVector2D::UnitVector;

	bool IsValid() const;
};

namespace WacomBattleEnemyPartOutlineProxyGeometry
{
	/**
	 * Resolves a padded proxy quad from Paper2D's baked XY/UV triangles.
	 * The proxy gains transparent canvas space while the sampled source silhouette
	 * stays at the original world-space size and pivot.
	 */
	WACOMAPP_API bool BuildFrame(
		TConstArrayView<FVector4> BakedRenderData,
		const FVector2D& AtlasStartUV,
		const FVector2D& AtlasSizeUV,
		const FVector2D& TextureSizePixels,
		float UnrealUnitsPerPixel,
		float PaddingSourcePixels,
		FWacomBattleEnemyPartOutlineProxyFrame& OutFrame);

	WACOMAPP_API FVector2D MapProxyUVToSourceLocal(
		const FWacomBattleEnemyPartOutlineProxyFrame& Frame,
		const FVector2D& ProxyUV);

	/** Maps the Basic Plane XY surface onto the Paper2D local XZ surface. */
	WACOMAPP_API FRotator ResolvePlaneToSpriteRotation();
}
