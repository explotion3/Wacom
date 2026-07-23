// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleStatusTooltipWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Battle/WacomBattleStatusTooltipPresentation.h"

#define LOCTEXT_NAMESPACE "WacomBattleStatusTooltip"

namespace
{
	constexpr float TooltipWidth = 300.0f;
	constexpr float TooltipIconSize = 28.0f;

	void StyleText(
		UTextBlock& TextBlock,
		const int32 FontSize,
		const FLinearColor& Color,
		const bool bBold = false)
	{
		FSlateFontInfo Font = TextBlock.GetFont();
		Font.Size = FontSize;
		Font.TypefaceFontName = bBold ? TEXT("Bold") : TEXT("Regular");
		TextBlock.SetFont(Font);
		TextBlock.SetColorAndOpacity(FSlateColor(Color));
		TextBlock.SetAutoWrapText(true);
		TextBlock.SetWrapTextAt(TooltipWidth - 28.0f);
		TextBlock.SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	UTextBlock* AddRuleText(
		UWidgetTree& WidgetTree,
		UVerticalBox& Column,
		const FName Name)
	{
		UTextBlock* Text = WidgetTree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		StyleText(*Text, 13, FLinearColor(0.78f, 0.84f, 0.91f, 1.0f));
		if (UVerticalBoxSlot* Slot = Column.AddChildToVerticalBox(Text))
		{
			Slot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));
		}
		return Text;
	}
}

void UWacomBattleStatusTooltipWidget::SetStatusView(
	const FWacomBattleStatusIconView& InView)
{
	CurrentStatusView = InView;
	CurrentOverflowViews.Reset();
	bShowingOverflow = false;
	bShowingHistoricalStatusEvent = false;
	HistoricalStatusDelta = 0;
	RefreshDisplay();
}

void UWacomBattleStatusTooltipWidget::SetHistoricalStatusEventView(
	const FWacomBattleStatusIconView& InView,
	const int32 StatusDelta)
{
	CurrentStatusView = InView;
	CurrentOverflowViews.Reset();
	bShowingOverflow = false;
	bShowingHistoricalStatusEvent = true;
	HistoricalStatusDelta = StatusDelta;
	RefreshDisplay();
}

void UWacomBattleStatusTooltipWidget::SetOverflowViews(
	const TArray<FWacomBattleStatusIconView>& InHiddenViews)
{
	CurrentStatusView = FWacomBattleStatusIconView();
	CurrentOverflowViews = InHiddenViews;
	bShowingOverflow = true;
	bShowingHistoricalStatusEvent = false;
	HistoricalStatusDelta = 0;
	RefreshDisplay();
}

