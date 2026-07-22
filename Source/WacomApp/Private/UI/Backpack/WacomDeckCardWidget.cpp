// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomDeckCardWidget.h"

#define LOCTEXT_NAMESPACE "WacomDeckCard"

#include "Components/Border.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"

#include "Cards/CardDefinition.h"
#include "UI/Backpack/WacomBackpackWorkspaceMotionCoordinator.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"

void UWacomDeckCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshContentFromCard();
	SetBackpackCardDisplayScale(BackpackCardDisplayScale);
	bHasAppliedBackpackRealtimePresentation = false;
	SetBackpackRealtimePresentation(
		bBackpackRealtimePresentationEnabled,
		LastBackpackPresentationPointer,
		bLastBackpackPresentationCarrying);
	ApplyBackpackLocalMotionPose(
		BackpackLocalMotionTranslation,
		BackpackLocalMotionAngleDegrees);
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
	UCardDefinition* NewCard = StorageCardView.Instance.Definition;
	const bool bCardFaceChanged = Card != NewCard;
	Card = NewCard;
	InstanceId = StorageCardView.Instance.InstanceId;
	FromZone = StorageCardView.PhysicalZone;
	FromZoneOwnerInstanceId = (FromZone == EZoneKind::SpecialZone) ? StorageCardView.ZoneOwnerInstanceId : FGuid();
	SetBattleEnabledBadgeVisible(StorageCardView.bShowBattleEnabledInSpecialZoneBadge);
	SetRightClickToggleEnabled(StorageCardView.bCanToggleBattleEnabledInSpecialZone);
	if (bCardFaceChanged)
	{
		RefreshContentFromCard();
	}
}

void UWacomDeckCardWidget::PrepareForBackpackListReuse(bool bPreserveTransientPresentation)
{
	UnbindWorkspacePointerEvents();
	SetWorkspacePointerPassthrough(false);
	SetWorkspaceVisualState(false, false, false);
	SetWorkspaceInteractionEnabled(true);
	SetWorkspaceReadOnlyKind(EWacomBackpackWorkspaceCardReadOnlyKind::None);
	SetWorkspaceDisplayZone(FromZone, FromZoneOwnerInstanceId);
	if (!bPreserveTransientPresentation)
	{
		SetBackpackRealtimePresentation(false, FVector2D::ZeroVector, false);
		ResetBackpackLocalMotionPose();
	}
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
	SetWorkspaceInteractionState(bSelected, bCurrent);
	FWacomBackpackWorkspaceCardVisualState State;
	State.Opacity = bReadOnly ? 0.72f : 1.0f;
	State.Tint = bSelected ? FLinearColor(0.65f, 0.88f, 1.0f, 1.0f) : FLinearColor::White;
	State.FeedbackOpacity = bSelected ? 0.22f : 0.0f;
	ApplyWorkspaceVisualState(State);
}

