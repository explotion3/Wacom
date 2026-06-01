// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattleCombatLogBlockWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	FSlateColor GetToneTextColor(EWacomBattleEventVisualTone Tone)
	{
		switch (Tone)
		{
		case EWacomBattleEventVisualTone::Positive:
			return FSlateColor(FLinearColor(0.55f, 0.95f, 0.62f, 1.0f));
		case EWacomBattleEventVisualTone::Warning:
			return FSlateColor(FLinearColor(1.0f, 0.78f, 0.34f, 1.0f));
		case EWacomBattleEventVisualTone::Danger:
			return FSlateColor(FLinearColor(1.0f, 0.42f, 0.38f, 1.0f));
		case EWacomBattleEventVisualTone::System:
			return FSlateColor(FLinearColor(0.62f, 0.78f, 1.0f, 1.0f));
		default:
			return FSlateColor(FLinearColor(0.94f, 0.92f, 0.86f, 1.0f));
		}
	}
}

TSharedRef<SWidget> UBattleCombatLogBlockWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HeaderText"));
		HeaderText->SetAutoWrapText(true);
		{
			FSlateFontInfo Font = HeaderText->GetFont();
			Font.Size = 14;
			HeaderText->SetFont(Font);
		}
		if (UVerticalBoxSlot* HeaderSlot = Root->AddChildToVerticalBox(HeaderText))
		{
			HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
		}

		DetailsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DetailsBox"));
		Root->AddChildToVerticalBox(DetailsBox);
	}
	return Super::RebuildWidget();
}

void UBattleCombatLogBlockWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyCurrentBlockToWidgets();
}

void UBattleCombatLogBlockWidget::SetCombatLogBlockData(
	const FWacomBattleCombatLogBlockView& InBlock)
{
	CurrentBlock = InBlock;
	ApplyCurrentBlockToWidgets();
	BP_OnCombatLogBlockUpdated(CurrentBlock);
}

void UBattleCombatLogBlockWidget::ApplyCurrentBlockToWidgets()
{
	if (HeaderText)
	{
		HeaderText->SetText(CurrentBlock.HeaderText);
		HeaderText->SetColorAndOpacity(GetToneTextColor(CurrentBlock.VisualTone));
	}

	if (!DetailsBox || !WidgetTree)
	{
		return;
	}

	DetailsBox->ClearChildren();
	for (const FWacomBattleCombatLogLineView& Line : CurrentBlock.DetailLines)
	{
		if (Line.MessageText.IsEmpty())
		{
			continue;
		}

		UTextBlock* LineText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LineText->SetText(FText::Format(NSLOCTEXT("WacomBattleCombatLog", "DetailLine", "  {0}"), Line.MessageText));
		LineText->SetAutoWrapText(true);
		LineText->SetColorAndOpacity(GetToneTextColor(Line.VisualTone));
		{
			FSlateFontInfo Font = LineText->GetFont();
			Font.Size = 12;
			LineText->SetFont(Font);
		}

		if (UVerticalBoxSlot* LineSlot = DetailsBox->AddChildToVerticalBox(LineText))
		{
			LineSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));
		}
	}
}
