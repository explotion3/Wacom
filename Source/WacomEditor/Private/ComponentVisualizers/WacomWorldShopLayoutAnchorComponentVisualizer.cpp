// Copyright Wacom. All Rights Reserved.

#include "ComponentVisualizers/WacomWorldShopLayoutAnchorComponentVisualizer.h"

#include "Actors/WacomWorldShopActor.h"
#include "CanvasTypes.h"
#include "Components/WacomWorldShopLayoutAnchorComponent.h"
#include "Engine/Engine.h"
#include "SceneManagement.h"
#include "SceneView.h"

namespace
{
	struct FWorldShopCardFrame
	{
		FVector Center = FVector::ZeroVector;
		FVector TopLeft = FVector::ZeroVector;
		FVector TopRight = FVector::ZeroVector;
		FVector BottomRight = FVector::ZeroVector;
		FVector BottomLeft = FVector::ZeroVector;
		FVector Front = FVector::ZeroVector;
		FVector Right = FVector::ZeroVector;
		FVector Up = FVector::ZeroVector;
	};

	FWorldShopCardFrame BuildCardFrame(
		const UWacomWorldShopLayoutAnchorComponent& Anchor,
		const FVector2D SizeCm)
	{
		const float HalfWidth = 0.5f * SizeCm.X;
		const float HalfHeight = 0.5f * SizeCm.Y;
		const FTransform Transform = Anchor.GetComponentTransform();

		FWorldShopCardFrame Frame;
		Frame.Center = Transform.GetLocation();
		Frame.Right = Transform.GetUnitAxis(EAxis::Y);
		Frame.Up = Transform.GetUnitAxis(EAxis::Z);
		Frame.Front = Transform.GetUnitAxis(EAxis::X);
		Frame.TopLeft =
			Frame.Center - Frame.Right * HalfWidth + Frame.Up * HalfHeight;
		Frame.TopRight =
			Frame.Center + Frame.Right * HalfWidth + Frame.Up * HalfHeight;
		Frame.BottomRight =
			Frame.Center + Frame.Right * HalfWidth - Frame.Up * HalfHeight;
		Frame.BottomLeft =
			Frame.Center - Frame.Right * HalfWidth - Frame.Up * HalfHeight;
		return Frame;
	}

	void DrawRectangle(
		const FWorldShopCardFrame& Frame,
		const FLinearColor& Color,
		const ESceneDepthPriorityGroup DepthPriority,
		const float Thickness,
		const bool bDrawDiagonals,
		FPrimitiveDrawInterface& PDI)
	{
		PDI.DrawLine(
			Frame.TopLeft,
			Frame.TopRight,
			Color,
			DepthPriority,
			Thickness);
		PDI.DrawLine(
			Frame.TopRight,
			Frame.BottomRight,
			Color,
			DepthPriority,
			Thickness);
		PDI.DrawLine(
			Frame.BottomRight,
			Frame.BottomLeft,
			Color,
			DepthPriority,
			Thickness);
		PDI.DrawLine(
			Frame.BottomLeft,
			Frame.TopLeft,
			Color,
			DepthPriority,
			Thickness);
		if (bDrawDiagonals)
		{
			PDI.DrawLine(
				Frame.TopLeft,
				Frame.BottomRight,
				Color,
				DepthPriority,
				FMath::Max(0.5f, Thickness * 0.35f));
			PDI.DrawLine(
				Frame.TopRight,
				Frame.BottomLeft,
				Color,
				DepthPriority,
				FMath::Max(0.5f, Thickness * 0.35f));
		}
	}

