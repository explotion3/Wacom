// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Styling/SlateBrush.h"
#include "InputCoreTypes.h"
#include "UI/Card/WacomCardView.h"

namespace
{
	float ComputeInterpAlpha(float Speed, float DeltaTime)
	{
		return Speed <= 0.0f ? 1.0f : FMath::Clamp(DeltaTime * Speed, 0.0f, 1.0f);
	}

	float ComputePulseAlpha(float ElapsedSeconds, float DurationSeconds)
	{
		if (DurationSeconds <= 0.0f || ElapsedSeconds >= DurationSeconds)
		{
			return 0.0f;
		}

		return 1.0f - FMath::Clamp(ElapsedSeconds / DurationSeconds, 0.0f, 1.0f);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardViewClass(TSubclassOf<UWacomCardView> InCardViewClass)
{
	TSubclassOf<UWacomCardView> NewCardViewClass = InCardViewClass;
	if (!NewCardViewClass)
	{
		NewCardViewClass = UWacomCardView::StaticClass();
	}

	if (CardViewClass == NewCardViewClass)
	{
		return;
	}

	CardViewClass = NewCardViewClass;
	if (CardView)
	{
		CardView->RemoveFromParent();
		CardView = nullptr;
	}
	EnsureCardView();
	ApplyCurrentSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::SetSlotView(const FWacomFirstPersonCardLayerSlotView& InSlotView)
{
	BeginSlotMotion(InSlotView, !bHasVisualSlotView);
}

void UWacomFirstPersonCardLayerSlotWidget::SetSlotViewImmediate(
	const FWacomFirstPersonCardLayerSlotView& InSlotView)
{
	if (bIsHoveredForFirstPersonLayer
		&& (CurrentSlotView.Entry.CardInstanceId != InSlotView.Entry.CardInstanceId
			|| !InSlotView.bProjected
			|| !InSlotView.Entry.CardInstanceId.IsValid()))
	{
		SetHoveredForFirstPersonLayer(false);
	}
	ClearInteractionFeedback();

	CurrentSlotView = InSlotView;
	TargetSlotView = InSlotView;
	VisualSlotView = InSlotView;
	bHasVisualSlotView = true;
	bIsExitingForFirstPersonLayer = false;
	ExitMotionElapsedSeconds = 0.0f;
	ApplyCurrentSlotView();
	ApplyVisualSlotView();
	SetTickEnabledForMotion(false);
}

void UWacomFirstPersonCardLayerSlotWidget::BeginSlotMotion(
	const FWacomFirstPersonCardLayerSlotView& InTargetSlotView,
	bool bTreatAsNewSlot)
{
	BeginSlotMotionWithEnterOffset(InTargetSlotView, bTreatAsNewSlot, TOptional<FVector2D>());
}

void UWacomFirstPersonCardLayerSlotWidget::BeginSlotMotionWithEnterOffset(
	const FWacomFirstPersonCardLayerSlotView& InTargetSlotView,
	bool bTreatAsNewSlot,
	const TOptional<FVector2D>& EnterOffsetOverride)
{
	if (!SlotMotionConfig.bEnabled)
	{
		SetSlotViewImmediate(InTargetSlotView);
		return;
	}

	if (bIsHoveredForFirstPersonLayer
		&& (CurrentSlotView.Entry.CardInstanceId != InTargetSlotView.Entry.CardInstanceId
			|| !InTargetSlotView.bProjected
			|| !InTargetSlotView.Entry.CardInstanceId.IsValid()))
	{
		SetHoveredForFirstPersonLayer(false);
	}
	if (CurrentSlotView.Entry.CardInstanceId != InTargetSlotView.Entry.CardInstanceId
		|| !InTargetSlotView.bProjected
		|| !InTargetSlotView.Entry.CardInstanceId.IsValid())
	{
		ClearInteractionFeedback();
	}

	const bool bCanReuseVisual =
		bHasVisualSlotView
		&& !bTreatAsNewSlot
		&& CurrentSlotView.Entry.CardInstanceId == InTargetSlotView.Entry.CardInstanceId;
	const float JumpDistance = bCanReuseVisual
		? FVector2D::Distance(VisualSlotView.ScreenPosition, InTargetSlotView.ScreenPosition)
		: 0.0f;
	const bool bLargeJump =
		bCanReuseVisual
		&& SlotMotionConfig.ResetDistancePixels > 0.0f
		&& JumpDistance > SlotMotionConfig.ResetDistancePixels;

	CurrentSlotView = InTargetSlotView;
	TargetSlotView = InTargetSlotView;
	bIsExitingForFirstPersonLayer = false;
	ExitMotionElapsedSeconds = 0.0f;
	ApplyCurrentSlotView();

	if (!bCanReuseVisual || bLargeJump)
	{
		VisualSlotView = InTargetSlotView;
		if (bTreatAsNewSlot && InTargetSlotView.bProjected)
		{
			const FVector2D EnterOffset = EnterOffsetOverride.Get(SlotMotionConfig.EnterOffsetPixels);
			VisualSlotView.ScreenPosition = InTargetSlotView.ScreenPosition + EnterOffset;
			VisualSlotView.WidgetPosition = VisualSlotView.ScreenPosition;
			VisualSlotView.SnappedWidgetPosition = VisualSlotView.ScreenPosition;
			VisualSlotView.RenderOpacity = FMath::Clamp(SlotMotionConfig.EnterOpacity, 0.0f, 1.0f);
		}
		bHasVisualSlotView = true;
	}

	ApplyVisualSlotView();
	SetTickEnabledForMotion(true);
}

void UWacomFirstPersonCardLayerSlotWidget::BeginExitMotion(
	const FWacomFirstPersonCardLayerSlotView& InExitTargetSlotView)
{
	BeginExitMotionWithOffset(InExitTargetSlotView, TOptional<FVector2D>());
}

void UWacomFirstPersonCardLayerSlotWidget::BeginExitMotionWithOffset(
	const FWacomFirstPersonCardLayerSlotView& InExitTargetSlotView,
	const TOptional<FVector2D>& ExitOffsetOverride)
{
	if (!SlotMotionConfig.bEnabled || SlotMotionConfig.ExitDuration <= 0.0f || !bHasVisualSlotView)
	{
		SetHoveredForFirstPersonLayer(false);
		ClearInteractionFeedback();
		bIsExitingForFirstPersonLayer = true;
		ExitMotionElapsedSeconds = SlotMotionConfig.ExitDuration;
		SetVisibility(ESlateVisibility::Collapsed);
		SetTickEnabledForMotion(false);
		return;
	}

	SetHoveredForFirstPersonLayer(false);
	ClearInteractionFeedback();
	CurrentSlotView = InExitTargetSlotView;
	CurrentSlotView.bProjected = false;
	TargetSlotView = InExitTargetSlotView;
	const FVector2D ExitOffset = ExitOffsetOverride.Get(SlotMotionConfig.ExitOffsetPixels);
	TargetSlotView.ScreenPosition = VisualSlotView.ScreenPosition + ExitOffset;
	TargetSlotView.WidgetPosition = TargetSlotView.ScreenPosition;
	TargetSlotView.SnappedWidgetPosition = TargetSlotView.ScreenPosition;
	TargetSlotView.RenderOpacity = 0.0f;
	TargetSlotView.bProjected = VisualSlotView.bProjected;
	bIsExitingForFirstPersonLayer = true;
	ExitMotionElapsedSeconds = 0.0f;
	ApplyCurrentSlotView();
	ApplyVisualSlotView();
	SetTickEnabledForMotion(true);
}

void UWacomFirstPersonCardLayerSlotWidget::SetSlotMotionConfig(
	const FWacomFirstPersonCardSlotMotionConfig& InConfig)
{
	SlotMotionConfig = InConfig;
	SlotMotionConfig.MotionSpeed = FMath::Max(0.0f, SlotMotionConfig.MotionSpeed);
	SlotMotionConfig.OpacitySpeed = FMath::Max(0.0f, SlotMotionConfig.OpacitySpeed);
	SlotMotionConfig.EnterOpacity = FMath::Clamp(SlotMotionConfig.EnterOpacity, 0.0f, 1.0f);
	SlotMotionConfig.ExitDuration = FMath::Max(0.0f, SlotMotionConfig.ExitDuration);
	SlotMotionConfig.ResetDistancePixels = FMath::Max(0.0f, SlotMotionConfig.ResetDistancePixels);
	if (!SlotMotionConfig.bEnabled && bHasVisualSlotView)
	{
		SetSlotViewImmediate(TargetSlotView);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::SetSlotFeedbackConfig(
	const FWacomFirstPersonCardSlotFeedbackConfig& InConfig)
{
	SlotFeedbackConfig = InConfig;
	SlotFeedbackConfig.PlayableHoverOpacity = FMath::Clamp(SlotFeedbackConfig.PlayableHoverOpacity, 0.0f, 1.0f);
	SlotFeedbackConfig.PressedScale = FMath::Max(0.01f, SlotFeedbackConfig.PressedScale);
	SlotFeedbackConfig.PressedOpacity = FMath::Clamp(SlotFeedbackConfig.PressedOpacity, 0.0f, 1.0f);
	SlotFeedbackConfig.ConfirmDuration = FMath::Max(0.0f, SlotFeedbackConfig.ConfirmDuration);
	SlotFeedbackConfig.ConfirmOpacity = FMath::Clamp(SlotFeedbackConfig.ConfirmOpacity, 0.0f, 1.0f);
	SlotFeedbackConfig.DenyDuration = FMath::Max(0.0f, SlotFeedbackConfig.DenyDuration);
	SlotFeedbackConfig.DenyShakePixels = FMath::Max(0.0f, SlotFeedbackConfig.DenyShakePixels);
	SlotFeedbackConfig.DenyOpacity = FMath::Clamp(SlotFeedbackConfig.DenyOpacity, 0.0f, 1.0f);
	if (!SlotFeedbackConfig.bEnabled)
	{
		ClearInteractionFeedback();
	}
	ApplyVisualSlotView();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsExitMotionFinished() const
{
	return bIsExitingForFirstPersonLayer
		&& ExitMotionElapsedSeconds >= FMath::Max(0.0f, SlotMotionConfig.ExitDuration);
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardLayerInteractionEnabled(bool bEnabled)
{
	if (bCardLayerInteractionEnabled == bEnabled)
	{
		return;
	}

	if (!bEnabled)
	{
		SetHoveredForFirstPersonLayer(false);
		ClearInteractionFeedback();
	}

	bCardLayerInteractionEnabled = bEnabled;
	UpdateVisibilityForInteractionMode();
}

TSharedRef<SWidget> UWacomFirstPersonCardLayerSlotWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}

	if (!WidgetTree->RootWidget)
	{
		RootOverlay = WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(),
			TEXT("FirstPersonCardLayerSlotRoot"));
		WidgetTree->RootWidget = RootOverlay;
	}
	else
	{
		RootOverlay = Cast<UOverlay>(WidgetTree->RootWidget);
	}

	EnsureCardView();
	UpdateVisibilityForInteractionMode();
	return Super::RebuildWidget();
}

void UWacomFirstPersonCardLayerSlotWidget::NativeDestruct()
{
	SetHoveredForFirstPersonLayer(false);
	ClearInteractionFeedback();
	SetTickEnabledForMotion(false);
	OnCardClickedNative.Clear();
	OnCardHoveredNative.Clear();
	OnCardUnhoveredNative.Clear();
	CardView = nullptr;
	RootOverlay = nullptr;
	Super::NativeDestruct();
}

void UWacomFirstPersonCardLayerSlotWidget::NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bWantsSlotMotionTick)
	{
		return;
	}
	if (!bHasVisualSlotView)
	{
		SetTickEnabledForMotion(false);
		return;
	}

	if (bIsExitingForFirstPersonLayer)
	{
		ExitMotionElapsedSeconds += FMath::Max(0.0f, InDeltaTime);
	}
	if (ConfirmFeedbackElapsedSeconds < SlotFeedbackConfig.ConfirmDuration)
	{
		ConfirmFeedbackElapsedSeconds += FMath::Max(0.0f, InDeltaTime);
	}
	if (DenyFeedbackElapsedSeconds < SlotFeedbackConfig.DenyDuration)
	{
		DenyFeedbackElapsedSeconds += FMath::Max(0.0f, InDeltaTime);
	}

	bool bNearTarget = true;
	if (!bIsExitingForFirstPersonLayer || SlotMotionConfig.bEnabled)
	{
		const float MotionAlpha = ComputeInterpAlpha(SlotMotionConfig.MotionSpeed, InDeltaTime);
		const float OpacityAlpha = ComputeInterpAlpha(SlotMotionConfig.OpacitySpeed, InDeltaTime);
		VisualSlotView = LerpSlotView(VisualSlotView, TargetSlotView, MotionAlpha, OpacityAlpha);
		ApplyVisualSlotView();

		bNearTarget =
			FVector2D::Distance(VisualSlotView.ScreenPosition, TargetSlotView.ScreenPosition) <= 0.1f
			&& FMath::Abs(VisualSlotView.RenderAngleDegrees - TargetSlotView.RenderAngleDegrees) <= 0.05f
			&& FMath::Abs(VisualSlotView.RenderScale - TargetSlotView.RenderScale) <= 0.001f
			&& FMath::Abs(VisualSlotView.RenderOpacity - TargetSlotView.RenderOpacity) <= 0.01f;
		if (bNearTarget)
		{
			VisualSlotView = TargetSlotView;
			ApplyVisualSlotView();
		}
	}
	else
	{
		ApplyVisualSlotView();
	}

	if (IsExitMotionFinished())
	{
		VisualSlotView.bProjected = false;
		ApplyVisualSlotView();
		UpdateWantsTick();
		return;
	}

	if (bNearTarget && !bIsExitingForFirstPersonLayer)
	{
		UpdateWantsTick();
	}
	else
	{
		UpdateWantsTick();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseEnter(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (CanInteractWithCurrentSlot())
	{
		SetHoveredForFirstPersonLayer(true);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	SetHoveredForFirstPersonLayer(false);
	SetPressedForFirstPersonLayer(false);
	Super::NativeOnMouseLeave(InMouseEvent);
}

FReply UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CanInteractWithCurrentSlot())
	{
		SetPressedForFirstPersonLayer(true);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CanInteractWithCurrentSlot())
	{
		SetPressedForFirstPersonLayer(false);
		if (CanClickCurrentSlot())
		{
			TriggerConfirmFeedback();
			OnCardClickedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, CurrentSlotView);
		}
		else
		{
			TriggerDenyFeedback();
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UWacomFirstPersonCardLayerSlotWidget::EnsureCardView()
{
	if (CardView)
	{
		return;
	}

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!WidgetTree)
	{
		return;
	}

	if (!RootOverlay)
	{
		if (WidgetTree->RootWidget)
		{
			RootOverlay = Cast<UOverlay>(WidgetTree->RootWidget);
		}
		else
		{
			RootOverlay = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				TEXT("FirstPersonCardLayerSlotRoot"));
			WidgetTree->RootWidget = RootOverlay;
		}
	}
	if (!RootOverlay)
	{
		return;
	}

	UClass* ClassToUse = CardViewClass ? CardViewClass.Get() : UWacomCardView::StaticClass();
	CardView = WidgetTree->ConstructWidget<UWacomCardView>(ClassToUse, TEXT("CardView"));
	if (!CardView)
	{
		return;
	}

	CardView->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* CardSlot = RootOverlay->AddChildToOverlay(CardView))
	{
		CardSlot->SetHorizontalAlignment(HAlign_Fill);
		CardSlot->SetVerticalAlignment(VAlign_Fill);
	}
	if (FeedbackOverlay)
	{
		FeedbackOverlay->RemoveFromParent();
		if (UOverlaySlot* OverlaySlot = RootOverlay->AddChildToOverlay(FeedbackOverlay))
		{
			OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(VAlign_Fill);
		}
	}
	else
	{
		EnsureFeedbackOverlay();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::EnsureFeedbackOverlay()
{
	if (FeedbackOverlay)
	{
		if (FeedbackOverlay->GetParent() != RootOverlay && RootOverlay)
		{
			FeedbackOverlay->RemoveFromParent();
			if (UOverlaySlot* OverlaySlot = RootOverlay->AddChildToOverlay(FeedbackOverlay))
			{
				OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
				OverlaySlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
		return;
	}

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!RootOverlay && WidgetTree && WidgetTree->RootWidget)
	{
		RootOverlay = Cast<UOverlay>(WidgetTree->RootWidget);
	}
	if (!WidgetTree || !RootOverlay)
	{
		return;
	}

	FeedbackOverlay = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("InteractionFeedbackOverlay"));
	if (!FeedbackOverlay)
	{
		return;
	}

	FeedbackOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
	FeedbackOverlay->SetRenderOpacity(0.0f);
	FSlateBrush FeedbackBrush;
	FeedbackBrush.DrawAs = ESlateBrushDrawType::Box;
	FeedbackOverlay->SetBrush(FeedbackBrush);
	if (UOverlaySlot* OverlaySlot = RootOverlay->AddChildToOverlay(FeedbackOverlay))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyCurrentSlotView()
{
	EnsureCardView();
	if (CardView)
	{
		CardView->SetCardViewData(CurrentSlotView.Entry.CardViewData);
	}
	EnsureFeedbackOverlay();
	UpdateVisibilityForInteractionMode();
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyVisualSlotView()
{
	if (!bHasVisualSlotView)
	{
		return;
	}
	ApplySlotViewToWidget(VisualSlotView);
}

void UWacomFirstPersonCardLayerSlotWidget::ApplySlotViewToWidget(
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(SlotView.ScreenPosition);
		CanvasSlot->SetZOrder(CurrentSlotView.ZOrder);
	}
	const bool bDenyActive = SlotFeedbackConfig.bEnabled
		&& DenyFeedbackElapsedSeconds < SlotFeedbackConfig.DenyDuration;
	float DenyShakeOffset = 0.0f;
	if (bDenyActive && SlotFeedbackConfig.DenyDuration > 0.0f)
	{
		const float Progress = FMath::Clamp(DenyFeedbackElapsedSeconds / SlotFeedbackConfig.DenyDuration, 0.0f, 1.0f);
		const float ShakeAlpha = 1.0f - Progress;
		DenyShakeOffset = FMath::Sin(Progress * PI * 6.0f) * SlotFeedbackConfig.DenyShakePixels * ShakeAlpha;
	}

	SetRenderOpacity(FMath::Clamp(SlotView.RenderOpacity, 0.0f, 1.0f));
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	FWidgetTransform CardRenderTransform;
	const float PressedScale = (SlotFeedbackConfig.bEnabled && bIsPressedForFirstPersonLayer)
		? SlotFeedbackConfig.PressedScale
		: 1.0f;
	CardRenderTransform.Translation = FVector2D(DenyShakeOffset, 0.0f);
	CardRenderTransform.Scale = FVector2D(FMath::Max(0.01f, SlotView.RenderScale * PressedScale));
	CardRenderTransform.Angle = SlotView.RenderAngleDegrees;
	SetRenderTransform(CardRenderTransform);
	ApplyFeedbackOverlay();
	UpdateVisibilityForInteractionMode();
}

bool UWacomFirstPersonCardLayerSlotWidget::CanInteractWithCurrentSlot() const
{
	return bCardLayerInteractionEnabled
		&& CurrentSlotView.bProjected
		&& CurrentSlotView.Entry.CardInstanceId.IsValid();
}

bool UWacomFirstPersonCardLayerSlotWidget::CanApplyPlayableHoverFeedback() const
{
	return CanClickCurrentSlot()
		&& !CurrentSlotView.Entry.bIsPendingTargeting
		&& bIsHoveredForFirstPersonLayer;
}

bool UWacomFirstPersonCardLayerSlotWidget::CanClickCurrentSlot() const
{
	return CanInteractWithCurrentSlot() && CurrentSlotView.Entry.bIsPlayable;
}

#if WITH_AUTOMATION_TESTS
bool UWacomFirstPersonCardLayerSlotWidget::RequestHoverForTest()
{
	if (!CanInteractWithCurrentSlot())
	{
		return false;
	}

	SetHoveredForFirstPersonLayer(true);
	return true;
}

void UWacomFirstPersonCardLayerSlotWidget::RequestUnhoverForTest()
{
	SetHoveredForFirstPersonLayer(false);
	SetPressedForFirstPersonLayer(false);
}

bool UWacomFirstPersonCardLayerSlotWidget::RequestPressForTest()
{
	if (!CanInteractWithCurrentSlot())
	{
		return false;
	}

	SetPressedForFirstPersonLayer(true);
	return true;
}

bool UWacomFirstPersonCardLayerSlotWidget::RequestClickForTest()
{
	if (!CanClickCurrentSlot())
	{
		return false;
	}

	TriggerConfirmFeedback();
	OnCardClickedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, CurrentSlotView);
	return true;
}

bool UWacomFirstPersonCardLayerSlotWidget::RequestMouseUpForTest()
{
	if (!CanInteractWithCurrentSlot())
	{
		return false;
	}

	SetPressedForFirstPersonLayer(false);
	if (CanClickCurrentSlot())
	{
		TriggerConfirmFeedback();
		OnCardClickedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, CurrentSlotView);
	}
	else
	{
		TriggerDenyFeedback();
	}
	return true;
}

void UWacomFirstPersonCardLayerSlotWidget::TickSlotMotionForTest(float DeltaTime)
{
	NativeTick(FGeometry(), DeltaTime);
}

float UWacomFirstPersonCardLayerSlotWidget::GetFeedbackOverlayRenderOpacityForTest() const
{
	return FeedbackOverlay ? FeedbackOverlay->GetRenderOpacity() : 0.0f;
}

FLinearColor UWacomFirstPersonCardLayerSlotWidget::GetFeedbackOverlayColorForTest() const
{
	return FeedbackOverlay ? FeedbackOverlay->GetColorAndOpacity() : FLinearColor::Transparent;
}
#endif

void UWacomFirstPersonCardLayerSlotWidget::SetHoveredForFirstPersonLayer(bool bHovered)
{
	if (bIsHoveredForFirstPersonLayer == bHovered)
	{
		return;
	}

	bIsHoveredForFirstPersonLayer = bHovered;
	if (!bIsHoveredForFirstPersonLayer)
	{
		SetPressedForFirstPersonLayer(false);
	}
	ApplyVisualSlotView();
	if (CurrentSlotView.Entry.CardInstanceId.IsValid())
	{
		if (bIsHoveredForFirstPersonLayer)
		{
			OnCardHoveredNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, CurrentSlotView);
		}
		else
		{
			OnCardUnhoveredNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, CurrentSlotView);
		}
	}
}

void UWacomFirstPersonCardLayerSlotWidget::SetPressedForFirstPersonLayer(bool bPressed)
{
	if (bIsPressedForFirstPersonLayer == bPressed)
	{
		return;
	}

	bIsPressedForFirstPersonLayer = bPressed && SlotFeedbackConfig.bEnabled && CanInteractWithCurrentSlot();
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerConfirmFeedback()
{
	if (!SlotFeedbackConfig.bEnabled || SlotFeedbackConfig.ConfirmDuration <= 0.0f)
	{
		return;
	}

	ConfirmFeedbackElapsedSeconds = 0.0f;
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerDenyFeedback()
{
	if (!SlotFeedbackConfig.bEnabled || SlotFeedbackConfig.DenyDuration <= 0.0f)
	{
		return;
	}

	DenyFeedbackElapsedSeconds = 0.0f;
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::ClearInteractionFeedback()
{
	bIsPressedForFirstPersonLayer = false;
	ConfirmFeedbackElapsedSeconds = SlotFeedbackConfig.ConfirmDuration;
	DenyFeedbackElapsedSeconds = SlotFeedbackConfig.DenyDuration;
	ApplyFeedbackOverlay();
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyFeedbackOverlay()
{
	EnsureFeedbackOverlay();
	if (!FeedbackOverlay)
	{
		return;
	}

	FLinearColor OverlayColor = FLinearColor::Transparent;
	float OverlayOpacity = 0.0f;
	if (SlotFeedbackConfig.bEnabled)
	{
		const float DenyAlpha = ComputePulseAlpha(DenyFeedbackElapsedSeconds, SlotFeedbackConfig.DenyDuration);
		const float ConfirmAlpha = ComputePulseAlpha(ConfirmFeedbackElapsedSeconds, SlotFeedbackConfig.ConfirmDuration);
		if (DenyAlpha > 0.0f)
		{
			OverlayColor = SlotFeedbackConfig.DenyColor;
			OverlayOpacity = SlotFeedbackConfig.DenyOpacity * DenyAlpha;
		}
		else if (bIsPressedForFirstPersonLayer)
		{
			OverlayColor = SlotFeedbackConfig.PressedColor;
			OverlayOpacity = SlotFeedbackConfig.PressedOpacity;
		}
		else if (ConfirmAlpha > 0.0f)
		{
			OverlayColor = SlotFeedbackConfig.PressedColor;
			OverlayOpacity = SlotFeedbackConfig.ConfirmOpacity * ConfirmAlpha;
		}
		else if (CanApplyPlayableHoverFeedback())
		{
			OverlayColor = SlotFeedbackConfig.PlayableHoverColor;
			OverlayOpacity = SlotFeedbackConfig.PlayableHoverOpacity;
		}
	}

	OverlayColor.A = 1.0f;
	FeedbackOverlay->SetColorAndOpacity(OverlayColor);
	FeedbackOverlay->SetRenderOpacity(FMath::Clamp(OverlayOpacity, 0.0f, 1.0f));
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateVisibilityForInteractionMode()
{
	const bool bVisible = bHasVisualSlotView
		? VisualSlotView.bProjected
		: CurrentSlotView.bProjected;
	SetVisibility(bVisible
		? (bCardLayerInteractionEnabled ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible)
		: ESlateVisibility::Collapsed);
	if (CardView)
	{
		CardView->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (FeedbackOverlay)
	{
		FeedbackOverlay->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::SetTickEnabledForMotion(bool bEnabled)
{
	bWantsSlotMotionTick = bEnabled;
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateWantsTick()
{
	const bool bFeedbackActive =
		(SlotFeedbackConfig.bEnabled && bIsPressedForFirstPersonLayer)
		|| ConfirmFeedbackElapsedSeconds < SlotFeedbackConfig.ConfirmDuration
		|| DenyFeedbackElapsedSeconds < SlotFeedbackConfig.DenyDuration;
	bWantsSlotMotionTick = bIsExitingForFirstPersonLayer
		|| (bHasVisualSlotView
			&& (FVector2D::Distance(VisualSlotView.ScreenPosition, TargetSlotView.ScreenPosition) > 0.1f
				|| FMath::Abs(VisualSlotView.RenderAngleDegrees - TargetSlotView.RenderAngleDegrees) > 0.05f
				|| FMath::Abs(VisualSlotView.RenderScale - TargetSlotView.RenderScale) > 0.001f
				|| FMath::Abs(VisualSlotView.RenderOpacity - TargetSlotView.RenderOpacity) > 0.01f))
		|| bFeedbackActive;
}

FWacomFirstPersonCardLayerSlotView UWacomFirstPersonCardLayerSlotWidget::LerpSlotView(
	const FWacomFirstPersonCardLayerSlotView& From,
	const FWacomFirstPersonCardLayerSlotView& To,
	float MotionAlpha,
	float OpacityAlpha)
{
	FWacomFirstPersonCardLayerSlotView Result = To;
	Result.ScreenPosition = FMath::Lerp(From.ScreenPosition, To.ScreenPosition, MotionAlpha);
	Result.WidgetPosition = Result.ScreenPosition;
	Result.SnappedWidgetPosition = Result.ScreenPosition;
	Result.RenderAngleDegrees = FMath::Lerp(From.RenderAngleDegrees, To.RenderAngleDegrees, MotionAlpha);
	Result.RenderScale = FMath::Lerp(From.RenderScale, To.RenderScale, MotionAlpha);
	Result.RenderOpacity = FMath::Lerp(From.RenderOpacity, To.RenderOpacity, OpacityAlpha);
	Result.bProjected = From.bProjected || To.bProjected;
	return Result;
}
