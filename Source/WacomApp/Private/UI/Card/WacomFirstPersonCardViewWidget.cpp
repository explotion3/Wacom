// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardViewWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/RetainerBox.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomFirstPersonCardInteractionCuePainter.h"

namespace
{
	const FName Fake3DTiltXParameterName(TEXT("TiltX"));
	const FName Fake3DTiltYParameterName(TEXT("TiltY"));
	const FName Fake3DPerspectiveStrengthParameterName(TEXT("PerspectiveStrength"));
	const FName ContactShadowEnabledParameterName(TEXT("ContactShadowEnabled"));
	const FName ContactShadowLiftParameterName(TEXT("ContactShadowLift"));
	const FName ContactShadowOpacityMultiplierParameterName(TEXT("ContactShadowOpacityMultiplier"));
	const FName ContactShadowTiltOffsetXUVParameterName(TEXT("ContactShadowTiltOffsetXUV"));
	const FName ContactShadowTiltOffsetYUVParameterName(TEXT("ContactShadowTiltOffsetYUV"));
	const FName CardUseEnabledParameterName(TEXT("CardUseEnabled"));
	const FName CardUseProgressParameterName(TEXT("CardUseProgress"));
	const FName CardUseFlipProgressParameterName(TEXT("CardUseFlipProgress"));
	const FName CardUseImpactProgressParameterName(TEXT("CardUseImpactProgress"));
	const FName CardUseTimeParameterName(TEXT("CardUseTime"));
	const FName CardUseReducedMotionParameterName(TEXT("CardUseReducedMotion"));
	const FName HandTargetImpactEnabledParameterName(TEXT("HandTargetImpactEnabled"));
	const FName HandTargetImpactPreviewAmountParameterName(TEXT("HandTargetImpactPreviewAmount"));
	const FName HandTargetImpactCommitProgressParameterName(TEXT("HandTargetImpactCommitProgress"));
	const FName HandTargetImpactTimeParameterName(TEXT("HandTargetImpactTime"));
	const FName HandTargetImpactSeedParameterName(TEXT("HandTargetImpactSeed"));
	const FName HandTargetImpactReducedMotionParameterName(TEXT("HandTargetImpactReducedMotion"));
	const FName HandTargetImpactCardBodyRectMinParameterName(TEXT("HandTargetImpactCardBodyRectMin"));
	const FName HandTargetImpactCardBodyRectMaxParameterName(TEXT("HandTargetImpactCardBodyRectMax"));
	const FName DrawRevealEnabledParameterName(TEXT("DrawRevealEnabled"));
	const FName DrawRevealProgressParameterName(TEXT("DrawRevealProgress"));
	const FName DrawRevealReducedMotionParameterName(TEXT("DrawRevealReducedMotion"));
	const FName DrawRevealBackHoldEndParameterName(TEXT("DrawRevealBackHoldEnd"));
	const FName DrawRevealFaceSwitchParameterName(TEXT("DrawRevealFaceSwitch"));
	const FName DrawRevealFaceExpandEndParameterName(TEXT("DrawRevealFaceExpandEnd"));
	const FName DrawRevealReducedCrossFadeStartParameterName(TEXT("DrawRevealReducedCrossFadeStart"));
	const FName DrawRevealReducedCrossFadeEndParameterName(TEXT("DrawRevealReducedCrossFadeEnd"));
	const FName DrawRevealCardBodyRectMinParameterName(TEXT("DrawRevealCardBodyRectMin"));
	const FName DrawRevealCardBodyRectMaxParameterName(TEXT("DrawRevealCardBodyRectMax"));
	const FName GainRevealEnabledParameterName(TEXT("GainRevealEnabled"));
	const FName GainRevealProgressParameterName(TEXT("GainRevealProgress"));
	const FName GainRevealReducedMotionParameterName(TEXT("GainRevealReducedMotion"));
	const FName GainRevealSeedParameterName(TEXT("GainRevealSeed"));
	const FName GainRevealRarityIndexParameterName(TEXT("GainRevealRarityIndex"));
	const FName GainRevealSeedEstablishEndParameterName(TEXT("GainRevealSeedEstablishEnd"));
	const FName GainRevealAssemblyEndParameterName(TEXT("GainRevealAssemblyEnd"));
	const FName GainRevealRarityEdgePeakParameterName(TEXT("GainRevealRarityEdgePeak"));
	const FName GainRevealSettleEndParameterName(TEXT("GainRevealSettleEnd"));
	const FName GainRevealReducedCrossFadeStartParameterName(TEXT("GainRevealReducedCrossFadeStart"));
	const FName GainRevealReducedCrossFadeEndParameterName(TEXT("GainRevealReducedCrossFadeEnd"));
	const FName RetainSealEnabledParameterName(TEXT("RetainSealEnabled"));
	const FName RetainSealPhaseParameterName(TEXT("RetainSealPhase"));
	const FName RetainSealProgressParameterName(TEXT("RetainSealProgress"));
	const FName RetainSealSeedParameterName(TEXT("RetainSealSeed"));
	const FName RetainSealReducedMotionParameterName(TEXT("RetainSealReducedMotion"));
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
	const FName PlayedOrderedDitherBayerSizeParameterName(TEXT("PlayedOrderedDitherBayerSize"));
	const FName PlayedOrderedDitherBandWidthParameterName(TEXT("PlayedOrderedDitherBandWidth"));
	const FName PlayedOrderedDitherResidueDensityParameterName(TEXT("PlayedOrderedDitherResidueDensity"));
	const FName PlayedOrderedDitherResidueTrailWidthParameterName(TEXT("PlayedOrderedDitherResidueTrailWidth"));
	const FName PlayedOrderedDitherResidueTravelPixelsParameterName(TEXT("PlayedOrderedDitherResidueTravelPixels"));
	const FName PlayedOrderedDitherResidueMainDirectionRatioParameterName(TEXT("PlayedOrderedDitherResidueMainDirectionRatio"));
	const FName PlayedOrderedDitherResidueDirectionSpreadParameterName(TEXT("PlayedOrderedDitherResidueDirectionSpread"));
	const FName PlayedOrderedDitherResidueScatterStrengthParameterName(TEXT("PlayedOrderedDitherResidueScatterStrength"));
	const FName PlayedDissolveShadowFadeFractionParameterName(TEXT("PlayedDissolveShadowFadeFraction"));
	const FName PlayedDissolveNoiseTextureParameterName(TEXT("PlayedDissolveNoiseTexture"));
	const FName SurfaceInvSizeParameterName(TEXT("SurfaceInvSize"));
}

