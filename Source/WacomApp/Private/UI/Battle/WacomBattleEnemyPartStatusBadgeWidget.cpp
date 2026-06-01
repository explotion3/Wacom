// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyPartStatusBadgeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Common/WacomProgressBar.h"

TSharedRef<SWidget> UWacomBattleEnemyPartStatusBadgeWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_StatusBadge"));
	}

	if (!WidgetTree->RootWidget)
	{
		BadgeBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BadgeBorder"));
		BadgeBorder->SetPadding(FMargin(7.0f, 4.0f));
		WidgetTree->RootWidget = BadgeBorder;

		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
		BadgeBorder->SetContent(Content);

		PartNameTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PartNameTextBlock"));
		PartNameTextBlock->SetJustification(ETextJustify::Center);
		PartNameTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		PartNameTextBlock->SetAutoWrapText(false);
		PartNameTextBlock->SetMinDesiredWidth(120.0f);
		PartNameTextBlock->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		if (UVerticalBoxSlot* NameSlot = Content->AddChildToVerticalBox(PartNameTextBlock))
		{
			NameSlot->SetHorizontalAlignment(HAlign_Center);
		}

		HpBar = WidgetTree->ConstructWidget<UWacomProgressBar>(UWacomProgressBar::StaticClass(), TEXT("HpBar"));
		HpBar->SetFillColor(FLinearColor(0.72f, 0.12f, 0.12f, 1.0f));
		HpBar->SetTextFormat(FText::FromString(TEXT("{0}/{1}")));
		if (UVerticalBoxSlot* HpSlot = Content->AddChildToVerticalBox(HpBar))
		{
			HpSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
		}

		CoreRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CoreRow"));
		if (UVerticalBoxSlot* CoreRowSlot = Content->AddChildToVerticalBox(CoreRow))
		{
			CoreRowSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
			CoreRowSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		InitiativeTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InitiativeTextBlock"));
		InitiativeTextBlock->SetJustification(ETextJustify::Center);
		InitiativeTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.86f, 0.38f, 1.0f)));
		InitiativeTextBlock->SetAutoWrapText(false);
		InitiativeTextBlock->SetMinDesiredWidth(58.0f);
		InitiativeTextBlock->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		if (UHorizontalBoxSlot* InitiativeSlot = CoreRow->AddChildToHorizontalBox(InitiativeTextBlock))
		{
			InitiativeSlot->SetHorizontalAlignment(HAlign_Fill);
			InitiativeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		IntentTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("IntentTextBlock"));
		IntentTextBlock->SetJustification(ETextJustify::Center);
		IntentTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.92f, 1.0f, 1.0f)));
		IntentTextBlock->SetAutoWrapText(false);
		IntentTextBlock->SetMinDesiredWidth(72.0f);
		IntentTextBlock->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		if (UHorizontalBoxSlot* IntentSlot = CoreRow->AddChildToHorizontalBox(IntentTextBlock))
		{
			IntentSlot->SetPadding(FMargin(5.0f, 0.0f, 0.0f, 0.0f));
			IntentSlot->SetHorizontalAlignment(HAlign_Fill);
			IntentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		ShieldTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ShieldTextBlock"));
		ShieldTextBlock->SetJustification(ETextJustify::Center);
		ShieldTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.70f, 0.92f, 1.0f, 1.0f)));
		ShieldTextBlock->SetAutoWrapText(false);
		ShieldTextBlock->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Content->AddChildToVerticalBox(ShieldTextBlock);

		StatusTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusTextBlock"));
		StatusTextBlock->SetJustification(ETextJustify::Center);
		StatusTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.72f, 1.0f, 1.0f)));
		StatusTextBlock->SetAutoWrapText(false);
		StatusTextBlock->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Content->AddChildToVerticalBox(StatusTextBlock);
	}

	return Super::RebuildWidget();
}

void UWacomBattleEnemyPartStatusBadgeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(false);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	ApplyStatusBadgeViewToWidgets();
}

void UWacomBattleEnemyPartStatusBadgeWidget::SetStatusBadgeView(
	const FWacomBattleEnemyPartStatusBadgeView& InView)
{
	CurrentView = InView;
	ApplyStatusBadgeViewToWidgets();
	BP_OnStatusBadgeViewChanged(CurrentView);
}

void UWacomBattleEnemyPartStatusBadgeWidget::ApplyStatusBadgeViewToWidgets()
{
	SetVisibility(CurrentView.bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (BadgeBorder)
	{
		BadgeBorder->SetBrushColor(BuildBadgeColor());
	}
	if (PartNameTextBlock)
	{
		PartNameTextBlock->SetText(CurrentView.PartNameText);
	}
	if (HpBar)
	{
		HpBar->SetValue(CurrentView.CurrentHp, CurrentView.MaxHp);
	}
	if (InitiativeTextBlock)
	{
		InitiativeTextBlock->SetText(CurrentView.InitiativeText);
	}
	if (IntentTextBlock)
	{
		IntentTextBlock->SetText(CurrentView.CurrentIntentText);
		IntentTextBlock->SetVisibility(
			CurrentView.CurrentIntentText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (ShieldTextBlock)
	{
		ShieldTextBlock->SetText(CurrentView.ShieldText);
		ShieldTextBlock->SetVisibility(
			CurrentView.ShieldText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (StatusTextBlock)
	{
		StatusTextBlock->SetText(CurrentView.StatusText);
		StatusTextBlock->SetVisibility(
			CurrentView.StatusText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

FLinearColor UWacomBattleEnemyPartStatusBadgeWidget::BuildBadgeColor() const
{
	if (!CurrentView.bVisible)
	{
		return FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
	}
	if (CurrentView.bDestroyed)
	{
		return FLinearColor(0.08f, 0.08f, 0.09f, 0.82f);
	}
	return FLinearColor(0.08f, 0.10f, 0.12f, 0.82f);
}
