// Copyright Wacom. All Rights Reserved.

#include "UI/Map/WacomRunMapNodeWidget.h"

#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "InputCoreTypes.h"

namespace
{
	FLinearColor ResolveNodeSemanticColor(const EWacomRunMapNodeVisualState State)
	{
		switch (State)
		{
		case EWacomRunMapNodeVisualState::Landmark:
			return FLinearColor(0.28f, 0.34f, 0.38f, 1.0f);
		case EWacomRunMapNodeVisualState::Revealed:
			return FLinearColor(0.42f, 0.48f, 0.54f, 1.0f);
		case EWacomRunMapNodeVisualState::Visited:
			return FLinearColor(0.58f, 0.50f, 0.80f, 1.0f);
		case EWacomRunMapNodeVisualState::Resolved:
			return FLinearColor(0.22f, 0.88f, 0.82f, 1.0f);
		case EWacomRunMapNodeVisualState::Current:
		default:
			return FLinearColor(1.0f, 0.68f, 0.20f, 1.0f);
		}
	}
}

UWacomRunMapNodeWidget::UWacomRunMapNodeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
	SetIsSelectable(true);
}

void UWacomRunMapNodeWidget::ApplyViewData(const FWacomRunMapNodeViewData& InViewData)
{
	ViewData = InViewData;
	SetButtonText(ViewData.Title);
	SetIsInteractionEnabled(ViewData.bCanSelect);
	SetIsSelected(ViewData.bIsSelected);
	RefreshPresentationState();
	if (NodeSemanticMarker)
	{
		NodeSemanticMarker->SetBrushColor(ResolveNodeSemanticColor(ViewData.VisualState));
		NodeSemanticMarker->SetRenderOpacity(ViewData.bCanSelect ? 1.0f : 0.55f);
	}
	if (NodeTypeText)
	{
		NodeTypeText->SetText(ViewData.TypeLabel);
	}
	BP_OnViewDataApplied(ViewData);
}

void UWacomRunMapNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	OnClicked().RemoveAll(this);
	ClickedHandle = OnClicked().AddUObject(this, &UWacomRunMapNodeWidget::HandleClicked);
}

void UWacomRunMapNodeWidget::NativeDestruct()
{
	if (ClickedHandle.IsValid())
	{
		OnClicked().Remove(ClickedHandle);
		ClickedHandle.Reset();
	}
	OnNodeSelectedNative.Clear();
	OnNodeConfirmRequestedNative.Clear();
	Super::NativeDestruct();
}

FReply UWacomRunMapNodeWidget::NativeOnMouseButtonDoubleClick(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& ViewData.bCanSelect)
	{
		OnNodeSelectedNative.Broadcast(ViewData.Handle);
		if (ViewData.bCanTravel)
		{
			OnNodeConfirmRequestedNative.Broadcast(ViewData.Handle);
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void UWacomRunMapNodeWidget::HandleClicked()
{
	if (ViewData.bCanSelect)
	{
		OnNodeSelectedNative.Broadcast(ViewData.Handle);
	}
}
