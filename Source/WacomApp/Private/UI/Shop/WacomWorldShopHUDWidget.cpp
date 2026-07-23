// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomWorldShopHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "WacomWorldShopHUDWidget"

void UWacomWorldShopHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureFallbackWidgetTree();
}

void UWacomWorldShopHUDWidget::SetGold(int32 Gold)
{
	if (GoldText)
	{
		GoldText->SetText(FText::Format(LOCTEXT("Gold", "金币：{0}    Esc 离开"), FText::AsNumber(Gold)));
	}
}

void UWacomWorldShopHUDWidget::EnsureFallbackWidgetTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("TransparentRoot"));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Root;
	GoldText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GoldText"));
	GoldText->SetVisibility(ESlateVisibility::HitTestInvisible);
	GoldText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UOverlaySlot* GoldSlot = Root->AddChildToOverlay(GoldText))
	{
		GoldSlot->SetHorizontalAlignment(HAlign_Left);
		GoldSlot->SetVerticalAlignment(VAlign_Top);
		GoldSlot->SetPadding(FMargin(28.0f));
	}
}

#undef LOCTEXT_NAMESPACE
