// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardDetailPanel.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Card/WacomCardDetailRichTextRenderer.h"
#include "UI/Card/WacomCardDetailSectionWidget.h"
#include "UI/Card/WacomCardDetailWidgetFactory.h"

#define LOCTEXT_NAMESPACE "WacomCardDetailPanel"

UWacomCardDetailPanel::UWacomCardDetailPanel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (UClass* Loaded = LoadObject<UClass>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_CardDetailSection.WBP_CardDetailSection_C")))
	{
		SectionWidgetClass = Loaded;
	}
	else
	{
		SectionWidgetClass = UWacomCardDetailSectionWidget::StaticClass();
	}
}

TSharedRef<SWidget> UWacomCardDetailPanel::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardDetailRoot"));
		RootBorder->SetBrushColor(FLinearColor(0.035f, 0.032f, 0.028f, 0.96f));
		RootBorder->SetPadding(FMargin(14.f));
		WidgetTree->RootWidget = RootBorder;

		SectionsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SectionsBox"));
		RootBorder->AddChild(SectionsBox);
	}

	return Super::RebuildWidget();
}

void UWacomCardDetailPanel::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyCurrentDataToWidgets();
}

void UWacomCardDetailPanel::SetCardDetailData(const FWacomCardDetailViewData& InData)
{
	CurrentData = InData;
	ApplyCurrentDataToWidgets();
}

FText UWacomCardDetailPanel::GetSectionTitleText(int32 Index) const
{
	return SectionWidgets.IsValidIndex(Index) && SectionWidgets[Index]
		? SectionWidgets[Index]->GetTitleText()
		: FText::GetEmpty();
}

void UWacomCardDetailPanel::ApplyCurrentDataToWidgets()
{
	if (!SectionsBox)
	{
		return;
	}

	SectionsBox->ClearChildren();
	SectionWidgets.Reset();

	for (const FWacomCardDetailSection& Section : CurrentData.Sections)
	{
		AddSection(Section);
	}

	SectionsBox->SetVisibility(
		SectionWidgets.Num() > 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
}

void UWacomCardDetailPanel::AddSection(const FWacomCardDetailSection& Section)
{
	if (!SectionsBox || Section.Blocks.IsEmpty())
	{
		return;
	}

	FWacomCardDetailSectionData SectionData;
	SectionData.Title = Section.Title;
	SectionData.Section = Section;
	SectionData.RichText = UWacomCardDetailRichTextRenderer::RenderSectionRichText(Section);

	UClass* WidgetClass = SectionWidgetClass
		? SectionWidgetClass.Get()
		: UWacomCardDetailSectionWidget::StaticClass();
	UWacomCardDetailSectionWidget* SectionWidget =
		WacomCardDetailWidgetFactory::CreateChildUserWidget<UWacomCardDetailSectionWidget>(
			*this,
			WidgetClass);
	if (!SectionWidget)
	{
		return;
	}

	SectionWidget->SetSectionData(SectionData);

	if (UVerticalBoxSlot* SectionSlot = Cast<UVerticalBoxSlot>(SectionsBox->AddChild(SectionWidget)))
	{
		SectionSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}
	SectionWidgets.Add(SectionWidget);
}

#undef LOCTEXT_NAMESPACE
