// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattleCardPileEntryWidget.h"

#include "UI/Battle/WacomBattleCardPileDetailsScreen.h"
#include "UI/Card/WacomCardView.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	constexpr TCHAR DefaultCardViewClassPath[] =
		TEXT("/Game/Wacom/UI/Card/WBP_CardView.WBP_CardView_C");
}

TSharedRef<SWidget> UBattleCardPileEntryWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		EntrySizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("EntrySizeBox"));
		EntrySizeBox->SetWidthOverride(320.0f);
		EntrySizeBox->SetHeightOverride(448.0f);
		WidgetTree->RootWidget = EntrySizeBox;

		UOverlay* EntryOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("EntryOverlay"));
		EntrySizeBox->SetContent(EntryOverlay);

		SelectionOutlineImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SelectionOutlineImage"));
		SelectionOutlineImage->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* SelectionSlot = EntryOverlay->AddChildToOverlay(SelectionOutlineImage))
		{
			SelectionSlot->SetPadding(FMargin(8.0f, 10.0f));
			SelectionSlot->SetHorizontalAlignment(HAlign_Fill);
			SelectionSlot->SetVerticalAlignment(VAlign_Fill);
		}

		CardHost = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CardHost"));
		CardHost->SetWidthOverride(296.0f);
		CardHost->SetHeightOverride(420.0f);
		if (UOverlaySlot* CardSlot = EntryOverlay->AddChildToOverlay(CardHost))
		{
			CardSlot->SetHorizontalAlignment(HAlign_Center);
			CardSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	return Super::RebuildWidget();
}

void UBattleCardPileEntryWidget::SetSelectionPresentation(
	UMaterialInterface* InOutlineMaterial,
	float InHoverAmount,
	float InLockedAmount,
	float InOutlineExtentPixels,
	bool bInReducedMotion)
{
	if (SelectionOutlineMaterial != InOutlineMaterial)
	{
		ReleaseSelectionMID();
		SelectionOutlineMaterial = InOutlineMaterial;
	}
	HoverOutlineAmount = FMath::Max(0.0f, InHoverAmount);
	LockedOutlineAmount = FMath::Max(0.0f, InLockedAmount);
	SelectionOutlineExtentPixels = FMath::Max(0.0f, InOutlineExtentPixels);
	bReducedMotion = bInReducedMotion;
	ApplySelectionState();
}

void UBattleCardPileEntryWidget::SetLockedSelected(bool bInLockedSelected)
{
	bLockedSelected = bInLockedSelected;
	ApplySelectionState();
}

void UBattleCardPileEntryWidget::SetOwnerReportedPointerHovered(bool bInPointerHovered)
{
	bPointerHovered = bInPointerHovered;
	ApplySelectionState();
}

void UBattleCardPileEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	// UUserWidget defaults to SelfHitTestInvisible. Virtualized list entries must
	// remain in the hit-test path so the table row can produce hover, click and focus edges.
	SetVisibility(ESlateVisibility::Visible);
	ItemViewModel = Cast<UWacomBattleCardPileItemViewModel>(ListItemObject);
	if (const UWacomBattleCardPileItemViewModel* Item = ItemViewModel.Get())
	{
		if (SelectionOutlineImage)
		{
			if (UOverlaySlot* SelectionSlot = Cast<UOverlaySlot>(SelectionOutlineImage->Slot))
			{
				const float HorizontalRoom = FMath::Max(
					0.0f,
					Item->EntrySize.X - Item->CardSize.X);
				const float VerticalRoom = FMath::Max(
					0.0f,
					Item->EntrySize.Y - Item->CardSize.Y);
				const float HorizontalPadding = FMath::Max(
					0.0f,
					HorizontalRoom * 0.5f - Item->SelectionOutlineExtentPixels);
				const float VerticalPadding = FMath::Max(
					0.0f,
					VerticalRoom * 0.5f - Item->SelectionOutlineExtentPixels);
				SelectionSlot->SetPadding(FMargin(
					HorizontalPadding,
					VerticalPadding));
			}
		}
		if (EntrySizeBox)
		{
			EntrySizeBox->SetWidthOverride(FMath::Max(1.0f, Item->EntrySize.X));
			EntrySizeBox->SetHeightOverride(FMath::Max(1.0f, Item->EntrySize.Y));
		}
		if (CardHost)
		{
			CardHost->SetWidthOverride(FMath::Max(1.0f, Item->CardSize.X));
			CardHost->SetHeightOverride(FMath::Max(1.0f, Item->CardSize.Y));
		}
		SetSelectionPresentation(
			Item->SelectionOutlineMaterial,
			Item->HoverOutlineAmount,
			Item->LockedOutlineAmount,
			Item->SelectionOutlineExtentPixels,
			Item->bReducedMotion);
		EnsureCardView(Item->CardViewClass);
		if (RuntimeCardView)
		{
			RuntimeCardView->SetCardViewData(Item->View.CardViewData);
		}
	}
	bLockedSelected = false;
	ApplySelectionState();
}

