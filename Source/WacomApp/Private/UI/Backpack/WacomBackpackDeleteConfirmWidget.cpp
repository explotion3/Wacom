// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackDeleteConfirmWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

#define LOCTEXT_NAMESPACE "WacomBackpackDeleteConfirm"

TSharedRef<SWidget> UWacomBackpackDeleteConfirmWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!WidgetTree->RootWidget)
	{
		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DeleteConfirmRoot"));
		WidgetTree->RootWidget = Root;
		SummaryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SummaryText"));
		Root->AddChildToVerticalBox(SummaryText);
		UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Root->AddChildToVerticalBox(Buttons);
		ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmButton"));
		CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CancelButton"));
		UTextBlock* ConfirmLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ConfirmLabel->SetText(LOCTEXT("Confirm", "确认销毁"));
		ConfirmButton->AddChild(ConfirmLabel);
		UTextBlock* CancelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		CancelLabel->SetText(LOCTEXT("Cancel", "取消"));
		CancelButton->AddChild(CancelLabel);
		Buttons->AddChildToHorizontalBox(ConfirmButton);
		Buttons->AddChildToHorizontalBox(CancelButton);
	}
	ApplyPreview();
	return Super::RebuildWidget();
}

void UWacomBackpackDeleteConfirmWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ConfirmButton) ConfirmButton->OnClicked.AddUniqueDynamic(this, &UWacomBackpackDeleteConfirmWidget::HandleConfirm);
	if (CancelButton) CancelButton->OnClicked.AddUniqueDynamic(this, &UWacomBackpackDeleteConfirmWidget::HandleCancel);
	ApplyPreview();
}

void UWacomBackpackDeleteConfirmWidget::SetPreview(int32 CardCount, int32 TotalGoldReward)
{
	PreviewCardCount = FMath::Max(0, CardCount);
	PreviewGoldReward = FMath::Max(0, TotalGoldReward);
	ApplyPreview();
}

void UWacomBackpackDeleteConfirmWidget::ApplyPreview()
{
	if (SummaryText)
	{
		SummaryText->SetText(FText::Format(
			LOCTEXT("Summary", "确认永久销毁 {0} 张卡牌并获得 {1} 金币？"),
			FText::AsNumber(PreviewCardCount),
			FText::AsNumber(PreviewGoldReward)));
	}
}

void UWacomBackpackDeleteConfirmWidget::HandleConfirm() { OnConfirmNative.Broadcast(); }
void UWacomBackpackDeleteConfirmWidget::HandleCancel() { OnCancelNative.Broadcast(); }

#undef LOCTEXT_NAMESPACE
