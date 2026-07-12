// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardViewWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/RetainerBox.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Styling/SlateBrush.h"
#include "UI/Card/WacomCardView.h"

namespace
{
	const FName FeedbackColorParameterName(TEXT("FeedbackColor"));
	const FName EdgeWidthParameterName(TEXT("EdgeWidth"));
	const FName EdgeSoftnessParameterName(TEXT("EdgeSoftness"));
	const FName VignetteStrengthParameterName(TEXT("VignetteStrength"));
	const FName VignetteRadiusParameterName(TEXT("VignetteRadius"));
	const FName VignetteSoftnessParameterName(TEXT("VignetteSoftness"));
	const FName OpacityParameterName(TEXT("Opacity"));
	const FName PulseParameterName(TEXT("Pulse"));
	const FName Fake3DTiltXParameterName(TEXT("TiltX"));
	const FName Fake3DTiltYParameterName(TEXT("TiltY"));
	const FName Fake3DPerspectiveStrengthParameterName(TEXT("PerspectiveStrength"));
	const FName ContactShadowEnabledParameterName(TEXT("ContactShadowEnabled"));
	const FName ContactShadowLiftParameterName(TEXT("ContactShadowLift"));
	const FName PlayedDissolveEnabledParameterName(TEXT("PlayedDissolveEnabled"));
	const FName PlayedDissolveAmountParameterName(TEXT("PlayedDissolveAmount"));
	const FName PlayedDissolveTimeParameterName(TEXT("PlayedDissolveTime"));
	const FName PlayedDissolveDurationParameterName(TEXT("PlayedDissolveDuration"));
	const FName PlayedDissolveSeedParameterName(TEXT("PlayedDissolveSeed"));
	const FName PlayedDissolveReducedMotionParameterName(TEXT("PlayedDissolveReducedMotion"));
	const FName PlayedDissolveGridColumnsParameterName(TEXT("PlayedDissolveGridColumns"));
	const FName PlayedDissolveDirectionAngleParameterName(TEXT("PlayedDissolveDirectionAngle"));
	const FName PlayedDissolveJitterParameterName(TEXT("PlayedDissolveJitter"));
	const FName PlayedDissolveEdgeColorParameterName(TEXT("PlayedDissolveEdgeColor"));
	const FName PlayedDissolveEdgeAccentColorParameterName(TEXT("PlayedDissolveEdgeAccentColor"));
	const FName PlayedDissolveEdgeWidthParameterName(TEXT("PlayedDissolveEdgeWidth"));
	const FName PlayedDissolveEdgeIntensityParameterName(TEXT("PlayedDissolveEdgeIntensity"));
	const FName PlayedDissolveAshDensityParameterName(TEXT("PlayedDissolveAshDensity"));
	const FName PlayedDissolveAshTrailWidthParameterName(TEXT("PlayedDissolveAshTrailWidth"));
	const FName PlayedDissolveAshLiftPixelsParameterName(TEXT("PlayedDissolveAshLiftPixels"));
	const FName PlayedDissolveAshDriftPixelsParameterName(TEXT("PlayedDissolveAshDriftPixels"));
	const FName PlayedDissolveShadowFadeFractionParameterName(TEXT("PlayedDissolveShadowFadeFraction"));
	const FName PlayedDissolveNoiseTextureParameterName(TEXT("PlayedDissolveNoiseTexture"));
	const FName SurfaceInvSizeParameterName(TEXT("SurfaceInvSize"));
}

void UWacomFirstPersonCardViewWidget::SetCardViewData(const FWacomCardViewData& InData)
{
	PendingCardViewData = InData;
	ApplyPendingCardViewData();
}

FVector2D UWacomFirstPersonCardViewWidget::GetCardBodyHitSize() const
{
	return CardView ? CardView->GetCardBodyHitSize() : UWacomCardView::GetDefaultCardBodyHitSize();
}

bool UWacomFirstPersonCardViewWidget::HasCardBodyHitGeometry() const
{
	return CardView && CardView->HasCardBodyHitGeometry();
}

