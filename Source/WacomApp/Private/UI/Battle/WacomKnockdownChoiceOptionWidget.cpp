// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomKnockdownChoiceOptionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Card/WacomCardView.h"

#define LOCTEXT_NAMESPACE "WacomKnockdownChoiceOption"

namespace
{
	constexpr TCHAR DefaultRewardCardViewClassPath[] =
		TEXT("/Game/Wacom/UI/Card/WBP_CardView.WBP_CardView_C");

	ESlateVisibility TextVisibility(const FText& Text)
	{
		return Text.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible;
	}
}

UWacomKnockdownChoiceOptionWidget::UWacomKnockdownChoiceOptionWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UWacomKnockdownChoiceOptionWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UBorder* Root = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("Root"));
		Root->SetPadding(FMargin(18.0f, 14.0f));
		Root->SetBrushColor(FLinearColor(0.055f, 0.065f, 0.09f, 0.98f));
		WidgetTree->RootWidget = Root;

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("ContentColumn"));
		Root->SetContent(Column);

		BranchLabelText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("BranchLabelText"));
		BranchLabelText->SetJustification(ETextJustify::Center);
		BranchLabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.68f, 0.72f, 0.82f)));
		Column->AddChildToVerticalBox(BranchLabelText);

		ChoiceLabelText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("ChoiceLabelText"));
		ChoiceLabelText->SetJustification(ETextJustify::Center);
		FSlateFontInfo ChoiceFont = ChoiceLabelText->GetFont();
		ChoiceFont.Size = 24;
		ChoiceLabelText->SetFont(ChoiceFont);
		if (UVerticalBoxSlot* ChoiceSlot = Column->AddChildToVerticalBox(ChoiceLabelText))
		{
			ChoiceSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 8.0f));
		}

		DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("DescriptionText"));
		DescriptionText->SetJustification(ETextJustify::Center);
		DescriptionText->SetAutoWrapText(true);
		if (UVerticalBoxSlot* DescriptionSlot = Column->AddChildToVerticalBox(DescriptionText))
		{
			DescriptionSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}

		USizeBox* RewardCardSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("RewardCardSize"));
		RewardCardSize->SetWidthOverride(178.0f);
		RewardCardSize->SetHeightOverride(252.0f);
		if (UVerticalBoxSlot* CardSlot = Column->AddChildToVerticalBox(RewardCardSize))
		{
			CardSlot->SetHorizontalAlignment(HAlign_Center);
		}

		RewardCardHost = WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("RewardCardHost"));
		RewardCardHost->SetStretch(EStretch::ScaleToFit);
		RewardCardHost->SetStretchDirection(EStretchDirection::DownOnly);
		RewardCardSize->SetContent(RewardCardHost);

		RewardFallbackText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("RewardFallbackText"));
		RewardFallbackText->SetJustification(ETextJustify::Center);
		RewardFallbackText->SetAutoWrapText(true);
		if (UVerticalBoxSlot* RewardFallbackSlot = Column->AddChildToVerticalBox(RewardFallbackText))
		{
			RewardFallbackSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
		}

		DisabledReasonText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("DisabledReasonText"));
		DisabledReasonText->SetJustification(ETextJustify::Center);
		DisabledReasonText->SetAutoWrapText(true);
		DisabledReasonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.47f, 0.42f)));
		if (UVerticalBoxSlot* DisabledReasonSlot = Column->AddChildToVerticalBox(DisabledReasonText))
		{
			DisabledReasonSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
		}
	}

	return Super::RebuildWidget();
}

void UWacomKnockdownChoiceOptionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyCurrentViewData();
}

void UWacomKnockdownChoiceOptionWidget::NativeDestruct()
{
	ChoiceRequestedNative.Clear();
	ClearRewardCardView();
	Super::NativeDestruct();
}

void UWacomKnockdownChoiceOptionWidget::NativeOnClicked()
{
	Super::NativeOnClicked();
	if (IsChoiceAvailable())
	{
		ChoiceRequestedNative.Broadcast(CurrentViewData.Choice);
	}
}

void UWacomKnockdownChoiceOptionWidget::SetOptionViewData(
	const FWacomKnockdownChoiceOptionViewData& InViewData)
{
	CurrentViewData = InViewData;
	ApplyCurrentViewData();
}

void UWacomKnockdownChoiceOptionWidget::ApplyCurrentViewData()
{
	SetIsEnabled(IsChoiceAvailable());
	SetIsInteractionEnabled(IsChoiceAvailable());
	BP_OnInteractabilityChanged(IsChoiceAvailable());

	if (BranchLabelText)
	{
		BranchLabelText->SetText(CurrentViewData.BranchLabel);
	}
	if (ChoiceLabelText)
	{
		ChoiceLabelText->SetText(CurrentViewData.ChoiceLabel);
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(CurrentViewData.DescriptionText);
	}
	if (DisabledReasonText)
	{
		DisabledReasonText->SetText(CurrentViewData.DisabledReasonText);
		DisabledReasonText->SetVisibility(TextVisibility(CurrentViewData.DisabledReasonText));
	}

	if (CurrentViewData.bHasRewardCardView)
	{
		EnsureRewardCardView();
		if (RuntimeRewardCardView)
		{
			RuntimeRewardCardView->SetCardViewData(CurrentViewData.RewardCardViewData);
		}
		if (RewardCardHost)
		{
			RewardCardHost->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (RewardFallbackText)
		{
			RewardFallbackText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		ClearRewardCardView();
		if (RewardCardHost)
		{
			RewardCardHost->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (RewardFallbackText)
		{
			RewardFallbackText->SetText(CurrentViewData.RewardFallbackText);
			RewardFallbackText->SetVisibility(TextVisibility(CurrentViewData.RewardFallbackText));
		}
	}
}

void UWacomKnockdownChoiceOptionWidget::EnsureRewardCardView()
{
	if (!RewardCardHost)
	{
		return;
	}

	UClass* ClassToUse = RewardCardViewClass.Get();
	if (!ClassToUse)
	{
		ClassToUse = LoadClass<UWacomCardView>(nullptr, DefaultRewardCardViewClassPath);
	}
	if (!ClassToUse)
	{
		ClassToUse = UWacomCardView::StaticClass();
	}
	if (RuntimeRewardCardView && RuntimeRewardCardView->GetClass() == ClassToUse)
	{
		return;
	}

	ClearRewardCardView();
	RuntimeRewardCardView = GetWorld()
		? CreateWidget<UWacomCardView>(this, ClassToUse)
		: NewObject<UWacomCardView>(this, ClassToUse);
	if (RuntimeRewardCardView)
	{
		RuntimeRewardCardView->SetSurfaceFoilEnabled(false);
		RuntimeRewardCardView->SetVisibility(ESlateVisibility::HitTestInvisible);
		RewardCardHost->SetContent(RuntimeRewardCardView);
	}
}

void UWacomKnockdownChoiceOptionWidget::ClearRewardCardView()
{
	if (RewardCardHost)
	{
		RewardCardHost->ClearChildren();
	}
	RuntimeRewardCardView = nullptr;
}

#undef LOCTEXT_NAMESPACE
