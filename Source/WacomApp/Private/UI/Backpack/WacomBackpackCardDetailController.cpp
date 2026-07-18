// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackCardDetailController.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

namespace
{
	const FVector2D CardDetailPanelEstimatedSize(360.f, 420.f);
	constexpr float CardDetailPanelPadding = 12.f;

	FSlateRect ConvertRectBetweenGeometries(
		const FGeometry& SourceGeometry,
		const FSlateRect& SourceLocalRect,
		const FGeometry& TargetGeometry)
	{
		const FVector2D SourceCorners[] = {
			FVector2D(SourceLocalRect.Left, SourceLocalRect.Top),
			FVector2D(SourceLocalRect.Right, SourceLocalRect.Top),
			FVector2D(SourceLocalRect.Right, SourceLocalRect.Bottom),
			FVector2D(SourceLocalRect.Left, SourceLocalRect.Bottom),
		};
		FSlateRect Result(
			TNumericLimits<float>::Max(),
			TNumericLimits<float>::Max(),
			TNumericLimits<float>::Lowest(),
			TNumericLimits<float>::Lowest());
		for (const FVector2D& SourceCorner : SourceCorners)
		{
			const FVector2D TargetPoint = TargetGeometry.AbsoluteToLocal(
				SourceGeometry.LocalToAbsolute(SourceCorner));
			Result.Left = FMath::Min(Result.Left, TargetPoint.X);
			Result.Top = FMath::Min(Result.Top, TargetPoint.Y);
			Result.Right = FMath::Max(Result.Right, TargetPoint.X);
			Result.Bottom = FMath::Max(Result.Bottom, TargetPoint.Y);
		}
		return Result;
	}
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

bool FWacomBackpackCardDetailController::RepositionVisibleSource()
{
	UWacomDeckCardWidget* SourceWidget = Screen.CardDetailSourceWidget.Get();
	if (!IsVisible() || !SourceWidget || !SourceWidget->GetCard())
	{
		return false;
	}
	PositionNear(SourceWidget);
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
	FSlateRect AnchorRect;
	FSlateRect WorkspaceLocalRect;
	if (Screen.WorkspaceWidget
		&& Screen.WorkspaceWidget->ResolveCardDetailAnchorRect(*SourceWidget, WorkspaceLocalRect))
	{
		const FGeometry& WorkspaceGeometry = Screen.WorkspaceWidget->GetCachedGeometry();
		AnchorRect = ConvertRectBetweenGeometries(
			WorkspaceGeometry,
			WorkspaceLocalRect,
			LayerGeometry);
	}
	else
	{
		const FGeometry& SourceGeometry = SourceWidget->GetCachedGeometry();
		AnchorRect = ConvertRectBetweenGeometries(
			SourceGeometry,
			FSlateRect(
				0.0f,
				0.0f,
				SourceGeometry.GetLocalSize().X,
				SourceGeometry.GetLocalSize().Y),
			LayerGeometry);
	}
	const FVector2D AnchorPosition(AnchorRect.Left, AnchorRect.Top);
	const FVector2D AnchorSize(
		AnchorRect.Right - AnchorRect.Left,
		AnchorRect.Bottom - AnchorRect.Top);
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
