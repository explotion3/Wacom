// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "InputCoreTypes.h"
#include "UI/Card/WacomCardView.h"

namespace
{
	float ComputeInterpAlpha(float Speed, float DeltaTime)
	{
		return Speed <= 0.0f ? 1.0f : FMath::Clamp(DeltaTime * Speed, 0.0f, 1.0f);
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
		bIsExitingForFirstPersonLayer = true;
		ExitMotionElapsedSeconds = SlotMotionConfig.ExitDuration;
		SetVisibility(ESlateVisibility::Collapsed);
		SetTickEnabledForMotion(false);
		return;
	}

	SetHoveredForFirstPersonLayer(false);
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

	const float MotionAlpha = ComputeInterpAlpha(SlotMotionConfig.MotionSpeed, InDeltaTime);
	const float OpacityAlpha = ComputeInterpAlpha(SlotMotionConfig.OpacitySpeed, InDeltaTime);
	VisualSlotView = LerpSlotView(VisualSlotView, TargetSlotView, MotionAlpha, OpacityAlpha);
	ApplyVisualSlotView();

	const bool bNearTarget =
		FVector2D::Distance(VisualSlotView.ScreenPosition, TargetSlotView.ScreenPosition) <= 0.1f
		&& FMath::Abs(VisualSlotView.RenderAngleDegrees - TargetSlotView.RenderAngleDegrees) <= 0.05f
		&& FMath::Abs(VisualSlotView.RenderScale - TargetSlotView.RenderScale) <= 0.001f
		&& FMath::Abs(VisualSlotView.RenderOpacity - TargetSlotView.RenderOpacity) <= 0.01f;
	if (bNearTarget)
	{
		VisualSlotView = TargetSlotView;
		ApplyVisualSlotView();
	}

	if (IsExitMotionFinished())
	{
		VisualSlotView.bProjected = false;
		ApplyVisualSlotView();
		SetTickEnabledForMotion(false);
		return;
	}

	if (bNearTarget && !bIsExitingForFirstPersonLayer)
	{
		SetTickEnabledForMotion(false);
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
	Super::NativeOnMouseLeave(InMouseEvent);
}

FReply UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CanClickCurrentSlot())
	{
		OnCardClickedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, CurrentSlotView);
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
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyCurrentSlotView()
{
	EnsureCardView();
	if (CardView)
	{
		CardView->SetCardViewData(CurrentSlotView.Entry.CardViewData);
	}
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
	SetRenderOpacity(FMath::Clamp(SlotView.RenderOpacity, 0.0f, 1.0f));
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	FWidgetTransform CardRenderTransform;
	CardRenderTransform.Scale = FVector2D(FMath::Max(0.01f, SlotView.RenderScale));
	CardRenderTransform.Angle = SlotView.RenderAngleDegrees;
	SetRenderTransform(CardRenderTransform);
	UpdateVisibilityForInteractionMode();
}

bool UWacomFirstPersonCardLayerSlotWidget::CanInteractWithCurrentSlot() const
{
	return bCardLayerInteractionEnabled
		&& CurrentSlotView.bProjected
		&& CurrentSlotView.Entry.CardInstanceId.IsValid();
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
}

bool UWacomFirstPersonCardLayerSlotWidget::RequestClickForTest()
{
	if (!CanClickCurrentSlot())
	{
		return false;
	}

	OnCardClickedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, CurrentSlotView);
	return true;
}

void UWacomFirstPersonCardLayerSlotWidget::TickSlotMotionForTest(float DeltaTime)
{
	NativeTick(FGeometry(), DeltaTime);
}
#endif

void UWacomFirstPersonCardLayerSlotWidget::SetHoveredForFirstPersonLayer(bool bHovered)
{
	if (bIsHoveredForFirstPersonLayer == bHovered)
	{
		return;
	}

	bIsHoveredForFirstPersonLayer = bHovered;
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
}

void UWacomFirstPersonCardLayerSlotWidget::SetTickEnabledForMotion(bool bEnabled)
{
	bWantsSlotMotionTick = bEnabled;
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
