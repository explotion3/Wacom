// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardDetailPanel.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Card/WacomCardDetailSectionWidget.h"

#define LOCTEXT_NAMESPACE "WacomCardDetailPanel"

namespace
{
	bool IsNonEmptyTokenLine(const FWacomCardDetailTokenLine& TokenLine)
	{
		return !TokenLine.Tokens.IsEmpty();
	}

	bool IsDescriptionSectionTokenLine(const FWacomCardDetailTokenLine& TokenLine)
	{
		return IsNonEmptyTokenLine(TokenLine)
			&& (TokenLine.Kind == EWacomCardDetailTokenLineKind::Description
				|| TokenLine.Kind == EWacomCardDetailTokenLineKind::Effect);
	}

	bool IsPassiveTokenLine(const FWacomCardDetailTokenLine& TokenLine)
	{
		return IsNonEmptyTokenLine(TokenLine)
			&& TokenLine.Kind == EWacomCardDetailTokenLineKind::Passive;
	}

	TArray<FWacomCardDetailTokenLine> FilterTokenLines(
		const TArray<FWacomCardDetailTokenLine>& TokenLines,
		bool (*Predicate)(const FWacomCardDetailTokenLine&))
	{
		TArray<FWacomCardDetailTokenLine> FilteredLines;
		for (const FWacomCardDetailTokenLine& TokenLine : TokenLines)
		{
			if (Predicate(TokenLine))
			{
				FilteredLines.Add(TokenLine);
			}
		}
		return FilteredLines;
	}
}

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

	const TArray<FWacomCardDetailTokenLine> DescriptionTokenLines =
		FilterTokenLines(CurrentData.TokenLines, &IsDescriptionSectionTokenLine);
	if (!DescriptionTokenLines.IsEmpty())
	{
		AddTokenSection(LOCTEXT("DescriptionSectionTitle", "描述"), DescriptionTokenLines);
	}
	else if (!CurrentData.Description.IsEmpty())
	{
		TArray<FText> DescriptionLines;
		DescriptionLines.Add(CurrentData.Description);
		AddSection(LOCTEXT("DescriptionSectionTitle", "描述"), DescriptionLines);
	}
	AddSection(LOCTEXT("TasksSectionTitle", "任务"), CurrentData.TaskLines);

	const TArray<FWacomCardDetailTokenLine> PassiveTokenLines =
		FilterTokenLines(CurrentData.TokenLines, &IsPassiveTokenLine);
	if (!PassiveTokenLines.IsEmpty())
	{
		AddTokenSection(LOCTEXT("PassivesSectionTitle", "被动"), PassiveTokenLines);
	}
	else
	{
		AddSection(LOCTEXT("PassivesSectionTitle", "被动"), CurrentData.PassiveLines);
	}

	SectionsBox->SetVisibility(
		SectionWidgets.Num() > 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
}

void UWacomCardDetailPanel::AddSection(const FText& Title, const TArray<FText>& Lines)
{
	if (!SectionsBox || Lines.Num() <= 0)
	{
		return;
	}

	TArray<FText> NonEmptyLines;
	for (const FText& Line : Lines)
	{
		if (!Line.IsEmpty())
		{
			NonEmptyLines.Add(Line);
		}
	}
	if (NonEmptyLines.Num() <= 0)
	{
		return;
	}

	FWacomCardDetailSectionData SectionData;
	SectionData.Title = Title;
	SectionData.Lines = NonEmptyLines;
	AddSectionData(SectionData);
}

void UWacomCardDetailPanel::AddTokenSection(
	const FText& Title,
	const TArray<FWacomCardDetailTokenLine>& TokenLines)
{
	if (!SectionsBox || TokenLines.IsEmpty())
	{
		return;
	}

	FWacomCardDetailSectionData SectionData;
	SectionData.Title = Title;
	SectionData.TokenLines = TokenLines;
	AddSectionData(SectionData);
}

void UWacomCardDetailPanel::AddSectionData(const FWacomCardDetailSectionData& SectionData)
{
	if (!SectionsBox)
	{
		return;
	}

	UClass* WidgetClass = SectionWidgetClass
		? SectionWidgetClass.Get()
		: UWacomCardDetailSectionWidget::StaticClass();
	UWacomCardDetailSectionWidget* SectionWidget = GetWorld()
		? CreateWidget<UWacomCardDetailSectionWidget>(this, WidgetClass)
		: NewObject<UWacomCardDetailSectionWidget>(this, WidgetClass);
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
