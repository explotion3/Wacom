// Copyright Wacom. All Rights Reserved.

#include "UI/Common/PileCountView.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> UPileCountView::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Root"));
		Root->SetWidthOverride(80.0f);
		Root->SetHeightOverride(80.0f);
		WidgetTree->RootWidget = Root;

		UOverlay* Content = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Content"));
		Root->AddChild(Content);

		CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountText"));
		CountText->SetText(FText::AsNumber(0));
		CountText->SetJustification(ETextJustify::Center);
		CountText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		{
			FSlateFontInfo F = CountText->GetFont();
			F.Size = 22;
			CountText->SetFont(F);
		}
		if (UOverlaySlot* CountSlot = Content->AddChildToOverlay(CountText))
		{
			CountSlot->SetHorizontalAlignment(HAlign_Center);
			CountSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	return Super::RebuildWidget();
}

void UPileCountView::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshDisplay();
}

void UPileCountView::SetCount(int32 InCount)
{
	Count = InCount;
	CountDisplayText = FText::AsNumber(Count);
	if (CountText) { CountText->SetText(CountDisplayText); }
}

void UPileCountView::SetCountDisplayText(FText InText)
{
	CountDisplayText = InText;
	if (CountText) { CountText->SetText(GetCountDisplayText()); }
}

void UPileCountView::RefreshDisplay()
{
	if (CountText) { CountText->SetText(GetCountDisplayText()); }
}
