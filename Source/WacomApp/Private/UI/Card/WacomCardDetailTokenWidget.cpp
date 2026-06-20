// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardDetailTokenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

#define LOCTEXT_NAMESPACE "WacomCardDetailTokenWidget"

namespace
{
	bool HasTypedTokenBinding(
		const UTextBlock* TextText,
		const UTextBlock* IconText,
		const UTextBlock* ValueText,
		const UTextBlock* PreviewArrowText,
		const UTextBlock* PreviewValueText)
	{
		return TextText || IconText || ValueText || PreviewArrowText || PreviewValueText;
	}

	void SetTextBlockVisibleText(UTextBlock* TextBlock, const FText& Text)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetText(Text);
		TextBlock->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	void SetWidgetFlagVisibility(UWidget* Widget, bool bVisible)
	{
		if (Widget)
		{
			Widget->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

UWacomCardDetailTokenWidget::UWacomCardDetailTokenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UWacomCardDetailTokenWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_CardDetailToken"));
		}

		TokenText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TokenText"));
		TokenText->SetAutoWrapText(true);
		WidgetTree->RootWidget = TokenText;
	}

	return Super::RebuildWidget();
}

void UWacomCardDetailTokenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bHasAppliedData = false;
	ApplyCurrentDataToWidgets();
}

void UWacomCardDetailTokenWidget::SetTokenData(const FWacomCardDetailToken& InData)
{
	if (bHasAppliedData && AreTokenDataEquivalent(CurrentData, InData))
	{
		return;
	}

	CurrentData = InData;
	ApplyCurrentDataToWidgets();
}

FText UWacomCardDetailTokenWidget::GetFallbackText() const
{
	return BuildTokenFallbackText(CurrentData);
}

FText UWacomCardDetailTokenWidget::GetIconFallbackText() const
{
	return BuildIconFallbackText(CurrentData.Icon);
}

FText UWacomCardDetailTokenWidget::GetValueText() const
{
	return CurrentData.bHasValue ? FText::AsNumber(CurrentData.Value) : FText::GetEmpty();
}

FText UWacomCardDetailTokenWidget::GetPreviewValueText() const
{
	return CurrentData.bHasPreviewValue ? FText::AsNumber(CurrentData.PreviewValue) : FText::GetEmpty();
}

FText UWacomCardDetailTokenWidget::BuildTokenFallbackText(const FWacomCardDetailToken& Token)
{
	switch (Token.Kind)
	{
	case EWacomCardDetailTokenKind::Icon:
		return BuildIconFallbackText(Token.Icon);
	case EWacomCardDetailTokenKind::Number:
		if (!Token.bHasValue)
		{
			return FText::GetEmpty();
		}
		if (Token.bHasPreviewValue)
		{
			return FText::Format(
				LOCTEXT("PreviewNumberFmt", "{0} -> {1}"),
				FText::AsNumber(Token.Value),
				FText::AsNumber(Token.PreviewValue));
		}
		return FText::AsNumber(Token.Value);
	case EWacomCardDetailTokenKind::Keyword:
	case EWacomCardDetailTokenKind::Text:
	default:
		return Token.Text;
	}
}

FText UWacomCardDetailTokenWidget::BuildIconFallbackText(EWacomCardDetailIcon Icon)
{
	switch (Icon)
	{
	case EWacomCardDetailIcon::Damage: return FText::FromString(TEXT("[伤]"));
	case EWacomCardDetailIcon::Heal: return FText::FromString(TEXT("[疗]"));
	case EWacomCardDetailIcon::Shield: return FText::FromString(TEXT("[盾]"));
	case EWacomCardDetailIcon::Poison: return FText::FromString(TEXT("[毒]"));
	case EWacomCardDetailIcon::Cost: return FText::FromString(TEXT("[费]"));
	case EWacomCardDetailIcon::Initiative: return FText::FromString(TEXT("[机]"));
	case EWacomCardDetailIcon::Draw: return FText::FromString(TEXT("[抽]"));
	case EWacomCardDetailIcon::Discard: return FText::FromString(TEXT("[弃]"));
	case EWacomCardDetailIcon::Exhaust: return FText::FromString(TEXT("[耗]"));
	case EWacomCardDetailIcon::Keyword: return FText::FromString(TEXT("[词]"));
	case EWacomCardDetailIcon::None:
	default:
		return FText::GetEmpty();
	}
}

