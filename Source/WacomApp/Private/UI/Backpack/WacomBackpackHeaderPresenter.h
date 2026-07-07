// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UTextBlock;
class UWacomBackpackZoneSectionWidget;
class UWacomRunViewModel;

struct FWacomBackpackHeaderPresenterContext
{
	UTextBlock* BattleDeckTitleText = nullptr;
	UTextBlock* BackpackTitleText = nullptr;
	UTextBlock* GoldText = nullptr;
	UWacomBackpackZoneSectionWidget* BattleDeckZoneSection = nullptr;
};

struct FWacomBackpackHeaderPresenter
{
	static void Apply(
		const FWacomBackpackHeaderPresenterContext& Context,
		const UWacomRunViewModel* ViewModel);
};
