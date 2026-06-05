// Copyright Wacom. All Rights Reserved.

#include "UI/Common/PileCountView.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"

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

		FrameBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FrameBorder"));
		FrameBorder->SetBrushColor(DefaultFrameColor);
		FrameBorder->SetPadding(FMargin(4));
		Root->AddChild(FrameBorder);

		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
		FrameBorder->SetContent(Content);

		LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LabelText"));
		LabelText->SetText(DefaultLabel);
		LabelText->SetJustification(ETextJustify::Center);
		LabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f)));
		{
			FSlateFontInfo F = LabelText->GetFont();
			F.Size = 10;
			LabelText->SetFont(F);
		}
		Content->AddChildToVerticalBox(LabelText);

		CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountText"));
		CountText->SetText(FText::AsNumber(0));
		CountText->SetJustification(ETextJustify::Center);
		CountText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		{
			FSlateFontInfo F = CountText->GetFont();
			F.Size = 22;
			CountText->SetFont(F);
		}
		Content->AddChildToVerticalBox(CountText);
	}
	return Super::RebuildWidget();
}

void UPileCountView::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (CachedLabel.IsEmpty()) { CachedLabel = DefaultLabel; }
	RefreshDisplay();
}

void UPileCountView::SetLabel(FText InLabel)
{
	CachedLabel = InLabel;
	if (LabelText) { LabelText->SetText(InLabel); }
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

void UPileCountView::SetFrameColor(FLinearColor Color)
{
	if (FrameBorder) { FrameBorder->SetBrushColor(Color); }
}

void UPileCountView::RefreshDisplay()
{
	if (LabelText) { LabelText->SetText(CachedLabel.IsEmpty() ? DefaultLabel : CachedLabel); }
	if (CountText) { CountText->SetText(GetCountDisplayText()); }
}
