// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackCardDetailController.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

namespace
{
	const FVector2D CardDetailPanelEstimatedSize(360.f, 420.f);
	constexpr float CardDetailPanelPadding = 12.f;
}

FWacomBackpackCardDetailController::FWacomBackpackCardDetailController(UWacomBackpackScreen& InScreen)
	: Screen(InScreen)
{
}

bool FWacomBackpackCardDetailController::IsVisible() const
{
	return Screen.CardDetailPanel
		&& Screen.CardDetailPanel->GetVisibility() != ESlateVisibility::Collapsed;
}

FText FWacomBackpackCardDetailController::GetNameText() const
{
	return Screen.CardDetailPanel ? Screen.CardDetailPanel->GetNameText() : FText::GetEmpty();
}

bool FWacomBackpackCardDetailController::ShowForCardWidget(UWacomDeckCardWidget* SourceWidget)
{
	if (!SourceWidget || !SourceWidget->GetCard())
	{
		Hide();
		return false;
	}

	UWacomCardDetailPanel* Panel = EnsurePanel();
	if (!Panel)
	{
		return false;
	}

	Panel->SetCardDetailData(
		UWacomCardPresentationBuilder::BuildCardDetailViewData(SourceWidget->GetCard()));
	PositionNear(SourceWidget);
	Panel->SetRenderOpacity(1.f);
	Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	Screen.CardDetailSourceWidget = SourceWidget;
	return true;
}

void FWacomBackpackCardDetailController::Hide()
{
	if (Screen.CardDetailPanel)
	{
		Screen.CardDetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	Screen.CardDetailSourceWidget = nullptr;
}

void FWacomBackpackCardDetailController::HideIfSourceRemoved(UWacomDeckCardWidget* RemovedWidget)
{
	if (RemovedWidget && Screen.CardDetailSourceWidget.Get() == RemovedWidget)
	{
		Hide();
	}
}

UWacomCardDetailPanel* FWacomBackpackCardDetailController::EnsurePanel()
{
	if (!Screen.CardDetailLayer)
	{
		return nullptr;
	}

	if (Screen.CardDetailPanel)
	{
		return Screen.CardDetailPanel;
	}

	UClass* PanelClass = Screen.CardDetailPanelClass
		? Screen.CardDetailPanelClass.Get()
		: UWacomCardDetailPanel::StaticClass();
	Screen.CardDetailPanel = Screen.GetWorld()
		? CreateWidget<UWacomCardDetailPanel>(&Screen, PanelClass)
		: NewObject<UWacomCardDetailPanel>(&Screen, PanelClass);
	if (!Screen.CardDetailPanel)
	{
		return nullptr;
	}

	Screen.CardDetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	Screen.CardDetailPanel->SetIsEnabled(true);
	Screen.CardDetailPanel->SetRenderOpacity(1.f);
	if (UCanvasPanelSlot* DetailSlot = Screen.CardDetailLayer->AddChildToCanvas(Screen.CardDetailPanel))
	{
		DetailSlot->SetAutoSize(false);
		DetailSlot->SetSize(CardDetailPanelEstimatedSize);
		DetailSlot->SetZOrder(1);
	}
	return Screen.CardDetailPanel;
}

void FWacomBackpackCardDetailController::PositionNear(UWacomDeckCardWidget* SourceWidget)
{
	if (!SourceWidget || !Screen.CardDetailLayer || !Screen.CardDetailPanel)
	{
		return;
	}

	const FGeometry& LayerGeometry = Screen.CardDetailLayer->GetCachedGeometry();
	const FGeometry& SourceGeometry = SourceWidget->GetCachedGeometry();
	const FVector2D AnchorPosition = LayerGeometry.AbsoluteToLocal(SourceGeometry.GetAbsolutePosition());
	const FVector2D AnchorSize = SourceGeometry.GetLocalSize();
	const FVector2D LayerSize = LayerGeometry.GetLocalSize();
	const FVector2D Position = ComputePanelPosition(
		AnchorPosition,
		AnchorSize,
		LayerSize,
		CardDetailPanelEstimatedSize,
		CardDetailPanelPadding);

	if (UCanvasPanelSlot* DetailSlot = Cast<UCanvasPanelSlot>(Screen.CardDetailPanel->Slot))
	{
		DetailSlot->SetPosition(Position);
		DetailSlot->SetSize(CardDetailPanelEstimatedSize);
	}
}

FVector2D FWacomBackpackCardDetailController::ComputePanelPosition(
	FVector2D AnchorPosition,
	FVector2D AnchorSize,
	FVector2D LayerSize,
	FVector2D PanelSize,
	float Padding)
{
	const float MaxX = FMath::Max(0.0f, LayerSize.X - PanelSize.X);
	const float MaxY = FMath::Max(0.0f, LayerSize.Y - PanelSize.Y);
	float X = AnchorPosition.X + AnchorSize.X + Padding;
	if (X + PanelSize.X > LayerSize.X)
	{
		X = AnchorPosition.X - PanelSize.X - Padding;
	}
	return FVector2D(
		FMath::Clamp(X, 0.0f, MaxX),
		FMath::Clamp(AnchorPosition.Y, 0.0f, MaxY));
}
