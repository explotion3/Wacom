// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardView.h"

#include "Blueprint/WidgetTree.h"
#include "Cards/CardDefinition.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "WacomCardView"

namespace
{
	constexpr float DefaultCardWidth = 260.f;
	constexpr float DefaultCardHeight = 380.f;

	FText GetCardDisplayName(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return LOCTEXT("UnknownCardName", "未知卡牌");
		}
		return Card->DisplayName.IsEmpty()
			? FText::FromName(Card->CardId)
			: Card->DisplayName;
	}

	FString ShortGameplayTagName(const FGameplayTag& Tag)
	{
		FString TagName = Tag.GetTagName().ToString();
		int32 LastDot = INDEX_NONE;
		TagName.FindLastChar(TEXT('.'), LastDot);
		return LastDot != INDEX_NONE ? TagName.Mid(LastDot + 1) : TagName;
	}

	FText BuildTypeLine(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return FText::GetEmpty();
		}

		if (Card->Physique.Capacity > 0)
		{
			return Card->Physique.CapacityEffect.IsValid()
				? LOCTEXT("TypeContainerB", "容器")
				: LOCTEXT("TypeContainerA", "背包");
		}

		for (const FGameplayTag& Tag : Card->Keywords)
		{
			return FText::FromString(ShortGameplayTagName(Tag));
		}

		return FText::GetEmpty();
	}

	FText BuildCompactDescriptionText(const UCardDefinition* Card)
	{
		if (!Card || Card->Description.IsEmpty())
		{
			return FText::GetEmpty();
		}

		FString Text = Card->Description.ToString();
		Text.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
		Text.ReplaceInline(TEXT("\r"), TEXT("\n"));

		int32 FirstBreak = INDEX_NONE;
		if (Text.FindChar(TEXT('\n'), FirstBreak))
		{
			Text = Text.Left(FirstBreak);
		}

		constexpr int32 MaxChars = 28;
		if (Text.Len() > MaxChars)
		{
			Text = Text.Left(MaxChars).TrimEnd() + TEXT("...");
		}

		return FText::FromString(Text);
	}

	void SetOptionalText(UTextBlock* TextBlock, const FText& Text)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetText(Text);
		TextBlock->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

TSharedRef<SWidget> UWacomCardView::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CardViewRoot"));
		Root->SetWidthOverride(DefaultCardWidth);
		Root->SetHeightOverride(DefaultCardHeight);
		WidgetTree->RootWidget = Root;

		UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("CardViewOverlay"));
		Root->AddChild(Stack);

		UBorder* Body = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardBody"));
		Body->SetBrushColor(FLinearColor(0.06f, 0.055f, 0.045f, 0.95f));
		Body->SetPadding(FMargin(8.f));
		if (UOverlaySlot* BodySlot = Stack->AddChildToOverlay(Body))
		{
			BodySlot->SetHorizontalAlignment(HAlign_Fill);
			BodySlot->SetVerticalAlignment(VAlign_Fill);
		}

		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CardContent"));
		Body->AddChild(Content);

		CostText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CostText"));
		CostText->SetJustification(ETextJustify::Left);
		CostText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.85f, 0.65f, 1.f)));
		if (UVerticalBoxSlot* CostSlot = Content->AddChildToVerticalBox(CostText))
		{
			CostSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
		}

		NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
		NameText->SetJustification(ETextJustify::Center);
		NameText->SetAutoWrapText(true);
		NameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		{
			FSlateFontInfo Font = NameText->GetFont();
			Font.Size = 14;
			NameText->SetFont(Font);
		}
		if (UVerticalBoxSlot* NameSlot = Content->AddChildToVerticalBox(NameText))
		{
			NameSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}

		CardArt = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CardArt"));
		CardArt->SetColorAndOpacity(FLinearColor(0.18f, 0.18f, 0.18f, 1.f));
		if (UVerticalBoxSlot* ArtSlot = Content->AddChildToVerticalBox(CardArt))
		{
			ArtSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ArtSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}

		TypeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TypeText"));
		TypeText->SetJustification(ETextJustify::Center);
		TypeText->SetAutoWrapText(true);
		TypeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.75f, 0.65f, 1.f)));
		if (UVerticalBoxSlot* TypeSlot = Content->AddChildToVerticalBox(TypeText))
		{
			TypeSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}

		DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescriptionText"));
		DescriptionText->SetJustification(ETextJustify::Center);
		DescriptionText->SetAutoWrapText(true);
		DescriptionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.92f, 0.86f, 1.f)));
		{
			FSlateFontInfo Font = DescriptionText->GetFont();
			Font.Size = 11;
			DescriptionText->SetFont(Font);
		}
		if (UVerticalBoxSlot* DescSlot = Content->AddChildToVerticalBox(DescriptionText))
		{
			DescSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		DisabledOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DisabledOverlay"));
		DisabledOverlay->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.45f));
		DisabledOverlay->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* DisabledSlot = Stack->AddChildToOverlay(DisabledOverlay))
		{
			DisabledSlot->SetHorizontalAlignment(HAlign_Fill);
			DisabledSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	return Super::RebuildWidget();
}

void UWacomCardView::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyCurrentDataToWidgets();
}

void UWacomCardView::SetCardViewData(const FWacomCardViewData& InData)
{
	CurrentData = InData;
	ApplyCurrentDataToWidgets();
}

FWacomCardViewData UWacomCardView::BuildFromCardDefinition(const UCardDefinition* Card)
{
	FWacomCardViewData Data;
	Data.Name = GetCardDisplayName(Card);
	Data.TypeText = BuildTypeLine(Card);
	Data.Description = BuildCompactDescriptionText(Card);
	Data.Cost = Card ? Card->BaseCost : 0;
	Data.bShowCost = Card != nullptr;
	return Data;
}

void UWacomCardView::ApplyCurrentDataToWidgets()
{
	if (CostText)
	{
		CostText->SetText(FText::AsNumber(CurrentData.Cost));
		CostText->SetVisibility(CurrentData.bShowCost ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	SetOptionalText(NameText, CurrentData.Name);
	SetOptionalText(TypeText, CurrentData.TypeText);
	SetOptionalText(DescriptionText, CurrentData.Description);

	if (CardArt)
	{
		if (CurrentData.Art)
		{
			CardArt->SetBrushFromTexture(CurrentData.Art);
			CardArt->SetColorAndOpacity(FLinearColor::White);
		}
	}

	if (DisabledOverlay)
	{
		DisabledOverlay->SetVisibility(CurrentData.bDisabled ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

#undef LOCTEXT_NAMESPACE
