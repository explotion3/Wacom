// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackPilePreviewWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Cards/CardDefinition.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "WacomBackpackPilePreview"

TSharedRef<SWidget> UWacomBackpackPilePreviewWidget::RebuildWidget()
{
	EnsureFallbackTree();
	ApplyView();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	return Super::RebuildWidget();
}

void UWacomBackpackPilePreviewWidget::SetPreviewView(
	const FWacomBackpackPilePreviewCardView& InView)
{
	PreviewView = InView;
	ApplyView();
}

void UWacomBackpackPilePreviewWidget::EnsureFallbackTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (WidgetTree->RootWidget)
	{
		return;
	}

	USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PreviewSize"));
	RootSize->SetWidthOverride(110.0f);
	RootSize->SetHeightOverride(160.0f);
	WidgetTree->RootWidget = RootSize;

	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PreviewFrame"));
	Frame->SetPadding(FMargin(5.0f));
	Frame->SetBrushColor(FLinearColor(0.08f, 0.105f, 0.14f, 1.0f));
	RootSize->AddChild(Frame);

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PreviewCanvas"));
	Frame->AddChild(Canvas);
	CardArt = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CardArt"));
	if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(CardArt))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f, 25.0f, 0.0f, 30.0f));
	}
	CostText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CostText"));
	if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(CostText))
	{
		CanvasSlot->SetPosition(FVector2D(4.0f, 2.0f));
		CanvasSlot->SetAutoSize(true);
	}
	NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
	NameText->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(NameText))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 1.0f, 1.0f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(2.0f, -28.0f, 2.0f, 2.0f));
	}
	RoleBadgeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RoleBadgeText"));
	if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(RoleBadgeText))
	{
		CanvasSlot->SetAnchors(FAnchors(1.0f, 0.0f));
		CanvasSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		CanvasSlot->SetPosition(FVector2D(-3.0f, 3.0f));
		CanvasSlot->SetAutoSize(true);
	}
}

void UWacomBackpackPilePreviewWidget::ApplyView()
{
	UCardDefinition* Definition = PreviewView.Definition.Get();
	if (CardArt)
	{
		if (Definition && Definition->CardIllustration)
		{
			CardArt->SetBrushFromTexture(Definition->CardIllustration, true);
			CardArt->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			CardArt->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (CostText)
	{
		CostText->SetText(Definition ? FText::AsNumber(Definition->BaseCost) : FText::GetEmpty());
	}
	if (NameText)
	{
		NameText->SetText(Definition ? Definition->DisplayName : LOCTEXT("UnknownCard", "未知卡牌"));
	}
	if (RoleBadgeText)
	{
		RoleBadgeText->SetText(PreviewView.bOwnerIdentity
			? LOCTEXT("OwnerBadge", "主卡")
			: (PreviewView.bProjected ? LOCTEXT("ProjectedBadge", "投影") : FText::GetEmpty()));
		RoleBadgeText->SetVisibility(
			PreviewView.bOwnerIdentity || PreviewView.bProjected
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}
}

#undef LOCTEXT_NAMESPACE