void UWacomFirstPersonCardViewWidget::SetCardViewData(const FWacomCardViewData& InData)
{
	PendingCardViewData = InData;
	ApplyPendingCardViewData();
}

void UWacomFirstPersonCardViewWidget::RequestPresentationRender()
{
	Invalidate(EInvalidateWidgetReason::Paint);
	if (Fake3DSurfaceRetainer)
	{
		#if WITH_AUTOMATION_TESTS
		++PresentationRenderRequestCount;
		#endif
		Fake3DSurfaceRetainer->RequestRender();
	}
}

void UWacomFirstPersonCardViewWidget::SetRetainedRenderingEnabled(bool bEnabled)
{
	if (!Fake3DSurfaceRetainer)
	{
		bRetainedRenderingEnabled = bEnabled;
		bRetainedRenderingStateApplied = false;
		return;
	}
	if (bRetainedRenderingStateApplied && bRetainedRenderingEnabled == bEnabled)
	{
		return;
	}
	bRetainedRenderingEnabled = bEnabled;
	#if WITH_AUTOMATION_TESTS
	++RetainedRenderingApplyCount;
	#endif
	Fake3DSurfaceRetainer->SetRetainRendering(bEnabled);
	if (bEnabled)
	{
		Fake3DSurfaceRetainer->RequestRender();
	}
	bRetainedRenderingStateApplied = true;
}

void UWacomFirstPersonCardViewWidget::SetRealtimePresentationEnabled(bool bEnabled)
{
	if (!Fake3DSurfaceRetainer)
	{
		bRealtimePresentationEnabled = bEnabled;
		bRealtimePresentationStateApplied = false;
		return;
	}
	if (bRealtimePresentationStateApplied && bRealtimePresentationEnabled == bEnabled)
	{
		return;
	}
	bRealtimePresentationEnabled = bEnabled;
	// WBP_FPCardView 使用 phase rendering。静态模式把 phase 周期移出正常运行区间，
	// 内容变化仍由 RequestRender 精确补绘；实时模式恢复每帧 phase。
	#if WITH_AUTOMATION_TESTS
	++RealtimePresentationApplyCount;
	#endif
	Fake3DSurfaceRetainer->SetRenderingPhase(0, bEnabled ? 1 : 1000000);
	Fake3DSurfaceRetainer->RequestRender();
	bRealtimePresentationStateApplied = true;
}

bool UWacomFirstPersonCardViewWidget::PrepareCostDigitRewrite(
	const FWacomCardViewData& InNewData)
{
	EnsureFallbackWidgetTree();
	return CardView && CardView->PrepareCostDigitRewrite(InNewData);
}

bool UWacomFirstPersonCardViewWidget::PrepareCostDigitRewrite(
	const FWacomCardViewData& InOldData,
	const FWacomCardViewData& InNewData)
{
	EnsureFallbackWidgetTree();
	return CardView && CardView->PrepareCostDigitRewrite(InOldData, InNewData);
}

void UWacomFirstPersonCardViewWidget::SetCardDataRewriteView(
	const FWacomFirstPersonCardDataRewriteView& View)
{
	EnsureFallbackWidgetTree();
	LastDataRewriteView = View;
	if (CardView)
	{
		CardView->SetCostDigitRewriteView(View);
	}
	if (Fake3DSurfaceRetainer)
	{
		Fake3DSurfaceRetainer->RequestRender();
	}
}

void UWacomFirstPersonCardViewWidget::ResetCardDataRewriteView()
{
	LastDataRewriteView = FWacomFirstPersonCardDataRewriteView();
	if (CardView)
	{
		CardView->ResetCostDigitRewrite();
	}
	if (Fake3DSurfaceRetainer)
	{
		Fake3DSurfaceRetainer->RequestRender();
	}
}

void UWacomFirstPersonCardViewWidget::SetCostDigitPreviewView(
	const FWacomFirstPersonCardCostPreviewView& View)
{
	if (CardView)
	{
		CardView->SetCostDigitPreviewView(View);
	}
}

void UWacomFirstPersonCardViewWidget::ResetCostDigitPreviewView()
{
	if (CardView)
	{
		CardView->ResetCostDigitPreview();
	}
}

void UWacomFirstPersonCardViewWidget::SetEffectBadgeFeedbackConfig(
	const FWacomFirstPersonCardEffectBadgeFeedbackConfig& InConfig)
{
	LastEffectBadgeFeedbackConfig = InConfig;
	if (CardView)
	{
		CardView->SetEffectBadgeFeedbackConfig(InConfig);
	}
}

void UWacomFirstPersonCardViewWidget::PrimeLocalPresentationMaterials(
	const FWacomFirstPersonCardDataRewriteConfig& DataRewriteConfig,
	const FWacomFirstPersonCardEffectBadgeFeedbackConfig& EffectBadgeConfig)
{
	LastEffectBadgeFeedbackConfig = EffectBadgeConfig;
	EnsureFallbackWidgetTree();
	if (!CardView)
	{
		return;
	}
	CardView->PrimeCostDigitPresentationMaterial(
		DataRewriteConfig.Style.DigitRewriteMaterialInstance);
	CardView->SetEffectBadgeFeedbackConfig(EffectBadgeConfig);
}

void UWacomFirstPersonCardViewWidget::SetEffectBadgeFeedbackView(
	const FWacomFirstPersonCardEffectBadgeFeedbackView& InView)
{
	LastEffectBadgeFeedbackView = InView;
	if (CardView)
	{
		CardView->SetEffectBadgeFeedbackView(InView);
	}
}

void UWacomFirstPersonCardViewWidget::ResetEffectBadgeFeedbackView()
{
	LastEffectBadgeFeedbackView = FWacomFirstPersonCardEffectBadgeFeedbackView();
	if (CardView)
	{
		CardView->ResetEffectBadgeFeedback();
	}
}

uint32 UWacomFirstPersonCardViewWidget::AllocatePresentationPreparationGeneration()
{
	uint32 Result = NextPresentationPreparationGeneration++;
	if (Result == 0)
	{
		Result = NextPresentationPreparationGeneration++;
	}
	return Result;
}

