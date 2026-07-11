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
		if (UMaterialInstanceDynamic* EffectMaterial = Fake3DSurfaceRetainer->GetEffectMaterial())
		{
			const FVector2D AppliedTilt = LastCardDepthView.bFake3DEnabled
				? LastCardDepthView.TiltDegrees
				: FVector2D::ZeroVector;
			EffectMaterial->SetScalarParameterValue(Fake3DTiltXParameterName, AppliedTilt.X);
			EffectMaterial->SetScalarParameterValue(Fake3DTiltYParameterName, AppliedTilt.Y);
			EffectMaterial->SetScalarParameterValue(
				Fake3DPerspectiveStrengthParameterName,
				LastCardDepthView.bFake3DEnabled ? LastCardDepthView.PerspectiveStrength : 0.0f);
			EffectMaterial->SetScalarParameterValue(
				ContactShadowEnabledParameterName,
				LastCardDepthView.bContactShadowEnabled ? 1.0f : 0.0f);
			EffectMaterial->SetScalarParameterValue(
				ContactShadowLiftParameterName,
				LastCardDepthView.ContactShadowLift);
		}
		Fake3DSurfaceRetainer->RequestRender();
	}
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
	View.bHasFake3DSurfaceRetainer = Fake3DSurfaceRetainer != nullptr;
	View.bFake3DEffectMaterialReady =
		Fake3DSurfaceRetainer && Fake3DSurfaceRetainer->GetEffectMaterial() != nullptr;
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
	ApplyPendingCardViewData();
	SetFeedbackOverlayView(LastFeedbackOverlayColor, LastFeedbackOverlayOpacity);
	ClearInteractionFeedbackView();
	SetCardDepthView(LastCardDepthView);
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
