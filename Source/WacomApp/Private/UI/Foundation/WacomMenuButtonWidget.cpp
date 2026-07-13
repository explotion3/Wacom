// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomMenuButtonWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"

TSharedRef<SWidget> UWacomMenuButtonWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!WidgetTree->RootWidget)
	{
		USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("ButtonSize"));
		Root->SetMinDesiredWidth(144.0f);
		Root->SetMinDesiredHeight(42.0f);
		WidgetTree->RootWidget = Root;

		UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("ButtonBackdrop"));
		Backdrop->SetBrushColor(FLinearColor(0.035f, 0.052f, 0.072f, 0.94f));
		Backdrop->SetPadding(FMargin(12.0f, 7.0f));
		Root->AddChild(Backdrop);

		ButtonText = WidgetTree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("ButtonText"));
		ButtonText->SetJustification(ETextJustify::Center);
		ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.90f, 0.82f, 1.0f)));
		Backdrop->AddChild(ButtonText);
	}
	return Super::RebuildWidget();
}
