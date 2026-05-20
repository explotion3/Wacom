// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardDetailSectionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	UTextBlock* MakeSectionText(UWidgetTree* WidgetTree, const FName Name, const FText& Text, int32 FontSize, const FLinearColor& Color)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TextBlock->SetText(Text);
		TextBlock->SetAutoWrapText(true);
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = FontSize;
		TextBlock->SetFont(Font);
		return TextBlock;
	}
}

TSharedRef<SWidget> UWacomCardDetailSectionWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardDetailSectionRoot"));
		RootBorder->SetBrushColor(FLinearColor(0.06f, 0.055f, 0.045f, 0.92f));
		RootBorder->SetPadding(FMargin(10.f, 8.f));
		WidgetTree->RootWidget = RootBorder;

		UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CardDetailSectionContent"));
		RootBorder->AddChild(RootBox);

		TitleText = MakeSectionText(
			WidgetTree,
			TEXT("TitleText"),
			FText::GetEmpty(),
			13,
			FLinearColor(0.75f, 0.82f, 1.f, 1.f));
		if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 5.f));
		}

		LinesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LinesBox"));
		RootBox->AddChildToVerticalBox(LinesBox);
	}

	return Super::RebuildWidget();
}

void UWacomCardDetailSectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyCurrentDataToWidgets();
}

void UWacomCardDetailSectionWidget::SetSectionData(const FWacomCardDetailSectionData& InData)
{
	CurrentData = InData;
	ApplyCurrentDataToWidgets();
}

void UWacomCardDetailSectionWidget::ApplyCurrentDataToWidgets()
{
	if (TitleText)
	{
		TitleText->SetText(CurrentData.Title);
		TitleText->SetVisibility(CurrentData.Title.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (!LinesBox)
	{
		return;
	}

	LinesBox->ClearChildren();
	for (int32 Index = 0; Index < CurrentData.Lines.Num(); ++Index)
	{
		const FText& Line = CurrentData.Lines[Index];
		if (Line.IsEmpty())
		{
			continue;
		}

		UTextBlock* LineText = MakeSectionText(
			WidgetTree,
			FName(*FString::Printf(TEXT("SectionLine_%d_%d"), GetUniqueID(), Index)),
			Line,
			12,
			FLinearColor(0.92f, 0.9f, 0.84f, 1.f));
		if (UVerticalBoxSlot* LineSlot = Cast<UVerticalBoxSlot>(LinesBox->AddChild(LineText)))
		{
			LineSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
		}
	}
	LinesBox->SetVisibility(LinesBox->GetChildrenCount() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
