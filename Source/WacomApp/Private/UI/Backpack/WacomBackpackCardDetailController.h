// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomBackpackScreen;
class UWacomCardDetailPanel;
class UWacomDeckCardWidget;

class WACOMAPP_API FWacomBackpackCardDetailController
{
public:
	explicit FWacomBackpackCardDetailController(UWacomBackpackScreen& InScreen);

	bool IsVisible() const;
	FText GetNameText() const;

	bool ShowForCardWidget(UWacomDeckCardWidget* SourceWidget);
	void Hide();
	void HideIfSourceRemoved(UWacomDeckCardWidget* RemovedWidget);
	UWacomCardDetailPanel* EnsurePanel();
	void PositionNear(UWacomDeckCardWidget* SourceWidget);
	static FVector2D ComputePanelPosition(
		FVector2D AnchorPosition,
		FVector2D AnchorSize,
		FVector2D LayerSize,
		FVector2D PanelSize,
		float Padding = 12.0f);

private:
	UWacomBackpackScreen& Screen;
};
