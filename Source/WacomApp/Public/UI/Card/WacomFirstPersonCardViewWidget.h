// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardViewWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class URetainerBox;
class UWidget;
class UWacomCardView;
#if WITH_AUTOMATION_TESTS
struct FWacomFirstPersonCardLayerTestAccess;
#endif

#if WITH_AUTOMATION_TESTS
struct WACOMAPP_API FWacomFirstPersonCardViewAutomationTestView
{
	float InteractionCueAmount = 0.0f;
	FLinearColor InteractionCueColor = FLinearColor::Transparent;
	FLinearColor InteractionCueAccentColor = FLinearColor::Transparent;
	float InteractionCueProgress = 0.0f;
	FVector2D InteractionCueDirection = FVector2D(0.0f, -1.0f);
	int32 InteractionCueSeed = 0;
	EWacomFirstPersonCardInteractionCueKind InteractionCueKind =
		EWacomFirstPersonCardInteractionCueKind::None;
	bool bInteractionCuePaintRequested = false;
	FWacomFirstPersonCardDepthView CardDepthView;
	FWacomFirstPersonCardSurfaceEffectView SurfaceEffectView;
	FWacomFirstPersonCardDataRewriteView DataRewriteView;
	bool bHasFake3DSurfaceRetainer = false;
	bool bFake3DEffectMaterialReady = false;
	bool bUsingSurfaceEffectMaterial = false;
	bool bBaseSurfaceEffectMaterialCached = false;
	bool bRetainerCaptureRootUsesIndependentClipping = false;
	FVector2D WrapperDesiredSize = FVector2D::ZeroVector;
	FVector2D RetainerDesiredSize = FVector2D::ZeroVector;
	FVector2D RetainerCaptureRootDesiredSize = FVector2D::ZeroVector;
	FVector2D CardContentDesiredSize = FVector2D::ZeroVector;
	FVector2D CardBodyUVRectMin = FVector2D::ZeroVector;
	FVector2D CardBodyUVRectMax = FVector2D::ZeroVector;
	bool bRetainedRenderingEnabled = true;
	bool bRealtimePresentationEnabled = true;
	int32 PresentationRenderRequestCount = 0;
	int32 CardViewDataApplyCount = 0;
	int32 RetainedRenderingApplyCount = 0;
	int32 RealtimePresentationApplyCount = 0;
	int32 CardDepthApplyCount = 0;
	uint32 SurfaceRequestedGeneration = 0;
	uint32 SurfaceMaterialReadyGeneration = 0;
	uint32 SurfacePaintedGeneration = 0;
	uint32 CostDigitRequestedGeneration = 0;
	uint32 CostDigitMaterialReadyGeneration = 0;
	uint32 CostDigitPaintedGeneration = 0;
	uint32 EffectBadgeRequestedGeneration = 0;
	uint32 EffectBadgeMaterialReadyGeneration = 0;
	uint32 EffectBadgePaintedGeneration = 0;
	bool bSurfacePresentationMaterialReady = false;
	bool bCostDigitPresentationMaterialReady = false;
	bool bEffectBadgePresentationMaterialReady = false;
};
#endif

/**
 * First-person card face wrapper.
 *
 * It composes the reusable UWacomCardView with first-person-only presentation
 * layers. Gameplay input and gesture ownership stay in the slot widget.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomFirstPersonCardViewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|First Person Card View")
	void SetCardViewData(const FWacomCardViewData& InData);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card View")
	UWacomCardView* GetInnerCardView() const { return CardView; }

	FVector2D GetCardBodyHitSize() const;
	bool HasCardBodyHitGeometry() const;
	bool IsScreenPositionInsideCardBody(const FVector2D& ScreenPosition) const;
	static FVector2D GetDefaultCardBodyHitSize();

	void SetInteractionCueView(const FWacomFirstPersonCardInteractionCueView& View);
	void ClearInteractionCueView();
	void SetCardDepthView(const FWacomFirstPersonCardDepthView& View);
	void SetCardSurfaceEffectView(const FWacomFirstPersonCardSurfaceEffectView& View);
	bool PrepareCostDigitRewrite(const FWacomCardViewData& InNewData);
	bool PrepareCostDigitRewrite(
		const FWacomCardViewData& InOldData,
		const FWacomCardViewData& InNewData);
	void SetCardDataRewriteView(const FWacomFirstPersonCardDataRewriteView& View);
	void ResetCardDataRewriteView();
	void SetCostDigitPreviewView(const FWacomFirstPersonCardCostPreviewView& View);
	void ResetCostDigitPreviewView();
	void SetEffectBadgeFeedbackConfig(
		const FWacomFirstPersonCardEffectBadgeFeedbackConfig& InConfig);
	/** Pre-creates reusable local digit MIDs while authoritative PaperSprite brushes remain active. */
	void PrimeLocalPresentationMaterials(
		const FWacomFirstPersonCardDataRewriteConfig& DataRewriteConfig,
		const FWacomFirstPersonCardEffectBadgeFeedbackConfig& EffectBadgeConfig);
	void SetEffectBadgeFeedbackView(
		const FWacomFirstPersonCardEffectBadgeFeedbackView& InView);
	void ResetEffectBadgeFeedbackView();
	/** Installs the current surface view at progress zero and returns its paint generation. */
	uint32 BeginSurfacePresentationPreparation(bool bReuseReadyGeneration = false);
	uint32 BeginCostDigitPresentationPreparation();
	uint32 BeginEffectBadgePresentationPreparation();
	void RefreshSurfacePresentationPreparation(uint32 Generation);
	void RefreshCostDigitPresentationPreparation(uint32 Generation);
	void RefreshEffectBadgePresentationPreparation(uint32 Generation);
	bool IsSurfacePresentationMaterialReady(uint32 Generation) const;
	bool IsSurfacePresentationPainted(uint32 Generation) const;
	bool IsCostDigitPresentationMaterialReady(uint32 Generation) const;
	bool IsCostDigitPresentationPainted(uint32 Generation) const;
	bool IsEffectBadgePresentationMaterialReady(uint32 Generation) const;
	bool IsEffectBadgePresentationPainted(uint32 Generation) const;
	void CancelSurfacePresentationPreparation();
	void CancelCostDigitPresentationPreparation();
	void CancelEffectBadgePresentationPreparation();
	void CancelAllPresentationPreparations();
	/** 背包等静态容器请求一次卡面重绘，不改变当前动态表现策略。 */
	void RequestPresentationRender();
	/** CommonUI 过渡期间可关闭 retained rendering，避免把父层透明度烘入缓存。 */
	void SetRetainedRenderingEnabled(bool bEnabled);
	/** 允许或暂停逐帧材质更新；静态模式仍可由 RequestPresentationRender 精确补绘。 */
	void SetRealtimePresentationEnabled(bool bEnabled);
	bool IsRealtimePresentationEnabled() const { return bRealtimePresentationEnabled; }

