// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardDetailSectionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Card/WacomCardDetailRichTextBlock.h"
#include "UI/Card/WacomCardDetailTheme.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"

namespace
{
	UTextBlock* MakeSectionTitleText(
		UWidgetTree* WidgetTree,
		const FName Name,
		const FText& Text)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TextBlock->SetText(Text);
		TextBlock->SetAutoWrapText(true);
		TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.82f, 1.f, 1.f)));
		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = 13;
		TextBlock->SetFont(Font);
		return TextBlock;
	}
}

UWacomCardDetailSectionWidget::UWacomCardDetailSectionWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UWacomCardDetailSectionWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardDetailSectionRoot"));
		RootBorder->SetBrushColor(FLinearColor(0.06f, 0.055f, 0.045f, 0.92f));
		RootBorder->SetPadding(FMargin(10.f, 8.f));
		WidgetTree->RootWidget = RootBorder;

		UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CardDetailSectionContent"));
		RootBorder->AddChild(RootBox);

		TitleText = MakeSectionTitleText(WidgetTree, TEXT("TitleText"), FText::GetEmpty());
		if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 5.f));
		}

		LinesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LinesBox"));
		RootBox->AddChildToVerticalBox(LinesBox);

		BodyText = WidgetTree->ConstructWidget<UWacomCardDetailRichTextBlock>(
			UWacomCardDetailRichTextBlock::StaticClass(),
			TEXT("BodyText"));
		BodyText->SetAutoWrapText(true);
		if (UVerticalBoxSlot* BodySlot = Cast<UVerticalBoxSlot>(LinesBox->AddChild(BodyText)))
		{
			BodySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
		}
	}

	return Super::RebuildWidget();
}

void UWacomCardDetailSectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyCurrentDataToWidgets();
}

void UWacomCardDetailSectionWidget::SetSectionData(
	const FWacomCardDetailSectionData& InData)
{
	CurrentData = InData;
	ApplyCurrentDataToWidgets();
}

void UWacomCardDetailSectionWidget::ApplyCurrentDataToWidgets()
{
	if (TitleText)
	{
		if (const UWacomCardDetailTheme* Theme = ResolveTheme())
		{
			if (UCommonTextBlock* CommonTitleText = Cast<UCommonTextBlock>(TitleText))
			{
				CommonTitleText->SetStyle(Theme->TitleTextStyle);
			}
		}
		TitleText->SetText(CurrentData.Title);
		TitleText->SetVisibility(CurrentData.Title.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	UWacomCardDetailRichTextBlock* EffectiveBodyText = EnsureBodyTextWidget();
	if (EffectiveBodyText)
	{
		EffectiveBodyText->SetCardDetailRichText(CurrentData.RichText, ResolveTheme());
		EffectiveBodyText->SetVisibility(CurrentData.RichText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (LinesBox)
	{
		LinesBox->SetVisibility(CurrentData.RichText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

UWacomCardDetailRichTextBlock* UWacomCardDetailSectionWidget::EnsureBodyTextWidget()
{
	if (BodyText)
	{
		return BodyText;
	}

	if (!WidgetTree)
	{
		return nullptr;
	}

	BodyText = WidgetTree->ConstructWidget<UWacomCardDetailRichTextBlock>(
		UWacomCardDetailRichTextBlock::StaticClass(),
		TEXT("BodyText"));
	BodyText->SetAutoWrapText(true);

	if (LinesBox)
	{
		if (UVerticalBoxSlot* BodySlot = Cast<UVerticalBoxSlot>(LinesBox->AddChild(BodyText)))
		{
			BodySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
		}
	}

	return BodyText;
}

const UWacomCardDetailTheme* UWacomCardDetailSectionWidget::ResolveTheme() const
{
	if (CardDetailTheme)
	{
		return CardDetailTheme;
	}

	const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
	return Settings && !Settings->CardDetailTheme.IsNull()
		? Settings->CardDetailTheme.LoadSynchronous()
		: nullptr;
}
