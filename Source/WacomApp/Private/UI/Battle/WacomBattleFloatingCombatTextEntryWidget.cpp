// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleFloatingCombatTextEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/Battle/WacomBattleFloatingCombatTextStyle.h"
#include "UI/Battle/WacomBattleFloatingCombatTextTypes.h"
#include "UI/Battle/WacomBattleStatusPresentationCatalog.h"
#include "UI/Battle/WacomBattleStatusPresentationCatalogProvider.h"

#define LOCTEXT_NAMESPACE "WacomBattleFloatingCombatTextEntry"

namespace
{
	void ConfigureText(UTextBlock& Text, int32 Size)
	{
		FSlateFontInfo Font = Text.GetFont();
		Font.Size = Size;
		Font.TypefaceFontName = TEXT("Bold");
		Text.SetFont(Font);
		Text.SetShadowOffset(FVector2D(1.0f, 2.0f));
		Text.SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
		Text.SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UWacomBattleFloatingCombatTextEntryWidget::ApplyRow(
	const FWacomBattleFloatingCombatTextRow& Row,
	const UWacomBattleFloatingCombatTextStyle& Style)
{
	FLinearColor Color = Style.HpDamageColor;
	FSlateBrush Icon;
	bool bShowIcon = false;
	bool bShowCritical = false;
	FText Value;

	switch (Row.Kind)
	{
	case EWacomBattleFloatingCombatTextKind::ShieldAbsorbed:
		Color = Style.ShieldColor;
		Icon = Style.ShieldIconBrush;
		bShowIcon = Icon.GetResourceObject() != nullptr || Icon.GetImageSize() != FVector2D::ZeroVector;
		Value = FText::Format(LOCTEXT("ShieldLoss", "-{0}"), FText::AsNumber(FMath::Abs(Row.Amount)));
		break;
	case EWacomBattleFloatingCombatTextKind::ShieldChanged:
		Color = Style.ShieldColor;
		Icon = Style.ShieldIconBrush;
		bShowIcon = Icon.GetResourceObject() != nullptr || Icon.GetImageSize() != FVector2D::ZeroVector;
		Value = Row.Amount >= 0
			? FText::Format(LOCTEXT("ShieldGain", "+{0}"), FText::AsNumber(Row.Amount))
			: FText::Format(LOCTEXT("ShieldChangeLoss", "-{0}"), FText::AsNumber(FMath::Abs(Row.Amount)));
		break;
	case EWacomBattleFloatingCombatTextKind::PeriodicDamage:
		Color = Style.PeriodicDamageColor;
		if (const FWacomBattleStatusPresentationEntry* StatusEntry =
			WacomBattleStatusPresentationCatalogProvider::GetCatalog().FindEntry(Row.IconTag))
		{
			Icon = StatusEntry->IconBrush;
			bShowIcon = true;
		}
		Value = FText::Format(LOCTEXT("PeriodicDamage", "-{0}"), FText::AsNumber(FMath::Abs(Row.Amount)));
		break;
	case EWacomBattleFloatingCombatTextKind::CriticalDamage:
		Color = Style.CriticalDamageColor;
		Icon = Style.CriticalIconBrush;
		bShowIcon = Icon.GetResourceObject() != nullptr || Icon.GetImageSize() != FVector2D::ZeroVector;
		bShowCritical = true;
		Value = FText::Format(LOCTEXT("CriticalDamage", "-{0}"), FText::AsNumber(FMath::Abs(Row.Amount)));
		break;
	case EWacomBattleFloatingCombatTextKind::HpDamage:
	default:
		Value = FText::Format(LOCTEXT("HpDamage", "-{0}"), FText::AsNumber(FMath::Abs(Row.Amount)));
		break;
	}

	if (SemanticIcon)
	{
		SemanticIcon->SetBrush(Icon);
		SemanticIcon->SetColorAndOpacity(Color);
		SemanticIcon->SetVisibility(
			bShowIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (CriticalText)
	{
		CriticalText->SetText(LOCTEXT("CriticalLabel", "暴击"));
		CriticalText->SetColorAndOpacity(FSlateColor(Color));
		CriticalText->SetVisibility(
			bShowCritical ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (ValueText)
	{
		ValueText->SetText(Value);
		ValueText->SetColorAndOpacity(FSlateColor(Color));
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UWacomBattleFloatingCombatTextEntryWidget::ApplyPlaybackFrame(
	const float Opacity,
	const FVector2D& Translation,
	const float Scale)
{
	SetRenderOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));
	SetRenderTranslation(Translation);
	SetRenderScale(FVector2D(FMath::Max(0.01f, Scale)));
}

void UWacomBattleFloatingCombatTextEntryWidget::ResetForPool()
{
	SetRenderOpacity(0.0f);
	SetRenderTranslation(FVector2D::ZeroVector);
	SetRenderScale(FVector2D(1.0f));
	SetVisibility(ESlateVisibility::Collapsed);
}

TSharedRef<SWidget> UWacomBattleFloatingCombatTextEntryWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("EntryRoot"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("EntryRow"));
		Row->SetVisibility(ESlateVisibility::HitTestInvisible);
		Root->AddChild(Row);

		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("SemanticIconBox"));
		IconBox->SetWidthOverride(28.0f);
		IconBox->SetHeightOverride(28.0f);
		IconBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UHorizontalBoxSlot* RowSlot = Row->AddChildToHorizontalBox(IconBox))
		{
			RowSlot->SetVerticalAlignment(VAlign_Center);
			RowSlot->SetPadding(FMargin(0.0f, 0.0f, 5.0f, 0.0f));
		}
		SemanticIcon = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("SemanticIcon"));
		SemanticIcon->SetVisibility(ESlateVisibility::Collapsed);
		IconBox->AddChild(SemanticIcon);

		CriticalText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("CriticalText"));
		ConfigureText(*CriticalText, 18);
		if (UHorizontalBoxSlot* RowSlot = Row->AddChildToHorizontalBox(CriticalText))
		{
			RowSlot->SetVerticalAlignment(VAlign_Center);
			RowSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
		}

		ValueText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("ValueText"));
		ConfigureText(*ValueText, 25);
		if (UHorizontalBoxSlot* RowSlot = Row->AddChildToHorizontalBox(ValueText))
		{
			RowSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	return Super::RebuildWidget();
}

#undef LOCTEXT_NAMESPACE
