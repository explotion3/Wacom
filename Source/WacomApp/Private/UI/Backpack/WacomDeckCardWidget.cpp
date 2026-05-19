// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomDeckCardWidget.h"

#define LOCTEXT_NAMESPACE "WacomDeckCard"

#include "Components/Border.h"
#include "Components/TextBlock.h"

#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Cards/CardDefinition.h"
#include "UI/Backpack/WacomCardDragOperation.h"
#include "UI/Card/WacomCardView.h"

namespace
{
constexpr float WacomDeckCardDragSourceOpacity = 0.5f;
constexpr float WacomDeckCardNormalOpacity = 1.0f;
}

void UWacomDeckCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshContentFromCard();
}

void UWacomDeckCardWidget::SetCard(const FCardInstance& Inst, EZoneKind InFromZone, FGuid InFromZoneOwnerInstanceId)
{
	Card = Inst.Definition;
	InstanceId = Inst.InstanceId;
	FromZone = InFromZone;
	FromZoneOwnerInstanceId = (FromZone == EZoneKind::SpecialZone) ? InFromZoneOwnerInstanceId : FGuid();
	SetBattleEnabledBadgeVisible(FromZone == EZoneKind::SpecialZone && Inst.bBattleEnabledInSpecialZone);
	RefreshContentFromCard();
}

void UWacomDeckCardWidget::SetMoveEnabled(bool bEnabled)
{
	bCardInteractionEnabled = bEnabled;
	if (CardBody) { CardBody->SetIsEnabled(bEnabled); }
	RefreshContentFromCard();
}

void UWacomDeckCardWidget::SetBattleEnabledBadgeVisible(bool bVisible)
{
	bBattleEnabledBadgeVisible = bVisible;
	if (BattleEnabledBadge)
	{
		BattleEnabledBadge->SetVisibility(bBattleEnabledBadgeVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UWacomDeckCardWidget::SetProjectedFromBadgeText(const FText& InText)
{
	ProjectedFromBadgeText = InText;
	if (ProjectedFromBadge)
	{
		ProjectedFromBadge->SetText(ProjectedFromBadgeText);
		ProjectedFromBadge->SetVisibility(ProjectedFromBadgeText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UWacomDeckCardWidget::SetRightClickToggleEnabled(bool bEnabled)
{
	bRightClickToggleEnabled = bEnabled;
}

void UWacomDeckCardWidget::SetDragVisualMode(bool bInDragVisualMode)
{
	bDragVisualMode = bInDragVisualMode;
	if (BattleEnabledBadge)
	{
		BattleEnabledBadge->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ProjectedFromBadge)
	{
		ProjectedFromBadge->SetVisibility(ESlateVisibility::Collapsed);
	}
	ApplyDragSourceVisualState(false);
}

void UWacomDeckCardWidget::RefreshContentFromCard()
{
	if (!CardView)
	{
		return;
	}

	if (!Card)
	{
		FWacomCardViewData EmptyData;
		EmptyData.Name = LOCTEXT("EmptyCard", "(none)");
		EmptyData.bShowCost = false;
		EmptyData.bDisabled = true;
		CardView->SetCardViewData(EmptyData);
		return;
	}

	CardView->SetCardViewData(BuildCurrentCardViewData());
}

FWacomCardViewData UWacomDeckCardWidget::BuildCurrentCardViewData() const
{
	FWacomCardViewData Data = UWacomCardView::BuildFromCardDefinition(Card);
	Data.bDisabled = !bCardInteractionEnabled;
	return Data;
}

void UWacomDeckCardWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	RequestCardHover();
}

void UWacomDeckCardWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	RequestCardUnhover();
	Super::NativeOnMouseLeave(InMouseEvent);
}

FReply UWacomDeckCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragVisualMode)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && InstanceId.IsValid())
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && RequestBattleEnabledToggle())
	{
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UWacomDeckCardWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
	RequestDragStartedForDetail();

	OutOperation = BuildDragOperation();
	if (OutOperation)
	{
		ApplyDragSourceVisualState(true);
		OutOperation->OnDrop.AddDynamic(this, &UWacomDeckCardWidget::HandleDragOperationFinished);
		OutOperation->OnDragCancelled.AddDynamic(this, &UWacomDeckCardWidget::HandleDragOperationFinished);
	}
}

void UWacomDeckCardWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

	ApplyDragSourceVisualState(false);
}