bool UWacomFirstPersonCardViewWidget::IsScreenPositionInsideCardBody(const FVector2D& ScreenPosition) const
{
	return CardView && CardView->IsScreenPositionInsideCardBody(ScreenPosition);
}

FVector2D UWacomFirstPersonCardViewWidget::GetDefaultCardBodyHitSize()
{
	return UWacomCardView::GetDefaultCardBodyHitSize();
}

void UWacomFirstPersonCardViewWidget::SetFeedbackOverlayView(const FLinearColor& Color, float Opacity)
{
	EnsureFallbackWidgetTree();
	LastFeedbackOverlayColor = Color;
	LastFeedbackOverlayOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
	if (!FeedbackOverlay)
	{
		return;
	}

	FLinearColor OverlayColor = Color;
	OverlayColor.A = 1.0f;
	FeedbackOverlay->SetColorAndOpacity(OverlayColor);
	FeedbackOverlay->SetRenderOpacity(LastFeedbackOverlayOpacity);
	FeedbackOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UWacomFirstPersonCardViewWidget::SetInteractionFeedbackView(
	const FWacomFirstPersonCardInteractionFeedbackView& View)
{
	EnsureFallbackWidgetTree();
	const float DesiredOpacity = FMath::Clamp(View.Opacity * View.Pulse, 0.0f, 1.0f);
	LastInteractionFeedbackView = View;
	LastInteractionFeedbackOpacity = 0.0f;
	bLastInteractionFeedbackUsedOverrideMaterial = false;
	bLastInteractionFeedbackUsedBrushMaterial = false;
	UImage* FeedbackImage = GetInteractionFeedbackImage();
	if (!FeedbackImage)
	{
		return;
	}
	CacheInteractionFeedbackBrushMaterial();

	EnsureInteractionFeedbackMaterialInstance(View);
	if (InteractionFeedbackMaterialInstance)
	{
		FLinearColor EdgeColor = View.Color;
		EdgeColor.A = 1.0f;
		FeedbackImage->SetColorAndOpacity(FLinearColor::White);
		InteractionFeedbackMaterialInstance->SetVectorParameterValue(FeedbackColorParameterName, EdgeColor);
		InteractionFeedbackMaterialInstance->SetScalarParameterValue(EdgeWidthParameterName, FMath::Max(0.0f, View.EdgeWidth));
		InteractionFeedbackMaterialInstance->SetScalarParameterValue(EdgeSoftnessParameterName, FMath::Max(0.0f, View.EdgeSoftness));
		InteractionFeedbackMaterialInstance->SetScalarParameterValue(
			VignetteStrengthParameterName,
			FMath::Max(0.0f, View.VignetteStrength));
		InteractionFeedbackMaterialInstance->SetScalarParameterValue(
			VignetteRadiusParameterName,
			FMath::Max(0.0f, View.VignetteRadius));
		InteractionFeedbackMaterialInstance->SetScalarParameterValue(
			VignetteSoftnessParameterName,
			FMath::Max(0.0f, View.VignetteSoftness));
		InteractionFeedbackMaterialInstance->SetScalarParameterValue(OpacityParameterName, FMath::Clamp(View.Opacity, 0.0f, 1.0f));
		InteractionFeedbackMaterialInstance->SetScalarParameterValue(PulseParameterName, FMath::Clamp(View.Pulse, 0.0f, 1.0f));
	}
	else
	{
		FLinearColor TintColor = View.Color;
		TintColor.A = 1.0f;
		FeedbackImage->SetColorAndOpacity(TintColor);
	}

	const bool bCanRender =
		DesiredOpacity > 0.0f
		&& (InteractionFeedbackMaterialInstance != nullptr
			|| View.Kind != EWacomFirstPersonCardInteractionFeedbackKind::Deny);
	LastInteractionFeedbackOpacity = bCanRender ? DesiredOpacity : 0.0f;
	FeedbackImage->SetRenderOpacity(bCanRender && InteractionFeedbackMaterialInstance ? 1.0f : LastInteractionFeedbackOpacity);
	FeedbackImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UWacomFirstPersonCardViewWidget::ClearInteractionFeedbackView()
{
	EnsureFallbackWidgetTree();
	LastInteractionFeedbackView.Kind = EWacomFirstPersonCardInteractionFeedbackKind::None;
	LastInteractionFeedbackView.Opacity = 0.0f;
	LastInteractionFeedbackView.Pulse = 0.0f;
	LastInteractionFeedbackOpacity = 0.0f;
	bLastInteractionFeedbackUsedOverrideMaterial = false;
	bLastInteractionFeedbackUsedBrushMaterial = false;
	if (InteractionFeedbackMaterialInstance)
	{
		InteractionFeedbackMaterialInstance->SetScalarParameterValue(OpacityParameterName, 0.0f);
		InteractionFeedbackMaterialInstance->SetScalarParameterValue(PulseParameterName, 0.0f);
	}
	if (UImage* FeedbackImage = GetInteractionFeedbackImage())
	{
		FeedbackImage->SetRenderOpacity(0.0f);
		FeedbackImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UWacomFirstPersonCardViewWidget::SetCardDepthView(const FWacomFirstPersonCardDepthView& View)
{
	EnsureFallbackWidgetTree();
	LastCardDepthView = View;
	LastCardDepthView.PerspectiveStrength = FMath::Max(0.0f, LastCardDepthView.PerspectiveStrength);
	LastCardDepthView.ContactShadowLift = FMath::Clamp(LastCardDepthView.ContactShadowLift, 0.0f, 1.0f);

	if (Fake3DSurfaceRetainer)
	{
		UMaterialInstanceDynamic* EffectMaterial = Fake3DSurfaceRetainer->GetEffectMaterial();
		if (!EffectMaterial)
		{
			// The authored material is synchronized into the UMG property before the
			// nested Retainer's Slate widget necessarily creates its runtime MID.
			// Reapplying the current source is safe and lets a late Slate rebuild
			// create the MID before we submit the first interactive depth frame.
			UMaterialInterface* EffectMaterialSource = const_cast<UMaterialInterface*>(
				Fake3DSurfaceRetainer->GetEffectMaterialInterface());
			if (EffectMaterialSource)
			{
				Fake3DSurfaceRetainer->SetEffectMaterial(EffectMaterialSource);
				EffectMaterial = Fake3DSurfaceRetainer->GetEffectMaterial();
			}
		}
		if (EffectMaterial)
		{
			ApplyCardDepthParameters(*EffectMaterial);
		}
		Fake3DSurfaceRetainer->RequestRender();
	}
}

void UWacomFirstPersonCardViewWidget::SetCardSurfaceEffectView(
	const FWacomFirstPersonCardSurfaceEffectView& View)
{
	EnsureFallbackWidgetTree();
	LastSurfaceEffectView = View;
	if (!Fake3DSurfaceRetainer)
	{
		return;
	}

	CacheBaseSurfaceEffectMaterial();
	const FWacomFirstPersonCardPlayedDissolveView& DissolveView = View.PlayedDissolve;
	if (DissolveView.bActive
		&& DissolveView.Style.SurfaceEffectMaterial
		&& DissolveView.Style.NoiseTexture)
	{
		EnsureSurfaceEffectMaterialInstance(DissolveView.Style.SurfaceEffectMaterial);
		if (ActiveSurfaceEffectMaterialInstance)
		{
			ApplyCardDepthParameters(*ActiveSurfaceEffectMaterialInstance);
			ApplyPlayedDissolveParameters(*ActiveSurfaceEffectMaterialInstance, DissolveView);
		}
	}
	else
	{
		RestoreBaseSurfaceEffectMaterial();
	}
	Fake3DSurfaceRetainer->RequestRender();
}

#if WITH_AUTOMATION_TESTS
FWacomFirstPersonCardViewAutomationTestView
UWacomFirstPersonCardViewWidget::GetAutomationTestViewForTest() const
{
	FWacomFirstPersonCardViewAutomationTestView View;
	View.FeedbackOverlayOpacity = FeedbackOverlay ? FeedbackOverlay->GetRenderOpacity() : LastFeedbackOverlayOpacity;
	View.FeedbackOverlayColor = FeedbackOverlay ? FeedbackOverlay->GetColorAndOpacity() : LastFeedbackOverlayColor;
	View.InteractionFeedbackOpacity = LastInteractionFeedbackOpacity;
	View.InteractionFeedbackKind = LastInteractionFeedbackView.Kind;
	const UImage* FeedbackImage = GetInteractionFeedbackImage();
	View.bHasInteractionFeedbackImage = FeedbackImage != nullptr;
	View.bInteractionFeedbackMaterialConfigured = InteractionFeedbackMaterial != nullptr;
	View.bInteractionFeedbackMaterialLoaded = InteractionFeedbackMaterialInstance != nullptr;
	View.bInteractionFeedbackUsesOverrideMaterial = bLastInteractionFeedbackUsedOverrideMaterial;
	View.bInteractionFeedbackUsesBrushMaterial = bLastInteractionFeedbackUsedBrushMaterial;
	View.CardDepthView = LastCardDepthView;
	View.SurfaceEffectView = LastSurfaceEffectView;
	View.bHasFake3DSurfaceRetainer = Fake3DSurfaceRetainer != nullptr;
	View.bFake3DEffectMaterialReady =
		Fake3DSurfaceRetainer && Fake3DSurfaceRetainer->GetEffectMaterial() != nullptr;
	View.bUsingSurfaceEffectMaterial =
		Fake3DSurfaceRetainer
		&& ActiveSurfaceEffectMaterialInstance
		&& Fake3DSurfaceRetainer->GetEffectMaterial() == ActiveSurfaceEffectMaterialInstance;
	View.bBaseSurfaceEffectMaterialCached = bBaseSurfaceEffectMaterialCached;
	const UWidget* RetainerCaptureRoot = Fake3DSurfaceRetainer
		? Fake3DSurfaceRetainer->GetContent()
		: nullptr;
	View.bRetainerCaptureRootUsesIndependentClipping =
		RetainerCaptureRoot
		&& RetainerCaptureRoot->GetClipping() == EWidgetClipping::ClipToBoundsWithoutIntersecting;
	const UPanelWidget* FeedbackParent = FeedbackOverlay ? FeedbackOverlay->GetParent() : nullptr;
	View.bInteractionFeedbackLayerAboveFeedbackOverlay =
		FeedbackParent
		&& FeedbackOverlay
		&& FeedbackImage
		&& FeedbackImage->GetParent() == FeedbackParent
		&& FeedbackParent->GetChildIndex(FeedbackImage) > FeedbackParent->GetChildIndex(FeedbackOverlay);
	return View;
}
#endif

TSharedRef<SWidget> UWacomFirstPersonCardViewWidget::RebuildWidget()
{
	EnsureFallbackWidgetTree();
	TSharedRef<SWidget> RebuiltWidget = Super::RebuildWidget();
	ConfigureRetainerCaptureRootClipping();
	return RebuiltWidget;
}

void UWacomFirstPersonCardViewWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ConfigureRetainerCaptureRootClipping();
	CacheBaseSurfaceEffectMaterial();
	ApplyPendingCardViewData();
	SetFeedbackOverlayView(LastFeedbackOverlayColor, LastFeedbackOverlayOpacity);
	ClearInteractionFeedbackView();
	SetCardDepthView(LastCardDepthView);
	SetCardSurfaceEffectView(LastSurfaceEffectView);
}

void UWacomFirstPersonCardViewWidget::NativeDestruct()
{
	RestoreBaseSurfaceEffectMaterial();
	ActiveSurfaceEffectMaterialInstance = nullptr;
	ActiveSurfaceEffectMaterialSource = nullptr;
	BaseSurfaceEffectMaterialInstance = nullptr;
	BaseSurfaceEffectMaterialSource = nullptr;
	bBaseSurfaceEffectMaterialCached = false;
	Super::NativeDestruct();
}

void UWacomFirstPersonCardViewWidget::CacheBaseSurfaceEffectMaterial()
{
	if (!Fake3DSurfaceRetainer)
	{
		return;
	}

	if (!BaseSurfaceEffectMaterialSource)
	{
		BaseSurfaceEffectMaterialSource = const_cast<UMaterialInterface*>(
			Fake3DSurfaceRetainer->GetEffectMaterialInterface());
	}

	if (!BaseSurfaceEffectMaterialInstance && BaseSurfaceEffectMaterialSource)
	{
		UMaterialInstanceDynamic* CurrentMaterialInstance = Fake3DSurfaceRetainer->GetEffectMaterial();
		const UMaterialInterface* CurrentMaterialSource =
			Fake3DSurfaceRetainer->GetEffectMaterialInterface();
		if (!CurrentMaterialInstance || CurrentMaterialSource != BaseSurfaceEffectMaterialSource)
		{
			Fake3DSurfaceRetainer->SetEffectMaterial(BaseSurfaceEffectMaterialSource);
			CurrentMaterialInstance = Fake3DSurfaceRetainer->GetEffectMaterial();
		}
		BaseSurfaceEffectMaterialInstance = CurrentMaterialInstance;
	}

	bBaseSurfaceEffectMaterialCached =
		BaseSurfaceEffectMaterialSource != nullptr
		|| BaseSurfaceEffectMaterialInstance != nullptr;
}

void UWacomFirstPersonCardViewWidget::RestoreBaseSurfaceEffectMaterial()
{
	if (!Fake3DSurfaceRetainer)
	{
		return;
	}

	CacheBaseSurfaceEffectMaterial();
	if (!bBaseSurfaceEffectMaterialCached)
	{
		return;
	}

	UMaterialInterface* RestoreMaterial = BaseSurfaceEffectMaterialSource
		? BaseSurfaceEffectMaterialSource.Get()
		: BaseSurfaceEffectMaterialInstance.Get();
	if (!RestoreMaterial)
	{
		return;
	}

	UMaterialInstanceDynamic* CurrentMaterialInstance = Fake3DSurfaceRetainer->GetEffectMaterial();
	const UMaterialInterface* CurrentMaterialSource =
		Fake3DSurfaceRetainer->GetEffectMaterialInterface();
	if (!CurrentMaterialInstance || CurrentMaterialSource != RestoreMaterial)
	{
		Fake3DSurfaceRetainer->SetEffectMaterial(RestoreMaterial);
		CurrentMaterialInstance = Fake3DSurfaceRetainer->GetEffectMaterial();
	}
	if (CurrentMaterialInstance)
	{
		BaseSurfaceEffectMaterialInstance = CurrentMaterialInstance;
		ApplyCardDepthParameters(*CurrentMaterialInstance);
	}
}

void UWacomFirstPersonCardViewWidget::EnsureSurfaceEffectMaterialInstance(
	UMaterialInterface* Material)
{
	if (!Material)
	{
		ActiveSurfaceEffectMaterialInstance = nullptr;
		ActiveSurfaceEffectMaterialSource = nullptr;
		return;
	}
	if (ActiveSurfaceEffectMaterialInstance
		&& ActiveSurfaceEffectMaterialSource == Material
		&& Fake3DSurfaceRetainer
		&& Fake3DSurfaceRetainer->GetEffectMaterial() == ActiveSurfaceEffectMaterialInstance)
	{
		return;
	}
	if (!Fake3DSurfaceRetainer)
	{
		ActiveSurfaceEffectMaterialInstance = nullptr;
		ActiveSurfaceEffectMaterialSource = nullptr;
		return;
	}

	ActiveSurfaceEffectMaterialSource = Material;
	Fake3DSurfaceRetainer->SetEffectMaterial(Material);
	ActiveSurfaceEffectMaterialInstance = Fake3DSurfaceRetainer->GetEffectMaterial();
}

void UWacomFirstPersonCardViewWidget::ApplyCardDepthParameters(
	UMaterialInstanceDynamic& Material) const
{
	const FVector2D AppliedTilt = LastCardDepthView.bFake3DEnabled
		? LastCardDepthView.TiltDegrees
		: FVector2D::ZeroVector;
	Material.SetScalarParameterValue(Fake3DTiltXParameterName, AppliedTilt.X);
	Material.SetScalarParameterValue(Fake3DTiltYParameterName, AppliedTilt.Y);
	Material.SetScalarParameterValue(
		Fake3DPerspectiveStrengthParameterName,
		LastCardDepthView.bFake3DEnabled ? LastCardDepthView.PerspectiveStrength : 0.0f);
	Material.SetScalarParameterValue(
		ContactShadowEnabledParameterName,
		LastCardDepthView.bContactShadowEnabled ? 1.0f : 0.0f);
	Material.SetScalarParameterValue(
		ContactShadowLiftParameterName,
		LastCardDepthView.ContactShadowLift);
}

void UWacomFirstPersonCardViewWidget::ApplyPlayedDissolveParameters(
	UMaterialInstanceDynamic& Material,
	const FWacomFirstPersonCardPlayedDissolveView& View) const
{
	const FWacomFirstPersonCardPlayedDissolveStyleData& Style = View.Style;
	Material.SetScalarParameterValue(PlayedDissolveEnabledParameterName, View.bActive ? 1.0f : 0.0f);
	Material.SetScalarParameterValue(PlayedDissolveAmountParameterName, FMath::Clamp(View.Amount, 0.0f, 1.0f));
	Material.SetScalarParameterValue(PlayedDissolveTimeParameterName, FMath::Max(0.0f, View.TimeSeconds));
	Material.SetScalarParameterValue(
		PlayedDissolveDurationParameterName,
		View.bReducedMotion ? 0.12f : FMath::Max(KINDA_SMALL_NUMBER, Style.DurationSeconds));
	Material.SetScalarParameterValue(PlayedDissolveSeedParameterName, FMath::Frac(FMath::Abs(View.Seed)));
	Material.SetScalarParameterValue(PlayedDissolveReducedMotionParameterName, View.bReducedMotion ? 1.0f : 0.0f);
	Material.SetScalarParameterValue(PlayedDissolveGridColumnsParameterName, FMath::Max(1.0f, Style.GridColumns));
	Material.SetScalarParameterValue(PlayedDissolveDirectionAngleParameterName, Style.DirectionAngleDegrees);
	Material.SetScalarParameterValue(PlayedDissolveJitterParameterName, FMath::Max(0.0f, Style.Jitter));
	Material.SetVectorParameterValue(PlayedDissolveEdgeColorParameterName, Style.EdgeColor);
	Material.SetVectorParameterValue(PlayedDissolveEdgeAccentColorParameterName, Style.EdgeAccentColor);
	Material.SetScalarParameterValue(PlayedDissolveEdgeWidthParameterName, FMath::Max(0.001f, Style.EdgeWidth));
	Material.SetScalarParameterValue(PlayedDissolveEdgeIntensityParameterName, FMath::Max(0.0f, Style.EdgeIntensity));
	Material.SetScalarParameterValue(PlayedDissolveAshDensityParameterName, FMath::Clamp(Style.AshDensity, 0.0f, 1.0f));
	Material.SetScalarParameterValue(PlayedDissolveAshTrailWidthParameterName, FMath::Max(0.001f, Style.AshTrailWidth));
	Material.SetScalarParameterValue(PlayedDissolveAshLiftPixelsParameterName, FMath::Max(0.0f, Style.AshLiftPixels));
	Material.SetScalarParameterValue(PlayedDissolveAshDriftPixelsParameterName, FMath::Max(0.0f, Style.AshDriftPixels));
	Material.SetScalarParameterValue(
		PlayedDissolveShadowFadeFractionParameterName,
		FMath::Clamp(Style.ShadowFadeFraction, KINDA_SMALL_NUMBER, 1.0f));
	Material.SetTextureParameterValue(PlayedDissolveNoiseTextureParameterName, Style.NoiseTexture);

	FVector2D SurfaceSize = Fake3DSurfaceRetainer
		? Fake3DSurfaceRetainer->GetCachedGeometry().GetLocalSize()
		: FVector2D::ZeroVector;
	if (SurfaceSize.X <= 1.0f || SurfaceSize.Y <= 1.0f)
	{
		SurfaceSize = FVector2D(360.0f, 484.0f);
	}
	Material.SetVectorParameterValue(
		SurfaceInvSizeParameterName,
		FLinearColor(1.0f / SurfaceSize.X, 1.0f / SurfaceSize.Y, 0.0f, 0.0f));
}

void UWacomFirstPersonCardViewWidget::EnsureFallbackWidgetTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(
		UOverlay::StaticClass(),
		TEXT("FirstPersonCardViewRoot"));
	WidgetTree->RootWidget = RootOverlay;

	Fake3DSurfaceRetainer = WidgetTree->ConstructWidget<URetainerBox>(
		URetainerBox::StaticClass(),
		TEXT("Fake3DSurfaceRetainer"));
	UOverlay* SurfaceOverlay = WidgetTree->ConstructWidget<UOverlay>(
		UOverlay::StaticClass(),
		TEXT("Fake3DSurfaceOverlay"));
	if (Fake3DSurfaceRetainer && SurfaceOverlay)
	{
		Fake3DSurfaceRetainer->SetVisibility(ESlateVisibility::HitTestInvisible);
		SurfaceOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
		SurfaceOverlay->SetClipping(EWidgetClipping::ClipToBoundsWithoutIntersecting);
		Fake3DSurfaceRetainer->SetContent(SurfaceOverlay);
		if (UOverlaySlot* SurfaceSlot = RootOverlay->AddChildToOverlay(Fake3DSurfaceRetainer))
		{
			SurfaceSlot->SetHorizontalAlignment(HAlign_Fill);
			SurfaceSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}
	UOverlay* ContentOverlay = SurfaceOverlay ? SurfaceOverlay : RootOverlay;

	CardView = WidgetTree->ConstructWidget<UWacomCardView>(
		UWacomCardView::StaticClass(),
		TEXT("CardView"));
	if (CardView)
	{
		CardView->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* CardSlot = ContentOverlay->AddChildToOverlay(CardView))
		{
			CardSlot->SetHorizontalAlignment(HAlign_Fill);
			CardSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	FeedbackOverlay = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("FeedbackOverlay"));
	if (FeedbackOverlay)
	{
		FeedbackOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
		FeedbackOverlay->SetRenderOpacity(0.0f);
		FSlateBrush FeedbackBrush;
		FeedbackBrush.DrawAs = ESlateBrushDrawType::Box;
		FeedbackOverlay->SetBrush(FeedbackBrush);
		if (UOverlaySlot* OverlaySlot = ContentOverlay->AddChildToOverlay(FeedbackOverlay))
		{
			OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	InteractionFeedbackImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("InteractionFeedbackImage"));
	if (InteractionFeedbackImage)
	{
		InteractionFeedbackImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		InteractionFeedbackImage->SetRenderOpacity(0.0f);
		FSlateBrush FeedbackBrush;
		FeedbackBrush.DrawAs = ESlateBrushDrawType::Box;
		InteractionFeedbackImage->SetBrush(FeedbackBrush);
		if (UOverlaySlot* EdgeSlot = ContentOverlay->AddChildToOverlay(InteractionFeedbackImage))
		{
			EdgeSlot->SetHorizontalAlignment(HAlign_Fill);
			EdgeSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}
}

void UWacomFirstPersonCardViewWidget::ConfigureRetainerCaptureRootClipping()
{
	if (!Fake3DSurfaceRetainer)
	{
		return;
	}

	if (UWidget* RetainerCaptureRoot = Fake3DSurfaceRetainer->GetContent())
	{
		RetainerCaptureRoot->SetClipping(EWidgetClipping::ClipToBoundsWithoutIntersecting);
	}
}

UImage* UWacomFirstPersonCardViewWidget::GetInteractionFeedbackImage() const
{
	return InteractionFeedbackImage.Get();
}

void UWacomFirstPersonCardViewWidget::CacheInteractionFeedbackBrushMaterial()
{
	if (InteractionFeedbackBrushMaterial)
	{
		return;
	}

	const UImage* FeedbackImage = GetInteractionFeedbackImage();
	if (!FeedbackImage)
	{
		return;
	}

	UMaterialInterface* BrushMaterial = Cast<UMaterialInterface>(FeedbackImage->GetBrush().GetResourceObject());
	if (BrushMaterial && BrushMaterial != InteractionFeedbackMaterialInstance)
	{
		InteractionFeedbackBrushMaterial = BrushMaterial;
	}
}

void UWacomFirstPersonCardViewWidget::EnsureInteractionFeedbackMaterialInstance(
	const FWacomFirstPersonCardInteractionFeedbackView& View)
{
	UImage* FeedbackImage = GetInteractionFeedbackImage();
	if (!FeedbackImage)
	{
		return;
	}

	bool bUsesOverrideMaterial = false;
	bool bUsesBrushMaterial = false;
	UMaterialInterface* Material = ResolveInteractionFeedbackMaterial(
		View,
		bUsesOverrideMaterial,
		bUsesBrushMaterial);
	bLastInteractionFeedbackUsedOverrideMaterial = Material != nullptr && bUsesOverrideMaterial;
	bLastInteractionFeedbackUsedBrushMaterial = Material != nullptr && bUsesBrushMaterial;
	if (InteractionFeedbackMaterial == Material)
	{
		return;
	}

	InteractionFeedbackMaterial = Material;
	InteractionFeedbackMaterialInstance = InteractionFeedbackMaterial
		? UMaterialInstanceDynamic::Create(InteractionFeedbackMaterial, this)
		: nullptr;
	if (InteractionFeedbackMaterialInstance)
	{
		FeedbackImage->SetBrushFromMaterial(InteractionFeedbackMaterialInstance);
	}
	else
	{
		FSlateBrush FeedbackBrush;
		FeedbackBrush.DrawAs = View.Kind == EWacomFirstPersonCardInteractionFeedbackKind::Deny
			? ESlateBrushDrawType::NoDrawType
			: ESlateBrushDrawType::Box;
		FeedbackImage->SetBrush(FeedbackBrush);
	}
}

UMaterialInterface* UWacomFirstPersonCardViewWidget::ResolveInteractionFeedbackMaterial(
	const FWacomFirstPersonCardInteractionFeedbackView& View,
	bool& bOutUsesOverrideMaterial,
	bool& bOutUsesBrushMaterial) const
{
	bOutUsesOverrideMaterial = false;
	bOutUsesBrushMaterial = false;

	if (!View.Material.IsNull())
	{
		UMaterialInterface* OverrideMaterial = View.Material.LoadSynchronous();
		bOutUsesOverrideMaterial = OverrideMaterial != nullptr;
		return OverrideMaterial;
	}

	const UImage* FeedbackImage = GetInteractionFeedbackImage();
	if (!FeedbackImage)
	{
		return nullptr;
	}

	if (InteractionFeedbackBrushMaterial)
	{
		bOutUsesBrushMaterial = true;
		return InteractionFeedbackBrushMaterial;
	}

	UMaterialInterface* BrushMaterial = Cast<UMaterialInterface>(FeedbackImage->GetBrush().GetResourceObject());
	if (BrushMaterial == InteractionFeedbackMaterialInstance)
	{
		return nullptr;
	}

	bOutUsesBrushMaterial = BrushMaterial != nullptr;
	return BrushMaterial;
}

void UWacomFirstPersonCardViewWidget::ApplyPendingCardViewData()
{
	if (CardView)
	{
		CardView->SetCardViewData(PendingCardViewData);
	}
	if (Fake3DSurfaceRetainer)
	{
		Fake3DSurfaceRetainer->RequestRender();
	}
}
