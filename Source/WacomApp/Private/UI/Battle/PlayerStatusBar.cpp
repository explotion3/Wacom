// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/PlayerStatusBar.h"

#define LOCTEXT_NAMESPACE "WacomPlayerStatus"
#include "UI/Common/WacomProgressBar.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"

#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
#include "Snapshots/BattleSnapshot.h"

TSharedRef<SWidget> UPlayerStatusBar::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// HP
		USizeBox* HpBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HpBox"));
		HpBox->SetWidthOverride(200.0f);
		HpBox->SetHeightOverride(22.0f);
		HpBar = WidgetTree->ConstructWidget<UWacomProgressBar>(UWacomProgressBar::StaticClass(), TEXT("HpBar"));
		HpBox->AddChild(HpBar);
		Root->AddChildToVerticalBox(HpBox);

		// Shield
		ShieldText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ShieldText"));
		ShieldText->SetText(LOCTEXT("ShieldDefault", "护盾 0"));
		ShieldText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.9f, 1.0f)));
		Root->AddChildToVerticalBox(ShieldText);

		StatusList = WidgetTree->ConstructWidget<UWacomBattleStatusIconListWidget>(
			UWacomBattleStatusIconListWidget::StaticClass(),
			TEXT("StatusList"));
		if (UVerticalBoxSlot* StatusSlot = Root->AddChildToVerticalBox(StatusList))
		{
			StatusSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
			StatusSlot->SetHorizontalAlignment(HAlign_Left);
		}
	}
	return Super::RebuildWidget();
}

void UPlayerStatusBar::NativeRefreshFromSnapshot(const FBattleSnapshot& Snap)
{
	if (HpBar)
	{
		HpBar->SetValue(Snap.Player.CurrentHp, Snap.Player.MaxHp);
	}

	if (ShieldText)
	{
		if (bHideShieldWhenZero && Snap.Player.Shield <= 0)
		{
			ShieldText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			ShieldText->SetVisibility(ESlateVisibility::HitTestInvisible);
			ShieldText->SetText(FText::Format(
				LOCTEXT("ShieldFmt", "护盾 {0}"), FFormatOrderedArguments{ FFormatArgumentValue(Snap.Player.Shield) }));
		}
	}

	if (StatusList)
	{
		StatusList->SetStatuses(Snap.Player.Statuses, Snap.Player.StatusStacks);
	}
}

#undef LOCTEXT_NAMESPACE

