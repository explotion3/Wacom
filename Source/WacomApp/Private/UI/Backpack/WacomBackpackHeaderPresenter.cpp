// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackHeaderPresenter.h"

#include "Components/TextBlock.h"
#include "UI/Backpack/WacomBackpackScreenPresenter.h"
#include "UI/Backpack/WacomBackpackZoneSectionWidget.h"
#include "UI/ViewModels/WacomRunViewModel.h"

void FWacomBackpackHeaderPresenter::Apply(
	const FWacomBackpackHeaderPresenterContext& Context,
	const UWacomRunViewModel* ViewModel)
{
	if (!ViewModel)
	{
		return;
	}

	const FText BattleDeckTitle = UWacomBackpackScreenPresenter::BuildBattleDeckTitleText(
		ViewModel->GetBattleDeckCount(),
		ViewModel->GetBattleDeckCapacity());
	if (Context.BattleDeckTitleText)
	{
		Context.BattleDeckTitleText->SetText(BattleDeckTitle);
	}
	if (Context.BattleDeckZoneSection)
	{
		Context.BattleDeckZoneSection->SetZoneTitleText(BattleDeckTitle);
	}

	if (Context.BackpackTitleText)
	{
		Context.BackpackTitleText->SetText(UWacomBackpackScreenPresenter::BuildBackpackTitleText());
	}
	if (Context.GoldText)
	{
		Context.GoldText->SetText(UWacomBackpackScreenPresenter::BuildGoldText(ViewModel->GetGold()));
	}
}
