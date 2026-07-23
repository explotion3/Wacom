// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceOverlayPainter.h"

#include "Layout/Geometry.h"
#include "Math/TransformCalculus2D.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"

namespace
{
bool IsPaintableBrush(const FSlateBrush* Brush)
{
	return Brush
		&& Brush->DrawAs != ESlateBrushDrawType::NoDrawType
		&& (Brush->GetResourceObject() || !Brush->GetResourceName().IsNone());
}

ESlateDrawEffect ResolveDrawEffect(bool bParentEnabled)
{
	return bParentEnabled
		? ESlateDrawEffect::None
		: ESlateDrawEffect::DisabledEffect;
}

FVector2D RotateVector(FVector2D Vector, float AngleDegrees)
{
	const float Radians = FMath::DegreesToRadians(AngleDegrees);
	const float CosAngle = FMath::Cos(Radians);
	const float SinAngle = FMath::Sin(Radians);
	return FVector2D(
		Vector.X * CosAngle - Vector.Y * SinAngle,
		Vector.X * SinAngle + Vector.Y * CosAngle);
}

bool IsOrderAbove(
	const FWacomBackpackOverlayVisualOrder& Candidate,
	const FWacomBackpackOverlayVisualOrder& Reference)
{
	if (Candidate.LayerPriority != Reference.LayerPriority)
	{
		return Candidate.LayerPriority > Reference.LayerPriority;
	}
	if (Candidate.ZOrder != Reference.ZOrder)
	{
		return Candidate.ZOrder > Reference.ZOrder;
	}
	if (Candidate.ChildIndex != Reference.ChildIndex)
	{
		return Candidate.ChildIndex > Reference.ChildIndex;
	}
	return Candidate.StableIndex > Reference.StableIndex;
}

bool DoOrientedRectsIntersect(
	FVector2D LeftCenter,
	FVector2D LeftSize,
	float LeftAngleDegrees,
	FVector2D RightCenter,
	FVector2D RightSize,
	float RightAngleDegrees)
{
	const FVector2D LeftHalf = LeftSize * 0.5f;
	const FVector2D RightHalf = RightSize * 0.5f;
	if (LeftHalf.X <= UE_SMALL_NUMBER || LeftHalf.Y <= UE_SMALL_NUMBER
		|| RightHalf.X <= UE_SMALL_NUMBER || RightHalf.Y <= UE_SMALL_NUMBER)
	{
		return false;
	}

	const FVector2D LeftAxisX = RotateVector(FVector2D(1.0f, 0.0f), LeftAngleDegrees);
	const FVector2D LeftAxisY = RotateVector(FVector2D(0.0f, 1.0f), LeftAngleDegrees);
	const FVector2D RightAxisX = RotateVector(FVector2D(1.0f, 0.0f), RightAngleDegrees);
	const FVector2D RightAxisY = RotateVector(FVector2D(0.0f, 1.0f), RightAngleDegrees);
	const FVector2D Delta = RightCenter - LeftCenter;
	const FVector2D Axes[] = { LeftAxisX, LeftAxisY, RightAxisX, RightAxisY };
	for (const FVector2D& Axis : Axes)
	{
		const float CenterDistance = FMath::Abs(FVector2D::DotProduct(Delta, Axis));
		const float LeftRadius =
			FMath::Abs(FVector2D::DotProduct(LeftAxisX, Axis)) * LeftHalf.X
			+ FMath::Abs(FVector2D::DotProduct(LeftAxisY, Axis)) * LeftHalf.Y;
		const float RightRadius =
			FMath::Abs(FVector2D::DotProduct(RightAxisX, Axis)) * RightHalf.X
			+ FMath::Abs(FVector2D::DotProduct(RightAxisY, Axis)) * RightHalf.Y;
		if (CenterDistance > LeftRadius + RightRadius)
		{
			return false;
		}
	}
	return true;
}
}

int32 FWacomBackpackWorkspaceOverlayPainter::ResolveMarqueeMaxLayer(
	int32 ChildMaxLayerId,
	bool bVisible,
	bool bHasArea)
{
	return bVisible && bHasArea ? ChildMaxLayerId + 2 : ChildMaxLayerId;
}

int32 FWacomBackpackWorkspaceOverlayPainter::ResolveCardMarkerMaxLayer(
	int32 ChildMaxLayerId,
	bool bHasFocusBrush,
	bool bHasSemanticBrush)
{
	return bHasFocusBrush || bHasSemanticBrush
		? ChildMaxLayerId + 1
		: ChildMaxLayerId;
}