#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardViewAutomationTestView GetAutomationTestViewForTest() const;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomCardView> CardView;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CardContentSizeBox;

	// The legacy widget name is retained because the existing WBP binds it.
	// Its current responsibility is Retainer-based material contact shadow only.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URetainerBox> Fake3DSurfaceRetainer;

private:
#if WITH_AUTOMATION_TESTS
	friend struct FWacomFirstPersonCardLayerTestAccess;
#endif

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BaseSurfaceEffectMaterialSource;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BaseSurfaceEffectMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ActiveSurfaceEffectMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ActiveSurfaceEffectMaterialSource;

	FWacomCardViewData PendingCardViewData;
	FWacomFirstPersonCardInteractionCueView LastInteractionCueView;
	FWacomFirstPersonCardDepthView LastCardDepthView;
	FWacomFirstPersonCardSurfaceEffectView LastSurfaceEffectView;
	FWacomFirstPersonCardDataRewriteView LastDataRewriteView;
	FWacomFirstPersonCardEffectBadgeFeedbackConfig LastEffectBadgeFeedbackConfig;
	FWacomFirstPersonCardEffectBadgeFeedbackView LastEffectBadgeFeedbackView;
	bool bBaseSurfaceEffectMaterialCached = false;
	bool bRetainedRenderingEnabled = true;
	bool bRetainedRenderingStateApplied = false;
	bool bRealtimePresentationEnabled = true;
	bool bRealtimePresentationStateApplied = false;
	uint32 NextPresentationPreparationGeneration = 1;
	uint32 SurfaceRequestedGeneration = 0;
	uint32 SurfaceMaterialReadyGeneration = 0;
	mutable uint32 SurfacePaintedGeneration = 0;
	uint32 CostDigitRequestedGeneration = 0;
	uint32 CostDigitMaterialReadyGeneration = 0;
	mutable uint32 CostDigitPaintedGeneration = 0;
	uint32 EffectBadgeRequestedGeneration = 0;
	uint32 EffectBadgeMaterialReadyGeneration = 0;
	mutable uint32 EffectBadgePaintedGeneration = 0;
	#if WITH_AUTOMATION_TESTS
	int32 PresentationRenderRequestCount = 0;
	int32 CardViewDataApplyCount = 0;
	int32 RetainedRenderingApplyCount = 0;
	int32 RealtimePresentationApplyCount = 0;
	int32 CardDepthApplyCount = 0;
	#endif

	void EnsureFallbackWidgetTree();
	void ConfigureRetainerCaptureRootClipping();
	void CacheBaseSurfaceEffectMaterial();
	void RestoreBaseSurfaceEffectMaterial();
	void EnsureSurfaceEffectMaterialInstance(UMaterialInterface* Material);
	UMaterialInterface* ResolveActiveSurfaceEffectMaterialSource() const;
	uint32 AllocatePresentationPreparationGeneration();
	void ApplyCardDepthParameters(UMaterialInstanceDynamic& Material) const;
	void ApplyCardUseEffectParameters(
		UMaterialInstanceDynamic& Material,
		const FWacomFirstPersonCardUseEffectView& View) const;
	void ApplyHandTargetImpactParameters(
		UMaterialInstanceDynamic& Material,
		const FWacomFirstPersonCardHandTargetImpactView& View) const;
	void ApplyDrawRevealParameters(
		UMaterialInstanceDynamic& Material,
		const FWacomFirstPersonCardDrawRevealView& View) const;
	void ApplyGainRevealParameters(
		UMaterialInstanceDynamic& Material,
		const FWacomFirstPersonCardGainRevealView& View) const;
	void ApplyRetainSealParameters(
		UMaterialInstanceDynamic& Material,
		const FWacomFirstPersonCardRetainSealView& View) const;
	void ApplyPlayedDissolveParameters(
		UMaterialInstanceDynamic& Material,
		const FWacomFirstPersonCardPlayedDissolveView& View) const;
	FSlateRect ResolveInteractionCueRect(const FGeometry& AllottedGeometry) const;
	static bool ResolveCenteredCardBodyUVRect(
		const FVector2D& SurfaceSize,
		const FVector2D& CardBodySize,
		FLinearColor& OutMin,
		FLinearColor& OutMax);
	bool ResolveCardBodyUVRect(
		FLinearColor& OutMin,
		FLinearColor& OutMax) const;
	void ApplyPendingCardViewData();
};
