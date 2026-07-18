// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardInteractionCuePainter.h"

#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"

int32 FWacomFirstPersonCardInteractionCuePainter::PaintCue(
	const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements,
	int32 BaseLayerId,
	const FSlateRect& CueRect,
	const FWacomFirstPersonCardInteractionCueView& View,
	float WidgetOpacity)
{
	const float RectWidth = FMath::Max(0.0f, CueRect.Right - CueRect.Left);
	const float RectHeight = FMath::Max(0.0f, CueRect.Bottom - CueRect.Top);
	const float ShortSide = FMath::Min(RectWidth, RectHeight);
	if (ShortSide <= KINDA_SMALL_NUMBER || View.Amount <= KINDA_SMALL_NUMBER)
	{
		return BaseLayerId;
	}

	const float Progress = FMath::Clamp(View.Progress, 0.0f, 1.0f);
	float AnimatedInset = View.CornerInsetPixels;
	if (View.Kind == EWacomFirstPersonCardInteractionCueKind::InvalidPreview)
	{
		AnimatedInset += View.TightenPixels * (View.bReducedMotion ? 1.0f : Progress);
	}
	else if (View.Kind == EWacomFirstPersonCardInteractionCueKind::Deny
		&& !View.bReducedMotion)
	{
		AnimatedInset += 3.0f * FMath::Sin(FMath::Min(1.0f, Progress * 2.0f) * PI);
	}
	const float Inset = FMath::RoundToFloat(FMath::Clamp(
		AnimatedInset,
		0.0f,
		0.25f * ShortSide));
	const float Thickness = FMath::Clamp(
		FMath::RoundToFloat(View.CornerThicknessPixels),
		1.0f,
		FMath::Max(1.0f, 0.25f * ShortSide));
	const float Length = FMath::Clamp(
		FMath::RoundToFloat(View.CornerLengthPixels),
		Thickness,
		FMath::Max(Thickness, 0.4f * ShortSide));
	const float Left = FMath::RoundToFloat(CueRect.Left + Inset);
	const float Right = FMath::RoundToFloat(CueRect.Right - Inset);
	const float Top = FMath::RoundToFloat(CueRect.Top + Inset);
	const float Bottom = FMath::RoundToFloat(CueRect.Bottom - Inset);
	FLinearColor PrimaryColor = View.Color;
	PrimaryColor.A *= FMath::Clamp(View.Amount, 0.0f, 1.0f)
		* FMath::Clamp(WidgetOpacity, 0.0f, 1.0f);
	FLinearColor AccentColor = View.AccentColor;
	AccentColor.A *= FMath::Clamp(View.Amount, 0.0f, 1.0f)
		* FMath::Clamp(WidgetOpacity, 0.0f, 1.0f);
	if (PrimaryColor.A <= KINDA_SMALL_NUMBER)
	{
		return BaseLayerId;
	}

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const int32 CueLayer = BaseLayerId + 1;
	auto DrawCueRect = [&](const FVector2D& InPosition, const FVector2D& InSize, const FLinearColor& Color)
	{
		const float ClampedLeft = FMath::RoundToFloat(FMath::Clamp(
			InPosition.X, CueRect.Left, CueRect.Right));
		const float ClampedTop = FMath::RoundToFloat(FMath::Clamp(
			InPosition.Y, CueRect.Top, CueRect.Bottom));
		const float ClampedRight = FMath::RoundToFloat(FMath::Clamp(
			InPosition.X + InSize.X, CueRect.Left, CueRect.Right));
		const float ClampedBottom = FMath::RoundToFloat(FMath::Clamp(
			InPosition.Y + InSize.Y, CueRect.Top, CueRect.Bottom));
		const FVector2D Position(ClampedLeft, ClampedTop);
		const FVector2D Size(
			FMath::Max(0.0f, ClampedRight - ClampedLeft),
			FMath::Max(0.0f, ClampedBottom - ClampedTop));
		if (Size.X <= KINDA_SMALL_NUMBER || Size.Y <= KINDA_SMALL_NUMBER)
		{
			return;
		}
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			CueLayer,
			AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)),
			WhiteBrush,
			ESlateDrawEffect::None,
			Color);
	};

	DrawCueRect(FVector2D(Left, Top), FVector2D(Length, Thickness), PrimaryColor);
	DrawCueRect(FVector2D(Left, Top), FVector2D(Thickness, Length), PrimaryColor);
	DrawCueRect(FVector2D(Right - Length, Top), FVector2D(Length, Thickness), PrimaryColor);
	DrawCueRect(FVector2D(Right - Thickness, Top), FVector2D(Thickness, Length), PrimaryColor);
	DrawCueRect(FVector2D(Left, Bottom - Thickness), FVector2D(Length, Thickness), PrimaryColor);
	DrawCueRect(FVector2D(Left, Bottom - Length), FVector2D(Thickness, Length), PrimaryColor);
	DrawCueRect(FVector2D(Right - Length, Bottom - Thickness), FVector2D(Length, Thickness), PrimaryColor);
	DrawCueRect(FVector2D(Right - Thickness, Bottom - Length), FVector2D(Thickness, Length), PrimaryColor);

	const float AccentSize = FMath::Max(1.0f, Thickness - 1.0f);
	DrawCueRect(
		FVector2D(Left + Length + Thickness, Top),
		FVector2D(AccentSize * 2.0f, AccentSize),
		AccentColor);
	DrawCueRect(
		FVector2D(Right - Length - Thickness - AccentSize * 2.0f, Bottom - AccentSize),
		FVector2D(AccentSize * 2.0f, AccentSize),
		AccentColor);

	if (View.Kind == EWacomFirstPersonCardInteractionCueKind::Deny)
	{
		const float CrackEnvelope = View.bReducedMotion
			? 1.0f - Progress
			: FMath::SmoothStep(0.03f, 0.28f, Progress)
				* (1.0f - FMath::SmoothStep(0.62f, 1.0f, Progress));
		const float CrackLength = FMath::Clamp(
			FMath::RoundToFloat(View.CrackLengthPixels * FMath::Max(0.2f, CrackEnvelope)),
			0.0f,
			0.45f * ShortSide);
		const float CrackThickness = FMath::Clamp(
			FMath::RoundToFloat(View.CrackThicknessPixels),
			1.0f,
			FMath::Max(1.0f, 0.12f * ShortSide));
		FLinearColor CrackPrimary = PrimaryColor;
		FLinearColor CrackAccent = AccentColor;
		CrackPrimary.A *= CrackEnvelope;
		CrackAccent.A *= CrackEnvelope;
		const FVector2D Direction = View.Direction.GetSafeNormal();
		const bool bHorizontalEdge = FMath::Abs(Direction.X) > FMath::Abs(Direction.Y);
		const int32 TangentSign = (View.Seed & 1) == 0 ? 1 : -1;
		const float SegmentLength = FMath::Max(CrackThickness, CrackLength * 0.25f);
		FVector2D Cursor;
		FVector2D Inward;
		FVector2D Tangent;
		if (bHorizontalEdge)
		{
			const bool bFromRight = Direction.X >= 0.0f;
			Cursor = FVector2D(bFromRight ? CueRect.Right - CrackThickness : CueRect.Left, CueRect.GetCenter().Y);
			Inward = FVector2D(bFromRight ? -1.0f : 1.0f, 0.0f);
			Tangent = FVector2D(0.0f, static_cast<float>(TangentSign));
		}
		else
		{
			const bool bFromBottom = Direction.Y >= 0.0f;
			Cursor = FVector2D(CueRect.GetCenter().X, bFromBottom ? CueRect.Bottom - CrackThickness : CueRect.Top);
			Inward = FVector2D(0.0f, bFromBottom ? -1.0f : 1.0f);
			Tangent = FVector2D(static_cast<float>(TangentSign), 0.0f);
		}
		for (int32 SegmentIndex = 0; SegmentIndex < 4; ++SegmentIndex)
		{
			const FVector2D Step = Inward * SegmentLength;
			const FVector2D Position(
				FMath::Min(Cursor.X, Cursor.X + Step.X),
				FMath::Min(Cursor.Y, Cursor.Y + Step.Y));
			const FVector2D Size = bHorizontalEdge
				? FVector2D(FMath::Abs(Step.X) + CrackThickness, CrackThickness)
				: FVector2D(CrackThickness, FMath::Abs(Step.Y) + CrackThickness);
			DrawCueRect(Position, Size, (SegmentIndex & 1) == 0 ? CrackPrimary : CrackAccent);
			Cursor += Step;
			Cursor += Tangent * CrackThickness * ((SegmentIndex & 1) == 0 ? 1.0f : 2.0f);
		}
	}
	return CueLayer;
}
