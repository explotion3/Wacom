// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/EquipmentBar.h"

#define LOCTEXT_NAMESPACE "WacomEquipment"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Snapshots/BattleSnapshot.h"

TSharedRef<SWidget> UEquipmentBar::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		FrameBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FrameBorder"));
		FrameBorder->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.15f, 0.85f));
		FrameBorder->SetPadding(FMargin(8, 4));
		WidgetTree->RootWidget = FrameBorder;

		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		TitleText->SetText(LOCTEXT("EquipNone", "装备: (无)"));
		TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.85f, 0.95f)));
		FrameBorder->SetContent(TitleText);
	}
	return Super::RebuildWidget();
}

void UEquipmentBar::NativeRefreshFromSnapshot(const FBattleSnapshot& /*Snap*/)
{
	// 第一阶段 Snapshot 没有装备数据，保持占位。
	if (TitleText)
	{
		TitleText->SetText(LOCTEXT("EquipNone", "装备: (无)"));
	}
}

#undef LOCTEXT_NAMESPACE

