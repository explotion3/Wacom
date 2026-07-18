// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/GameViewportClient.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UI/Card/WacomFirstPersonCardDragPickupPlayback.h"
#include "UI/Card/WacomFirstPersonCardGestureController.h"
#include "UI/Card/WacomFirstPersonCardInteractionFeedbackPlayback.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"

namespace
{
	bool IsFormalDragGestureState(EWacomFirstPersonCardGestureState State)
	{
		return State == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
			|| State == EWacomFirstPersonCardGestureState::ArmedForCommit
			|| State == EWacomFirstPersonCardGestureState::AimingTargetedCard;
	}

	bool IsResolvedInvalidTargetFeedbackState(
		EWacomFirstPersonCardDragTargetFeedbackState State)
	{
		return State == EWacomFirstPersonCardDragTargetFeedbackState::Invalid
			|| State == EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget;
	}

	FReply BuildPointerRouteReply(
		const FWacomFirstPersonCardPointerRouteResult& RouteResult,
		const TSharedRef<SWidget>& CaptureWidget)
	{
		switch (RouteResult.Action)
		{
		case EWacomFirstPersonCardPointerRouteAction::Handled:
			return FReply::Handled();
		case EWacomFirstPersonCardPointerRouteAction::CaptureMouse:
			return FReply::Handled().CaptureMouse(CaptureWidget);
		case EWacomFirstPersonCardPointerRouteAction::ReleaseMouseCapture:
			return FReply::Handled().ReleaseMouseCapture();
		case EWacomFirstPersonCardPointerRouteAction::Unhandled:
		default:
			return FReply::Unhandled();
		}
	}
}
void FWacomFirstPersonCardGestureControllerDeleter::operator()(
	FWacomFirstPersonCardGestureController* Controller) const
{
	delete Controller;
}

FWacomFirstPersonCardGestureControllerState&
UWacomFirstPersonCardLayerSlotWidget::GestureRuntime()
{
	check(GestureController);
	return GestureController->GetMutableState();
}

const FWacomFirstPersonCardGestureControllerState&
UWacomFirstPersonCardLayerSlotWidget::GestureRuntime() const
{
	check(GestureController);
	return GestureController->GetState();
}

