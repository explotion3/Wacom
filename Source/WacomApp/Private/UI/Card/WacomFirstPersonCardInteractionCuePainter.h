// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FSlateWindowElementList;
struct FGeometry;
class FSlateRect;

/** Stateless Slate painter for hard-pixel first-person card interaction cues. */
class FWacomFirstPersonCardInteractionCuePainter
{
public:
	static int32 PaintCorners(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 BaseLayerId,
		const FSlateRect& CueRect,
		const FLinearColor& Color,
		float Amount,
		float WidgetOpacity,
		float CornerInsetPixels,
		float CornerLengthPixels,
		float CornerThicknessPixels);
};
