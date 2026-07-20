// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattleCombatActivityRowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

namespace
{
	FLinearColor ResolveToneColor(EWacomBattleEventVisualTone Tone)
	{
		switch (Tone)
		{
		case EWacomBattleEventVisualTone::Positive:
			return FLinearColor(0.58f, 0.90f, 0.76f, 1.0f);
		case EWacomBattleEventVisualTone::Warning:
			return FLinearColor(0.96f, 0.78f, 0.38f, 1.0f);
		case EWacomBattleEventVisualTone::Danger:
			return FLinearColor(0.96f, 0.42f, 0.50f, 1.0f);
		case EWacomBattleEventVisualTone::System:
			return FLinearColor(0.56f, 0.78f, 1.0f, 1.0f);
		default:
			return FLinearColor(0.94f, 0.95f, 1.0f, 1.0f);
		}
	}
}

TSharedRef<SWidget> UBattleCombatActivityRowWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}
		RowRoot = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RowRoot"));
		RowRoot->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.055f, 0.76f));
		RowRoot->SetPadding(FMargin(6.0f, 3.0f));
		WidgetTree->RootWidget = RowRoot;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
		RowRoot->SetContent(Row);
		IndentSpacer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("IndentSpacer"));
		IndentSpacer->SetWidthOverride(0.0f);
		IndentSpacer->SetHeightOverride(1.0f);
		IndentSpacer->SetVisibility(ESlateVisibility::HitTestInvisible);
		Row->AddChildToHorizontalBox(IndentSpacer);
		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("IconSize"));
		IconSize->SetWidthOverride(32.0f);
		IconSize->SetHeightOverride(32.0f);
		ActivityIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ActivityIcon"));
		ActivityIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		IconSize->SetContent(ActivityIcon);
		if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconSize))
		{
			IconSlot->SetPadding(FMargin(0.0f, 0.0f, 7.0f, 0.0f));
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}

		ActivityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ActivityText"));
		FSlateFontInfo Font = ActivityText->GetFont();
		Font.Size = 16;
		ActivityText->SetFont(Font);
		if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(ActivityText))
		{
			TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	return Super::RebuildWidget();
}

void UBattleCombatActivityRowWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	ApplyCurrentRow();
}

void UBattleCombatActivityRowWidget::SetActivityRowData(
	const FWacomBattleCombatActivityRowView& InRow,
	const FSlateBrush& InIconBrush)
{
	CurrentRow = InRow;
	CurrentIconBrush = InIconBrush;
	bHasRow = true;
	ApplyCurrentRow();
	BP_OnActivityRowUpdated(CurrentRow);
}

void UBattleCombatActivityRowWidget::SetPlaybackPresentation(
	const float Opacity,
	const float ContentOpacity,
	const float IconOpacity,
	const float TranslationY)
{
	PlaybackOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
	PlaybackContentOpacity = FMath::Clamp(ContentOpacity, 0.0f, 1.0f);
	PlaybackIconOpacity = FMath::Clamp(IconOpacity, 0.0f, 1.0f);
	PlaybackTranslationY = TranslationY;
	ApplyPlaybackPresentation();
}

void UBattleCombatActivityRowWidget::ClearActivityRow()
{
	bHasRow = false;
	PlaybackOpacity = 1.0f;
	PlaybackContentOpacity = 1.0f;
	PlaybackIconOpacity = 1.0f;
	PlaybackTranslationY = 0.0f;
	ApplyPlaybackPresentation();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UBattleCombatActivityRowWidget::ApplyCurrentRow()
{
	if (!bHasRow)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (ActivityText)
	{
		ActivityText->SetText(CurrentRow.MessageText);
		ActivityText->SetColorAndOpacity(FSlateColor(ResolveToneColor(CurrentRow.VisualTone)));
	}
	if (ActivityIcon)
	{
		ActivityIcon->SetBrush(CurrentIconBrush);
		ActivityIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (IndentSpacer)
	{
		IndentSpacer->SetWidthOverride(
			CurrentRow.RowKind == EWacomBattleCombatActivityRowKind::RootAction ? 0.0f : 22.0f);
	}
	ApplyPlaybackPresentation();
}

void UBattleCombatActivityRowWidget::ApplyPlaybackPresentation()
{
	SetRenderOpacity(PlaybackOpacity);
	SetRenderTranslation(FVector2D(0.0f, PlaybackTranslationY));
	if (ActivityText)
	{
		ActivityText->SetRenderOpacity(PlaybackContentOpacity);
	}
	if (ActivityIcon)
	{
		ActivityIcon->SetRenderOpacity(PlaybackIconOpacity);
	}
	if (RowRoot)
	{
		const float RootAlpha = CurrentRow.RowKind == EWacomBattleCombatActivityRowKind::RootAction
			? 0.84f
			: 0.66f;
		RowRoot->SetBrushColor(FLinearColor(
			0.025f, 0.035f, 0.055f, RootAlpha * PlaybackContentOpacity));
	}
}