UDragDropOperation* UWacomDeckCardWidget::BuildDragOperation()
{
	if (bDragVisualMode || !Card || !InstanceId.IsValid())
	{
		return nullptr;
	}

	UWacomCardDragOperation* DragOp = NewObject<UWacomCardDragOperation>(this);
	DragOp->InstanceId = InstanceId;
	DragOp->FromZone = FromZone;
	DragOp->FromZoneOwnerInstanceId = (FromZone == EZoneKind::SpecialZone) ? FromZoneOwnerInstanceId : FGuid();
	DragOp->Definition = Card;

	if (UWacomDeckCardWidget* DragVisual = CreateWidget<UWacomDeckCardWidget>(GetOwningPlayer(), GetClass()))
	{
		FCardInstance VisualInst;
		VisualInst.InstanceId = InstanceId;
		VisualInst.Definition = Card;
		VisualInst.bBattleEnabledInSpecialZone = bBattleEnabledBadgeVisible;

		DragVisual->SetCard(VisualInst, FromZone, FromZoneOwnerInstanceId);
		DragVisual->SetRightClickToggleEnabled(false);
		DragVisual->SetDragVisualMode(true);
		DragOp->DefaultDragVisual = DragVisual;
	}
	DragOp->Pivot = EDragPivot::MouseDown;
	return DragOp;
}

bool UWacomDeckCardWidget::HasMoveButtonClickBindings() const
{
	return false;
}

bool UWacomDeckCardWidget::IsBattleEnabledBadgeVisible() const
{
	return BattleEnabledBadge ? BattleEnabledBadge->GetVisibility() != ESlateVisibility::Collapsed : bBattleEnabledBadgeVisible;
}

bool UWacomDeckCardWidget::IsProjectedFromBadgeVisible() const
{
	return ProjectedFromBadge ? ProjectedFromBadge->GetVisibility() != ESlateVisibility::Collapsed : !ProjectedFromBadgeText.IsEmpty();
}

FText UWacomDeckCardWidget::GetProjectedFromBadgeText() const
{
	return ProjectedFromBadge ? ProjectedFromBadge->GetText() : ProjectedFromBadgeText;
}

bool UWacomDeckCardWidget::RequestBattleEnabledToggle()
{
	if (!InstanceId.IsValid() || !bRightClickToggleEnabled)
	{
		return false;
	}

	OnBattleEnabledToggleRequestedNative.Broadcast(InstanceId);
	return true;
}

bool UWacomDeckCardWidget::RequestCardHover()
{
	if (bDragVisualMode || !Card || !InstanceId.IsValid())
	{
		return false;
	}

	OnCardHoveredNative.Broadcast(this);
	return true;
}

bool UWacomDeckCardWidget::RequestCardUnhover()
{
	if (bDragVisualMode || !Card || !InstanceId.IsValid())
	{
		return false;
	}

	OnCardUnhoveredNative.Broadcast(this);
	return true;
}

bool UWacomDeckCardWidget::RequestDragStartedForDetail()
{
	if (bDragVisualMode || !Card || !InstanceId.IsValid())
	{
		return false;
	}

	OnCardUnhoveredNative.Broadcast(this);
	return true;
}

void UWacomDeckCardWidget::HandleDragOperationFinished(UDragDropOperation* Operation)
{
	ApplyDragSourceVisualState(false);
}

void UWacomDeckCardWidget::ApplyDragSourceVisualState(bool bDragging)
{
	if (bDragVisualMode)
	{
		SetRenderOpacity(WacomDeckCardNormalOpacity);
		return;
	}

	SetRenderOpacity(bDragging ? WacomDeckCardDragSourceOpacity : WacomDeckCardNormalOpacity);
}

#undef LOCTEXT_NAMESPACE
