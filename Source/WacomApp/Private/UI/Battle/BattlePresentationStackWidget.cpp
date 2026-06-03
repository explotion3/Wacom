// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattlePresentationStackWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "UI/Card/WacomCardView.h"

#define LOCTEXT_NAMESPACE "WacomBattlePresentationStack"

namespace
{
	const TCHAR* CardViewPath = TEXT("/Game/Wacom/UI/Card/WBP_CardView.WBP_CardView_C");
}

UBattlePresentationStackWidget::UBattlePresentationStackWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (UClass* CardViewClass = LoadClass<UWacomCardView>(nullptr, CardViewPath))
	{
		MiniCardViewClass = CardViewClass;
	}
	else
	{
		MiniCardViewClass = UWacomCardView::StaticClass();
	}
}

TSharedRef<SWidget> UBattlePresentationStackWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		StackCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("StackCanvas"));
		if (UOverlaySlot* StackSlot = Root->AddChildToOverlay(StackCanvas))
		{
			StackSlot->SetHorizontalAlignment(HAlign_Fill);
			StackSlot->SetVerticalAlignment(VAlign_Fill);
		}

	}

	return Super::RebuildWidget();
}

void UBattlePresentationStackWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	RebuildEntryWidgets();
}

void UBattlePresentationStackWidget::SetPresentationStackEntries(
	const TArray<FWacomBattlePresentationStackEntryView>& InEntries)
{
	CurrentEntries = InEntries;
	RebuildEntryWidgets();
}

void UBattlePresentationStackWidget::ClearPresentationStack()
{
	CurrentEntries.Reset();
	RebuildEntryWidgets();
}

int32 UBattlePresentationStackWidget::GetVisibleEntryCount() const
{
	return FMath::Min(CurrentEntries.Num(), FMath::Max(1, MaxVisibleEntries));
}

void UBattlePresentationStackWidget::RebuildEntryWidgets()
{
	if (!StackCanvas)
	{
		return;
	}

	StackCanvas->ClearChildren();
	EntryWidgets.Reset();

	const int32 VisibleCount = GetVisibleEntryCount();
	const int32 StartIndex = 0;
	const TSubclassOf<UWacomCardView> ResolvedMiniCardClass = ResolveMiniCardViewClass();

	for (int32 VisibleIndex = VisibleCount - 1; VisibleIndex >= 0; --VisibleIndex)
	{
		const int32 EntryIndex = StartIndex + VisibleIndex;
		if (!CurrentEntries.IsValidIndex(EntryIndex))
		{
			continue;
		}

		UBattlePresentationStackEntryWidget* EntryWidget = GetWorld()
			? CreateWidget<UBattlePresentationStackEntryWidget>(this, UBattlePresentationStackEntryWidget::StaticClass())
			: NewObject<UBattlePresentationStackEntryWidget>(this, UBattlePresentationStackEntryWidget::StaticClass());
		if (!EntryWidget)
		{
			continue;
		}

		EntryWidget->SetMiniCardViewClass(ResolvedMiniCardClass);
		EntryWidget->SetPresentationStackEntryData(CurrentEntries[EntryIndex]);
		EntryWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		EntryWidgets.Add(EntryWidget);

		if (UCanvasPanelSlot* EntrySlot = StackCanvas->AddChildToCanvas(EntryWidget))
		{
			const FVector2D Position = EntryOffset * static_cast<float>(VisibleIndex);
			EntrySlot->SetAutoSize(false);
			EntrySlot->SetSize(MiniCardSize);
			EntrySlot->SetPosition(Position);
			EntrySlot->SetAlignment(FVector2D(0.0f, 0.0f));
			EntrySlot->SetZOrder(VisibleCount - VisibleIndex);
		}
	}

	SetVisibility(CurrentEntries.IsEmpty()
		? ESlateVisibility::Collapsed
		: ESlateVisibility::HitTestInvisible);
}

TSubclassOf<UWacomCardView> UBattlePresentationStackWidget::ResolveMiniCardViewClass() const
{
	if (MiniCardViewClass)
	{
		return MiniCardViewClass;
	}
	if (UClass* CardViewClass = LoadClass<UWacomCardView>(nullptr, CardViewPath))
	{
		return CardViewClass;
	}
	return UWacomCardView::StaticClass();
}

#undef LOCTEXT_NAMESPACE