void UWacomDeckCardWidget::SetWorkspaceInteractionState(bool bSelected, bool bCurrent)
{
	bWorkspaceSelected = bSelected;
	bWorkspaceCurrent = bCurrent;
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

void UWacomDeckCardWidget::RequestBackpackCardFaceRender()
{
	if (BackpackCardView)
	{
		BackpackCardView->RequestPresentationRender();
	}
}

void UWacomDeckCardWidget::SetBackpackCardFaceRetainedRenderingEnabled(bool bEnabled)
{
	if (BackpackCardView)
	{
		BackpackCardView->SetRetainedRenderingEnabled(bEnabled);
	}
}

void UWacomDeckCardWidget::SetBackpackCardDisplayScale(float InScale)
{
	const float SafeScale = FMath::Max(InScale, 0.01f);
	if (FMath::IsNearlyEqual(BackpackCardDisplayScale, SafeScale, 0.0001f)
		&& (!CardFaceScaleBox
			|| FMath::IsNearlyEqual(
				CardFaceScaleBox->GetUserSpecifiedScale(), SafeScale, 0.0001f)))
	{
		return;
	}
	BackpackCardDisplayScale = SafeScale;
	if (CardFaceScaleBox)
	{
		CardFaceScaleBox->SetStretch(EStretch::UserSpecified);
		CardFaceScaleBox->SetStretchDirection(EStretchDirection::Both);
		CardFaceScaleBox->SetUserSpecifiedScale(BackpackCardDisplayScale);
		InvalidateLayoutAndVolatility();
	}
}

void UWacomDeckCardWidget::SetBackpackRealtimePresentation(
	bool bEnabled,
	FVector2D NormalizedPointer,
	bool bCarrying)
{
	bBackpackRealtimePresentationEnabled = bEnabled;
	if (!BackpackCardView)
	{
		bHasAppliedBackpackRealtimePresentation = false;
		return;
	}
	if (bHasAppliedBackpackRealtimePresentation
		&& bEnabled == BackpackCardView->IsRealtimePresentationEnabled()
		&& LastBackpackPresentationPointer.Equals(NormalizedPointer, 0.001f)
		&& bLastBackpackPresentationCarrying == bCarrying)
	{
		return;
	}
	bHasAppliedBackpackRealtimePresentation = true;
	LastBackpackPresentationPointer = NormalizedPointer;
	bLastBackpackPresentationCarrying = bCarrying;

	FWacomFirstPersonCardDepthView Depth;
	if (bEnabled)
	{
		const float MaxTilt = bCarrying ? 2.5f : 6.0f;
		Depth.bFake3DEnabled = true;
		Depth.TiltDegrees = FVector2D(
			-NormalizedPointer.Y * MaxTilt,
			NormalizedPointer.X * MaxTilt);
		Depth.PerspectiveStrength = 0.12f;
		Depth.bContactShadowEnabled = true;
		Depth.ContactShadowLift = bCarrying ? 1.0f : 0.55f;
		Depth.SurfacePerspective.bEnabled = true;
		Depth.SurfacePerspective.Strength = 1.0f;
		Depth.SurfacePerspective.TiltDegrees = Depth.TiltDegrees;
		Depth.SurfacePerspective.AttachmentOffsetPixels = FVector2D(
			NormalizedPointer.X * 5.0f,
			-NormalizedPointer.Y * 5.0f);
	}
	BackpackCardView->SetRealtimePresentationEnabled(bEnabled);
	BackpackCardView->SetCardDepthView(Depth);
}

void UWacomDeckCardWidget::ApplyBackpackLocalMotionPose(
	FVector2D Translation,
	float AngleDegrees)
{
	BackpackLocalMotionTranslation = Translation;
	BackpackLocalMotionAngleDegrees = AngleDegrees;
	UWidget* MotionRoot = CardMotionRoot.Get();
	if (!MotionRoot)
	{
		MotionRoot = CardFaceScaleBox.Get();
	}
	if (!MotionRoot)
	{
		return;
	}
	MotionRoot->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	MotionRoot->SetRenderTranslation(Translation);
	MotionRoot->SetRenderTransformAngle(AngleDegrees);
}

void UWacomDeckCardWidget::ResetBackpackLocalMotionPose()
{
	ApplyBackpackLocalMotionPose(FVector2D::ZeroVector, 0.0f);
}

void UWacomDeckCardWidget::ApplyBackpackDepthPresentation(
	bool bRealtimeEnabled,
	const FWacomFirstPersonCardDepthView& DepthView)
{
	bBackpackRealtimePresentationEnabled = bRealtimeEnabled;
	bHasAppliedBackpackRealtimePresentation = true;
	if (!BackpackCardView)
	{
		return;
	}
	BackpackCardView->SetRealtimePresentationEnabled(bRealtimeEnabled);
	BackpackCardView->SetCardDepthView(DepthView);
}

FVector2D UWacomDeckCardWidget::GetBackpackLocalMotionTranslation() const
{
	return BackpackLocalMotionTranslation;
}

float UWacomDeckCardWidget::GetBackpackLocalMotionAngle() const
{
	return BackpackLocalMotionAngleDegrees;
}

void UWacomDeckCardWidget::SetMoveEnabled(bool bEnabled)
{
	if (bCardInteractionEnabled == bEnabled)
	{
		return;
	}
	bCardInteractionEnabled = bEnabled;
	RefreshContentFromCard();
}

void UWacomDeckCardWidget::SetWorkspaceInteractionEnabled(bool bEnabled)
{
	bWorkspaceInteractionEnabled = bEnabled;
	RefreshWorkspaceHitTestVisibility();
}

void UWacomDeckCardWidget::SetWorkspacePointerPassthrough(bool bEnabled)
{
	bWorkspacePointerPassthrough = bEnabled;
	RefreshWorkspaceHitTestVisibility();
}

void UWacomDeckCardWidget::RefreshWorkspaceHitTestVisibility()
{
	SetVisibility(bWorkspaceInteractionEnabled && !bWorkspacePointerPassthrough
		? ESlateVisibility::Visible
		: ESlateVisibility::HitTestInvisible);
}

void UWacomDeckCardWidget::SetWorkspaceReadOnlyKind(
	EWacomBackpackWorkspaceCardReadOnlyKind InKind)
{
	WorkspaceReadOnlyKind = InKind;
}

void UWacomDeckCardWidget::SetWorkspaceDisplayZone(EZoneKind InZone, FGuid InOwnerInstanceId)
{
	WorkspaceDisplayZone = InZone;
	WorkspaceDisplayOwnerInstanceId = InZone == EZoneKind::SpecialZone
		? InOwnerInstanceId
		: FGuid();
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
	if (!BackpackCardView)
	{
		return;
	}

	if (!Card)
	{
		FWacomCardViewData EmptyData;
		EmptyData.Name = LOCTEXT("EmptyCard", "(none)");
		EmptyData.bShowCost = false;
		EmptyData.bDisabled = true;
		BackpackCardView->SetCardViewData(EmptyData);
		return;
	}

	BackpackCardView->SetCardViewData(BuildCurrentCardViewData());
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
	if (!InstanceId.IsValid() || !IsMoveEnabled() || !bRightClickToggleEnabled)
	{
		return false;
	}

	OnBattleEnabledToggleRequestedNative.Broadcast(InstanceId);
	return true;
}

bool UWacomDeckCardWidget::RequestCardHover()
{
	if (!Card || !InstanceId.IsValid() || !bWorkspaceInteractionEnabled)
	{
		return false;
	}

	OnCardHoveredNative.Broadcast(this);
	return true;
}

bool UWacomDeckCardWidget::RequestCardUnhover()
{
	if (!Card || !InstanceId.IsValid() || !bWorkspaceInteractionEnabled)
	{
		return false;
	}

	OnCardUnhoveredNative.Broadcast(this);
	return true;
}

#undef LOCTEXT_NAMESPACE
