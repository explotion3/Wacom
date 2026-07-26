// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardSemanticTooltipWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

UWacomCardSemanticTooltipWidget::UWacomCardSemanticTooltipWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetIsFocusable(false);
}

TSharedRef<SWidget> UWacomCardSemanticTooltipWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		USizeBox* WidthBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			TEXT("TooltipWidth"));
		WidthBox->SetWidthOverride(300.0f);
		WidgetTree->RootWidget = WidthBox;

		UBorder* Background = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("TooltipBackground"));
		Background->SetBrushColor(
			FLinearColor(0.018f, 0.021f, 0.027f, 0.97f));
		Background->SetPadding(FMargin(14.0f, 11.0f));
		Background->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidthBox->SetContent(Background);

		UVerticalBox* TextStack = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			TEXT("TooltipTextStack"));
		Background->SetContent(TextStack);

		TitleText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("TitleText"));
		FSlateFontInfo TitleFont = TitleText->GetFont();
		TitleFont.Size = 19;
		TitleText->SetFont(TitleFont);
		TitleText->SetColorAndOpacity(FSlateColor(
			FLinearColor(0.96f, 0.88f, 0.61f, 1.0f)));
		TitleText->SetAutoWrapText(true);
		TextStack->AddChildToVerticalBox(TitleText);

		DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("DescriptionText"));
		FSlateFontInfo BodyFont = DescriptionText->GetFont();
		BodyFont.Size = 16;
		DescriptionText->SetFont(BodyFont);
		DescriptionText->SetColorAndOpacity(FSlateColor(
			FLinearColor(0.92f, 0.93f, 0.95f, 1.0f)));
		DescriptionText->SetAutoWrapText(true);
		if (UVerticalBoxSlot* DescriptionSlot =
			TextStack->AddChildToVerticalBox(DescriptionText))
		{
			DescriptionSlot->SetPadding(FMargin(0.0f, 7.0f, 0.0f, 0.0f));
		}
	}
	return Super::RebuildWidget();
}

void UWacomCardSemanticTooltipWidget::SetSemanticTooltip(
	const FText& InTitle,
	const FText& InDescription,
	const float InWidthPixels)
{
	if (USizeBox* WidthBox = Cast<USizeBox>(
		WidgetTree ? WidgetTree->RootWidget : nullptr))
	{
		WidthBox->SetWidthOverride(FMath::Max(1.0f, InWidthPixels));
	}
	if (TitleText)
	{
		TitleText->SetText(InTitle);
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(InDescription);
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
}
