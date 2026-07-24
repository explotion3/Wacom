// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleIntentEffectRowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	void ConfigureText(UTextBlock& Text, const int32 FontSize)
	{
		FSlateFontInfo Font = Text.GetFont();
		Font.Size = FontSize;
		Text.SetFont(Font);
		Text.SetAutoWrapText(true);
	}
}

void UWacomBattleIntentEffectRowWidget::SetEffectRowViewData(
	const FWacomBattleIntentEffectRowViewData& InView)
{
	CurrentView = InView;
	RefreshDisplay();
}

TSharedRef<SWidget> UWacomBattleIntentEffectRowWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UHorizontalBox* Root = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("EffectIconBox"));
		IconBox->SetWidthOverride(24.0f);
		IconBox->SetHeightOverride(24.0f);
		if (UHorizontalBoxSlot* WidgetSlot = Root->AddChildToHorizontalBox(IconBox))
		{
			WidgetSlot->SetPadding(FMargin(0.0f, 2.0f, 8.0f, 2.0f));
			WidgetSlot->SetVerticalAlignment(VAlign_Top);
		}
		EffectIcon = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("EffectIcon"));
		IconBox->AddChild(EffectIcon);

		UVerticalBox* Copy = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("Copy"));
		if (UHorizontalBoxSlot* WidgetSlot = Root->AddChildToHorizontalBox(Copy))
		{
			WidgetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			WidgetSlot->SetVerticalAlignment(VAlign_Center);
		}

		UHorizontalBox* Summary = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("Summary"));
		Copy->AddChildToVerticalBox(Summary);
		TargetText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("TargetText"));
		ConfigureText(*TargetText, 14);
		Summary->AddChildToHorizontalBox(TargetText);
		EffectText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("EffectText"));
		ConfigureText(*EffectText, 14);
		if (UHorizontalBoxSlot* WidgetSlot =
			Summary->AddChildToHorizontalBox(EffectText))
		{
			WidgetSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
		}
		CoreRuleText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("CoreRuleText"));
		ConfigureText(*CoreRuleText, 11);
		if (UVerticalBoxSlot* WidgetSlot =
			Copy->AddChildToVerticalBox(CoreRuleText))
		{
			WidgetSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
		}
	}
	return Super::RebuildWidget();
}

void UWacomBattleIntentEffectRowWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshDisplay();
}

void UWacomBattleIntentEffectRowWidget::RefreshDisplay()
{
	SetVisibility(CurrentView.EffectType.IsValid()
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed);
	if (EffectIcon)
	{
		EffectIcon->SetBrush(CurrentView.IconBrush);
		EffectIcon->SetColorAndOpacity(CurrentView.Tint);
	}
	if (TargetText)
	{
		TargetText->SetText(CurrentView.TargetText);
	}
	if (EffectText)
	{
		EffectText->SetText(CurrentView.EffectText);
		EffectText->SetColorAndOpacity(FSlateColor(CurrentView.Tint));
	}
	if (CoreRuleText)
	{
		CoreRuleText->SetText(CurrentView.CoreRuleText);
		CoreRuleText->SetVisibility(CurrentView.CoreRuleText.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}
}
