// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardDetailPanel.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "WacomCardDetailPanel"

namespace
{
	UTextBlock* AddSectionLabel(UWidgetTree* WidgetTree, UVerticalBox* Root, const FName Name, const FText& Text)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Label->SetText(Text);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.82f, 1.f, 1.f)));
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = 12;
		Label->SetFont(Font);
		if (UVerticalBoxSlot* Slot = Root->AddChildToVerticalBox(Label))
		{
			Slot->SetPadding(FMargin(0.f, 10.f, 0.f, 4.f));
		}
		return Label;
	}

	UTextBlock* MakeBodyText(UWidgetTree* WidgetTree, const FName Name, const FText& Text)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TextBlock->SetText(Text);
		TextBlock->SetAutoWrapText(true);
		TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.9f, 0.84f, 1.f)));
		return TextBlock;
	}

	void SetOptionalText(UTextBlock* TextBlock, const FText& Text)
	{
		if (!TextBlock)
		{
			return;
		}
		TextBlock->SetText(Text);
		TextBlock->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

TSharedRef<SWidget> UWacomCardDetailPanel::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardDetailRoot"));
		RootBorder->SetBrushColor(FLinearColor(0.035f, 0.032f, 0.028f, 0.96f));
		RootBorder->SetPadding(FMargin(14.f));
		WidgetTree->RootWidget = RootBorder;

		UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CardDetailContent"));
		RootBorder->AddChild(RootBox);

		NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
		NameText->SetAutoWrapText(true);
		NameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		{
			FSlateFontInfo Font = NameText->GetFont();
			Font.Size = 18;
			NameText->SetFont(Font);
		}
		if (UVerticalBoxSlot* NameSlot = RootBox->AddChildToVerticalBox(NameText))
		{
			NameSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}

		DescriptionText = MakeBodyText(WidgetTree, TEXT("DescriptionText"), FText::GetEmpty());
		if (UVerticalBoxSlot* DescriptionSlot = RootBox->AddChildToVerticalBox(DescriptionText))
		{
			DescriptionSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
		}

		AddSectionLabel(WidgetTree, RootBox, FName(TEXT("TasksLabel")), LOCTEXT("TasksLabel", "任务"));
		TasksBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName(TEXT("TasksBox")));
		RootBox->AddChildToVerticalBox(TasksBox);

		AddSectionLabel(WidgetTree, RootBox, FName(TEXT("ChangesLabel")), LOCTEXT("ChangesLabel", "变化"));
		ChangesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName(TEXT("ChangesBox")));
		RootBox->AddChildToVerticalBox(ChangesBox);

		AddSectionLabel(WidgetTree, RootBox, FName(TEXT("PassivesLabel")), LOCTEXT("PassivesLabel", "被动"));
		PassivesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName(TEXT("PassivesBox")));
		RootBox->AddChildToVerticalBox(PassivesBox);
	}

	return Super::RebuildWidget();
}

void UWacomCardDetailPanel::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyCurrentDataToWidgets();
}

void UWacomCardDetailPanel::SetCardDetailData(const FWacomCardDetailViewData& InData)
{
	CurrentData = InData;
	ApplyCurrentDataToWidgets();
}

void UWacomCardDetailPanel::ApplyCurrentDataToWidgets()
{
	SetOptionalText(NameText, CurrentData.Name);
	SetOptionalText(DescriptionText, CurrentData.Description);
	RebuildLineBox(TasksBox, CurrentData.TaskLines);
	RebuildLineBox(ChangesBox, CurrentData.ChangeLines);
	RebuildLineBox(PassivesBox, CurrentData.PassiveLines);
}

void UWacomCardDetailPanel::RebuildLineBox(UPanelWidget* Box, const TArray<FText>& Lines)
{
	if (!Box)
	{
		return;
	}

	Box->ClearChildren();
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		const FText& Line = Lines[Index];
		if (Line.IsEmpty())
		{
			continue;
		}

		UTextBlock* TextBlock = MakeBodyText(
			WidgetTree,
			FName(*FString::Printf(TEXT("DetailLine_%d_%d"), GetUniqueID(), Index)),
			Line);
		Box->AddChild(TextBlock);
	}
	Box->SetVisibility(Box->GetChildrenCount() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

#undef LOCTEXT_NAMESPACE
