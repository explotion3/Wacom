// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleCombatLogDetailsEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "UI/Battle/WacomBattleStatusPresentationCatalog.h"
#include "UI/Battle/WacomBattleStatusPresentationCatalogProvider.h"
#include "UI/Battle/WacomBattleStatusTooltipPresentation.h"
#include "UI/Battle/WacomBattleStatusTooltipWidget.h"

namespace
{
	constexpr float RootIconSize = 28.0f;
	constexpr float ResultIconSize = 24.0f;
	constexpr float ResultIndent = 28.0f;
	constexpr float FactIndent = 52.0f;
	constexpr float DetailsTextWrapWidth = 520.0f;

	FLinearColor ResolveDetailsToneColor(
		const EWacomBattleEventVisualTone Tone)
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
			return FLinearColor(0.92f, 0.94f, 1.0f, 1.0f);
		}
	}

	void StyleDetailsText(
		UTextBlock& Text,
		const int32 FontSize,
		const FLinearColor& Color,
		const bool bBold)
	{
		FSlateFontInfo Font = Text.GetFont();
		Font.Size = FontSize;
		Font.TypefaceFontName = bBold ? TEXT("Bold") : TEXT("Regular");
		Text.SetFont(Font);
		Text.SetColorAndOpacity(FSlateColor(Color));
		Text.SetAutoWrapText(true);
		Text.SetWrapTextAt(DetailsTextWrapWidth);
		Text.SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

UWacomBattleCombatLogDetailsEntryWidget::
	UWacomBattleCombatLogDetailsEntryWidget(
		const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	StatusTooltipWidgetClass = UWacomBattleStatusTooltipWidget::StaticClass();
}

void UWacomBattleCombatLogDetailsEntryWidget::SetDetailsEntryData(
	const FWacomBattleCombatLogDetailsEntryView& InEntry,
	const FSlateBrush& InIconBrush)
{
	CurrentEntry = InEntry;
	CurrentEntry.Depth = FMath::Clamp(CurrentEntry.Depth, 0, 2);
	CurrentIconBrush = InIconBrush;
	bHasEntry = true;
	CachedStatusTooltip = nullptr;
	ApplyCurrentEntry();
}

void UWacomBattleCombatLogDetailsEntryWidget::ClearDetailsEntry()
{
	bHasEntry = false;
	CurrentEntry = FWacomBattleCombatLogDetailsEntryView();
	CachedStatusTooltip = nullptr;
	ClearStatusTooltipBinding();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UWacomBattleCombatLogDetailsEntryWidget::SetStatusTooltipWidgetClass(
	TSubclassOf<UWacomBattleStatusTooltipWidget> InTooltipWidgetClass)
{
	StatusTooltipWidgetClass = InTooltipWidgetClass
		? InTooltipWidgetClass
		: TSubclassOf<UWacomBattleStatusTooltipWidget>(
			UWacomBattleStatusTooltipWidget::StaticClass());
	CachedStatusTooltip = nullptr;
	RefreshStatusTooltipBinding();
}

float UWacomBattleCombatLogDetailsEntryWidget::GetAppliedIndentWidth() const
{
	if (!IndentSpacer)
	{
		return 0.0f;
	}
	return IndentSpacer->GetWidthOverride();
}

bool UWacomBattleCombatLogDetailsEntryWidget::HasHistoricalStatusTooltip() const
{
	return bHasEntry
		&& CurrentEntry.bShowStatusTooltip
		&& CurrentEntry.IconTag.IsValid()
		&& CurrentEntry.SourceEventType == EBattleEventType::StatusApplied;
}

TSharedRef<SWidget> UWacomBattleCombatLogDetailsEntryWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(
				this,
				TEXT("WidgetTree_Default"));
		}

		DetailsEntrySize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			TEXT("DetailsEntrySize"));
		DetailsEntrySize->SetMinDesiredHeight(34.0f);
		DetailsEntrySize->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		WidgetTree->RootWidget = DetailsEntrySize;

		EntryRoot = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("EntryRoot"));
		EntryRoot->SetPadding(FMargin(6.0f, 5.0f));
		EntryRoot->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		DetailsEntrySize->SetContent(EntryRoot);

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			TEXT("EntryRow"));
		Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		EntryRoot->SetContent(Row);

		IndentSpacer = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			TEXT("IndentSpacer"));
		IndentSpacer->SetWidthOverride(0.0f);
		IndentSpacer->SetHeightOverride(1.0f);
		IndentSpacer->SetVisibility(ESlateVisibility::HitTestInvisible);
		Row->AddChildToHorizontalBox(IndentSpacer);

		EntryIconSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			TEXT("EntryIconSize"));
		EntryIconSize->SetWidthOverride(ResultIconSize);
		EntryIconSize->SetHeightOverride(ResultIconSize);
		EntryIconSize->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UHorizontalBoxSlot* IconSlot =
			Row->AddChildToHorizontalBox(EntryIconSize))
		{
			IconSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
			IconSlot->SetVerticalAlignment(VAlign_Top);
		}

		EntryIcon = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			TEXT("EntryIcon"));
		EntryIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		EntryIconSize->SetContent(EntryIcon);

		UWrapBox* ContentWrap = WidgetTree->ConstructWidget<UWrapBox>(
			UWrapBox::StaticClass(),
			TEXT("ContentWrap"));
		ContentWrap->SetInnerSlotPadding(FVector2D(5.0f, 2.0f));
		ContentWrap->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UHorizontalBoxSlot* ContentSlot =
			Row->AddChildToHorizontalBox(ContentWrap))
		{
			ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ContentSlot->SetVerticalAlignment(VAlign_Center);
		}

		TargetText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("TargetText"));
		StyleDetailsText(
			*TargetText,
			16,
			FLinearColor(0.72f, 0.80f, 0.90f, 1.0f),
			true);
		ContentWrap->AddChildToWrapBox(TargetText);

		MessageText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("MessageText"));
		StyleDetailsText(
			*MessageText,
			16,
			FLinearColor(0.92f, 0.94f, 1.0f, 1.0f),
			false);
		ContentWrap->AddChildToWrapBox(MessageText);

		ValueText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("ValueText"));
		StyleDetailsText(
			*ValueText,
			16,
			FLinearColor(0.98f, 0.80f, 0.30f, 1.0f),
			true);
		ContentWrap->AddChildToWrapBox(ValueText);
	}
	return Super::RebuildWidget();
}

void UWacomBattleCombatLogDetailsEntryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyCurrentEntry();
}

void UWacomBattleCombatLogDetailsEntryWidget::NativeDestruct()
{
	CachedStatusTooltip = nullptr;
	ClearStatusTooltipBinding();
	Super::NativeDestruct();
}

void UWacomBattleCombatLogDetailsEntryWidget::ApplyCurrentEntry()
{
	if (!bHasEntry)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	const int32 SafeDepth = FMath::Clamp(CurrentEntry.Depth, 0, 2);
	const bool bRoot =
		CurrentEntry.EntryKind
			== EWacomBattleCombatLogDetailsEntryKind::RootAction;
	const bool bFact =
		CurrentEntry.EntryKind
			== EWacomBattleCombatLogDetailsEntryKind::Fact;

	if (DetailsEntrySize)
	{
		DetailsEntrySize->SetMinDesiredHeight(bRoot ? 40.0f : 34.0f);
	}
	if (IndentSpacer)
	{
		IndentSpacer->SetWidthOverride(
			SafeDepth <= 0
				? 0.0f
				: (SafeDepth == 1 ? ResultIndent : FactIndent));
	}
	if (EntryRoot)
	{
		const float Alpha = bRoot ? 0.78f : (bFact ? 0.20f : 0.48f);
		EntryRoot->SetBrushColor(FLinearColor(
			0.025f,
			0.035f,
			0.055f,
			Alpha));
	}

	if (EntryIconSize)
	{
		const float IconSize = bRoot ? RootIconSize : ResultIconSize;
		EntryIconSize->SetWidthOverride(IconSize);
		EntryIconSize->SetHeightOverride(IconSize);
		EntryIconSize->SetVisibility(
			bFact
				? ESlateVisibility::Collapsed
				: ESlateVisibility::SelfHitTestInvisible);
	}
	if (EntryIcon)
	{
		EntryIcon->SetBrush(CurrentIconBrush);
	}

	if (TargetText)
	{
		TargetText->SetText(CurrentEntry.TargetLabel);
		TargetText->SetVisibility(
			CurrentEntry.TargetLabel.IsEmpty()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::HitTestInvisible);
	}
	if (MessageText)
	{
		MessageText->SetText(CurrentEntry.MessageText);
		const FLinearColor Tone = ResolveDetailsToneColor(
			CurrentEntry.VisualTone);
		MessageText->SetColorAndOpacity(FSlateColor(
			bFact
				? FLinearColor(Tone.R, Tone.G, Tone.B, 0.82f)
				: Tone));
		MessageText->SetVisibility(
			CurrentEntry.MessageText.IsEmpty()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::HitTestInvisible);
	}
	if (ValueText)
	{
		ValueText->SetText(CurrentEntry.ValueText);
		ValueText->SetColorAndOpacity(FSlateColor(
			ResolveDetailsToneColor(CurrentEntry.VisualTone)));
		ValueText->SetVisibility(
			CurrentEntry.ValueText.IsEmpty()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::HitTestInvisible);
	}

	RefreshStatusTooltipBinding();
}