EWacomFirstPersonCardGestureState
UWacomFirstPersonCardLayerSlotWidget::GetGestureStateForFirstPersonLayer() const
{
	return GestureRuntime().State;
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardDragFeedbackTarget(
	const FWacomInteractionTargetHandle& TargetHandle,
	bool bValidTarget,
	EWacomFirstPersonCardDragTargetFeedbackState FeedbackState,
	const TOptional<FVector2D>& InFeedbackTargetScreenPosition)
{
	if (GestureRuntime().State == EWacomFirstPersonCardGestureState::Idle
		|| GestureRuntime().State == EWacomFirstPersonCardGestureState::Cancelled)
	{
		return;
	}

	GestureRuntime().FeedbackTargetHandle = TargetHandle;
	GestureRuntime().bTargetValid = TargetHandle.IsValid() && bValidTarget;
	DirectDragTargetFeedbackState = FeedbackState;
	DragTargetFeedbackState = ResolveEffectiveDragTargetFeedbackState();
	if (!InteractionFeedbackPlayback)
	{
		InteractionFeedbackPlayback.Reset(
			new FWacomFirstPersonCardInteractionFeedbackPlayback());
		InteractionFeedbackPlayback->SetConfig(InteractionFeedbackConfig);
	}
	InteractionFeedbackPlayback->SetInvalidTargetPreview(
		IsFormalDragGestureState(GestureRuntime().State)
		&& TargetHandle.IsValid()
		&& IsResolvedInvalidTargetFeedbackState(FeedbackState));
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::Layout);
	if (InFeedbackTargetScreenPosition.IsSet())
	{
		bHasFeedbackTargetScreenPosition = true;
		FeedbackTargetScreenPosition = InFeedbackTargetScreenPosition.GetValue();
	}
	else
	{
		bHasFeedbackTargetScreenPosition = false;
		FeedbackTargetScreenPosition = FVector2D::ZeroVector;
	}
	ApplyVisualSlotView();
	UpdateWantsTick();
	Invalidate(EInvalidateWidgetReason::Paint);
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardDragProbeFeedback(bool bEnabled, bool bValidTarget)
{
	SetCardDragTargetFocusFeedback(
		bEnabled
			? (bValidTarget
				? EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
				: EWacomFirstPersonCardDragTargetFeedbackState::CardProbe)
			: EWacomFirstPersonCardDragTargetFeedbackState::None,
		bValidTarget);
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardDragTargetAffordanceFeedback(
	EWacomFirstPersonCardDragTargetFeedbackState FeedbackState,
	bool bValidTarget)
{
	const bool bEnableFeedback = FeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None;
	const bool bValidFeedback =
		bEnableFeedback
		&& (bValidTarget
			|| FeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	const EWacomFirstPersonCardDragTargetFeedbackState NextState =
		bEnableFeedback ? FeedbackState : EWacomFirstPersonCardDragTargetFeedbackState::None;

	if (CardDragTargetAffordanceFeedbackState == NextState
		&& bCardDragTargetAffordanceFeedback == bEnableFeedback
		&& bCardDragTargetAffordanceFeedbackValid == bValidFeedback)
	{
		return;
	}

	CardDragTargetAffordanceFeedbackState = NextState;
	bCardDragTargetAffordanceFeedback = bEnableFeedback;
	bCardDragTargetAffordanceFeedbackValid = bValidFeedback;
	DragTargetFeedbackState = ResolveEffectiveDragTargetFeedbackState();
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::Layout);
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardDragTargetFocusFeedback(
	EWacomFirstPersonCardDragTargetFeedbackState FeedbackState,
	bool bValidTarget)
{
	const bool bEnableFeedback = FeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None;
	const bool bValidFeedback =
		bEnableFeedback
		&& (bValidTarget
			|| FeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	const EWacomFirstPersonCardDragTargetFeedbackState NextState =
		bEnableFeedback ? FeedbackState : EWacomFirstPersonCardDragTargetFeedbackState::None;

	if (CardDragTargetFocusFeedbackState == NextState
		&& bCardDragProbeFeedback == bEnableFeedback
		&& bCardDragProbeFeedbackValid == bValidFeedback)
	{
		return;
	}

	CardDragTargetFocusFeedbackState = NextState;
	bCardDragProbeFeedback = bEnableFeedback;
	bCardDragProbeFeedbackValid = bValidFeedback;
	if (bValidFeedback)
	{
		BeginHandTargetImpactPreview();
	}
	else
	{
		EndHandTargetImpactPreview();
	}
	DragTargetFeedbackState = ResolveEffectiveDragTargetFeedbackState();
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::DragTargetFocus);
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::ClearCardDragTargetFeedback()
{
	const bool bHadFocusFeedback =
		CardDragTargetFocusFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None
		|| bCardDragProbeFeedback;
	const bool bHadFeedback =
		DragTargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None
		|| CardDragTargetAffordanceFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None
		|| CardDragTargetFocusFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None
		|| DirectDragTargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None
		|| bCardDragTargetAffordanceFeedback
		|| bCardDragProbeFeedback;
	CardDragTargetAffordanceFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	CardDragTargetFocusFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	DirectDragTargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	DragTargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	bCardDragTargetAffordanceFeedback = false;
	bCardDragTargetAffordanceFeedbackValid = false;
	bCardDragProbeFeedback = false;
	bCardDragProbeFeedbackValid = false;
	EndHandTargetImpactPreview();
	if (InteractionFeedbackPlayback)
	{
		InteractionFeedbackPlayback->SetInvalidTargetPreview(false);
	}
	if (bHadFeedback)
	{
		RefreshPresentationTarget(
			true,
			bHadFocusFeedback
				? EWacomFirstPersonCardMotionIntent::DragTargetFocus
				: EWacomFirstPersonCardMotionIntent::Layout);
		ApplyVisualSlotView();
		UpdateWantsTick();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::CancelCardDragGesture(bool bBroadcastCancel)
{
	ClearGestureState(bBroadcastCancel);
}

bool UWacomFirstPersonCardLayerSlotWidget::CanExposeCardTarget() const
{
	return bCardLayerInteractionEnabled
		&& !bIsExitingForFirstPersonLayer
		&& bHasVisualSlotView
		&& VisualSlotView.bProjected
		&& CurrentSlotView.Entry.CardInstanceId.IsValid();
}

bool UWacomFirstPersonCardLayerSlotWidget::CanUpdateGestureFromSlotPointer() const
{
	return GestureRuntime().InputSource == EWacomFirstPersonCardGestureInputSource::MousePointer
		&& (GestureRuntime().State == EWacomFirstPersonCardGestureState::Pressed
			|| GestureRuntime().State == EWacomFirstPersonCardGestureState::Inspecting
			|| IsFormalDragGestureState(GestureRuntime().State));
}

bool UWacomFirstPersonCardLayerSlotWidget::CanUpdateGestureFromExternalPointer() const
{
	return IsFormalDragGestureState(GestureRuntime().State)
		&& GestureRuntime().InputSource == EWacomFirstPersonCardGestureInputSource::ExternalPointer;
}

bool UWacomFirstPersonCardLayerSlotWidget::IsInspectScrubActiveForFirstPersonLayer() const
{
	return GestureRuntime().State == EWacomFirstPersonCardGestureState::Inspecting
		&& GestureRuntime().Source == EWacomFirstPersonCardGestureSource::MousePress
		&& GestureRuntime().InputSource == EWacomFirstPersonCardGestureInputSource::MousePointer;
}

bool UWacomFirstPersonCardLayerSlotWidget::CanBeginInspectScrubFromFirstPersonLayer() const
{
	return CanStartCardDragGesture();
}

FWacomInteractionTargetHandle UWacomFirstPersonCardLayerSlotWidget::BuildCardTargetHandle() const
{
	if (!CanExposeCardTarget())
	{
		return FWacomInteractionTargetHandle();
	}

	return FWacomInteractionTargetHandle::ForCardTarget(
		CurrentSlotView.Entry.CardInstanceId,
		const_cast<UWacomFirstPersonCardLayerSlotWidget*>(this),
		VisualSlotView.ScreenPosition);
}

FVector2D UWacomFirstPersonCardLayerSlotWidget::GetCardBodyHitSizeForFirstPersonLayer() const
{
	return CardView
		? CardView->GetCardBodyHitSize()
		: UWacomFirstPersonCardViewWidget::GetDefaultCardBodyHitSize();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsWidgetPositionInsideCardBodyForFirstPersonLayer(
	const FVector2D& WidgetPosition) const
{
	if (WidgetPosition.ContainsNaN())
	{
		return false;
	}

	FVector2D BodySize = GetCardBodyHitSizeForFirstPersonLayer();
	if (BodySize.X <= 1.0f || BodySize.Y <= 1.0f)
	{
		BodySize = UWacomFirstPersonCardViewWidget::GetDefaultCardBodyHitSize();
	}
	if (BodySize.X <= 1.0f || BodySize.Y <= 1.0f)
	{
		return false;
	}

	const FWacomFirstPersonCardLayerSlotView& HitSlotView = bHasVisualSlotView
		? VisualSlotView
		: CurrentSlotView;
	const float RenderScale = FMath::Max(0.01f, HitSlotView.RenderScale);
	FVector2D LocalDelta = (WidgetPosition - HitSlotView.ScreenPosition) / RenderScale;
	const float InverseAngleRadians = FMath::DegreesToRadians(-HitSlotView.RenderAngleDegrees);
	const float CosAngle = FMath::Cos(InverseAngleRadians);
	const float SinAngle = FMath::Sin(InverseAngleRadians);
	LocalDelta = FVector2D(
		LocalDelta.X * CosAngle - LocalDelta.Y * SinAngle,
		LocalDelta.X * SinAngle + LocalDelta.Y * CosAngle);

	const FVector2D HalfBodySize = BodySize * 0.5f;
	return FMath::Abs(LocalDelta.X) <= HalfBodySize.X
		&& FMath::Abs(LocalDelta.Y) <= HalfBodySize.Y;
}

void UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseEnter(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (UWacomFirstPersonCardLayerWidget* Layer = OwningFirstPersonCardLayer.Get())
	{
		Layer->HandleSlotPointerEntered(*this, InMouseEvent.GetScreenSpacePosition());
	}
	else
	{
		UpdateBodyHoverFromScreenPosition(InMouseEvent.GetScreenSpacePosition());
	}
}

void UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	if (GestureRuntime().State == EWacomFirstPersonCardGestureState::Idle
		|| GestureRuntime().State == EWacomFirstPersonCardGestureState::Cancelled)
	{
		SetPressedForFirstPersonLayer(false);
	}
	if (UWacomFirstPersonCardLayerWidget* Layer = OwningFirstPersonCardLayer.Get())
	{
		Layer->HandleSlotPointerLeft(*this, InMouseEvent.GetScreenSpacePosition());
	}
	else
	{
		SetHoveredForFirstPersonLayer(false);
	}
	Super::NativeOnMouseLeave(InMouseEvent);
}

FReply UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (UWacomFirstPersonCardLayerWidget* Layer = OwningFirstPersonCardLayer.Get())
		{
			const FWacomFirstPersonCardPointerRouteResult RouteResult =
				Layer->HandleSlotPointerPressed(*this, InMouseEvent.GetScreenSpacePosition());
			return RouteResult.IsHandled()
				? BuildPointerRouteReply(RouteResult, TakeWidget())
				: Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (UWacomFirstPersonCardLayerWidget* Layer = OwningFirstPersonCardLayer.Get())
		{
			const FWacomFirstPersonCardPointerRouteResult RouteResult =
				Layer->HandleSlotPointerReleased(*this, InMouseEvent.GetScreenSpacePosition());
			return RouteResult.IsHandled()
				? BuildPointerRouteReply(RouteResult, TakeWidget())
				: Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
		}
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseMove(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (UWacomFirstPersonCardLayerWidget* Layer = OwningFirstPersonCardLayer.Get())
	{
		const FWacomFirstPersonCardPointerRouteResult RouteResult =
			Layer->HandleSlotPointerMoved(*this, InMouseEvent.GetScreenSpacePosition());
		return RouteResult.IsHandled()
			? BuildPointerRouteReply(RouteResult, TakeWidget())
			: Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

bool UWacomFirstPersonCardLayerSlotWidget::CanInteractWithCurrentSlot() const
{
	return bCardLayerInteractionEnabled
		&& !IsEnterTransitionBlockingInteraction()
		&& !IsCardUseReformPlaybackActive()
		&& CurrentSlotView.bProjected
		&& CurrentSlotView.Entry.CardInstanceId.IsValid();
}

bool UWacomFirstPersonCardLayerSlotWidget::CanStartCardDragGesture() const
{
	return CardDragConfig.bEnableFirstPersonCardDragCommit
		&& CanInteractWithCurrentSlot()
		&& CurrentSlotView.Entry.bIsPlayable;
}

bool UWacomFirstPersonCardLayerSlotWidget::IsNoTargetDragCard() const
{
	return CurrentSlotView.Entry.InteractionIntent
			== EWacomFirstPersonCardInteractionIntent::CommitNoTarget
		|| CurrentSlotView.Entry.InteractionIntent
			== EWacomFirstPersonCardInteractionIntent::DragToDropTarget;
}

bool UWacomFirstPersonCardLayerSlotWidget::IsTargetedAimCard() const
{
	return CurrentSlotView.Entry.InteractionIntent
			== EWacomFirstPersonCardInteractionIntent::AimWorldTarget
		|| CurrentSlotView.Entry.InteractionIntent
			== EWacomFirstPersonCardInteractionIntent::AimCardTarget;
}

bool UWacomFirstPersonCardLayerSlotWidget::ResolveInspectScreenPosition(
	FVector2D& OutScreenPosition) const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			FVector2D ViewportSize = FVector2D::ZeroVector;
			ViewportClient->GetViewportSize(ViewportSize);
			if (ViewportSize.X > 0.0f && ViewportSize.Y > 0.0f)
			{
				const APlayerController* PC = GetOwningPlayer();
				const float ViewportScale = PC
					? FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(PC))
					: 1.0f;
				ViewportSize /= ViewportScale;
				OutScreenPosition = FVector2D(
					ViewportSize.X * CardDragConfig.CardInspectScreenPosition.X,
					ViewportSize.Y * CardDragConfig.CardInspectScreenPosition.Y);
				return true;
			}
		}
	}

	if (!TargetSlotView.AnchorWidgetPosition.IsNearlyZero())
	{
		OutScreenPosition = TargetSlotView.AnchorWidgetPosition + FVector2D(0.0f, -96.0f);
		return true;
	}
	return false;
}

bool UWacomFirstPersonCardLayerSlotWidget::ResolvePointerWidgetPosition(
	const FPointerEvent& InMouseEvent,
	FVector2D& OutScreenPosition) const
{
	if (!ResolveAbsoluteScreenPositionToWidgetPosition(InMouseEvent.GetScreenSpacePosition(), OutScreenPosition))
	{
		return false;
	}

	const_cast<UWacomFirstPersonCardLayerSlotWidget*>(this)->UpdatePointerViewportDiagnostics(OutScreenPosition);
	return true;
}

bool UWacomFirstPersonCardLayerSlotWidget::ResolveAbsoluteScreenPositionToWidgetPosition(
	const FVector2D& AbsoluteScreenPosition,
	FVector2D& OutWidgetPosition) const
{
	FVector2D PixelPosition = FVector2D::ZeroVector;
	FVector2D ViewportPosition = FVector2D::ZeroVector;
	USlateBlueprintLibrary::AbsoluteToViewport(
		this,
		AbsoluteScreenPosition,
		PixelPosition,
		ViewportPosition);

	if (ViewportPosition.ContainsNaN())
	{
		return false;
	}

	OutWidgetPosition = ViewportPosition;
	return true;
}

bool UWacomFirstPersonCardLayerSlotWidget::IsScreenPositionInsideCardBody(const FVector2D& ScreenPosition) const
{
	FVector2D WidgetPosition = FVector2D::ZeroVector;
	if (ResolveAbsoluteScreenPositionToWidgetPosition(ScreenPosition, WidgetPosition))
	{
		return IsWidgetPositionInsideCardBodyForFirstPersonLayer(WidgetPosition);
	}

	if (CardView && CardView->HasCardBodyHitGeometry())
	{
		return CardView->IsScreenPositionInsideCardBody(ScreenPosition);
	}

	const FGeometry& SlotGeometry = GetCachedGeometry();
	return IsLocalPositionInsideCardBody(SlotGeometry.AbsoluteToLocal(ScreenPosition));
}

bool UWacomFirstPersonCardLayerSlotWidget::IsLocalPositionInsideCardBody(const FVector2D& LocalPosition) const
{
	FVector2D BodySize = CardView
		? CardView->GetCardBodyHitSize()
		: UWacomFirstPersonCardViewWidget::GetDefaultCardBodyHitSize();
	if (BodySize.X <= 1.0f || BodySize.Y <= 1.0f)
	{
		BodySize = UWacomFirstPersonCardViewWidget::GetDefaultCardBodyHitSize();
	}

	FVector2D SlotSize = GetCachedGeometry().GetLocalSize();
#if WITH_AUTOMATION_TESTS
	if (LocalHitCanvasSizeOverrideForTest.IsSet())
	{
		SlotSize = LocalHitCanvasSizeOverrideForTest.GetValue();
	}
#endif
	const FVector2D EffectiveSlotSize = (SlotSize.X > 1.0f && SlotSize.Y > 1.0f)
		? SlotSize
		: BodySize;
	const FVector2D HitMin = (EffectiveSlotSize - BodySize) * 0.5f;
	const FVector2D HitMax = HitMin + BodySize;
	return LocalPosition.X >= HitMin.X
		&& LocalPosition.Y >= HitMin.Y
		&& LocalPosition.X <= HitMax.X
		&& LocalPosition.Y <= HitMax.Y;
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateBodyHoverFromScreenPosition(const FVector2D& ScreenPosition)
{
	SetHoveredForFirstPersonLayer(CanInteractWithCurrentSlot() && IsScreenPositionInsideCardBody(ScreenPosition));
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateBodyHoverFromLocalPosition(const FVector2D& LocalPosition)
{
	SetHoveredForFirstPersonLayer(CanInteractWithCurrentSlot() && IsLocalPositionInsideCardBody(LocalPosition));
}

void UWacomFirstPersonCardLayerSlotWidget::BeginGesturePress(
	const FVector2D& ScreenPosition,
	EWacomFirstPersonCardGestureSource Source,
	EWacomFirstPersonCardGestureInputSource InputSource)
{
	if (!CanInteractWithCurrentSlot())
	{
		return;
	}

	ClearGestureState(false);
	GestureController->BeginPress(CurrentSlotView, ScreenPosition, Source, InputSource);
	UpdatePointerViewportDiagnostics(ScreenPosition);
	bCardDragProbeFeedback = false;
	bCardDragProbeFeedbackValid = false;
	CardDragTargetFocusFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	DragTargetFeedbackState = ResolveEffectiveDragTargetFeedbackState();
	SetPressedForFirstPersonLayer(true);
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateGesture(
	float DeltaTime,
	const FVector2D& ScreenPosition,
	bool bSuppressInspectDragPromotion,
	bool bBroadcastDragUpdate)
{
	if (GestureRuntime().State == EWacomFirstPersonCardGestureState::Idle
		|| GestureRuntime().State == EWacomFirstPersonCardGestureState::Cancelled)
	{
		return;
	}

	GestureController->UpdatePointer(DeltaTime, ScreenPosition);
	UpdatePointerViewportDiagnostics(ScreenPosition);
	const float DragDistance = FVector2D::Distance(GestureRuntime().CurrentScreenPosition, GestureRuntime().PressScreenPosition);

	if (GestureRuntime().State == EWacomFirstPersonCardGestureState::Pressed)
	{
		if (CanStartCardDragGesture()
			&& DragDistance >= CardDragConfig.CardDragStartThresholdPixels)
		{
			PromoteGestureToCardDrag(true);
		}
		else if (GestureRuntime().ElapsedSeconds >= CardDragConfig.CardInspectHoldDelaySeconds)
		{
			SetGestureState(EWacomFirstPersonCardGestureState::Inspecting, true);
		}
	}
	else if (GestureRuntime().State == EWacomFirstPersonCardGestureState::Inspecting
		&& !bSuppressInspectDragPromotion
		&& CanStartCardDragGesture()
		&& CurrentSlotView.Entry.InteractionIntent
			!= EWacomFirstPersonCardInteractionIntent::InspectOnly
		&& DragDistance >= CardDragConfig.CardDragStartThresholdPixels)
	{
		PromoteGestureToCardDrag(true);
	}

	const bool bBattleNoTargetCommit =
		CurrentSlotView.Entry.InteractionIntent
		== EWacomFirstPersonCardInteractionIntent::CommitNoTarget;
	if (bBattleNoTargetCommit
		&& (GestureRuntime().State == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
			|| GestureRuntime().State == EWacomFirstPersonCardGestureState::ArmedForCommit))
	{
		const bool bNowArmed =
			ComputeNoTargetDragOutDistance()
			>= CardDragConfig.NoTargetCardDragOutCommitDistancePixels;
		SetGestureState(
			bNowArmed
				? EWacomFirstPersonCardGestureState::ArmedForCommit
				: EWacomFirstPersonCardGestureState::DraggingNoTargetCard,
			false);
	}

	UpdateGestureOverrideTarget();
	if (bBroadcastDragUpdate)
	{
		BroadcastDragUpdated();
	}
	UpdateWantsTick();
}

bool UWacomFirstPersonCardLayerSlotWidget::ReleaseGesture(
	const FVector2D& ScreenPosition,
	bool bSuppressInspectDragPromotion)
{
	if (GestureRuntime().State == EWacomFirstPersonCardGestureState::Idle
		|| GestureRuntime().State == EWacomFirstPersonCardGestureState::Cancelled)
	{
		return false;
	}

	GestureController->UpdatePointer(0.0f, ScreenPosition);
	UpdateGesture(0.0f, ScreenPosition, bSuppressInspectDragPromotion, false);

	const EWacomFirstPersonCardGestureState ReleaseState = GestureRuntime().State;
	// Release delegates synchronously submit commands and may refresh the entire
	// card layer. Capture the semantic outcome before broadcasting so that a
	// successful refresh cannot clear GestureRuntime().bTargetValid and turn acceptance
	// into a false Deny pulse on return.
	const bool bAcceptedRelease =
		ReleaseState == EWacomFirstPersonCardGestureState::ArmedForCommit
		|| (ReleaseState == EWacomFirstPersonCardGestureState::AimingTargetedCard
			&& GestureRuntime().bTargetValid);
	const bool bResolvedInvalidTargetRelease =
		IsFormalDragGestureState(ReleaseState)
		&& GestureRuntime().FeedbackTargetHandle.IsValid()
		&& !GestureRuntime().bTargetValid
		&& IsResolvedInvalidTargetFeedbackState(DirectDragTargetFeedbackState);
	const bool bNeutralRelease =
		ReleaseState == EWacomFirstPersonCardGestureState::Inspecting
		|| ReleaseState == EWacomFirstPersonCardGestureState::Pressed
		|| !bResolvedInvalidTargetRelease;
	const FVector2D FrozenReleaseDirection =
		(ScreenPosition - GestureRuntime().PressScreenPosition).GetSafeNormal();
	const int32 FrozenDenySeed = HashCombineFast(
		GetTypeHash(CurrentSlotView.Entry.CardInstanceId),
		GetTypeHash(DirectDragTargetFeedbackState));
	SetPressedForFirstPersonLayer(false);
	BroadcastDragReleased();

	if (bAcceptedRelease || bNeutralRelease)
	{
		// Authority-owned Commit feedback begins only after the accepted command is
		// reflected back into the layer. A pointer release has no optimistic cue.
	}
	else if (bResolvedInvalidTargetRelease)
	{
		TriggerDenyFeedback(FrozenReleaseDirection, FrozenDenySeed);
	}

	ClearGestureState(false);
	return true;
}

bool UWacomFirstPersonCardLayerSlotWidget::PromoteGestureToCardDrag(
	bool bBroadcastStartOrCancel)
{
	EWacomFirstPersonCardMotionIntent PromotionMotionIntent = EWacomFirstPersonCardMotionIntent::Layout;
	EWacomFirstPersonCardGestureState PromotedState = EWacomFirstPersonCardGestureState::Cancelled;
	if (IsNoTargetDragCard())
	{
		PromotionMotionIntent = EWacomFirstPersonCardMotionIntent::Layout;
		PromotedState = EWacomFirstPersonCardGestureState::DraggingNoTargetCard;
	}
	else if (IsTargetedAimCard())
	{
		PromotionMotionIntent = EWacomFirstPersonCardMotionIntent::Pending;
		PromotedState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	}
	else
	{
		SetGestureState(EWacomFirstPersonCardGestureState::Cancelled, bBroadcastStartOrCancel);
		return false;
	}

	SetGestureState(PromotedState, bBroadcastStartOrCancel);
	ActiveMotionIntent = PromotionMotionIntent;
	UpdateWantsTick();
	return true;
}

void UWacomFirstPersonCardLayerSlotWidget::SetGestureState(
	EWacomFirstPersonCardGestureState NewState,
	bool bBroadcastStartOrCancel)
{
	if (GestureRuntime().State == NewState)
	{
		return;
	}

	const EWacomFirstPersonCardGestureState PreviousState =
		GestureController->TransitionTo(NewState);
	UpdateGestureOverrideTarget();
	const bool bWasFormalDrag = IsFormalDragGestureState(PreviousState);
	const bool bIsFormalDrag = IsFormalDragGestureState(NewState);
	if (!bWasFormalDrag && bIsFormalDrag)
	{
		SetPressedForFirstPersonLayer(false);
		BeginDragPickupFeedback();
	}
	else if (bWasFormalDrag && !bIsFormalDrag)
	{
		ResetDragPickupFeedback();
	}
	UpdateWantsTick();

	if (bBroadcastStartOrCancel
		&& PreviousState == EWacomFirstPersonCardGestureState::Pressed
		&& (NewState == EWacomFirstPersonCardGestureState::Inspecting
			|| NewState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
			|| NewState == EWacomFirstPersonCardGestureState::AimingTargetedCard))
	{
		BroadcastDragStarted();
	}
	else if (bBroadcastStartOrCancel && NewState == EWacomFirstPersonCardGestureState::Cancelled)
	{
		BroadcastDragCancelled();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateGestureOverrideTarget()
{
	switch (GestureRuntime().State)
	{
	case EWacomFirstPersonCardGestureState::Inspecting:
		GestureRuntime().OverrideTargetSlotView = BuildInspectOverrideSlotView();
		break;
	case EWacomFirstPersonCardGestureState::DraggingNoTargetCard:
	case EWacomFirstPersonCardGestureState::ArmedForCommit:
		GestureRuntime().OverrideTargetSlotView = BuildNoTargetDragOverrideSlotView();
		break;
	case EWacomFirstPersonCardGestureState::AimingTargetedCard:
		GestureRuntime().OverrideTargetSlotView = BuildAimOverrideSlotView();
		break;
	default:
		GestureRuntime().OverrideTargetSlotView.Reset();
		break;
	}
}

void UWacomFirstPersonCardLayerSlotWidget::ClearGestureState(bool bBroadcastCancel)
{
	const bool bHadGesture = GestureController->IsActive();
	if (bHadGesture && bBroadcastCancel)
	{
		BroadcastDragCancelled();
	}

	GestureController->Reset(SlotMotionConfig.bEnabled && bHasVisualSlotView);
	DirectDragTargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	bHasFeedbackTargetScreenPosition = false;
	FeedbackTargetScreenPosition = FVector2D::ZeroVector;
	bCardDragProbeFeedback = false;
	bCardDragProbeFeedbackValid = false;
	CardDragTargetFocusFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	DragTargetFeedbackState = ResolveEffectiveDragTargetFeedbackState();
	if (InteractionFeedbackPlayback)
	{
		InteractionFeedbackPlayback->SetInvalidTargetPreview(false);
	}
	ClearPointerViewportDiagnostics();
	SetPressedForFirstPersonLayer(false);
	ResetDragPickupFeedback();
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::Layout);
	ApplyVisualSlotView();
	UpdateWantsTick();
}

float UWacomFirstPersonCardLayerSlotWidget::ComputeNoTargetDragOutDistance() const
{
	switch (CardDragConfig.NoTargetCardDragOutDirection)
	{
	case EWacomFirstPersonCardDragOutDirection::Up:
	default:
		return FMath::Max(0.0f, GestureRuntime().PressScreenPosition.Y - GestureRuntime().CurrentScreenPosition.Y);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::SetHoveredFromFirstPersonLayer(bool bHovered)
{
	SetHoveredForFirstPersonLayer(bHovered, false);
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateGestureFromFirstPersonLayer(
	float DeltaTime,
	const FVector2D& WidgetPosition,
	bool bSuppressInspectDragPromotion)
{
	UpdateGesture(DeltaTime, WidgetPosition, bSuppressInspectDragPromotion);
}

bool UWacomFirstPersonCardLayerSlotWidget::ReleaseGestureFromFirstPersonLayer(
	const FVector2D& WidgetPosition,
	bool bSuppressInspectDragPromotion)
{
	return ReleaseGesture(WidgetPosition, bSuppressInspectDragPromotion);
}

void UWacomFirstPersonCardLayerSlotWidget::ClearInspectScrubGestureFromFirstPersonLayer()
{
	if (GestureRuntime().State == EWacomFirstPersonCardGestureState::Inspecting)
	{
		ClearGestureState(false);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::BroadcastDragStarted()
{
	if (CurrentSlotView.Entry.CardInstanceId.IsValid())
	{
		OnCardDragStartedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, BuildDragView());
	}
}

void UWacomFirstPersonCardLayerSlotWidget::BroadcastDragUpdated()
{
	if (CurrentSlotView.Entry.CardInstanceId.IsValid()
		&& GestureRuntime().State != EWacomFirstPersonCardGestureState::Idle
		&& GestureRuntime().State != EWacomFirstPersonCardGestureState::Cancelled)
	{
		OnCardDragUpdatedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, BuildDragView());
	}
}

void UWacomFirstPersonCardLayerSlotWidget::BroadcastDragReleased()
{
	if (CurrentSlotView.Entry.CardInstanceId.IsValid())
	{
		OnCardDragReleasedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, BuildDragView());
	}
}

void UWacomFirstPersonCardLayerSlotWidget::BroadcastDragCancelled()
{
	if (CurrentSlotView.Entry.CardInstanceId.IsValid())
	{
		OnCardDragCancelledNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, BuildDragView());
	}
}

FWacomFirstPersonCardDragView UWacomFirstPersonCardLayerSlotWidget::BuildDragView() const
{
	FWacomFirstPersonCardDragView View;
	View.GestureState = GestureRuntime().State;
	View.GestureSource = GestureRuntime().Source;
	View.CardInstanceId = CurrentSlotView.Entry.CardInstanceId;
	View.SourceSlotView = bHasVisualSlotView ? VisualSlotView : CurrentSlotView;
	View.SourceSlotView.GestureState = GestureRuntime().State;
	View.PressScreenPosition = GestureRuntime().PressScreenPosition;
	View.CurrentScreenPosition = GestureRuntime().CurrentScreenPosition;
	View.CurrentTarget = GestureRuntime().FeedbackTargetHandle;
	View.bCommitArmed = GestureRuntime().bCommitArmed;
	View.bTargetValid = GestureRuntime().bTargetValid;
	View.bHasPointerViewportPosition = bHasPointerViewportPosition;
	View.PointerViewportPosition = PointerViewportPosition;
	View.PointerNormalizedViewportPosition = PointerNormalizedViewportPosition;
	View.TargetFeedbackState = ResolveEffectiveDragTargetFeedbackState();
	View.bHasFeedbackTargetScreenPosition = bHasFeedbackTargetScreenPosition;
	View.FeedbackTargetScreenPosition = FeedbackTargetScreenPosition;
	return View;
}

bool UWacomFirstPersonCardLayerSlotWidget::BeginGesturePressFromFirstPersonLayer(
	const FVector2D& WidgetPosition)
{
	if (!CanInteractWithCurrentSlot())
	{
		return false;
	}

	BeginGesturePress(
		WidgetPosition,
		EWacomFirstPersonCardGestureSource::MousePress,
		EWacomFirstPersonCardGestureInputSource::MousePointer);
	return true;
}

bool UWacomFirstPersonCardLayerSlotWidget::BeginDragGestureFromFirstPersonLayer(
	const FVector2D& WidgetPosition)
{
	return BeginDragGestureFromFirstPersonLayer(WidgetPosition, WidgetPosition);
}

bool UWacomFirstPersonCardLayerSlotWidget::BeginDragGestureFromFirstPersonLayer(
	const FVector2D& GestureOriginPosition,
	const FVector2D& InitialPointerPosition)
{
	if (!CanStartCardDragGesture())
	{
		return false;
	}

	BeginGesturePress(
		GestureOriginPosition,
		EWacomFirstPersonCardGestureSource::KeyboardShortcut,
		EWacomFirstPersonCardGestureInputSource::ExternalPointer);
	GestureRuntime().CurrentScreenPosition = InitialPointerPosition;
	UpdatePointerViewportDiagnostics(InitialPointerPosition);
	if (!PromoteGestureToCardDrag(true))
	{
		return false;
	}

	UpdateGesture(0.0f, InitialPointerPosition);
	return true;
}

bool UWacomFirstPersonCardLayerSlotWidget::BeginInspectScrubFromFirstPersonLayer(
	const FVector2D& WidgetPosition)
{
	if (!CanBeginInspectScrubFromFirstPersonLayer())
	{
		return false;
	}

	BeginGesturePress(
		WidgetPosition,
		EWacomFirstPersonCardGestureSource::MousePress,
		EWacomFirstPersonCardGestureInputSource::MousePointer);
	GestureRuntime().CurrentScreenPosition = WidgetPosition;
	UpdatePointerViewportDiagnostics(WidgetPosition);
	GestureRuntime().ElapsedSeconds = FMath::Max(
		GestureRuntime().ElapsedSeconds,
		CardDragConfig.CardInspectHoldDelaySeconds);
	SetGestureState(EWacomFirstPersonCardGestureState::Inspecting, true);
	UpdateGesture(0.0f, WidgetPosition, true);
	return GestureRuntime().State == EWacomFirstPersonCardGestureState::Inspecting;
}

void UWacomFirstPersonCardLayerSlotWidget::BeginDragPickupFeedback()
{
	if (!DragPickupPlayback)
	{
		DragPickupPlayback.Reset(new FWacomFirstPersonCardDragPickupPlayback());
	}

	const bool bIsFarKeyboardNoTargetDrag =
		GestureRuntime().Source == EWacomFirstPersonCardGestureSource::KeyboardShortcut
		&& GestureRuntime().State == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
		&& GestureRuntime().OverrideTargetSlotView.IsSet()
		&& FVector2D::Distance(
			VisualSlotView.ScreenPosition,
			GestureRuntime().OverrideTargetSlotView->ScreenPosition)
			> FMath::Max(1.0f, CardDragConfig.CardDragStartThresholdPixels);
	DragPickupPlayback->Begin(DragPickupConfig, !bIsFarKeyboardNoTargetDrag);
#if WITH_AUTOMATION_TESTS
	++DragPickupTriggerCountForTest;
#endif
	PlayPendingDragPickupSound();
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::TickDragPickupFeedback(float DeltaTime)
{
	if (!DragPickupPlayback || !DragPickupPlayback->IsActive())
	{
		return;
	}

	DragPickupPlayback->Tick(DeltaTime);
	ApplyVisualSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::TryStartDeferredDragPickupFeedback()
{
	const float PointerAcquireDistancePixels = FMath::Max(
		24.0f,
		CardDragConfig.CardDragStartThresholdPixels * 2.0f);
	if (!DragPickupPlayback
		|| !DragPickupPlayback->IsWaitingForVisualStart()
		|| !IsFormalDragGestureState(GestureRuntime().State)
		|| FVector2D::Distance(
			VisualSlotView.ScreenPosition,
			GetEffectiveTargetSlotView().ScreenPosition) > PointerAcquireDistancePixels)
	{
		return;
	}

	DragPickupPlayback->StartVisualPlayback();
	ApplyVisualSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::ResetDragPickupFeedback()
{
	if (DragPickupPlayback)
	{
		DragPickupPlayback->Reset();
	}
	ApplyActiveSurfaceEffectView();
	ApplyVisualSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::PlayPendingDragPickupSound()
{
	if (!DragPickupPlayback)
	{
		return;
	}

	const TOptional<FWacomFirstPersonCardDragPickupSoundRequest> PendingRequest =
		DragPickupPlayback->ConsumePendingSoundRequest();
	if (!PendingRequest.IsSet())
	{
		return;
	}

	const FWacomFirstPersonCardDragPickupSoundRequest& Request = PendingRequest.GetValue();
#if WITH_AUTOMATION_TESTS
	++DragPickupSoundRequestCountForTest;
	LastDragPickupSoundPitchMultiplierForTest = Request.PitchMultiplier;
#endif
	if (USoundBase* Sound = Request.Sound.Get(); Sound && GetWorld())
	{
		UGameplayStatics::PlaySound2D(
			GetWorld(),
			Sound,
			Request.VolumeMultiplier,
			Request.PitchMultiplier);
	}
}

float UWacomFirstPersonCardLayerSlotWidget::GetDragPickupAlpha() const
{
	return DragPickupPlayback ? DragPickupPlayback->GetAlpha() : 0.0f;
}

void UWacomFirstPersonCardLayerSlotWidget::SetHoveredForFirstPersonLayer(bool bHovered, bool bBroadcast)
{
	if (bIsHoveredForFirstPersonLayer == bHovered)
	{
		return;
	}

	bIsHoveredForFirstPersonLayer = bHovered;
	CurrentSlotView.bIsHovered = bIsHoveredForFirstPersonLayer;
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::Hover);
	if (!bIsHoveredForFirstPersonLayer)
	{
		const bool bGestureActive =
			GestureRuntime().State != EWacomFirstPersonCardGestureState::Idle
			&& GestureRuntime().State != EWacomFirstPersonCardGestureState::Cancelled;
		if (!bGestureActive)
		{
			SetPressedForFirstPersonLayer(false);
		}
	}
	ApplyVisualSlotView();
	UpdateWantsTick();
	if (!bBroadcast)
	{
		return;
	}
	if (CurrentSlotView.Entry.CardInstanceId.IsValid())
	{
		if (bIsHoveredForFirstPersonLayer)
		{
			FWacomFirstPersonCardLayerSlotView VisualHoverSlotView = VisualSlotView;
			VisualHoverSlotView.bIsHovered = true;
			OnCardHoveredNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, VisualHoverSlotView);
			if (const FWacomInteractionTargetHandle CardTargetHandle = BuildCardTargetHandle(); CardTargetHandle.IsValid())
			{
				OnCardTargetHoveredNative.Broadcast(CardTargetHandle, VisualHoverSlotView);
			}
		}
		else
		{
			const FWacomInteractionTargetHandle CardTargetHandle = FWacomInteractionTargetHandle::ForCardTarget(
				CurrentSlotView.Entry.CardInstanceId,
				this,
				VisualSlotView.ScreenPosition);
			FWacomFirstPersonCardLayerSlotView VisualTargetSlotView = VisualSlotView;
			VisualTargetSlotView.bIsHovered = false;
			OnCardUnhoveredNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, VisualTargetSlotView);
			OnCardTargetUnhoveredNative.Broadcast(CardTargetHandle, VisualTargetSlotView);
		}
	}
}

void UWacomFirstPersonCardLayerSlotWidget::SetPressedForFirstPersonLayer(bool bPressed)
{
	if (!InteractionFeedbackPlayback)
	{
		InteractionFeedbackPlayback.Reset(
			new FWacomFirstPersonCardInteractionFeedbackPlayback());
		InteractionFeedbackPlayback->SetConfig(InteractionFeedbackConfig);
	}
	const bool bAcceptedPressed = bPressed
		&& InteractionFeedbackConfig.bEnabled
		&& CanInteractWithCurrentSlot();
	if (InteractionFeedbackPlayback->BuildView().bPressed == bAcceptedPressed)
	{
		return;
	}
	InteractionFeedbackPlayback->SetPressed(bAcceptedPressed);
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::UpdatePressedFeedback(float DeltaTime)
{
	if (InteractionFeedbackPlayback)
	{
		InteractionFeedbackPlayback->Tick(DeltaTime);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerDenyFeedback(
	const FVector2D& ReleaseDirection,
	int32 Seed)
{
	if (!InteractionFeedbackConfig.bEnabled || InteractionFeedbackConfig.DenyDuration <= 0.0f)
	{
		return;
	}

	if (!InteractionFeedbackPlayback)
	{
		InteractionFeedbackPlayback.Reset(
			new FWacomFirstPersonCardInteractionFeedbackPlayback());
		InteractionFeedbackPlayback->SetConfig(InteractionFeedbackConfig);
	}
	InteractionFeedbackPlayback->TriggerDeny(ReleaseDirection, Seed);
	PlayPendingDenySound();
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::PlayPendingDenySound()
{
	if (!InteractionFeedbackPlayback)
	{
		return;
	}

	const TOptional<FWacomFirstPersonCardDenySoundRequest> PendingRequest =
		InteractionFeedbackPlayback->ConsumePendingDenySoundRequest();
	if (!PendingRequest.IsSet())
	{
		return;
	}

	const FWacomFirstPersonCardDenySoundRequest& Request = PendingRequest.GetValue();
#if WITH_AUTOMATION_TESTS
	++DenySoundRequestCountForTest;
	LastDenySoundPitchMultiplierForTest = Request.PitchMultiplier;
#endif
	if (USoundBase* Sound = Request.Sound.Get(); Sound && GetWorld())
	{
		UGameplayStatics::PlaySound2D(
			GetWorld(),
			Sound,
			Request.VolumeMultiplier,
			Request.PitchMultiplier);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerCommitFeedback()
{
	if (!InteractionFeedbackConfig.bEnabled
		|| !InteractionFeedbackConfig.bEnablePlayCommitFeedback
		|| InteractionFeedbackConfig.PlayCommitDuration <= 0.0f)
	{
		return;
	}

	if (!InteractionFeedbackPlayback)
	{
		InteractionFeedbackPlayback.Reset(
			new FWacomFirstPersonCardInteractionFeedbackPlayback());
		InteractionFeedbackPlayback->SetConfig(InteractionFeedbackConfig);
	}
	InteractionFeedbackPlayback->TriggerCommit();
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::ClearInteractionFeedback()
{
	if (InteractionFeedbackPlayback)
	{
		InteractionFeedbackPlayback->Reset();
	}
	ResetDragPickupFeedback();
	ClearCardDragTargetFeedback();
	bHasFeedbackTargetScreenPosition = false;
	FeedbackTargetScreenPosition = FVector2D::ZeroVector;
	ApplyInteractionCue();
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyInteractionCue()
{
	if (!CardView || !InteractionFeedbackPlayback)
	{
		if (CardView)
		{
			CardView->ClearInteractionCueView();
		}
		return;
	}

	const FWacomFirstPersonCardInteractionFeedbackPlaybackView PlaybackView =
		InteractionFeedbackPlayback->BuildView();
	if (PlaybackView.bDenyActive && PlaybackView.DenyPulseAlpha > KINDA_SMALL_NUMBER)
	{
		FWacomFirstPersonCardInteractionCueView CueView;
		CueView.Kind = EWacomFirstPersonCardInteractionCueKind::Deny;
		CueView.Color = InteractionFeedbackConfig.DenyColor;
		CueView.AccentColor = InteractionFeedbackConfig.DenyAccentColor;
		CueView.Amount = FMath::Clamp(
			PlaybackView.DenyPulseAlpha * InteractionFeedbackConfig.DenyOpacity,
			0.0f,
			1.0f);
		CueView.Progress = PlaybackView.DenyProgress;
		CueView.CornerInsetPixels = InteractionFeedbackConfig.DenyCornerInsetPixels;
		CueView.CornerLengthPixels = InteractionFeedbackConfig.DenyCornerLengthPixels;
		CueView.CornerThicknessPixels = InteractionFeedbackConfig.DenyCornerThicknessPixels;
		CueView.CrackLengthPixels = InteractionFeedbackConfig.DenyCrackLengthPixels;
		CueView.CrackThicknessPixels = InteractionFeedbackConfig.DenyCrackThicknessPixels;
		CueView.Direction = PlaybackView.DenyDirection;
		CueView.Seed = PlaybackView.DenySeed;
		CueView.bReducedMotion = InteractionFeedbackConfig.bReduceInteractionMotion;
		CardView->SetInteractionCueView(CueView);
		return;
	}

	if (PlaybackView.bInvalidTargetPreviewActive
		&& PlaybackView.InvalidTargetPreviewAmount > KINDA_SMALL_NUMBER)
	{
		FWacomFirstPersonCardInteractionCueView CueView;
		CueView.Kind = EWacomFirstPersonCardInteractionCueKind::InvalidPreview;
		CueView.Color = InteractionFeedbackConfig.InvalidTargetPreviewColor;
		CueView.AccentColor = InteractionFeedbackConfig.InvalidTargetPreviewAccentColor;
		CueView.Amount = FMath::Clamp(
			PlaybackView.InvalidTargetPreviewAmount
				* InteractionFeedbackConfig.InvalidTargetPreviewOpacity,
			0.0f,
			1.0f);
		CueView.Progress = PlaybackView.InvalidTargetPreviewAmount;
		CueView.CornerInsetPixels = InteractionFeedbackConfig.DenyCornerInsetPixels;
		CueView.CornerLengthPixels = InteractionFeedbackConfig.DenyCornerLengthPixels;
		CueView.CornerThicknessPixels = InteractionFeedbackConfig.DenyCornerThicknessPixels;
		CueView.TightenPixels = InteractionFeedbackConfig.InvalidTargetPreviewTightenPixels;
		CueView.bReducedMotion = InteractionFeedbackConfig.bReduceInteractionMotion;
		CardView->SetInteractionCueView(CueView);
		return;
	}

	CardView->ClearInteractionCueView();
}
