// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyInspectionPartRowWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UWacomBattleEnemyInspectionPartRowWidget::SetPartViewData(
	const FWacomBattleEnemyPartEntryViewData& InView)
{
	CurrentView = InView;
	bHasViewData = true;
	RefreshPresentation();
}

void UWacomBattleEnemyInspectionPartRowWidget::SetSelected(const bool bInSelected)
{
	if (bSelected == bInSelected)
	{
		return;
	}

	bSelected = bInSelected;
	RefreshPresentation();
}

void UWacomBattleEnemyInspectionPartRowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (PartSelectButton)
	{
		PartSelectButton->OnClicked.RemoveAll(this);
		PartSelectButton->OnClicked.AddDynamic(this, &ThisClass::HandleSelectClicked);
	}
	RefreshPresentation();
}

void UWacomBattleEnemyInspectionPartRowWidget::NativeDestruct()
{
	if (PartSelectButton)
	{
		PartSelectButton->OnClicked.RemoveAll(this);
	}
	OnPartSelectedNative.Clear();
	Super::NativeDestruct();
}

void UWacomBattleEnemyInspectionPartRowWidget::RefreshPresentation()
{
	if (PartNameText)
	{
		PartNameText->SetText(CurrentView.PartDisplayName.IsEmpty()
			? FText::FromName(CurrentView.PartSlotId)
			: CurrentView.PartDisplayName);
	}
	if (HpText)
	{
		HpText->SetText(FText::FromString(FString::Printf(
			TEXT("%d / %d"), CurrentView.CurrentHp, CurrentView.MaxHp)));
	}
	if (ShieldText)
	{
		ShieldText->SetText(FText::AsNumber(CurrentView.Shield));
	}
	if (ShieldContainer)
	{
		ShieldContainer->SetVisibility(CurrentView.Shield > 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (InitiativeText)
	{
		InitiativeText->SetText(FText::AsNumber(CurrentView.CurrentInitiative));
	}
	if (SelectionHighlight)
	{
		SelectionHighlight->SetVisibility(bSelected
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (DestroyedOverlay)
	{
		DestroyedOverlay->SetVisibility(CurrentView.bDestroyed
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (PartSelectButton)
	{
		PartSelectButton->SetIsEnabled(bHasViewData && CurrentView.Identity.IsValidSlot());
	}
}

void UWacomBattleEnemyInspectionPartRowWidget::HandleSelectClicked()
{
	if (bHasViewData && CurrentView.Identity.IsValidSlot())
	{
		OnPartSelectedNative.Broadcast(CurrentView.Identity);
	}
}
