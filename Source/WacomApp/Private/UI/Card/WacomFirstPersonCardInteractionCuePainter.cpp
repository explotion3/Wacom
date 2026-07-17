// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardInteractionCuePainter.h"

#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"

int32 FWacomFirstPersonCardInteractionCuePainter::PaintCorners(
	const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements,
	int32 BaseLayerId,
	const FSlateRect& CueRect,
	const FLinearColor& Color,
	float Amount,
	float WidgetOpacity,
	float CornerInsetPixels,
	float CornerLengthPixels,
	float CornerThicknessPixels)
{
	const float RectWidth = FMath::Max(0.0f, CueRect.Right - CueRect.Left);
	const float RectHeight = FMath::Max(0.0f, CueRect.Bottom - CueRect.Top);
	const float ShortSide = FMath::Min(RectWidth, RectHeight);
	if (ShortSide <= KINDA_SMALL_NUMBER || Amount <= KINDA_SMALL_NUMBER)
	{
		return BaseLayerId;
	}

	const float Inset = FMath::Clamp(CornerInsetPixels, 0.0f, 0.25f * ShortSide);
	const float Thickness = FMath::Clamp(
		CornerThicknessPixels,
		1.0f,
		FMath::Max(1.0f, 0.25f * ShortSide));
	const float Length = FMath::Clamp(
		CornerLengthPixels,
		Thickness,
		FMath::Max(Thickness, 0.4f * ShortSide));
	const float Left = CueRect.Left + Inset;
	const float Right = CueRect.Right - Inset;
	const float Top = CueRect.Top + Inset;
	const float Bottom = CueRect.Bottom - Inset;
	FLinearColor DrawColor = Color;
	DrawColor.A *= FMath::Clamp(Amount, 0.0f, 1.0f) * FMath::Clamp(WidgetOpacity, 0.0f, 1.0f);
	if (DrawColor.A <= KINDA_SMALL_NUMBER)
	{
		return BaseLayerId;
	}

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const int32 CueLayer = BaseLayerId + 1;
	auto DrawCueRect = [&](const FVector2D& Position, const FVector2D& Size)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			CueLayer,
			AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)),
			WhiteBrush,
			ESlateDrawEffect::None,
			DrawColor);
	};

	DrawCueRect(FVector2D(Left, Top), FVector2D(Length, Thickness));
	DrawCueRect(FVector2D(Left, Top), FVector2D(Thickness, Length));
	DrawCueRect(FVector2D(Right - Length, Top), FVector2D(Length, Thickness));
	DrawCueRect(FVector2D(Right - Thickness, Top), FVector2D(Thickness, Length));
	DrawCueRect(FVector2D(Left, Bottom - Thickness), FVector2D(Length, Thickness));
	DrawCueRect(FVector2D(Left, Bottom - Length), FVector2D(Thickness, Length));
	DrawCueRect(FVector2D(Right - Length, Bottom - Thickness), FVector2D(Length, Thickness));
	DrawCueRect(FVector2D(Right - Thickness, Bottom - Length), FVector2D(Thickness, Length));
	return CueLayer;
}
