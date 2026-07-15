// Copyright Wacom. All Rights Reserved.

#include "UI/Map/WacomRunMapEdgeLayerWidget.h"

#include "Rendering/DrawElements.h"

void UWacomRunMapEdgeLayerWidget::ApplyEdges(
	const TArray<FWacomRunMapEdgeViewData>& InEdges)
{
	Edges = InEdges;
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 UWacomRunMapEdgeLayerWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	const int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	const bool bParentEnabled) const
{
	int32 MaxLayerId = Super::NativePaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId,
		InWidgetStyle,
		bParentEnabled);

	const FLinearColor EdgeColor(0.10f, 0.62f, 0.68f, 0.72f);
	for (const FWacomRunMapEdgeViewData& Edge : Edges)
	{
		const FVector2D Delta = Edge.TargetDesignPosition - Edge.SourceDesignPosition;
		if (Delta.SizeSquared() <= 1.0f)
		{
			continue;
		}

		const FVector2D Direction = Delta.GetSafeNormal();
		const FVector2D Perpendicular(-Direction.Y, Direction.X);
		TArray<FVector2D> Line{ Edge.SourceDesignPosition, Edge.TargetDesignPosition };
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			MaxLayerId + 1,
			AllottedGeometry.ToPaintGeometry(),
			Line,
			ESlateDrawEffect::None,
			EdgeColor,
			true,
			3.0f);

		constexpr float ArrowLength = 18.0f;
		constexpr float ArrowWidth = 9.0f;
		const FVector2D HeadBase = Edge.TargetDesignPosition - Direction * ArrowLength;
		TArray<FVector2D> LeftHead{
			Edge.TargetDesignPosition,
			HeadBase + Perpendicular * ArrowWidth };
		TArray<FVector2D> RightHead{
			Edge.TargetDesignPosition,
			HeadBase - Perpendicular * ArrowWidth };
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			MaxLayerId + 2,
			AllottedGeometry.ToPaintGeometry(),
			LeftHead,
			ESlateDrawEffect::None,
			EdgeColor,
			true,
			3.0f);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			MaxLayerId + 2,
			AllottedGeometry.ToPaintGeometry(),
			RightHead,
			ESlateDrawEffect::None,
			EdgeColor,
			true,
			3.0f);
		MaxLayerId += 2;
	}
	return MaxLayerId;
}
