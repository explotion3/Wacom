// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardAnchorDebugWidget.h"

#include "Rendering/DrawElements.h"

namespace
{
	const FVector2D DebugPointSize(10.0f, 10.0f);
	const FLinearColor CenterPointColor(1.0f, 0.78f, 0.18f, 0.95f);
	const FLinearColor SidePointColor(0.05f, 0.9f, 1.0f, 0.85f);
}

void UWacomFirstPersonCardAnchorDebugWidget::SetDebugView(
	const FWacomFirstPersonCardAnchorDebugView& InDebugView)
{
	DebugView = InDebugView;
	InvalidateLayoutAndVolatility();
}

int32 UWacomFirstPersonCardAnchorDebugWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	int32 ResultLayer = Super::NativePaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId,
		InWidgetStyle,
		bParentEnabled);

	const int32 CenterIndex = DebugView.ProjectedPoints.Num() / 2;
	for (int32 Index = 0; Index < DebugView.ProjectedPoints.Num(); ++Index)
	{
		const FWacomFirstPersonCardProjectedPoint& Point = DebugView.ProjectedPoints[Index];
		if (!Point.bProjected)
		{
			continue;
		}

		const FVector2D LocalPosition = Point.ScreenPosition - (DebugPointSize * 0.5f);
		const FPaintGeometry PaintGeometry =
			AllottedGeometry.ToPaintGeometry(DebugPointSize, FSlateLayoutTransform(LocalPosition));
		const FLinearColor PointColor = (Index == CenterIndex) ? CenterPointColor : SidePointColor;
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			++ResultLayer,
			PaintGeometry,
			FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),
			ESlateDrawEffect::None,
			Point.bClamped ? PointColor.CopyWithNewOpacity(0.45f) : PointColor);
	}

	return ResultLayer;
}
