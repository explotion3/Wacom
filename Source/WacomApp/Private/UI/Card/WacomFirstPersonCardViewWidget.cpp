// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardViewWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
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
	const UOverlay* RootOverlay = WidgetTree ? Cast<UOverlay>(WidgetTree->RootWidget) : nullptr;
	View.bInteractionFeedbackLayerAboveFeedbackOverlay =
		RootOverlay
		&& FeedbackOverlay
		&& FeedbackImage
		&& RootOverlay->GetChildIndex(FeedbackImage) > RootOverlay->GetChildIndex(FeedbackOverlay);
	return View;
}
#endif

TSharedRef<SWidget> UWacomFirstPersonCardViewWidget::RebuildWidget()
{
	EnsureFallbackWidgetTree();
	return Super::RebuildWidget();
}

void UWacomFirstPersonCardViewWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyPendingCardViewData();
	SetFeedbackOverlayView(LastFeedbackOverlayColor, LastFeedbackOverlayOpacity);
	ClearInteractionFeedbackView();
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

	CardView = WidgetTree->ConstructWidget<UWacomCardView>(
		UWacomCardView::StaticClass(),
		TEXT("CardView"));
	if (CardView)
	{
		CardView->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* CardSlot = RootOverlay->AddChildToOverlay(CardView))
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
		if (UOverlaySlot* OverlaySlot = RootOverlay->AddChildToOverlay(FeedbackOverlay))
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
		if (UOverlaySlot* EdgeSlot = RootOverlay->AddChildToOverlay(InteractionFeedbackImage))
		{
			EdgeSlot->SetHorizontalAlignment(HAlign_Fill);
			EdgeSlot->SetVerticalAlignment(VAlign_Fill);
		}
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
}
