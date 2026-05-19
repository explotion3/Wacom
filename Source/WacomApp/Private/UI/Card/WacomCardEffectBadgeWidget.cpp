// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardEffectBadgeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

#define LOCTEXT_NAMESPACE "WacomCardEffectBadge"

TSharedRef<SWidget> UWacomCardEffectBadgeWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_EffectBadge"));
		}

		BadgeBody = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BadgeBody"));
		BadgeBody->SetPadding(FMargin(6.f, 4.f));
		WidgetTree->RootWidget = BadgeBody;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BadgeRow"));
		BadgeBody->AddChild(Row);

		IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IconImage"));
		IconImage->SetDesiredSizeOverride(FVector2D(12.f, 12.f));
		Row->AddChild(IconImage);

		UVerticalBox* TextColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TextColumn"));
		Row->AddChild(TextColumn);

		ValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ValueText"));
		ValueText->SetJustification(ETextJustify::Center);
		TextColumn->AddChildToVerticalBox(ValueText);

		LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LabelText"));
		LabelText->SetJustification(ETextJustify::Center);
		{
			FSlateFontInfo Font = LabelText->GetFont();
			Font.Size = 9;
			LabelText->SetFont(Font);
		}
		TextColumn->AddChildToVerticalBox(LabelText);
	}

	return Super::RebuildWidget();
}

void UWacomCardEffectBadgeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyCurrentDataToWidgets();
}

void UWacomCardEffectBadgeWidget::SetEffectBadgeData(const FWacomCardViewEffectBadge& InData)
{
	CurrentData = InData;
	ApplyCurrentDataToWidgets();
}

FText UWacomCardEffectBadgeWidget::GetValueText() const
{
	return ValueText ? ValueText->GetText() : FText::AsNumber(CurrentData.Value);
}

FText UWacomCardEffectBadgeWidget::GetLabelText() const
{
	return LabelText ? LabelText->GetText() : BuildLabelText(CurrentData.Kind);
}

void UWacomCardEffectBadgeWidget::ApplyCurrentDataToWidgets()
{
	if (BadgeBody)
	{
		BadgeBody->SetBrushColor(BuildBadgeColor(CurrentData.Kind));
	}

	if (IconImage)
	{
		IconImage->SetColorAndOpacity(FLinearColor::White);
		IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (ValueText)
	{
		ValueText->SetText(FText::AsNumber(CurrentData.Value));
		ValueText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (LabelText)
	{
		LabelText->SetText(BuildLabelText(CurrentData.Kind));
		LabelText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

FText UWacomCardEffectBadgeWidget::BuildLabelText(EWacomCardViewEffectBadgeKind Kind)
{
	switch (Kind)
	{
	case EWacomCardViewEffectBadgeKind::Damage:
		return LOCTEXT("DamageLabel", "伤害");
	case EWacomCardViewEffectBadgeKind::Heal:
		return LOCTEXT("HealLabel", "治疗");
	case EWacomCardViewEffectBadgeKind::Poison:
		return LOCTEXT("PoisonLabel", "中毒");
	case EWacomCardViewEffectBadgeKind::Slow:
		return LOCTEXT("SlowLabel", "减速");
	case EWacomCardViewEffectBadgeKind::Freeze:
		return LOCTEXT("FreezeLabel", "冻结");
	case EWacomCardViewEffectBadgeKind::Twilight:
		return LOCTEXT("TwilightLabel", "暮气");
	case EWacomCardViewEffectBadgeKind::Draw:
		return LOCTEXT("DrawLabel", "抽牌");
	case EWacomCardViewEffectBadgeKind::Discard:
		return LOCTEXT("DiscardLabel", "弃牌");
	case EWacomCardViewEffectBadgeKind::Initiative:
		return LOCTEXT("InitiativeLabel", "先机");
	case EWacomCardViewEffectBadgeKind::Cost:
		return LOCTEXT("CostLabel", "费用");
	default:
		return LOCTEXT("GenericLabel", "效果");
	}
}

FLinearColor UWacomCardEffectBadgeWidget::BuildBadgeColor(EWacomCardViewEffectBadgeKind Kind)
{
	switch (Kind)
	{
	case EWacomCardViewEffectBadgeKind::Damage:
		return FLinearColor(0.72f, 0.12f, 0.08f, 0.92f);
	case EWacomCardViewEffectBadgeKind::Heal:
		return FLinearColor(0.12f, 0.55f, 0.22f, 0.92f);
	case EWacomCardViewEffectBadgeKind::Poison:
		return FLinearColor(0.25f, 0.55f, 0.18f, 0.92f);
	case EWacomCardViewEffectBadgeKind::Slow:
		return FLinearColor(0.70f, 0.45f, 0.12f, 0.92f);
	case EWacomCardViewEffectBadgeKind::Freeze:
		return FLinearColor(0.16f, 0.45f, 0.75f, 0.92f);
	case EWacomCardViewEffectBadgeKind::Twilight:
		return FLinearColor(0.42f, 0.24f, 0.70f, 0.92f);
	default:
		return FLinearColor(0.18f, 0.18f, 0.18f, 0.92f);
	}
}

#undef LOCTEXT_NAMESPACE