void UWacomCardDetailTokenWidget::ApplyCurrentDataToWidgets()
{
	const FText FallbackText = GetFallbackText();
	const bool bUseTypedBindings =
		HasTypedTokenBinding(TextText, IconText, ValueText, PreviewArrowText, PreviewValueText);

	if (TokenText)
	{
		TokenText->SetText(FallbackText);
		TokenText->SetAutoWrapText(CurrentData.Kind == EWacomCardDetailTokenKind::Text);
		TokenText->SetVisibility(
			bUseTypedBindings || FallbackText.IsEmpty()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::HitTestInvisible);
		TokenText->SetColorAndOpacity(FSlateColor(GetFallbackColor()));
		FSlateFontInfo Font = TokenText->GetFont();
		Font.Size = FMath::Max(1, FallbackFontSize);
		TokenText->SetFont(Font);
	}

	const bool bTextLike =
		CurrentData.Kind == EWacomCardDetailTokenKind::Text
		|| CurrentData.Kind == EWacomCardDetailTokenKind::Keyword;
	const bool bIcon = CurrentData.Kind == EWacomCardDetailTokenKind::Icon;
	const bool bNumber = CurrentData.Kind == EWacomCardDetailTokenKind::Number;

	SetTextBlockVisibleText(TextText, bTextLike ? CurrentData.Text : FText::GetEmpty());
	if (TextText)
	{
		TextText->SetAutoWrapText(CurrentData.Kind == EWacomCardDetailTokenKind::Text);
	}
	SetTextBlockVisibleText(IconText, bIcon ? GetIconFallbackText() : FText::GetEmpty());
	SetTextBlockVisibleText(ValueText, bNumber ? GetValueText() : FText::GetEmpty());
	SetTextBlockVisibleText(PreviewArrowText, bNumber && CurrentData.bHasPreviewValue ? FText::FromString(TEXT("->")) : FText::GetEmpty());
	SetTextBlockVisibleText(PreviewValueText, bNumber ? GetPreviewValueText() : FText::GetEmpty());

	SetWidgetFlagVisibility(SkippedOverlay, CurrentData.bSkipped);
	SetWidgetFlagVisibility(EmphasisOverlay, CurrentData.bEmphasized || CurrentData.bHasPreviewValue);

	SetVisibility(FallbackText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(CurrentData.bSkipped ? SkippedOpacity : 1.0f);
	bHasAppliedData = true;
}

FLinearColor UWacomCardDetailTokenWidget::GetFallbackColor() const
{
	if (CurrentData.bEmphasized || CurrentData.bHasPreviewValue)
	{
		return EmphasizedColor;
	}

	switch (CurrentData.Kind)
	{
	case EWacomCardDetailTokenKind::Icon:
		return IconColor;
	case EWacomCardDetailTokenKind::Number:
		return NumberColor;
	case EWacomCardDetailTokenKind::Keyword:
	case EWacomCardDetailTokenKind::Text:
	default:
		return TextColor;
	}
}

bool UWacomCardDetailTokenWidget::AreTokenDataEquivalent(
	const FWacomCardDetailToken& A,
	const FWacomCardDetailToken& B)
{
	return A.StableId == B.StableId
		&& A.Kind == B.Kind
		&& A.Text.EqualTo(B.Text)
		&& A.Icon == B.Icon
		&& A.Value == B.Value
		&& A.bHasValue == B.bHasValue
		&& A.PreviewValue == B.PreviewValue
		&& A.bHasPreviewValue == B.bHasPreviewValue
		&& A.bSkipped == B.bSkipped
		&& A.bEmphasized == B.bEmphasized;
}

#undef LOCTEXT_NAMESPACE