uint32 UWacomFirstPersonCardViewWidget::BeginSurfacePresentationPreparation(
	bool bReuseReadyGeneration)
{
	const UMaterialInterface* DesiredSource = ResolveActiveSurfaceEffectMaterialSource();
	if (!DesiredSource)
	{
		CancelSurfacePresentationPreparation();
		return 0;
	}
	if (bReuseReadyGeneration
		&& SurfaceRequestedGeneration != 0
		&& ActiveSurfaceEffectMaterialSource == DesiredSource
		&& IsSurfacePresentationMaterialReady(SurfaceRequestedGeneration)
		&& IsSurfacePresentationPainted(SurfaceRequestedGeneration))
	{
		return SurfaceRequestedGeneration;
	}

	SurfaceRequestedGeneration = AllocatePresentationPreparationGeneration();
	SurfaceMaterialReadyGeneration = 0;
	SurfacePaintedGeneration = 0;
	RefreshSurfacePresentationPreparation(SurfaceRequestedGeneration);
	return SurfaceRequestedGeneration;
}

uint32 UWacomFirstPersonCardViewWidget::BeginCostDigitPresentationPreparation()
{
	CostDigitRequestedGeneration = AllocatePresentationPreparationGeneration();
	CostDigitMaterialReadyGeneration = 0;
	CostDigitPaintedGeneration = 0;
	RefreshCostDigitPresentationPreparation(CostDigitRequestedGeneration);
	return CostDigitRequestedGeneration;
}

uint32 UWacomFirstPersonCardViewWidget::BeginEffectBadgePresentationPreparation()
{
	EffectBadgeRequestedGeneration = AllocatePresentationPreparationGeneration();
	EffectBadgeMaterialReadyGeneration = 0;
	EffectBadgePaintedGeneration = 0;
	RefreshEffectBadgePresentationPreparation(EffectBadgeRequestedGeneration);
	return EffectBadgeRequestedGeneration;
}

void UWacomFirstPersonCardViewWidget::RefreshSurfacePresentationPreparation(
	uint32 Generation)
{
	if (Generation == 0 || Generation != SurfaceRequestedGeneration)
	{
		return;
	}
	const UMaterialInterface* DesiredSource = ResolveActiveSurfaceEffectMaterialSource();
	if (!DesiredSource)
	{
		return;
	}
	// Retainer's Slate resource may create its runtime MID after SetEffectMaterial.
	// Re-submit the progress-zero view so a late MID never reaches Paint with defaults.
	SetCardSurfaceEffectView(LastSurfaceEffectView);
	if (ActiveSurfaceEffectMaterialInstance
		&& ActiveSurfaceEffectMaterialSource == DesiredSource
		&& Fake3DSurfaceRetainer
		&& Fake3DSurfaceRetainer->GetEffectMaterial() == ActiveSurfaceEffectMaterialInstance)
	{
		SurfaceMaterialReadyGeneration = Generation;
	}
	RequestPresentationRender();
}

void UWacomFirstPersonCardViewWidget::RefreshCostDigitPresentationPreparation(
	uint32 Generation)
{
	if (Generation == 0 || Generation != CostDigitRequestedGeneration)
	{
		return;
	}
	if (CardView && CardView->IsCostDigitRewriteMaterialReady())
	{
		CostDigitMaterialReadyGeneration = Generation;
	}
	RequestPresentationRender();
}

void UWacomFirstPersonCardViewWidget::RefreshEffectBadgePresentationPreparation(
	uint32 Generation)
{
	if (Generation == 0 || Generation != EffectBadgeRequestedGeneration)
	{
		return;
	}
	if (CardView && CardView->IsEffectBadgeFeedbackMaterialReady())
	{
		EffectBadgeMaterialReadyGeneration = Generation;
	}
	RequestPresentationRender();
}

bool UWacomFirstPersonCardViewWidget::IsSurfacePresentationMaterialReady(
	uint32 Generation) const
{
	return Generation != 0 && SurfaceMaterialReadyGeneration == Generation;
}

bool UWacomFirstPersonCardViewWidget::IsSurfacePresentationPainted(uint32 Generation) const
{
	return Generation != 0 && SurfacePaintedGeneration == Generation;
}

bool UWacomFirstPersonCardViewWidget::IsCostDigitPresentationMaterialReady(
	uint32 Generation) const
{
	return Generation != 0 && CostDigitMaterialReadyGeneration == Generation;
}

bool UWacomFirstPersonCardViewWidget::IsCostDigitPresentationPainted(uint32 Generation) const
{
	return Generation != 0 && CostDigitPaintedGeneration == Generation;
}

bool UWacomFirstPersonCardViewWidget::IsEffectBadgePresentationMaterialReady(
	uint32 Generation) const
{
	return Generation != 0 && EffectBadgeMaterialReadyGeneration == Generation;
}

bool UWacomFirstPersonCardViewWidget::IsEffectBadgePresentationPainted(uint32 Generation) const
{
	return Generation != 0 && EffectBadgePaintedGeneration == Generation;
}

void UWacomFirstPersonCardViewWidget::CancelSurfacePresentationPreparation()
{
	SurfaceRequestedGeneration = 0;
	SurfaceMaterialReadyGeneration = 0;
	SurfacePaintedGeneration = 0;
}

void UWacomFirstPersonCardViewWidget::CancelCostDigitPresentationPreparation()
{
	CostDigitRequestedGeneration = 0;
	CostDigitMaterialReadyGeneration = 0;
	CostDigitPaintedGeneration = 0;
}

void UWacomFirstPersonCardViewWidget::CancelEffectBadgePresentationPreparation()
{
	EffectBadgeRequestedGeneration = 0;
	EffectBadgeMaterialReadyGeneration = 0;
	EffectBadgePaintedGeneration = 0;
}

