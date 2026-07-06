// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomBackpackScreen;
class UWacomCardDetailPanel;
class UWacomDeckCardWidget;

class FWacomBackpackCardDetailController
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

private:
	UWacomBackpackScreen& Screen;
};
