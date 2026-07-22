// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyPartPredictionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UWacomBattleEnemyPartPredictionWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Prediction"));
	}

	if (!WidgetTree->RootWidget)
	{
		BadgeBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BadgeBorder"));
		BadgeBorder->SetPadding(FMargin(7.0f, 4.0f));
		WidgetTree->RootWidget = BadgeBorder;

		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
		BadgeBorder->SetContent(Content);

		MainTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MainTextBlock"));
		MainTextBlock->SetJustification(ETextJustify::Center);
		MainTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		MainTextBlock->SetAutoWrapText(false);
		MainTextBlock->SetMinDesiredWidth(118.0f);
		MainTextBlock->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		if (UVerticalBoxSlot* MainSlot = Content->AddChildToVerticalBox(MainTextBlock))
		{
			MainSlot->SetHorizontalAlignment(HAlign_Center);
		}

		DetailTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailTextBlock"));
		DetailTextBlock->SetJustification(ETextJustify::Center);
		DetailTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.86f, 0.42f, 1.0f)));
		DetailTextBlock->SetAutoWrapText(false);
		DetailTextBlock->SetMinDesiredWidth(118.0f);
		DetailTextBlock->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		if (UVerticalBoxSlot* DetailSlot = Content->AddChildToVerticalBox(DetailTextBlock))
		{
			DetailSlot->SetHorizontalAlignment(HAlign_Center);
			DetailSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
		}
	}

	return Super::RebuildWidget();
}

void UWacomBattleEnemyPartPredictionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(false);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	ApplyPredictionViewToWidgets();
}

void UWacomBattleEnemyPartPredictionWidget::SetPredictionView(
	const FWacomBattleEnemyPartPredictionView& InView)
{
	CurrentView = InView;
	ApplyPredictionViewToWidgets();
	BP_OnPredictionViewChanged(CurrentView);
}

void UWacomBattleEnemyPartPredictionWidget::ApplyPredictionViewToWidgets()
{
	SetVisibility(CurrentView.bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (BadgeBorder)
	{
		BadgeBorder->SetBrushColor(BuildBadgeColor());
	}
	if (MainTextBlock)
	{
		MainTextBlock->SetText(CurrentView.MainText);
	}
	if (DetailTextBlock)
	{
		DetailTextBlock->SetText(CurrentView.DetailText);
		DetailTextBlock->SetVisibility(
			CurrentView.DetailText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

FLinearColor UWacomBattleEnemyPartPredictionWidget::BuildBadgeColor() const
{
	switch (CurrentView.Mode)
	{
	case EWacomBattleEnemyPartPredictionMode::CardPrediction:
		if (CurrentView.bResistanceWillStun || CurrentView.bPerfectReleaseCandidate)
		{
			return FLinearColor(0.08f, 0.48f, 0.24f, 0.92f);
		}
		if (CurrentView.bActionRisk)
		{
			return FLinearColor(0.62f, 0.16f, 0.06f, 0.92f);
		}
		return FLinearColor(0.06f, 0.23f, 0.42f, 0.88f);
	case EWacomBattleEnemyPartPredictionMode::Rejected:
		return FLinearColor(0.42f, 0.06f, 0.07f, 0.90f);
	case EWacomBattleEnemyPartPredictionMode::HoverInitiative:
		return FLinearColor(0.08f, 0.09f, 0.11f, 0.76f);
	case EWacomBattleEnemyPartPredictionMode::Hidden:
	default:
		return FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
	}
}
