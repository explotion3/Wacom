// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattleCombatLogFeedWidget.h"

#include "UI/Battle/BattleCombatLogBlockWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "WacomBattleCombatLogFeed"

TSharedRef<SWidget> UBattleCombatLogFeedWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Frame"));
		Frame->SetBrushColor(FLinearColor(0.02f, 0.025f, 0.035f, 0.62f));
		Frame->SetPadding(FMargin(10.0f));
		WidgetTree->RootWidget = Frame;

		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		Frame->SetContent(Root);

		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		TitleText->SetText(LOCTEXT("Title", "战斗记录"));
		TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.95f, 0.82f, 1.0f)));
		{
			FSlateFontInfo Font = TitleText->GetFont();
			Font.Size = 14;
			TitleText->SetFont(Font);
		}
		if (UVerticalBoxSlot* TitleSlot = Root->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}

		BlocksScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("BlocksScrollBox"));
		if (UVerticalBoxSlot* ScrollSlot = Root->AddChildToVerticalBox(BlocksScrollBox))
		{
			ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UVerticalBox* RuntimeBlocksBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BlocksBox"));
		if (BlocksScrollBox)
		{
			BlocksScrollBox->AddChild(RuntimeBlocksBox);
		}
		BlocksBox = RuntimeBlocksBox;
	}
	return Super::RebuildWidget();
}

void UBattleCombatLogFeedWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!BlockWidgetClass)
	{
		BlockWidgetClass = UBattleCombatLogBlockWidget::StaticClass();
	}
	RebuildBlockWidgets();
}

void UBattleCombatLogFeedWidget::SetCombatLogBlocks(
	const TArray<FWacomBattleCombatLogBlockView>& Blocks)
{
	CurrentBlocks.Reset();
	for (const FWacomBattleCombatLogBlockView& Block : Blocks)
	{
		if (Block.bShouldDisplay)
		{
			CurrentBlocks.Add(Block);
		}
	}
	TrimToVisibleBlocks();
	SetVisibility(CurrentBlocks.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	RebuildBlockWidgets();
	if (bAutoScrollToLatest && BlocksScrollBox)
	{
		BlocksScrollBox->ScrollToEnd();
	}
}

void UBattleCombatLogFeedWidget::ClearCombatLog()
{
	CurrentBlocks.Reset();
	SetVisibility(ESlateVisibility::Collapsed);
	RebuildBlockWidgets();
	if (BlocksScrollBox)
	{
		BlocksScrollBox->ScrollToStart();
	}
}

void UBattleCombatLogFeedWidget::TrimToVisibleBlocks()
{
	const int32 SafeMax = FMath::Max(1, MaxVisibleBlocks);
	if (CurrentBlocks.Num() > SafeMax)
	{
		CurrentBlocks.RemoveAt(0, CurrentBlocks.Num() - SafeMax);
	}
}

void UBattleCombatLogFeedWidget::RebuildBlockWidgets()
{
	if (!BlocksBox)
	{
		return;
	}

	BlocksBox->ClearChildren();
	for (const FWacomBattleCombatLogBlockView& Block : CurrentBlocks)
	{
		UBattleCombatLogBlockWidget* BlockWidget = CreateBlockWidget(Block);
		if (!BlockWidget)
		{
			continue;
		}

		if (UPanelSlot* AddedSlot = BlocksBox->AddChild(BlockWidget))
		{
			if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(AddedSlot))
			{
				VerticalSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
			}
		}
	}
}

UBattleCombatLogBlockWidget* UBattleCombatLogFeedWidget::CreateBlockWidget(
	const FWacomBattleCombatLogBlockView& Block)
{
	UClass* WidgetClass = BlockWidgetClass ? BlockWidgetClass.Get() : UBattleCombatLogBlockWidget::StaticClass();
	UBattleCombatLogBlockWidget* BlockWidget = GetWorld()
		? CreateWidget<UBattleCombatLogBlockWidget>(this, WidgetClass)
		: NewObject<UBattleCombatLogBlockWidget>(this, WidgetClass);
	if (BlockWidget)
	{
		BlockWidget->SetCombatLogBlockData(Block);
	}
	return BlockWidget;
}

#undef LOCTEXT_NAMESPACE
