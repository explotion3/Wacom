// Copyright Wacom. All Rights Reserved.

#include "UI/Run/WacomRunMenuDropTargetWidget.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "GameFramework/WacomPlayerController.h"

UWacomRunMenuDropTargetWidget::UWacomRunMenuDropTargetWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Visible);
}

TSharedRef<SWidget> UWacomRunMenuDropTargetWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}

	if (!WidgetTree->RootWidget)
	{
		RootBorder = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("RunMenuDropTargetRoot"));
		if (RootBorder)
		{
			RootBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
			RootBorder->SetPadding(FMargin(0.0f));
			WidgetTree->RootWidget = RootBorder;
		}
	}
	else if (!RootBorder)
	{
		RootBorder = Cast<UBorder>(WidgetTree->RootWidget);
	}

	if (RootBorder && DropContent && DropContent->GetParent() != RootBorder)
	{
		RootBorder->SetContent(DropContent);
	}

	return Super::RebuildWidget();
}

void UWacomRunMenuDropTargetWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(GetOwningPlayer()))
	{
		WacomPC->RegisterRunMenuDropTarget(this);
	}
}

void UWacomRunMenuDropTargetWidget::NativeDestruct()
{
	if (AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(GetOwningPlayer()))
	{
		WacomPC->UnregisterRunMenuDropTarget(this);
	}

	ClearRunMenuDropPreviewState();
	Super::NativeDestruct();
}

void UWacomRunMenuDropTargetWidget::SetDropContent(UWidget* InContent)
{
	DropContent = InContent;
	if (RootBorder)
	{
		RootBorder->SetContent(DropContent);
	}
}

FWacomInteractionTargetHandle UWacomRunMenuDropTargetWidget::BuildZoneTargetHandle(
	FVector2D ScreenPosition) const
{
	if (ZoneId.IsNone())
	{
		return FWacomInteractionTargetHandle();
	}

	FWacomInteractionTargetHandle Handle =
		FWacomInteractionTargetHandle::ForZoneTarget(ZoneId, const_cast<UWacomRunMenuDropTargetWidget*>(this), ScreenPosition);
	Handle.StableTargetId = ResolveStableTargetId();
	return Handle;
}

bool UWacomRunMenuDropTargetWidget::CanProbeRunMenuDropTarget() const
{
	return bEnableRunMenuDropProbe
		&& !ZoneId.IsNone()
		&& IsVisibleForProbe();
}

bool UWacomRunMenuDropTargetWidget::ContainsWidgetPosition(FVector2D WidgetPosition) const
{
	LastProbeWidgetPosition = WidgetPosition;

	if (!CanProbeRunMenuDropTarget())
	{
		return false;
	}

	const FGeometry Geometry = GetCachedGeometry();
	const FVector2D LocalSize = Geometry.GetLocalSize();
	if (LocalSize.X <= 0.0f || LocalSize.Y <= 0.0f)
	{
		return false;
	}

	const FVector2D AbsoluteTopLeft = Geometry.LocalToAbsolute(FVector2D::ZeroVector);
	const FVector2D AbsoluteBottomRight = Geometry.LocalToAbsolute(LocalSize);
	FVector2D PixelTopLeft = FVector2D::ZeroVector;
	FVector2D PixelBottomRight = FVector2D::ZeroVector;
	FVector2D ViewportTopLeft = FVector2D::ZeroVector;
	FVector2D ViewportBottomRight = FVector2D::ZeroVector;
	USlateBlueprintLibrary::AbsoluteToViewport(
		this,
		AbsoluteTopLeft,
		PixelTopLeft,
		ViewportTopLeft);
	USlateBlueprintLibrary::AbsoluteToViewport(
		this,
		AbsoluteBottomRight,
		PixelBottomRight,
		ViewportBottomRight);
	if (ViewportTopLeft.ContainsNaN() || ViewportBottomRight.ContainsNaN())
	{
		return false;
	}

	const FVector2D Min(
		FMath::Min(ViewportTopLeft.X, ViewportBottomRight.X),
		FMath::Min(ViewportTopLeft.Y, ViewportBottomRight.Y));
	const FVector2D Max(
		FMath::Max(ViewportTopLeft.X, ViewportBottomRight.X),
		FMath::Max(ViewportTopLeft.Y, ViewportBottomRight.Y));
	return WidgetPosition.X >= Min.X
		&& WidgetPosition.Y >= Min.Y
		&& WidgetPosition.X <= Max.X
		&& WidgetPosition.Y <= Max.Y;
}

void UWacomRunMenuDropTargetWidget::SetRunMenuDropPreviewState(
	EWacomRunMenuDropTargetPreviewState NewState)
{
	if (!bHasCapturedOriginalRenderScale)
	{
		OriginalRenderScale = GetRenderTransform().Scale;
		if (OriginalRenderScale.IsNearlyZero())
		{
			OriginalRenderScale = FVector2D(1.0f, 1.0f);
		}
		bHasCapturedOriginalRenderScale = true;
	}

	if (PreviewState == NewState)
	{
		return;
	}

	PreviewState = NewState;
	ApplyFallbackPreview();
	BP_OnRunMenuDropPreviewStateChanged(PreviewState);
	if (bLogRunMenuDropTargetDebug)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[WacomRunMenuDropTarget] Preview %s"),
			*GetRunMenuDropTargetDebugSummary());
	}
}

