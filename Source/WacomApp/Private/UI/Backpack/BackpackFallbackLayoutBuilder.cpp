// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/BackpackFallbackLayoutBuilder.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "WacomBackpack"

namespace
{
UBorder* CreateBackpackSectionBorder(UWidgetTree* WidgetTree, FName Name, const FLinearColor& Color)
{
	UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
	Border->SetBrushColor(Color);
	Border->SetPadding(FMargin(12.f, 10.f));
	return Border;
}

}

UTextBlock* FBackpackFallbackLayoutBuilder::CreateBackpackText(UWidgetTree* WidgetTree, FName Name, const FText& Text, int32 FontSize)
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	TextBlock->SetText(Text);
	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	return TextBlock;
}

void FBackpackFallbackLayoutBuilder::Build(const FBackpackFallbackLayoutBuilderContext& Context)
{
	UWidgetTree* WidgetTree = Context.WidgetTree;
	if (!Context.Owner || !WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* DimBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DimBg"));
	DimBg->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.7f));
	DimBg->SetPadding(FMargin(0.f));
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(DimBg))
	{
		Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		Slot->SetOffsets(FMargin(0.f));
	}

	UBorder* MainPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainPanel"));
	MainPanel->SetBrushColor(FLinearColor(0.04f, 0.05f, 0.08f, 0.92f));
	MainPanel->SetPadding(FMargin(16.f, 12.f));
	if (UCanvasPanelSlot* MainPanelSlot = Root->AddChildToCanvas(MainPanel))
	{
		MainPanelSlot->SetAnchors(FAnchors(0.05f, 0.05f, 0.95f, 0.95f));
		MainPanelSlot->SetOffsets(FMargin(0.f));
		MainPanelSlot->SetAutoSize(false);
	}

	if (Context.CardDetailLayer)
	{
		*Context.CardDetailLayer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CardDetailLayer"));
		(*Context.CardDetailLayer)->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* DetailLayerSlot = Root->AddChildToCanvas(Context.CardDetailLayer->Get()))
		{
			DetailLayerSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			DetailLayerSlot->SetOffsets(FMargin(0.f));
			DetailLayerSlot->SetAutoSize(false);
			DetailLayerSlot->SetZOrder(10);
		}
	}
	if (Context.DeleteConfirmHost)
	{
		UVerticalBox* Host = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DeleteConfirmHost"));
		Host->SetVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* ConfirmSlot = Root->AddChildToCanvas(Host))
		{
			ConfirmSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			ConfirmSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			ConfirmSlot->SetAutoSize(true);
			ConfirmSlot->SetZOrder(20);
		}
		*Context.DeleteConfirmHost = Host;
	}

	UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
	MainPanel->AddChild(MainVBox);

	UHorizontalBox* TopRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopRow"));
	if (UVerticalBoxSlot* TopSlot = MainVBox->AddChildToVerticalBox(TopRow))
	{
		TopSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	if (Context.TitleText && !*Context.TitleText)
	{
		*Context.TitleText = CreateBackpackText(WidgetTree, TEXT("TitleText"), LOCTEXT("Title", "背包工作台"), 28);
		if (UHorizontalBoxSlot* Slot = TopRow->AddChildToHorizontalBox(Context.TitleText->Get()))
		{
			Slot->SetPadding(FMargin(8.f, 4.f));
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
	}

	if (Context.GoldText && !*Context.GoldText)
	{
		*Context.GoldText = CreateBackpackText(WidgetTree, TEXT("GoldText"), LOCTEXT("GoldPlaceholder", "金币：0"), 18);
		if (UHorizontalBoxSlot* Slot = TopRow->AddChildToHorizontalBox(Context.GoldText->Get()))
		{
			Slot->SetPadding(FMargin(8.f, 4.f));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
	}

	if (Context.ArrangeAllButton && !*Context.ArrangeAllButton)
	{
		*Context.ArrangeAllButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ArrangeAllButton"));
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(LOCTEXT("ArrangeAll", "整理全部"));
		Label->SetJustification(ETextJustify::Center);
		(*Context.ArrangeAllButton)->AddChild(Label);
		if (UHorizontalBoxSlot* Slot = TopRow->AddChildToHorizontalBox(Context.ArrangeAllButton->Get()))
		{
			Slot->SetPadding(FMargin(8.f, 4.f));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
	}

	if (Context.ResetPilePositionsButton && !*Context.ResetPilePositionsButton)
	{
		*Context.ResetPilePositionsButton = WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), TEXT("ResetPilePositionsButton"));
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(LOCTEXT("ResetPiles", "重置牌堆位置"));
		Label->SetJustification(ETextJustify::Center);
		(*Context.ResetPilePositionsButton)->AddChild(Label);
		if (UHorizontalBoxSlot* ButtonSlot = TopRow->AddChildToHorizontalBox(
			Context.ResetPilePositionsButton->Get()))
		{
			ButtonSlot->SetPadding(FMargin(8.f, 4.f));
			ButtonSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	if (Context.CloseButton && !*Context.CloseButton)
	{
		*Context.CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(LOCTEXT("Close", "关闭"));
		Label->SetJustification(ETextJustify::Center);
		(*Context.CloseButton)->AddChild(Label);

		USizeBox* CloseSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		CloseSize->SetWidthOverride(80.f);
		CloseSize->SetHeightOverride(36.f);
		CloseSize->AddChild(Context.CloseButton->Get());
		if (UHorizontalBoxSlot* Slot = TopRow->AddChildToHorizontalBox(CloseSize))
		{
			Slot->SetPadding(FMargin(8.f, 4.f));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
	}

	UHorizontalBox* WorkspaceRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("WorkspaceRow"));
	if (UVerticalBoxSlot* WorkspaceRowSlot = MainVBox->AddChildToVerticalBox(WorkspaceRow))
	{
		WorkspaceRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UBorder* WorkspaceBorder = CreateBackpackSectionBorder(
		WidgetTree,
		TEXT("WorkspaceBorder"),
		FLinearColor(0.055f, 0.075f, 0.105f, 0.95f));
	if (UHorizontalBoxSlot* Slot = WorkspaceRow->AddChildToHorizontalBox(WorkspaceBorder))
	{
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	UOverlay* WorkspaceOverlay = WidgetTree->ConstructWidget<UOverlay>(
		UOverlay::StaticClass(), TEXT("WorkspaceOverlay"));
	WorkspaceBorder->AddChild(WorkspaceOverlay);
	if (Context.WorkspaceHost)
	{
		UOverlay* Host = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("WorkspaceHost"));
		if (UOverlaySlot* WorkspaceSlot = WorkspaceOverlay->AddChildToOverlay(Host))
		{
			WorkspaceSlot->SetHorizontalAlignment(HAlign_Fill);
			WorkspaceSlot->SetVerticalAlignment(VAlign_Fill);
		}
		*Context.WorkspaceHost = Host;
	}
	if (Context.DeleteTargetHost)
	{
		USizeBox* DeleteSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("DeleteTargetSize"));
		DeleteSize->SetWidthOverride(220.f);
		DeleteSize->SetHeightOverride(120.f);
		if (UOverlaySlot* DeleteOverlaySlot = WorkspaceOverlay->AddChildToOverlay(DeleteSize))
		{
			DeleteOverlaySlot->SetHorizontalAlignment(HAlign_Right);
			DeleteOverlaySlot->SetVerticalAlignment(VAlign_Bottom);
			DeleteOverlaySlot->SetPadding(FMargin(0.f, 0.f, 12.f, 12.f));
		}
		UBorder* DeleteBorder = CreateBackpackSectionBorder(
			WidgetTree,
			TEXT("DeleteTargetBorder"),
			FLinearColor(0.32f, 0.07f, 0.07f, 0.95f));
		DeleteSize->AddChild(DeleteBorder);
		UVerticalBox* Host = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DeleteTargetHost"));
		DeleteBorder->AddChild(Host);
		Host->AddChildToVerticalBox(CreateBackpackText(WidgetTree, TEXT("DeleteTargetText"), LOCTEXT("DeleteTarget", "销毁卡牌"), 16));
		*Context.DeleteTargetHost = Host;
	}

}

#undef LOCTEXT_NAMESPACE
