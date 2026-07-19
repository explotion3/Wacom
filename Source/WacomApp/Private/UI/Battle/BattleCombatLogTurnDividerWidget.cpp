// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattleCombatLogTurnDividerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "WacomBattleCombatLogTurnDivider"

TSharedRef<SWidget> UBattleCombatLogTurnDividerWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}
		DividerRoot = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DividerRoot"));
		DividerRoot->SetPadding(FMargin(8.0f, 5.0f));
		WidgetTree->RootWidget = DividerRoot;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DividerRow"));
		DividerRoot->SetContent(Row);
		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TurnIconSize"));
		IconSize->SetWidthOverride(24.0f);
		IconSize->SetHeightOverride(24.0f);
		TurnIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TurnIcon"));
		TurnIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		IconSize->SetContent(TurnIcon);
		if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconSize))
		{
			IconSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}
		TurnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TurnText"));
		FSlateFontInfo Font = TurnText->GetFont();
		Font.Size = 18;
		TurnText->SetFont(Font);
		Row->AddChildToHorizontalBox(TurnText);
	}
	return Super::RebuildWidget();
}

void UBattleCombatLogTurnDividerWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	ApplyCurrentData();
}

void UBattleCombatLogTurnDividerWidget::SetTurnDividerData(
	int32 InTurnNumber,
	EWacomBattleCombatLogTurnBoundaryKind InBoundaryKind,
	const FSlateBrush& InTurnIconBrush)
{
	TurnNumber = FMath::Max(InTurnNumber, 1);
	BoundaryKind = InBoundaryKind;
	TurnIconBrush = InTurnIconBrush;
	ApplyCurrentData();
}

void UBattleCombatLogTurnDividerWidget::ApplyCurrentData()
{
	const bool bIsStart = BoundaryKind == EWacomBattleCombatLogTurnBoundaryKind::Start;
	if (TurnText)
	{
		TurnText->SetText(FText::Format(
			bIsStart ? LOCTEXT("TurnStart", "第 {0} 回合开始") : LOCTEXT("TurnEnd", "第 {0} 回合结束"),
			FText::AsNumber(TurnNumber)));
		TurnText->SetColorAndOpacity(FSlateColor(
			bIsStart
				? FLinearColor(0.48f, 0.82f, 1.0f, 1.0f)
				: FLinearColor(1.0f, 0.42f, 0.52f, 1.0f)));
	}
	if (TurnIcon)
	{
		TurnIcon->SetBrush(TurnIconBrush);
	}
	if (DividerRoot)
	{
		DividerRoot->SetBrushColor(
			bIsStart
				? FLinearColor(0.025f, 0.065f, 0.10f, 0.82f)
				: FLinearColor(0.10f, 0.025f, 0.045f, 0.82f));
	}
}

#undef LOCTEXT_NAMESPACE
