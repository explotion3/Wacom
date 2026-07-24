// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleIntentTooltipWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Battle/WacomBattleIntentEffectRowWidget.h"

#define LOCTEXT_NAMESPACE "WacomBattleIntentTooltip"

UWacomBattleIntentTooltipWidget::UWacomBattleIntentTooltipWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EffectRowWidgetClass = UWacomBattleIntentEffectRowWidget::StaticClass();
}

void UWacomBattleIntentTooltipWidget::SetIntentViewData(
	const FWacomBattleIntentPresentationViewData& InView)
{
	CurrentView = InView;
	RefreshDisplay();
}

void UWacomBattleIntentTooltipWidget::SetEffectRowWidgetClass(
	TSubclassOf<UWacomBattleIntentEffectRowWidget> InClass)
{
	EffectRowWidgetClass = InClass
		? InClass
		: TSubclassOf<UWacomBattleIntentEffectRowWidget>(
			UWacomBattleIntentEffectRowWidget::StaticClass());
	RebuildEffectRows();
}

TSharedRef<SWidget> UWacomBattleIntentTooltipWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}
		UBorder* Root = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("Root"));
		Root->SetBrushColor(FLinearColor(0.025f, 0.055f, 0.085f, 0.97f));
		Root->SetPadding(FMargin(14.0f, 12.0f));
		WidgetTree->RootWidget = Root;

		USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("Width"));
		Width->SetWidthOverride(320.0f);
		Root->SetContent(Width);
		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("Content"));
		Width->AddChild(Content);

		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("Header"));
		Content->AddChildToVerticalBox(Header);
		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("IntentIconBox"));
		IconBox->SetWidthOverride(32.0f);
		IconBox->SetHeightOverride(32.0f);
		Header->AddChildToHorizontalBox(IconBox);
		IntentIcon = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("IntentIcon"));
		IconBox->AddChild(IntentIcon);
		UVerticalBox* HeaderCopy = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("HeaderCopy"));
		if (UHorizontalBoxSlot* WidgetSlot =
			Header->AddChildToHorizontalBox(HeaderCopy))
		{
			WidgetSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
			WidgetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		IntentNameText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("IntentNameText"));
		HeaderCopy->AddChildToVerticalBox(IntentNameText);
		IntentMetaText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("IntentMetaText"));
		HeaderCopy->AddChildToVerticalBox(IntentMetaText);

		EffectsList = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("EffectsList"));
		if (UVerticalBoxSlot* WidgetSlot =
			Content->AddChildToVerticalBox(EffectsList))
		{
			WidgetSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
		}
		OverflowText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("OverflowText"));
		OverflowText->SetAutoWrapText(true);
		if (UVerticalBoxSlot* WidgetSlot =
			Content->AddChildToVerticalBox(OverflowText))
		{
			WidgetSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
		}
	}
	return Super::RebuildWidget();
}

void UWacomBattleIntentTooltipWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshDisplay();
}

void UWacomBattleIntentTooltipWidget::NativeDestruct()
{
	if (EffectsList)
	{
		EffectsList->ClearChildren();
	}
	EffectRows.Reset();
	Super::NativeDestruct();
}

void UWacomBattleIntentTooltipWidget::RefreshDisplay()
{
	SetVisibility(CurrentView.HasIntent()
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed);
	if (IntentIcon)
	{
		IntentIcon->SetBrush(CurrentView.IntentIconBrush);
	}
	if (IntentNameText)
	{
		IntentNameText->SetText(CurrentView.IntentDisplayName);
	}
	if (IntentMetaText)
	{
		IntentMetaText->SetText(CurrentView.HeaderMetaText);
	}
	if (OverflowText)
	{
		OverflowText->SetText(CurrentView.HiddenEffectRowCount > 0
			? FText::Format(
				LOCTEXT("Overflow", "另有 {0} 项，请打开敌情档案查看"),
				FText::AsNumber(CurrentView.HiddenEffectRowCount))
			: FText::GetEmpty());
		OverflowText->SetVisibility(CurrentView.HiddenEffectRowCount > 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	RebuildEffectRows();
}

void UWacomBattleIntentTooltipWidget::RebuildEffectRows()
{
	if (!EffectsList)
	{
		return;
	}
	EffectsList->ClearChildren();
	EffectRows.Reset();
	UClass* RowClass = EffectRowWidgetClass
		? EffectRowWidgetClass.Get()
		: UWacomBattleIntentEffectRowWidget::StaticClass();
	for (int32 Index = 0; Index < CurrentView.EffectRows.Num(); ++Index)
	{
		UWacomBattleIntentEffectRowWidget* Row = GetWorld()
			? CreateWidget<UWacomBattleIntentEffectRowWidget>(
				GetWorld(), RowClass)
			: NewObject<UWacomBattleIntentEffectRowWidget>(
				this, RowClass);
		if (!Row)
		{
			continue;
		}
		Row->SetEffectRowViewData(CurrentView.EffectRows[Index]);
		EffectsList->AddChild(Row);
		EffectRows.Add(Row);
	}
}

#undef LOCTEXT_NAMESPACE
