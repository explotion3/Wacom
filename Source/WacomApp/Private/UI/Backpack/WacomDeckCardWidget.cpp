// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomDeckCardWidget.h"

#define LOCTEXT_NAMESPACE "WacomDeckCard"

#include "Components/Border.h"
#include "Components/TextBlock.h"

#include "Cards/CardDefinition.h"
#include "UI/Backpack/WacomBackpackScreenPresenter.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomCardView.h"

void UWacomDeckCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshContentFromCard();
}

void UWacomDeckCardWidget::SetCard(const FCardInstance& Inst, EZoneKind InFromZone, FGuid InFromZoneOwnerInstanceId)
{
	FRunStorageCardView StorageCardView;
	StorageCardView.Instance = Inst;
	StorageCardView.PhysicalZone = InFromZone;
	StorageCardView.ZoneOwnerInstanceId = (InFromZone == EZoneKind::SpecialZone) ? InFromZoneOwnerInstanceId : FGuid();
	StorageCardView.bShowBattleEnabledInSpecialZoneBadge =
		InFromZone == EZoneKind::SpecialZone && Inst.bBattleEnabledInSpecialZone;
	SetStorageCardView(StorageCardView);
}

void UWacomDeckCardWidget::SetStorageCardView(const FRunStorageCardView& StorageCardView)
{
	Card = StorageCardView.Instance.Definition;
	InstanceId = StorageCardView.Instance.InstanceId;
	FromZone = StorageCardView.PhysicalZone;
	FromZoneOwnerInstanceId = (FromZone == EZoneKind::SpecialZone) ? StorageCardView.ZoneOwnerInstanceId : FGuid();
	SetBattleEnabledBadgeVisible(StorageCardView.bShowBattleEnabledInSpecialZoneBadge);
	SetRightClickToggleEnabled(StorageCardView.bCanToggleBattleEnabledInSpecialZone);
	RefreshContentFromCard();
}

void UWacomDeckCardWidget::PrepareForBackpackListReuse()
{
	UnbindWorkspacePointerEvents();
	SetWorkspaceVisualState(false, false, false);
	SetRenderOpacity(1.0f);
	SetProjectedFromBadgeText(FText::GetEmpty());
	SetRightClickToggleEnabled(false);
}

void UWacomDeckCardWidget::UnbindWorkspacePointerEvents()
{
	OnWorkspacePointerDownNative.Unbind();
	OnWorkspacePointerMoveNative.Unbind();
	OnWorkspacePointerUpNative.Unbind();
}

void UWacomDeckCardWidget::SetWorkspaceVisualState(bool bSelected, bool bCurrent, bool bReadOnly)
{
	bWorkspaceSelected = bSelected;
	bWorkspaceCurrent = bCurrent;
	FWacomBackpackWorkspaceCardVisualState State;
	State.Opacity = bReadOnly ? 0.72f : 1.0f;
	State.Tint = bSelected ? FLinearColor(0.65f, 0.88f, 1.0f, 1.0f) : FLinearColor::White;
	State.FeedbackOpacity = bSelected ? 0.22f : 0.0f;
	ApplyWorkspaceVisualState(State);
}

void UWacomDeckCardWidget::ApplyWorkspaceVisualState(
	const FWacomBackpackWorkspaceCardVisualState& VisualState)
{
	SetRenderOpacity(VisualState.Opacity);
	SetRenderScale(FVector2D::UnitVector);
	if (WorkspaceFeedbackOverlay)
	{
		FSlateBrush FeedbackBrush = WorkspaceFeedbackOverlay->Background;
		FeedbackBrush.SetResourceObject(VisualState.FeedbackMaterial);
		WorkspaceFeedbackOverlay->SetBrush(FeedbackBrush);
		FLinearColor FeedbackColor = VisualState.Tint;
		FeedbackColor.A = FMath::Clamp(
			FeedbackColor.A * VisualState.FeedbackOpacity,
			0.0f,
			1.0f);
		WorkspaceFeedbackOverlay->SetBrushColor(FeedbackColor);
		WorkspaceFeedbackOverlay->SetVisibility(
			VisualState.FeedbackOpacity > UE_SMALL_NUMBER
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}
	else if (CardBody)
	{
		CardBody->SetBrushColor(VisualState.Tint);
	}
}

void UWacomDeckCardWidget::SetMoveEnabled(bool bEnabled)
{
	bCardInteractionEnabled = bEnabled;
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
	FWacomCardViewData Data = UWacomCardPresentationBuilder::BuildCardViewData(Card);
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
	if (OnWorkspacePointerDownNative.IsBound())
	{
		return OnWorkspacePointerDownNative.Execute(this, InGeometry, InMouseEvent);
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && RequestBattleEnabledToggle())
	{
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UWacomDeckCardWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (OnWorkspacePointerMoveNative.IsBound())
	{
		return OnWorkspacePointerMoveNative.Execute(this, InGeometry, InMouseEvent);
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UWacomDeckCardWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (OnWorkspacePointerUpNative.IsBound())
	{
		return OnWorkspacePointerUpNative.Execute(this, InGeometry, InMouseEvent);
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
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
	if (!InstanceId.IsValid() || !bCardInteractionEnabled || !bRightClickToggleEnabled)
	{
		return false;
	}

	OnBattleEnabledToggleRequestedNative.Broadcast(InstanceId);
	return true;
}

bool UWacomDeckCardWidget::RequestCardHover()
{
	if (!Card || !InstanceId.IsValid())
	{
		return false;
	}

	OnCardHoveredNative.Broadcast(this);
	return true;
}

bool UWacomDeckCardWidget::RequestCardUnhover()
{
	if (!Card || !InstanceId.IsValid())
	{
		return false;
	}

	OnCardUnhoveredNative.Broadcast(this);
	return true;
}

#undef LOCTEXT_NAMESPACE
