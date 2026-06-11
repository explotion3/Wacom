// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardAnchorDebugWidget.generated.h"

/**
 * Non-interactive HUD debug overlay for first-person card anchor projection.
 */
UCLASS()
class WACOMAPP_API UWacomFirstPersonCardAnchorDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetDebugView(const FWacomFirstPersonCardAnchorDebugView& InDebugView);

protected:
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	FWacomFirstPersonCardAnchorDebugView DebugView;
};