void UWacomRunMenuDropTargetWidget::ClearRunMenuDropPreviewState()
{
	SetRunMenuDropPreviewState(EWacomRunMenuDropTargetPreviewState::Normal);
}

FWacomRunMenuDropTargetDebugView
UWacomRunMenuDropTargetWidget::GetRunMenuDropTargetDebugView() const
{
	FWacomRunMenuDropTargetDebugView View;
	View.ZoneId = ZoneId;
	View.StableTargetId = ResolveStableTargetId();
	View.bProbeEnabled = bEnableRunMenuDropProbe;
	View.bVisibleAndEnabled = IsVisibleForProbe();
	View.PreviewState = PreviewState;
	View.bPreviewActive = PreviewState != EWacomRunMenuDropTargetPreviewState::Normal;
	View.LastProbeWidgetPosition = LastProbeWidgetPosition;
	View.LastPreviewScale = GetRenderTransform().Scale;
	return View;
}

FString UWacomRunMenuDropTargetWidget::GetRunMenuDropTargetDebugSummary() const
{
	const FWacomRunMenuDropTargetDebugView View = GetRunMenuDropTargetDebugView();
	return FString::Printf(
		TEXT("RunMenuDropTarget{Widget=%s ZoneId=%s StableTargetId=%s ProbeEnabled=%s VisibleEnabled=%s Preview=%d PreviewActive=%s LastPointer=%s Scale=%s}"),
		*GetNameSafe(this),
		*View.ZoneId.ToString(),
		*View.StableTargetId.ToString(),
		View.bProbeEnabled ? TEXT("true") : TEXT("false"),
		View.bVisibleAndEnabled ? TEXT("true") : TEXT("false"),
		static_cast<int32>(View.PreviewState),
		View.bPreviewActive ? TEXT("true") : TEXT("false"),
		*View.LastProbeWidgetPosition.ToString(),
		*View.LastPreviewScale.ToString());
}

void UWacomRunMenuDropTargetWidget::LogRunMenuDropTargetDebugSummary() const
{
	UE_LOG(LogTemp, Display,
		TEXT("[WacomRunMenuDropTarget] %s"),
		*GetRunMenuDropTargetDebugSummary());
}

void UWacomRunMenuDropTargetWidget::ApplyFallbackPreview()
{
	if (!bEnableFallbackPreview)
	{
		return;
	}

	if (!bHasCapturedOriginalRenderScale)
	{
		OriginalRenderScale = GetRenderTransform().Scale;
		if (OriginalRenderScale.IsNearlyZero())
		{
			OriginalRenderScale = FVector2D(1.0f, 1.0f);
		}
		bHasCapturedOriginalRenderScale = true;
	}

	FLinearColor PreviewColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
	float PreviewAlpha = 0.0f;
	float ScaleMultiplier = 1.0f;
	switch (PreviewState)
	{
	case EWacomRunMenuDropTargetPreviewState::Probe:
		PreviewColor = ProbePreviewColor;
		PreviewAlpha = PreviewOpacity;
		ScaleMultiplier = ProbePreviewScale;
		break;
	case EWacomRunMenuDropTargetPreviewState::Invalid:
		PreviewColor = InvalidPreviewColor;
		PreviewAlpha = PreviewOpacity;
		break;
	case EWacomRunMenuDropTargetPreviewState::ReleasedProbe:
	case EWacomRunMenuDropTargetPreviewState::SubmitReady:
	case EWacomRunMenuDropTargetPreviewState::Submitted:
		PreviewColor = ReleasedProbePreviewColor;
		PreviewAlpha = PreviewOpacity;
		ScaleMultiplier = ProbePreviewScale;
		break;
	case EWacomRunMenuDropTargetPreviewState::Normal:
	default:
		break;
	}

	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	FWidgetTransform Transform = GetRenderTransform();
	Transform.Scale = OriginalRenderScale * FMath::Max(0.01f, ScaleMultiplier);
	SetRenderTransform(Transform);

	if (RootBorder)
	{
		PreviewColor.A = FMath::Clamp(PreviewAlpha, 0.0f, 1.0f);
		RootBorder->SetBrushColor(PreviewColor);
	}
}

bool UWacomRunMenuDropTargetWidget::IsVisibleForProbe() const
{
	const ESlateVisibility CurrentVisibility = GetVisibility();
	return GetIsEnabled()
		&& CurrentVisibility != ESlateVisibility::Collapsed
		&& CurrentVisibility != ESlateVisibility::Hidden;
}

FName UWacomRunMenuDropTargetWidget::ResolveStableTargetId() const
{
	return StableTargetId.IsNone() ? ZoneId : StableTargetId;
}
