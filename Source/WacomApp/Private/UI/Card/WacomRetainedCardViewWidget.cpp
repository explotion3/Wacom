// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomRetainedCardViewWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/RetainerBox.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomStaticRetainerBox.h"

void UWacomRetainedCardViewWidget::SetCardViewData(const FWacomCardViewData& InData)
{
	PendingCardViewData = InData;
	ApplySurfaceFoilPolicy();
	ApplyPendingCardViewData();
}

void UWacomRetainedCardViewWidget::RequestCardFaceRender()
{
	if (CardFaceRetainer)
	{
		CardFaceRetainer->RequestRender();
	}
}

void UWacomRetainedCardViewWidget::SetRetainedRenderingEnabled(bool bEnabled)
{
	bRetainedRenderingEnabled = bEnabled;
	ApplyRetainedRenderingPolicy();
}

TSharedRef<SWidget> UWacomRetainedCardViewWidget::RebuildWidget()
{
	EnsureFallbackWidgetTree();
	ApplySurfaceFoilPolicy();
	TSharedRef<SWidget> RebuiltWidget = Super::RebuildWidget();
	ApplySurfaceFoilPolicy();
	ApplyRetainedRenderingPolicy();
	return RebuiltWidget;
}

void UWacomRetainedCardViewWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplySurfaceFoilPolicy();
	ApplyRetainedRenderingPolicy();
	ApplyPendingCardViewData();
}

void UWacomRetainedCardViewWidget::EnsureFallbackWidgetTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (WidgetTree->RootWidget)
	{
		if (!CardFaceRetainer)
		{
			CardFaceRetainer = Cast<URetainerBox>(WidgetTree->FindWidget(TEXT("CardFaceRetainer")));
		}
		if (!CardView)
		{
			CardView = Cast<UWacomCardView>(WidgetTree->FindWidget(TEXT("CardView")));
		}
		return;
	}

	CardFaceRetainer = WidgetTree->ConstructWidget<UWacomStaticRetainerBox>(
		UWacomStaticRetainerBox::StaticClass(), TEXT("CardFaceRetainer"));
	WidgetTree->RootWidget = CardFaceRetainer;

	CardView = WidgetTree->ConstructWidget<UWacomCardView>(
		UWacomCardView::StaticClass(), TEXT("CardView"));
	CardFaceRetainer->SetContent(CardView);
}

void UWacomRetainedCardViewWidget::ApplySurfaceFoilPolicy()
{
	if (CardView)
	{
		CardView->SetSurfaceFoilEnabled(bEnableSurfaceFoil);
		RequestCardFaceRender();
	}
}

void UWacomRetainedCardViewWidget::ApplyRetainedRenderingPolicy()
{
	if (!CardFaceRetainer)
	{
		return;
	}

	CardFaceRetainer->SetRetainRendering(bRetainedRenderingEnabled);
	if (bRetainedRenderingEnabled)
	{
		CardFaceRetainer->RequestRender();
	}
}

void UWacomRetainedCardViewWidget::ApplyPendingCardViewData()
{
	if (!CardView)
	{
		return;
	}
	CardView->SetCardViewData(PendingCardViewData);
	RequestCardFaceRender();
}