TSharedRef<SWidget> UWacomBattleStatusTooltipWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("TooltipRoot"));
		Root->SetWidthOverride(TooltipWidth);
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;

		UBorder* Surface = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("TooltipSurface"));
		Surface->SetBrushColor(FLinearColor(0.025f, 0.055f, 0.085f, 0.97f));
		Surface->SetPadding(FMargin(12.0f, 10.0f));
		Surface->SetVisibility(ESlateVisibility::HitTestInvisible);
		Root->AddChild(Surface);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("TooltipColumn"));
		Column->SetVisibility(ESlateVisibility::HitTestInvisible);
		Surface->SetContent(Column);

		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("TooltipHeader"));
		Header->SetVisibility(ESlateVisibility::HitTestInvisible);
		Column->AddChildToVerticalBox(Header);

		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("TooltipIconBox"));
		IconBox->SetWidthOverride(TooltipIconSize);
		IconBox->SetHeightOverride(TooltipIconSize);
		IconBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		Header->AddChildToHorizontalBox(IconBox);

		TooltipIcon = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("TooltipIcon"));
		TooltipIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		IconBox->AddChild(TooltipIcon);

		TitleText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("TitleText"));
		StyleText(*TitleText, 16, FLinearColor(0.94f, 0.97f, 1.0f, 1.0f), true);
		if (UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(TitleText))
		{
			TitleSlot->SetPadding(FMargin(8.0f, 2.0f, 0.0f, 0.0f));
			TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			TitleSlot->SetVerticalAlignment(VAlign_Center);
		}

		StackText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("StackText"));
		StyleText(*StackText, 14, FLinearColor(0.98f, 0.80f, 0.30f, 1.0f), true);
		if (UHorizontalBoxSlot* StackSlot = Header->AddChildToHorizontalBox(StackText))
		{
			StackSlot->SetPadding(FMargin(8.0f, 2.0f, 0.0f, 0.0f));
			StackSlot->SetVerticalAlignment(VAlign_Center);
		}

		CoreEffectText = AddRuleText(*WidgetTree, *Column, TEXT("CoreEffectText"));
		TriggerTimingText = AddRuleText(*WidgetTree, *Column, TEXT("TriggerTimingText"));
		StackPolicyText = AddRuleText(*WidgetTree, *Column, TEXT("StackPolicyText"));

		OverflowBodyText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("OverflowBodyText"));
		StyleText(*OverflowBodyText, 13, FLinearColor(0.82f, 0.88f, 0.94f, 1.0f));
		if (UVerticalBoxSlot* OverflowSlot = Column->AddChildToVerticalBox(OverflowBodyText))
		{
			OverflowSlot->SetPadding(FMargin(0.0f, 7.0f, 0.0f, 0.0f));
		}
	}

	return Super::RebuildWidget();
}

void UWacomBattleStatusTooltipWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshDisplay();
}

void UWacomBattleStatusTooltipWidget::RefreshDisplay()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (bShowingOverflow)
	{
		if (TooltipIcon)
		{
			TooltipIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (TitleText)
		{
			TitleText->SetText(FText::Format(
				LOCTEXT("OverflowTitle", "其他状态 · {0}"),
				FText::AsNumber(CurrentOverflowViews.Num())));
		}
		if (StackText)
		{
			StackText->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (CoreEffectText)
		{
			CoreEffectText->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (TriggerTimingText)
		{
			TriggerTimingText->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (StackPolicyText)
		{
			StackPolicyText->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (OverflowBodyText)
		{
			OverflowBodyText->SetText(
				FWacomBattleStatusTooltipPresentationBuilder::BuildOverflowBody(
					CurrentOverflowViews));
			OverflowBodyText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		return;
	}

	if (TooltipIcon)
	{
		TooltipIcon->SetBrush(CurrentStatusView.IconBrush);
		TooltipIcon->SetVisibility(CurrentStatusView.StatusTag.IsValid()
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (TitleText)
	{
		TitleText->SetText(CurrentStatusView.DisplayName);
	}
	if (StackText)
	{
		if (bShowingHistoricalStatusEvent)
		{
			const FText SignedDelta = FText::FromString(
				HistoricalStatusDelta > 0
					? FString::Printf(TEXT("+%d"), HistoricalStatusDelta)
					: FString::FromInt(HistoricalStatusDelta));
			StackText->SetText(FText::Format(
				LOCTEXT("HistoricalStatusDelta", "本次 {0}"),
				SignedDelta));
		}
		else
		{
			StackText->SetText(FText::Format(
				LOCTEXT("StackCount", "×{0}"),
				FText::AsNumber(FMath::Max(1, CurrentStatusView.StackCount))));
		}
		StackText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (CoreEffectText)
	{
		CoreEffectText->SetText(CurrentStatusView.CoreEffectText);
		CoreEffectText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (TriggerTimingText)
	{
		TriggerTimingText->SetText(CurrentStatusView.TriggerTimingText);
		TriggerTimingText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (StackPolicyText)
	{
		StackPolicyText->SetText(CurrentStatusView.StackPolicyText);
		StackPolicyText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (OverflowBodyText)
	{
		OverflowBodyText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

#undef LOCTEXT_NAMESPACE