	void DrawCardFrame(
		const UWacomWorldShopLayoutAnchorComponent& Anchor,
		const bool bSelected,
		FPrimitiveDrawInterface& PDI)
	{
		const FWorldShopCardFrame RenderPlane = BuildCardFrame(
			Anchor,
			Anchor.GetCardPreviewSizeCm());
		const FWorldShopCardFrame VisibleProduct = BuildCardFrame(
			Anchor,
			Anchor.GetVisibleProductPreviewSizeCm());
		const ESceneDepthPriorityGroup DepthPriority = bSelected
			? SDPG_Foreground
			: SDPG_World;
		const FLinearColor RenderPlaneColor = bSelected
			? FLinearColor(0.18f, 0.42f, 0.48f, 0.9f)
			: FLinearColor(0.05f, 0.20f, 0.24f, 0.75f);
		const FLinearColor VisibleProductColor = bSelected
			? FLinearColor(0.15f, 0.95f, 1.0f, 1.0f)
			: FLinearColor(0.08f, 0.48f, 0.58f, 1.0f);
		const float VisibleThickness = bSelected ? 3.0f : 1.25f;

		DrawRectangle(
			RenderPlane,
			RenderPlaneColor,
			DepthPriority,
			bSelected ? 1.25f : 0.75f,
			/*bDrawDiagonals*/ false,
			PDI);
		DrawRectangle(
			VisibleProduct,
			VisibleProductColor,
			DepthPriority,
			VisibleThickness,
			/*bDrawDiagonals*/ true,
			PDI);

		const FVector FooterLeft =
			VisibleProduct.BottomLeft
			+ VisibleProduct.Up
				* Anchor.GetVisibleFooterPreviewHeightCm();
		const FVector FooterRight =
			VisibleProduct.BottomRight
			+ VisibleProduct.Up
				* Anchor.GetVisibleFooterPreviewHeightCm();
		PDI.DrawLine(
			FooterLeft,
			FooterRight,
			VisibleProductColor,
			DepthPriority,
			VisibleThickness);
		PDI.DrawPoint(
			VisibleProduct.Center,
			VisibleProductColor,
			bSelected ? 10.0f : 6.0f,
			DepthPriority);

		const FVector FrontEnd =
			VisibleProduct.Center
			+ VisibleProduct.Front * (bSelected ? 28.0f : 18.0f);
		PDI.DrawLine(
			VisibleProduct.Center,
			FrontEnd,
			VisibleProductColor,
			DepthPriority,
			VisibleThickness);
		PDI.DrawPoint(
			FrontEnd,
			VisibleProductColor,
			bSelected ? 8.0f : 5.0f,
			DepthPriority);
	}

	TArray<UWacomWorldShopLayoutAnchorComponent*> ResolveLayoutAnchors(
		const UWacomWorldShopLayoutAnchorComponent& SelectedAnchor)
	{
		if (const AWacomWorldShopActor* Shop =
			Cast<AWacomWorldShopActor>(SelectedAnchor.GetOwner()))
		{
			return Shop->GetOfferLayoutAnchorsSorted();
		}
		return {
			const_cast<UWacomWorldShopLayoutAnchorComponent*>(
				&SelectedAnchor),
		};
	}
}

void FWacomWorldShopLayoutAnchorComponentVisualizer::DrawVisualization(
	const UActorComponent* Component,
	const FSceneView* /*View*/,
	FPrimitiveDrawInterface* PDI)
{
	const UWacomWorldShopLayoutAnchorComponent* Anchor =
		Cast<UWacomWorldShopLayoutAnchorComponent>(Component);
	if (!Anchor || !PDI)
	{
		return;
	}

	for (const UWacomWorldShopLayoutAnchorComponent* LayoutAnchor :
		ResolveLayoutAnchors(*Anchor))
	{
		if (LayoutAnchor)
		{
			DrawCardFrame(
				*LayoutAnchor,
				LayoutAnchor == Anchor,
				*PDI);
		}
	}
}

void FWacomWorldShopLayoutAnchorComponentVisualizer::DrawVisualizationHUD(
	const UActorComponent* Component,
	const FViewport* /*Viewport*/,
	const FSceneView* View,
	FCanvas* Canvas)
{
	const UWacomWorldShopLayoutAnchorComponent* Anchor =
		Cast<UWacomWorldShopLayoutAnchorComponent>(Component);
	if (!Anchor || !View || !Canvas || !GEngine)
	{
		return;
	}

	const TArray<UWacomWorldShopLayoutAnchorComponent*> LayoutAnchors =
		ResolveLayoutAnchors(*Anchor);
	for (int32 Index = 0; Index < LayoutAnchors.Num(); ++Index)
	{
		const UWacomWorldShopLayoutAnchorComponent* LayoutAnchor =
			LayoutAnchors[Index];
		if (!LayoutAnchor)
		{
			continue;
		}

		const FWorldShopCardFrame Frame =
			BuildCardFrame(
				*LayoutAnchor,
				LayoutAnchor->GetVisibleProductPreviewSizeCm());
		FVector2D PixelPosition;
		if (!View->WorldToPixel(Frame.TopLeft, PixelPosition))
		{
			continue;
		}

		const bool bSelected = LayoutAnchor == Anchor;
		const FLinearColor LabelColor = bSelected
			? FLinearColor(0.15f, 0.95f, 1.0f, 1.0f)
			: FLinearColor(0.35f, 0.72f, 0.78f, 1.0f);
		const FString Label = FString::Printf(
			TEXT("%02d  Visible %.1fx%.1f  Plane %.1fx%.1f cm"),
			Index + 1,
			LayoutAnchor->GetVisibleProductPreviewSizeCm().X,
			LayoutAnchor->GetVisibleProductPreviewSizeCm().Y,
			LayoutAnchor->GetCardPreviewSizeCm().X,
			LayoutAnchor->GetCardPreviewSizeCm().Y);
		Canvas->DrawShadowedString(
			PixelPosition.X + 4.0f,
			PixelPosition.Y - 16.0f,
			*Label,
			GEngine->GetSmallFont(),
			LabelColor);
	}
}