int32 FWacomBackpackWorkspaceOverlayPainter::PaintMarquee(
	const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements,
	int32 ChildMaxLayerId,
	const FWacomBackpackWorkspaceMarqueePaintView& View,
	float WidgetOpacity,
	bool bParentEnabled)
{
	const FVector2D Minimum(
		FMath::Min(View.Start.X, View.End.X),
		FMath::Min(View.Start.Y, View.End.Y));
	const FVector2D Size(
		FMath::Abs(View.End.X - View.Start.X),
		FMath::Abs(View.End.Y - View.Start.Y));
	const bool bHasArea = Size.X > UE_SMALL_NUMBER && Size.Y > UE_SMALL_NUMBER;
	const int32 MaxLayerId = ResolveMarqueeMaxLayer(
		ChildMaxLayerId,
		View.bVisible,
		bHasArea);
	if (MaxLayerId == ChildMaxLayerId)
	{
		return ChildMaxLayerId;
	}

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const ESlateDrawEffect DrawEffect = ResolveDrawEffect(bParentEnabled);
	FLinearColor FillColor = View.Color;
	FillColor.A *= FMath::Clamp(View.FillOpacity, 0.0f, 1.0f)
		* FMath::Clamp(WidgetOpacity, 0.0f, 1.0f);
	FLinearColor BorderColor = View.Color;
	BorderColor.A *= FMath::Clamp(WidgetOpacity, 0.0f, 1.0f);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		ChildMaxLayerId + 1,
		AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Minimum)),
		WhiteBrush,
		DrawEffect,
		FillColor);

	const float Thickness = FMath::Clamp(
		View.BorderThickness,
		1.0f,
		FMath::Max(1.0f, 0.5f * FMath::Min(Size.X, Size.Y)));
	const int32 BorderLayerId = ChildMaxLayerId + 2;
	const auto DrawBorderPiece = [&](FVector2D Position, FVector2D PieceSize)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			BorderLayerId,
			AllottedGeometry.ToPaintGeometry(
				PieceSize,
				FSlateLayoutTransform(Position)),
			WhiteBrush,
			DrawEffect,
			BorderColor);
	};
	DrawBorderPiece(Minimum, FVector2D(Size.X, Thickness));
	DrawBorderPiece(
		FVector2D(Minimum.X, Minimum.Y + Size.Y - Thickness),
		FVector2D(Size.X, Thickness));
	DrawBorderPiece(Minimum, FVector2D(Thickness, Size.Y));
	DrawBorderPiece(
		FVector2D(Minimum.X + Size.X - Thickness, Minimum.Y),
		FVector2D(Thickness, Size.Y));
	return MaxLayerId;
}

int32 FWacomBackpackWorkspaceOverlayPainter::PaintCardMarkers(
	const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements,
	int32 ChildMaxLayerId,
	const FWacomBackpackCardOverlayPaintView& View,
	const FLinearColor& WidgetTint,
	bool bParentEnabled)
{
	const bool bPaintFocus = IsPaintableBrush(View.FocusBrush);
	const bool bPaintSemantic = IsPaintableBrush(View.SemanticBrush);
	const int32 MaxLayerId = ResolveCardMarkerMaxLayer(
		ChildMaxLayerId,
		bPaintFocus,
		bPaintSemantic);
	if (MaxLayerId == ChildMaxLayerId)
	{
		return ChildMaxLayerId;
	}

	const FVector2D CardSize = FVector2D(AllottedGeometry.GetLocalSize());
	const FVector2D SafeIconSize(
		FMath::Min(FMath::Max(0.0f, View.IconSize.X), CardSize.X),
		FMath::Min(FMath::Max(0.0f, View.IconSize.Y), CardSize.Y));
	if (SafeIconSize.X <= UE_SMALL_NUMBER || SafeIconSize.Y <= UE_SMALL_NUMBER)
	{
		return ChildMaxLayerId;
	}

	const float SafePadding = FMath::Max(0.0f, View.Padding);
	const FSlateRenderTransform MotionTransform = ::Concatenate(
		FQuat2D(FMath::DegreesToRadians(View.LocalMotionAngleDegrees)),
		FVector2D(View.LocalMotionTranslation));
	const FGeometry MotionGeometry = AllottedGeometry.MakeChild(
		CardSize,
		FSlateLayoutTransform(),
		MotionTransform,
		FVector2D(0.5f, 0.5f));
	const ESlateDrawEffect DrawEffect = ResolveDrawEffect(bParentEnabled);
	const int32 MarkerLayerId = ChildMaxLayerId + 1;
	const auto DrawMarker = [&](const FSlateBrush* Brush, FVector2D Position)
	{
		if (!IsPaintableBrush(Brush))
		{
			return;
		}
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			MarkerLayerId,
			MotionGeometry.ToPaintGeometry(
				SafeIconSize,
				FSlateLayoutTransform(Position)),
			Brush,
			DrawEffect,
			WidgetTint);
	};

	DrawMarker(View.FocusBrush, FVector2D(SafePadding, SafePadding));
	DrawMarker(
		View.SemanticBrush,
		FVector2D(
			FMath::Max(SafePadding, CardSize.X - SafePadding - SafeIconSize.X),
			SafePadding));
	return MaxLayerId;
}

FVector2D FWacomBackpackWorkspaceOverlayPainter::ResolveCardMarkerCenter(
	FVector2D CardCenter,
	FVector2D CardSize,
	float CardAngleDegrees,
	bool bSemanticMarker,
	FVector2D IconSize,
	float Padding)
{
	const FVector2D HalfCard = CardSize * 0.5f;
	const FVector2D HalfIcon = IconSize * 0.5f;
	const float HorizontalOffset = bSemanticMarker
		? HalfCard.X - FMath::Max(0.0f, Padding) - HalfIcon.X
		: -HalfCard.X + FMath::Max(0.0f, Padding) + HalfIcon.X;
	const FVector2D LocalCenter(
		HorizontalOffset,
		-HalfCard.Y + FMath::Max(0.0f, Padding) + HalfIcon.Y);
	return CardCenter + RotateVector(LocalCenter, CardAngleDegrees);
}

bool FWacomBackpackWorkspaceOverlayPainter::IsMarkerOccludedByHigherCard(
	FVector2D MarkerCenter,
	FVector2D MarkerSize,
	float MarkerAngleDegrees,
	const FWacomBackpackOverlayVisualOrder& MarkerCardOrder,
	TConstArrayView<FWacomBackpackCardMarkerOccluder> CardBodies)
{
	for (const FWacomBackpackCardMarkerOccluder& CardBody : CardBodies)
	{
		if (IsOrderAbove(CardBody.Order, MarkerCardOrder)
			&& DoOrientedRectsIntersect(
				MarkerCenter,
				MarkerSize,
				MarkerAngleDegrees,
				CardBody.Center,
				CardBody.Size,
				CardBody.AngleDegrees))
		{
			return true;
		}
	}
	return false;
}