void UBattleCardPileEntryWidget::NativeOnItemSelectionChanged(bool /*bIsSelected*/)
{
	ApplySelectionState();
}

void UBattleCardPileEntryWidget::NativeOnEntryReleased()
{
	if (RuntimeCardView)
	{
		RuntimeCardView->ResetEffectBadgeFeedback();
		RuntimeCardView->ResetCostDigitPreview();
		RuntimeCardView->ResetCostDigitRewrite();
		RuntimeCardView->ResetCardSurfacePerspectiveView();
		RuntimeCardView->SetCardViewData(FWacomCardViewData());
	}
	ItemViewModel.Reset();
	bPointerHovered = false;
	bKeyboardFocused = false;
	bLockedSelected = false;
	ReleaseSelectionMID();
	ApplySelectionState();
}

void UBattleCardPileEntryWidget::NativeOnMouseEnter(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	SetOwnerReportedPointerHovered(true);
	HoverChangedNative.Broadcast(*this, true);
}

void UBattleCardPileEntryWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	SetOwnerReportedPointerHovered(false);
	HoverChangedNative.Broadcast(*this, false);
}

void UBattleCardPileEntryWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bKeyboardFocused = true;
	ApplySelectionState();
	FocusChangedNative.Broadcast(*this, true);
}

void UBattleCardPileEntryWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bKeyboardFocused = false;
	ApplySelectionState();
	FocusChangedNative.Broadcast(*this, false);
}

void UBattleCardPileEntryWidget::EnsureCardView(
	TSubclassOf<UWacomCardView> RequestedClass)
{
	if (!CardHost)
	{
		return;
	}

	UClass* ClassToUse = RequestedClass.Get();
	if (!ClassToUse)
	{
		ClassToUse = LoadClass<UWacomCardView>(nullptr, DefaultCardViewClassPath);
	}
	if (!ClassToUse)
	{
		ClassToUse = UWacomCardView::StaticClass();
	}
	if (RuntimeCardView && RuntimeCardView->GetClass() == ClassToUse)
	{
		return;
	}

	CardHost->ClearChildren();
	RuntimeCardView = GetWorld()
		? CreateWidget<UWacomCardView>(this, ClassToUse)
		: NewObject<UWacomCardView>(this, ClassToUse);
	if (RuntimeCardView)
	{
		RuntimeCardView->SetSurfaceFoilEnabled(false);
		RuntimeCardView->SetVisibility(ESlateVisibility::HitTestInvisible);
		CardHost->SetContent(RuntimeCardView);
	}
}

void UBattleCardPileEntryWidget::ApplySelectionState()
{
	const bool bActive = bLockedSelected || bPointerHovered || bKeyboardFocused;
	if (!SelectionOutlineImage)
	{
		return;
	}
	if (!bActive || !SelectionOutlineMaterial)
	{
		SelectionOutlineImage->SetVisibility(ESlateVisibility::Collapsed);
		ReleaseSelectionMID();
		return;
	}

	EnsureSelectionMID();
	if (SelectionOutlineMID)
	{
		SelectionOutlineMID->SetScalarParameterValue(
			TEXT("SelectionAmount"),
			bLockedSelected ? LockedOutlineAmount : HoverOutlineAmount);
		SelectionOutlineMID->SetScalarParameterValue(
			TEXT("SelectionReducedMotion"),
			bReducedMotion ? 1.0f : 0.0f);
		const FGuid InstanceId = ItemViewModel.IsValid()
			? ItemViewModel->View.InstanceId
			: FGuid();
		SelectionOutlineMID->SetScalarParameterValue(
			TEXT("SelectionSeed"),
			static_cast<float>(GetTypeHash(InstanceId) & 0xffffu) / 65535.0f);
		SelectionOutlineImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UBattleCardPileEntryWidget::EnsureSelectionMID()
{
	if (!SelectionOutlineImage || SelectionOutlineMID || !SelectionOutlineMaterial)
	{
		return;
	}
	SelectionOutlineImage->SetBrushFromMaterial(SelectionOutlineMaterial);
	SelectionOutlineMID = SelectionOutlineImage->GetDynamicMaterial();
}

void UBattleCardPileEntryWidget::ReleaseSelectionMID()
{
	SelectionOutlineMID = nullptr;
	if (SelectionOutlineImage)
	{
		SelectionOutlineImage->SetBrush(FSlateBrush());
	}
}
