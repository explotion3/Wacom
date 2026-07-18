// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class FSlateWindowElementList;
struct FGeometry;
class FSlateRect;

/** Stateless Slate painter for hard-pixel first-person card interaction cues. */
class FWacomFirstPersonCardInteractionCuePainter
{
public:
	static int32 PaintCue(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 BaseLayerId,
		const FSlateRect& CueRect,
		const FWacomFirstPersonCardInteractionCueView& View,
		float WidgetOpacity);
};
