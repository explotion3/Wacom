// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDCardPresentationAnchors.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Widget.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"
#include "UI/Common/PileCountView.h"

namespace
{
	bool IsFiniteWidgetPosition(const FVector2D& Position)
	{
		return FMath::IsFinite(Position.X) && FMath::IsFinite(Position.Y);
	}

	FWacomFirstPersonCardPresentationAnchorPoint ResolveWidgetCenter(
		UBattleHUD& HUD,
		const UWidget* Widget)
	{
		FWacomFirstPersonCardPresentationAnchorPoint Result;
		if (!Widget || Widget->GetVisibility() == ESlateVisibility::Collapsed)
		{
			return Result;
		}

		const FGeometry Geometry = Widget->GetCachedGeometry();
		const FVector2D LocalSize = Geometry.GetLocalSize();
		if (!IsFiniteWidgetPosition(LocalSize) || LocalSize.X <= 0.0f || LocalSize.Y <= 0.0f)
		{
			return Result;
		}

		const FVector2D AbsoluteCenter = Geometry.LocalToAbsolute(LocalSize * 0.5f);
		FVector2D PixelPosition = FVector2D::ZeroVector;
		FVector2D ViewportPosition = FVector2D::ZeroVector;
		USlateBlueprintLibrary::AbsoluteToViewport(
			&HUD,
			AbsoluteCenter,
			PixelPosition,
			ViewportPosition);
		if (!IsFiniteWidgetPosition(ViewportPosition))
		{
			return Result;
		}

		Result.bValid = true;
		Result.WidgetPosition = ViewportPosition;
		return Result;
	}

	FWacomFirstPersonCardPresentationAnchorPoint ResolveWithFallback(
		UBattleHUD& HUD,
		const UWidget* DedicatedAnchor,
		const UWidget* FallbackWidget)
	{
		FWacomFirstPersonCardPresentationAnchorPoint Result =
			ResolveWidgetCenter(HUD, DedicatedAnchor);
		return Result.bValid ? Result : ResolveWidgetCenter(HUD, FallbackWidget);
	}
}

FWacomFirstPersonCardPresentationAnchorSet FWacomBattleHUDCardPresentationAnchors::Build(
	UBattleHUD& HUD,
	const FWacomBattleHUDRuntimeHost& Host)
{
	FWacomFirstPersonCardPresentationAnchorSet Result;
	Result.DrawPile = ResolveWithFallback(
		HUD,
		Host.GetDrawPileMotionAnchor(),
		Host.GetDrawPileView());
	Result.DiscardPile = ResolveWithFallback(
		HUD,
		Host.GetDiscardPileMotionAnchor(),
		Host.GetDiscardPileView());
	Result.PlayTarget = ResolveWidgetCenter(HUD, Host.GetPlayTargetMotionAnchor());
	return Result;
}