void UWacomFirstPersonCardViewWidget::CancelAllPresentationPreparations()
{
	CancelSurfacePresentationPreparation();
	CancelCostDigitPresentationPreparation();
	CancelEffectBadgePresentationPreparation();
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

void UWacomFirstPersonCardViewWidget::SetInteractionCueView(
	const FWacomFirstPersonCardInteractionCueView& View)
{
	LastInteractionCueView = View;
	LastInteractionCueView.Amount = FMath::Clamp(View.Amount, 0.0f, 1.0f);
	LastInteractionCueView.CornerInsetPixels = FMath::Max(0.0f, View.CornerInsetPixels);
	LastInteractionCueView.CornerLengthPixels = FMath::Max(0.0f, View.CornerLengthPixels);
	LastInteractionCueView.CornerThicknessPixels = FMath::Max(0.0f, View.CornerThicknessPixels);
	Invalidate(EInvalidateWidgetReason::Paint);
}

void UWacomFirstPersonCardViewWidget::ClearInteractionCueView()
{
	LastInteractionCueView = FWacomFirstPersonCardInteractionCueView();
	Invalidate(EInvalidateWidgetReason::Paint);
}

void UWacomFirstPersonCardViewWidget::SetCardDepthView(const FWacomFirstPersonCardDepthView& View)
{
	EnsureFallbackWidgetTree();
	#if WITH_AUTOMATION_TESTS
	++CardDepthApplyCount;
	#endif
	LastCardDepthView = View;
	LastCardDepthView.PerspectiveStrength = FMath::Max(0.0f, LastCardDepthView.PerspectiveStrength);
	LastCardDepthView.ContactShadowLift = FMath::Clamp(LastCardDepthView.ContactShadowLift, 0.0f, 1.0f);
	LastCardDepthView.ContactShadowOpacityMultiplier =
		FMath::Max(0.0f, LastCardDepthView.ContactShadowOpacityMultiplier);
	if (!FMath::IsFinite(LastCardDepthView.ContactShadowOffsetPixels.X)
		|| !FMath::IsFinite(LastCardDepthView.ContactShadowOffsetPixels.Y))
	{
		LastCardDepthView.ContactShadowOffsetPixels = FVector2D::ZeroVector;
	}
	if (CardView)
	{
		CardView->SetCardSurfacePerspectiveView(LastCardDepthView.SurfacePerspective);
	}

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
	const FWacomFirstPersonCardUseEffectView& CardUseView = View.CardUse;
	const FWacomFirstPersonCardDrawRevealView& DrawRevealView = View.DrawReveal;
	const FWacomFirstPersonCardGainRevealView& GainRevealView = View.GainReveal;
	const FWacomFirstPersonCardPlayedDissolveView& DissolveView = View.PlayedDissolve;
	const FWacomFirstPersonCardHandTargetImpactView& HandTargetImpactView =
		View.HandTargetImpact;
	const FWacomFirstPersonCardRetainSealView& RetainSealView = View.RetainSeal;
	if (DrawRevealView.bActive
		&& DrawRevealView.Style.SurfaceEffectMaterialInstance)
	{
		EnsureSurfaceEffectMaterialInstance(
			DrawRevealView.Style.SurfaceEffectMaterialInstance);
		if (ActiveSurfaceEffectMaterialInstance)
		{
			ApplyCardDepthParameters(*ActiveSurfaceEffectMaterialInstance);
			ApplyDrawRevealParameters(*ActiveSurfaceEffectMaterialInstance, DrawRevealView);
		}
	}
	else if (GainRevealView.bActive
		&& GainRevealView.Style.SurfaceEffectMaterialInstance)
	{
		EnsureSurfaceEffectMaterialInstance(
			GainRevealView.Style.SurfaceEffectMaterialInstance);
		if (ActiveSurfaceEffectMaterialInstance)
		{
			ApplyCardDepthParameters(*ActiveSurfaceEffectMaterialInstance);
			ApplyGainRevealParameters(*ActiveSurfaceEffectMaterialInstance, GainRevealView);
		}
	}
	else if (HandTargetImpactView.bActive
		&& HandTargetImpactView.Style.SurfaceEffectMaterialInstance)
	{
		EnsureSurfaceEffectMaterialInstance(
			HandTargetImpactView.Style.SurfaceEffectMaterialInstance);
		if (ActiveSurfaceEffectMaterialInstance)
		{
			ApplyCardDepthParameters(*ActiveSurfaceEffectMaterialInstance);
			ApplyHandTargetImpactParameters(
				*ActiveSurfaceEffectMaterialInstance,
				HandTargetImpactView);
		}
	}
	else if (CardUseView.bActive
		&& CardUseView.Style.SurfaceEffectMaterialInstance)
	{
		EnsureSurfaceEffectMaterialInstance(CardUseView.Style.SurfaceEffectMaterialInstance);
		if (ActiveSurfaceEffectMaterialInstance)
		{
			ApplyCardDepthParameters(*ActiveSurfaceEffectMaterialInstance);
			ApplyCardUseEffectParameters(*ActiveSurfaceEffectMaterialInstance, CardUseView);
		}
	}
	else if (DissolveView.bActive
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
	else if (RetainSealView.bActive
		&& RetainSealView.Style.SurfaceEffectMaterialInstance)
	{
		EnsureSurfaceEffectMaterialInstance(
			RetainSealView.Style.SurfaceEffectMaterialInstance);
		if (ActiveSurfaceEffectMaterialInstance)
		{
			ApplyCardDepthParameters(*ActiveSurfaceEffectMaterialInstance);
			ApplyRetainSealParameters(*ActiveSurfaceEffectMaterialInstance, RetainSealView);
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
	View.InteractionCueAmount = LastInteractionCueView.Amount;
	View.InteractionCueColor = LastInteractionCueView.Color;
	View.InteractionCueKind = LastInteractionCueView.Kind;
	View.bInteractionCuePaintRequested = LastInteractionCueView.Amount > KINDA_SMALL_NUMBER;
	View.CardDepthView = LastCardDepthView;
	View.SurfaceEffectView = LastSurfaceEffectView;
	View.DataRewriteView = LastDataRewriteView;
	View.bHasFake3DSurfaceRetainer = Fake3DSurfaceRetainer != nullptr;
	View.bFake3DEffectMaterialReady =
		Fake3DSurfaceRetainer && Fake3DSurfaceRetainer->GetEffectMaterial() != nullptr;
	View.bUsingSurfaceEffectMaterial =
		Fake3DSurfaceRetainer
		&& ActiveSurfaceEffectMaterialInstance
		&& Fake3DSurfaceRetainer->GetEffectMaterial() == ActiveSurfaceEffectMaterialInstance;
	View.bBaseSurfaceEffectMaterialCached = bBaseSurfaceEffectMaterialCached;
	View.bRetainedRenderingEnabled = bRetainedRenderingEnabled;
	View.bRealtimePresentationEnabled = bRealtimePresentationEnabled;
	View.PresentationRenderRequestCount = PresentationRenderRequestCount;
	View.CardViewDataApplyCount = CardViewDataApplyCount;
	View.RetainedRenderingApplyCount = RetainedRenderingApplyCount;
	View.RealtimePresentationApplyCount = RealtimePresentationApplyCount;
	View.CardDepthApplyCount = CardDepthApplyCount;
	View.SurfaceRequestedGeneration = SurfaceRequestedGeneration;
	View.SurfaceMaterialReadyGeneration = SurfaceMaterialReadyGeneration;
	View.SurfacePaintedGeneration = SurfacePaintedGeneration;
	View.CostDigitRequestedGeneration = CostDigitRequestedGeneration;
	View.CostDigitMaterialReadyGeneration = CostDigitMaterialReadyGeneration;
	View.CostDigitPaintedGeneration = CostDigitPaintedGeneration;
	View.EffectBadgeRequestedGeneration = EffectBadgeRequestedGeneration;
	View.EffectBadgeMaterialReadyGeneration = EffectBadgeMaterialReadyGeneration;
	View.EffectBadgePaintedGeneration = EffectBadgePaintedGeneration;
	View.bSurfacePresentationMaterialReady = IsSurfacePresentationMaterialReady(
		SurfaceRequestedGeneration);
	View.bCostDigitPresentationMaterialReady = IsCostDigitPresentationMaterialReady(
		CostDigitRequestedGeneration);
	View.bEffectBadgePresentationMaterialReady = IsEffectBadgePresentationMaterialReady(
		EffectBadgeRequestedGeneration);
	const UWidget* RetainerCaptureRoot = Fake3DSurfaceRetainer
		? Fake3DSurfaceRetainer->GetContent()
		: nullptr;
	View.bRetainerCaptureRootUsesIndependentClipping =
		RetainerCaptureRoot
		&& RetainerCaptureRoot->GetClipping() == EWidgetClipping::ClipToBoundsWithoutIntersecting;
	View.WrapperDesiredSize = GetDesiredSize();
	View.RetainerDesiredSize = Fake3DSurfaceRetainer
		? Fake3DSurfaceRetainer->GetDesiredSize()
		: FVector2D::ZeroVector;
	View.RetainerCaptureRootDesiredSize = RetainerCaptureRoot
		? RetainerCaptureRoot->GetDesiredSize()
		: FVector2D::ZeroVector;
	const UWidget* CardContentWidget = CardContentSizeBox;
	View.CardContentDesiredSize = CardContentWidget
		? CardContentWidget->GetDesiredSize()
		: FVector2D::ZeroVector;
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
	bRetainedRenderingStateApplied = false;
	bRealtimePresentationStateApplied = false;
	ConfigureRetainerCaptureRootClipping();
	if (!CardContentSizeBox && WidgetTree)
	{
		CardContentSizeBox = WidgetTree->FindWidget(TEXT("CardContentSizeBox"));
	}
	CacheBaseSurfaceEffectMaterial();
	ApplyPendingCardViewData();
	ClearInteractionCueView();
	SetCardDepthView(LastCardDepthView);
	SetCardSurfaceEffectView(LastSurfaceEffectView);
	SetCardDataRewriteView(LastDataRewriteView);
	SetEffectBadgeFeedbackConfig(LastEffectBadgeFeedbackConfig);
	SetEffectBadgeFeedbackView(LastEffectBadgeFeedbackView);
	SetRetainedRenderingEnabled(bRetainedRenderingEnabled);
	SetRealtimePresentationEnabled(bRealtimePresentationEnabled);
}

void UWacomFirstPersonCardViewWidget::NativeDestruct()
{
	CancelAllPresentationPreparations();
	if (CardView)
	{
		CardView->ResetEffectBadgeFeedback();
		CardView->ResetCostDigitRewrite();
		CardView->ResetCardSurfacePerspectiveView();
	}
	RestoreBaseSurfaceEffectMaterial();
	bRetainedRenderingStateApplied = false;
	ActiveSurfaceEffectMaterialInstance = nullptr;
	ActiveSurfaceEffectMaterialSource = nullptr;
	BaseSurfaceEffectMaterialInstance = nullptr;
	BaseSurfaceEffectMaterialSource = nullptr;
	bBaseSurfaceEffectMaterialCached = false;
	bRealtimePresentationStateApplied = false;
	ClearInteractionCueView();
	Super::NativeDestruct();
}

int32 UWacomFirstPersonCardViewWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	const int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	const bool bParentEnabled) const
{
	const int32 MaxLayerId = Super::NativePaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId,
		InWidgetStyle,
		bParentEnabled);
	if (SurfaceRequestedGeneration != 0
		&& SurfaceMaterialReadyGeneration == SurfaceRequestedGeneration)
	{
		SurfacePaintedGeneration = SurfaceRequestedGeneration;
	}
	if (CostDigitRequestedGeneration != 0
		&& CostDigitMaterialReadyGeneration == CostDigitRequestedGeneration)
	{
		CostDigitPaintedGeneration = CostDigitRequestedGeneration;
	}
	if (EffectBadgeRequestedGeneration != 0
		&& EffectBadgeMaterialReadyGeneration == EffectBadgeRequestedGeneration)
	{
		EffectBadgePaintedGeneration = EffectBadgeRequestedGeneration;
	}
	if (LastInteractionCueView.Kind != EWacomFirstPersonCardInteractionCueKind::Deny
		|| LastInteractionCueView.Amount <= KINDA_SMALL_NUMBER)
	{
		return MaxLayerId;
	}

	return FWacomFirstPersonCardInteractionCuePainter::PaintCorners(
		AllottedGeometry,
		OutDrawElements,
		MaxLayerId,
		ResolveInteractionCueRect(AllottedGeometry),
		LastInteractionCueView.Color,
		LastInteractionCueView.Amount,
		InWidgetStyle.GetColorAndOpacityTint().A,
		LastInteractionCueView.CornerInsetPixels,
		LastInteractionCueView.CornerLengthPixels,
		LastInteractionCueView.CornerThicknessPixels);
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

UMaterialInterface* UWacomFirstPersonCardViewWidget::ResolveActiveSurfaceEffectMaterialSource() const
{
	const FWacomFirstPersonCardDrawRevealView& DrawReveal = LastSurfaceEffectView.DrawReveal;
	if (DrawReveal.bActive && DrawReveal.Style.SurfaceEffectMaterialInstance)
	{
		return DrawReveal.Style.SurfaceEffectMaterialInstance;
	}
	const FWacomFirstPersonCardGainRevealView& GainReveal = LastSurfaceEffectView.GainReveal;
	if (GainReveal.bActive && GainReveal.Style.SurfaceEffectMaterialInstance)
	{
		return GainReveal.Style.SurfaceEffectMaterialInstance;
	}
	const FWacomFirstPersonCardHandTargetImpactView& HandTarget =
		LastSurfaceEffectView.HandTargetImpact;
	if (HandTarget.bActive && HandTarget.Style.SurfaceEffectMaterialInstance)
	{
		return HandTarget.Style.SurfaceEffectMaterialInstance;
	}
	const FWacomFirstPersonCardUseEffectView& CardUse = LastSurfaceEffectView.CardUse;
	if (CardUse.bActive && CardUse.Style.SurfaceEffectMaterialInstance)
	{
		return CardUse.Style.SurfaceEffectMaterialInstance;
	}
	const FWacomFirstPersonCardPlayedDissolveView& Dissolve =
		LastSurfaceEffectView.PlayedDissolve;
	if (Dissolve.bActive && Dissolve.Style.SurfaceEffectMaterial && Dissolve.Style.NoiseTexture)
	{
		return Dissolve.Style.SurfaceEffectMaterial;
	}
	const FWacomFirstPersonCardRetainSealView& RetainSeal = LastSurfaceEffectView.RetainSeal;
	return RetainSeal.bActive ? RetainSeal.Style.SurfaceEffectMaterialInstance.Get() : nullptr;
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
	Material.SetScalarParameterValue(
		ContactShadowOpacityMultiplierParameterName,
		LastCardDepthView.ContactShadowOpacityMultiplier);

	FVector2D SurfaceSize = Fake3DSurfaceRetainer
		? Fake3DSurfaceRetainer->GetCachedGeometry().GetLocalSize()
		: FVector2D::ZeroVector;
	if (!FMath::IsFinite(SurfaceSize.X)
		|| !FMath::IsFinite(SurfaceSize.Y)
		|| SurfaceSize.X <= 1.0f
		|| SurfaceSize.Y <= 1.0f)
	{
		SurfaceSize = FVector2D(456.0f, 520.0f);
	}
	const FVector2D ContactShadowOffsetPixels = LastCardDepthView.bContactShadowEnabled
		? LastCardDepthView.ContactShadowOffsetPixels
		: FVector2D::ZeroVector;
	Material.SetScalarParameterValue(
		ContactShadowTiltOffsetXUVParameterName,
		ContactShadowOffsetPixels.X / SurfaceSize.X);
	Material.SetScalarParameterValue(
		ContactShadowTiltOffsetYUVParameterName,
		ContactShadowOffsetPixels.Y / SurfaceSize.Y);
}

void UWacomFirstPersonCardViewWidget::ApplyCardUseEffectParameters(
	UMaterialInstanceDynamic& Material,
	const FWacomFirstPersonCardUseEffectView& View) const
{
	Material.SetScalarParameterValue(CardUseEnabledParameterName, View.bActive ? 1.0f : 0.0f);
	Material.SetScalarParameterValue(CardUseProgressParameterName, FMath::Clamp(View.Amount, 0.0f, 1.0f));
	Material.SetScalarParameterValue(CardUseFlipProgressParameterName, FMath::Clamp(View.FlipProgress, 0.0f, 1.0f));
	Material.SetScalarParameterValue(CardUseImpactProgressParameterName, FMath::Clamp(View.ImpactProgress, 0.0f, 1.0f));
	Material.SetScalarParameterValue(CardUseTimeParameterName, FMath::Max(0.0f, View.TimeSeconds));
	Material.SetScalarParameterValue(
		CardUseReducedMotionParameterName,
		View.bReducedMotion ? 1.0f : 0.0f);

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

void UWacomFirstPersonCardViewWidget::ApplyHandTargetImpactParameters(
	UMaterialInstanceDynamic& Material,
	const FWacomFirstPersonCardHandTargetImpactView& View) const
{
	Material.SetScalarParameterValue(
		HandTargetImpactEnabledParameterName,
		View.bActive ? 1.0f : 0.0f);
	Material.SetScalarParameterValue(
		HandTargetImpactPreviewAmountParameterName,
		FMath::Clamp(View.PreviewAmount, 0.0f, 1.0f));
	Material.SetScalarParameterValue(
		HandTargetImpactCommitProgressParameterName,
		FMath::Clamp(View.CommitProgress, 0.0f, 1.0f));
	Material.SetScalarParameterValue(
		HandTargetImpactTimeParameterName,
		FMath::Max(0.0f, View.TimeSeconds));
	Material.SetScalarParameterValue(
		HandTargetImpactSeedParameterName,
		FMath::Frac(FMath::Abs(View.Seed)));
	Material.SetScalarParameterValue(
		HandTargetImpactReducedMotionParameterName,
		View.bReducedMotion ? 1.0f : 0.0f);
	FLinearColor BodyRectMin;
	FLinearColor BodyRectMax;
	ResolveCardBodyUVRect(BodyRectMin, BodyRectMax);
	Material.SetVectorParameterValue(HandTargetImpactCardBodyRectMinParameterName, BodyRectMin);
	Material.SetVectorParameterValue(HandTargetImpactCardBodyRectMaxParameterName, BodyRectMax);
	FVector2D SurfaceSize = Fake3DSurfaceRetainer
		? Fake3DSurfaceRetainer->GetCachedGeometry().GetLocalSize()
		: FVector2D::ZeroVector;
	if (SurfaceSize.X <= 1.0f || SurfaceSize.Y <= 1.0f)
	{
		SurfaceSize = FVector2D(456.0f, 520.0f);
	}
	Material.SetVectorParameterValue(
		SurfaceInvSizeParameterName,
		FLinearColor(
			1.0f / SurfaceSize.X,
			1.0f / SurfaceSize.Y,
			0.0f,
			0.0f));
}

void UWacomFirstPersonCardViewWidget::ApplyDrawRevealParameters(
	UMaterialInstanceDynamic& Material,
	const FWacomFirstPersonCardDrawRevealView& View) const
{
	const FWacomFirstPersonCardDrawRevealStyleData& Style = View.Style;
	Material.SetScalarParameterValue(DrawRevealEnabledParameterName, View.bActive ? 1.0f : 0.0f);
	Material.SetScalarParameterValue(
		DrawRevealProgressParameterName,
		FMath::Clamp(View.Progress, 0.0f, 1.0f));
	Material.SetScalarParameterValue(
		DrawRevealReducedMotionParameterName,
		View.bReducedMotion ? 1.0f : 0.0f);
	Material.SetScalarParameterValue(DrawRevealBackHoldEndParameterName, Style.BackHoldEndProgress);
	Material.SetScalarParameterValue(DrawRevealFaceSwitchParameterName, Style.FaceSwitchProgress);
	Material.SetScalarParameterValue(DrawRevealFaceExpandEndParameterName, Style.FaceExpandEndProgress);
	Material.SetScalarParameterValue(
		DrawRevealReducedCrossFadeStartParameterName,
		Style.ReducedCrossFadeStartProgress);
	Material.SetScalarParameterValue(
		DrawRevealReducedCrossFadeEndParameterName,
		Style.ReducedCrossFadeEndProgress);

	FLinearColor BodyRectMin;
	FLinearColor BodyRectMax;
	ResolveCardBodyUVRect(BodyRectMin, BodyRectMax);
	Material.SetVectorParameterValue(DrawRevealCardBodyRectMinParameterName, BodyRectMin);
	Material.SetVectorParameterValue(DrawRevealCardBodyRectMaxParameterName, BodyRectMax);

	FVector2D SurfaceSize = Fake3DSurfaceRetainer
		? Fake3DSurfaceRetainer->GetCachedGeometry().GetLocalSize()
		: FVector2D::ZeroVector;
	if (SurfaceSize.X <= 1.0f || SurfaceSize.Y <= 1.0f)
	{
		SurfaceSize = FVector2D(456.0f, 520.0f);
	}
	Material.SetVectorParameterValue(
		SurfaceInvSizeParameterName,
		FLinearColor(1.0f / SurfaceSize.X, 1.0f / SurfaceSize.Y, 0.0f, 0.0f));
}

void UWacomFirstPersonCardViewWidget::ApplyGainRevealParameters(
	UMaterialInstanceDynamic& Material,
	const FWacomFirstPersonCardGainRevealView& View) const
{
	const FWacomFirstPersonCardGainRevealStyleData& Style = View.Style;
	Material.SetScalarParameterValue(
		GainRevealEnabledParameterName,
		View.bActive ? 1.0f : 0.0f);
	Material.SetScalarParameterValue(
		GainRevealProgressParameterName,
		FMath::Clamp(View.Progress, 0.0f, 1.0f));
	Material.SetScalarParameterValue(
		GainRevealReducedMotionParameterName,
		View.bReducedMotion ? 1.0f : 0.0f);
	Material.SetScalarParameterValue(
		GainRevealSeedParameterName,
		FMath::Frac(FMath::Abs(View.Seed)));
	Material.SetScalarParameterValue(
		GainRevealRarityIndexParameterName,
		static_cast<float>(View.Rarity));
	Material.SetScalarParameterValue(
		GainRevealSeedEstablishEndParameterName,
		Style.SeedEstablishEndProgress);
	Material.SetScalarParameterValue(
		GainRevealAssemblyEndParameterName,
		Style.AssemblyEndProgress);
	Material.SetScalarParameterValue(
		GainRevealRarityEdgePeakParameterName,
		Style.RarityEdgePeakProgress);
	Material.SetScalarParameterValue(
		GainRevealSettleEndParameterName,
		Style.SettleEndProgress);
	Material.SetScalarParameterValue(
		GainRevealReducedCrossFadeStartParameterName,
		Style.ReducedCrossFadeStartProgress);
	Material.SetScalarParameterValue(
		GainRevealReducedCrossFadeEndParameterName,
		Style.ReducedCrossFadeEndProgress);

	FVector2D SurfaceSize = Fake3DSurfaceRetainer
		? Fake3DSurfaceRetainer->GetCachedGeometry().GetLocalSize()
		: FVector2D::ZeroVector;
	if (SurfaceSize.X <= 1.0f || SurfaceSize.Y <= 1.0f)
	{
		SurfaceSize = FVector2D(456.0f, 520.0f);
	}
	Material.SetVectorParameterValue(
		SurfaceInvSizeParameterName,
		FLinearColor(1.0f / SurfaceSize.X, 1.0f / SurfaceSize.Y, 0.0f, 0.0f));
}

void UWacomFirstPersonCardViewWidget::ApplyRetainSealParameters(
	UMaterialInstanceDynamic& Material,
	const FWacomFirstPersonCardRetainSealView& View) const
{
	Material.SetScalarParameterValue(
		RetainSealEnabledParameterName,
		View.bActive ? 1.0f : 0.0f);
	Material.SetScalarParameterValue(
		RetainSealPhaseParameterName,
		static_cast<float>(View.Phase));
	Material.SetScalarParameterValue(
		RetainSealProgressParameterName,
		FMath::Clamp(View.Progress, 0.0f, 1.0f));
	Material.SetScalarParameterValue(
		RetainSealSeedParameterName,
		FMath::Frac(FMath::Abs(View.Seed)));
	Material.SetScalarParameterValue(
		RetainSealReducedMotionParameterName,
		View.bReducedMotion ? 1.0f : 0.0f);

	FVector2D SurfaceSize = Fake3DSurfaceRetainer
		? Fake3DSurfaceRetainer->GetCachedGeometry().GetLocalSize()
		: FVector2D::ZeroVector;
	if (SurfaceSize.X <= 1.0f || SurfaceSize.Y <= 1.0f)
	{
		SurfaceSize = FVector2D(456.0f, 520.0f);
	}
	Material.SetVectorParameterValue(
		SurfaceInvSizeParameterName,
		FLinearColor(1.0f / SurfaceSize.X, 1.0f / SurfaceSize.Y, 0.0f, 0.0f));
}

bool UWacomFirstPersonCardViewWidget::ResolveCardBodyUVRect(
	FLinearColor& OutMin,
	FLinearColor& OutMax) const
{
	FVector2D SurfaceSize = Fake3DSurfaceRetainer
		? Fake3DSurfaceRetainer->GetCachedGeometry().GetLocalSize()
		: FVector2D::ZeroVector;
	if (!FMath::IsFinite(SurfaceSize.X) || !FMath::IsFinite(SurfaceSize.Y)
		|| SurfaceSize.X <= 1.0f || SurfaceSize.Y <= 1.0f)
	{
		SurfaceSize = FVector2D(456.0f, 520.0f);
	}

	FVector2D LocalMin(48.0f, 48.0f);
	FVector2D LocalMax = LocalMin + FVector2D(360.0f, 424.0f);
	const UWidget* CardContent = WidgetTree
		? WidgetTree->FindWidget(TEXT("CardContentSizeBox"))
		: nullptr;
	if (Fake3DSurfaceRetainer && CardContent)
	{
		const FGeometry SurfaceGeometry = Fake3DSurfaceRetainer->GetCachedGeometry();
		const FGeometry ContentGeometry = CardContent->GetCachedGeometry();
		const FVector2D ContentSize = ContentGeometry.GetLocalSize();
		if (ContentSize.X > 1.0f && ContentSize.Y > 1.0f)
		{
			LocalMin = SurfaceGeometry.AbsoluteToLocal(ContentGeometry.LocalToAbsolute(FVector2D::ZeroVector));
			LocalMax = SurfaceGeometry.AbsoluteToLocal(ContentGeometry.LocalToAbsolute(ContentSize));
		}
	}
	const FVector2D UVMin(
		FMath::Clamp(LocalMin.X / SurfaceSize.X, 0.0f, 1.0f),
		FMath::Clamp(LocalMin.Y / SurfaceSize.Y, 0.0f, 1.0f));
	const FVector2D UVMax(
		FMath::Clamp(LocalMax.X / SurfaceSize.X, UVMin.X + KINDA_SMALL_NUMBER, 1.0f),
		FMath::Clamp(LocalMax.Y / SurfaceSize.Y, UVMin.Y + KINDA_SMALL_NUMBER, 1.0f));
	OutMin = FLinearColor(UVMin.X, UVMin.Y, 0.0f, 0.0f);
	OutMax = FLinearColor(UVMax.X, UVMax.Y, 0.0f, 0.0f);
	return UVMax.X > UVMin.X && UVMax.Y > UVMin.Y;
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
	if (Style.EffectKind == EWacomFirstPersonCardPlayedDissolveEffectKind::OrderedDither)
	{
		const FWacomFirstPersonCardOrderedDitherStyleData& Dither = Style.OrderedDither;
		Material.SetScalarParameterValue(
			PlayedOrderedDitherBayerSizeParameterName,
			Dither.BayerMatrixSize <= 4 ? 4.0f : 8.0f);
		Material.SetScalarParameterValue(
			PlayedOrderedDitherBandWidthParameterName,
			FMath::Max(0.001f, Dither.BandWidth));
		Material.SetScalarParameterValue(
			PlayedOrderedDitherResidueDensityParameterName,
			FMath::Clamp(Dither.ResidueDensity, 0.0f, 1.0f));
		Material.SetScalarParameterValue(
			PlayedOrderedDitherResidueTrailWidthParameterName,
			FMath::Max(0.001f, Dither.ResidueTrailWidth));
		Material.SetScalarParameterValue(
			PlayedOrderedDitherResidueTravelPixelsParameterName,
			FMath::Max(0.0f, Dither.ResidueTravelPixels));
		Material.SetScalarParameterValue(
			PlayedOrderedDitherResidueMainDirectionRatioParameterName,
			FMath::Clamp(Dither.ResidueMainDirectionRatio, 0.0f, 1.0f));
		Material.SetScalarParameterValue(
			PlayedOrderedDitherResidueDirectionSpreadParameterName,
			FMath::Clamp(FMath::Abs(Dither.ResidueDirectionSpreadDegrees), 0.0f, 180.0f));
		Material.SetScalarParameterValue(
			PlayedOrderedDitherResidueScatterStrengthParameterName,
			FMath::Max(0.0f, Dither.ResidueScatterStrength));
	}
	else
	{
		Material.SetVectorParameterValue(PlayedDissolveEdgeColorParameterName, Style.EdgeColor);
		Material.SetVectorParameterValue(PlayedDissolveEdgeAccentColorParameterName, Style.EdgeAccentColor);
		Material.SetScalarParameterValue(PlayedDissolveEdgeWidthParameterName, FMath::Max(0.001f, Style.EdgeWidth));
		Material.SetScalarParameterValue(PlayedDissolveEdgeIntensityParameterName, FMath::Max(0.0f, Style.EdgeIntensity));
		Material.SetScalarParameterValue(PlayedDissolveAshDensityParameterName, FMath::Clamp(Style.AshDensity, 0.0f, 1.0f));
		Material.SetScalarParameterValue(PlayedDissolveAshTrailWidthParameterName, FMath::Max(0.001f, Style.AshTrailWidth));
		Material.SetScalarParameterValue(PlayedDissolveAshLiftPixelsParameterName, FMath::Max(0.0f, Style.AshLiftPixels));
		Material.SetScalarParameterValue(PlayedDissolveAshDriftPixelsParameterName, FMath::Max(0.0f, Style.AshDriftPixels));
	}
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
FSlateRect UWacomFirstPersonCardViewWidget::ResolveInteractionCueRect(
	const FGeometry& AllottedGeometry) const
{
	const FVector2D AllottedSize = AllottedGeometry.GetLocalSize();
	if (CardContentSizeBox)
	{
		const FGeometry& ContentGeometry = CardContentSizeBox->GetCachedGeometry();
		const FVector2D ContentSize = ContentGeometry.GetLocalSize();
		if (ContentSize.X > 1.0f && ContentSize.Y > 1.0f)
		{
			const FVector2D LocalTopLeft = AllottedGeometry.AbsoluteToLocal(
				ContentGeometry.GetAbsolutePosition());
			return FSlateRect(
				LocalTopLeft.X,
				LocalTopLeft.Y,
				LocalTopLeft.X + ContentSize.X,
				LocalTopLeft.Y + ContentSize.Y);
		}
	}

	const FVector2D DefaultCardSize(360.0f, 424.0f);
	const FVector2D CueSize(
		FMath::Min(AllottedSize.X, DefaultCardSize.X),
		FMath::Min(AllottedSize.Y, DefaultCardSize.Y));
	const FVector2D CueTopLeft = (AllottedSize - CueSize) * 0.5f;
	return FSlateRect(
		CueTopLeft.X,
		CueTopLeft.Y,
		CueTopLeft.X + CueSize.X,
		CueTopLeft.Y + CueSize.Y);
}

void UWacomFirstPersonCardViewWidget::ApplyPendingCardViewData()
{
	#if WITH_AUTOMATION_TESTS
	++CardViewDataApplyCount;
	#endif
	if (CardView)
	{
		CardView->SetCardViewData(PendingCardViewData);
	}
	if (Fake3DSurfaceRetainer)
	{
		Fake3DSurfaceRetainer->RequestRender();
	}
}
