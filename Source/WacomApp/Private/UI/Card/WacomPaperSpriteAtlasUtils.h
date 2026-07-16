// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PaperSprite.h"
#include "Slate/SlateTextureAtlasInterface.h"

class UTexture;

/** App-private, reflection-free atlas data shared by local UImage effects. */
struct FWacomPaperSpriteAtlasView
{
	UTexture* Texture = nullptr;
	FVector2D StartUV = FVector2D::ZeroVector;
	FVector2D SizeUV = FVector2D::ZeroVector;

	bool IsValid() const
	{
		return ::IsValid(Texture)
			&& FMath::IsFinite(StartUV.X)
			&& FMath::IsFinite(StartUV.Y)
			&& FMath::IsFinite(SizeUV.X)
			&& FMath::IsFinite(SizeUV.Y)
			&& SizeUV.X > 0.0f
			&& SizeUV.Y > 0.0f;
	}
};

namespace WacomPaperSpriteAtlas
{
	inline bool Resolve(const UPaperSprite* Sprite, FWacomPaperSpriteAtlasView& OutView)
	{
		OutView = FWacomPaperSpriteAtlasView();
		if (!IsValid(Sprite))
		{
			return false;
		}

		const FSlateAtlasData Atlas = Sprite->GetSlateAtlasData();
		OutView.Texture = Atlas.AtlasTexture;
		OutView.StartUV = Atlas.StartUV;
		OutView.SizeUV = Atlas.SizeUV;
		return OutView.IsValid();
	}
}