void UWacomBattleCombatLogDetailsEntryWidget::RefreshStatusTooltipBinding()
{
	if (!EntryIcon)
	{
		return;
	}
	if (!HasHistoricalStatusTooltip())
	{
		ClearStatusTooltipBinding();
		EntryIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		return;
	}

	EntryIcon->SetToolTip(nullptr);
	EntryIcon->SetToolTipText(FText::GetEmpty());
	EntryIcon->ToolTipWidgetDelegate.Unbind();
	EntryIcon->ToolTipWidgetDelegate.BindDynamic(
		this,
		&UWacomBattleCombatLogDetailsEntryWidget::HandleBuildStatusTooltipWidget);
	EntryIcon->SetVisibility(ESlateVisibility::Visible);
	// Details rows are populated after their child Slate widgets may already
	// exist. Re-sync the image so UWidget installs the newly bound delegate on
	// the real Slate hover path instead of leaving it only on the UObject.
	EntryIcon->SynchronizeProperties();
}

void UWacomBattleCombatLogDetailsEntryWidget::ClearStatusTooltipBinding()
{
	if (!EntryIcon)
	{
		return;
	}
	EntryIcon->ToolTipWidgetDelegate.Unbind();
	EntryIcon->SetToolTip(nullptr);
	EntryIcon->SetToolTipText(FText::GetEmpty());
	CachedStatusTooltip = nullptr;
}

UWidget*
UWacomBattleCombatLogDetailsEntryWidget::HandleBuildStatusTooltipWidget()
{
	if (!HasHistoricalStatusTooltip())
	{
		return nullptr;
	}

	if (!CachedStatusTooltip)
	{
		UClass* TooltipClass = StatusTooltipWidgetClass
			? StatusTooltipWidgetClass.Get()
			: UWacomBattleStatusTooltipWidget::StaticClass();
		CachedStatusTooltip = GetWorld()
			? CreateWidget<UWacomBattleStatusTooltipWidget>(
				this,
				TooltipClass)
			: NewObject<UWacomBattleStatusTooltipWidget>(
				this,
				TooltipClass);
	}
	if (!CachedStatusTooltip)
	{
		return nullptr;
	}

	const UWacomBattleStatusPresentationCatalog& Catalog =
		WacomBattleStatusPresentationCatalogProvider::GetCatalog();
	FWacomBattleStatusIconView StatusView;
	StatusView.StatusTag = CurrentEntry.IconTag;
	StatusView.DisplayName = Catalog.ResolveDisplayName(CurrentEntry.IconTag);
	StatusView.StackCount = FMath::Max(
		1,
		FMath::Abs(CurrentEntry.StatusDelta));
	StatusView.InspectionHost = CurrentEntry.StatusInspectionHost;
	if (const FSlateBrush* CatalogBrush =
		Catalog.ResolveIconBrush(CurrentEntry.IconTag))
	{
		StatusView.IconBrush = *CatalogBrush;
	}
	else
	{
		StatusView.IconBrush = CurrentIconBrush;
	}
	FWacomBattleStatusTooltipPresentationBuilder::PopulateRuleText(
		StatusView);
	CachedStatusTooltip->SetHistoricalStatusEventView(
		StatusView,
		CurrentEntry.StatusDelta);
	return CachedStatusTooltip;
}
