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
class UWacomCardView;
#if WITH_AUTOMATION_TESTS
struct FWacomFirstPersonCardLayerTestAccess;
#endif

#if WITH_AUTOMATION_TESTS
struct WACOMAPP_API FWacomFirstPersonCardViewAutomationTestView
{
	float FeedbackOverlayOpacity = 0.0f;
	FLinearColor FeedbackOverlayColor = FLinearColor::Transparent;
	float InteractionFeedbackOpacity = 0.0f;
	EWacomFirstPersonCardInteractionFeedbackKind InteractionFeedbackKind =
		EWacomFirstPersonCardInteractionFeedbackKind::None;
	bool bHasInteractionFeedbackImage = false;
	bool bInteractionFeedbackMaterialConfigured = false;
	bool bInteractionFeedbackMaterialLoaded = false;
	bool bInteractionFeedbackUsesOverrideMaterial = false;
	bool bInteractionFeedbackUsesBrushMaterial = false;
	bool bInteractionFeedbackLayerAboveFeedbackOverlay = false;
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
	bool bRetainedRenderingEnabled = true;
	bool bRealtimePresentationEnabled = true;
	int32 PresentationRenderRequestCount = 0;
	int32 RealtimePresentationApplyCount = 0;
	int32 CardDepthApplyCount = 0;
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

	void SetFeedbackOverlayView(const FLinearColor& Color, float Opacity);
	void SetInteractionFeedbackView(const FWacomFirstPersonCardInteractionFeedbackView& View);
	void ClearInteractionFeedbackView();
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
	void SetEffectBadgeFeedbackView(
		const FWacomFirstPersonCardEffectBadgeFeedbackView& InView);
	void ResetEffectBadgeFeedbackView();
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

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomCardView> CardView;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> FeedbackOverlay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> InteractionFeedbackImage;

	// The legacy widget name is retained because the existing WBP binds it.
	// Its current responsibility is Retainer-based material contact shadow only.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URetainerBox> Fake3DSurfaceRetainer;

private:
#if WITH_AUTOMATION_TESTS
	friend struct FWacomFirstPersonCardLayerTestAccess;
#endif

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> InteractionFeedbackMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> InteractionFeedbackBrushMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> InteractionFeedbackMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BaseSurfaceEffectMaterialSource;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BaseSurfaceEffectMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ActiveSurfaceEffectMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ActiveSurfaceEffectMaterialSource;

	FWacomCardViewData PendingCardViewData;
	FLinearColor LastFeedbackOverlayColor = FLinearColor::Transparent;
	float LastFeedbackOverlayOpacity = 0.0f;
	FWacomFirstPersonCardInteractionFeedbackView LastInteractionFeedbackView;
	float LastInteractionFeedbackOpacity = 0.0f;
	FWacomFirstPersonCardDepthView LastCardDepthView;
	FWacomFirstPersonCardSurfaceEffectView LastSurfaceEffectView;
	FWacomFirstPersonCardDataRewriteView LastDataRewriteView;
	FWacomFirstPersonCardEffectBadgeFeedbackConfig LastEffectBadgeFeedbackConfig;
	FWacomFirstPersonCardEffectBadgeFeedbackView LastEffectBadgeFeedbackView;
	bool bLastInteractionFeedbackUsedOverrideMaterial = false;
	bool bLastInteractionFeedbackUsedBrushMaterial = false;
	bool bBaseSurfaceEffectMaterialCached = false;
	bool bRetainedRenderingEnabled = true;
	bool bRealtimePresentationEnabled = true;
	bool bRealtimePresentationStateApplied = false;
	#if WITH_AUTOMATION_TESTS
	int32 PresentationRenderRequestCount = 0;
	int32 RealtimePresentationApplyCount = 0;
	int32 CardDepthApplyCount = 0;
	#endif

	void EnsureFallbackWidgetTree();
	void ConfigureRetainerCaptureRootClipping();
	void CacheBaseSurfaceEffectMaterial();
	void RestoreBaseSurfaceEffectMaterial();
	void EnsureSurfaceEffectMaterialInstance(UMaterialInterface* Material);
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
	UImage* GetInteractionFeedbackImage() const;
	void CacheInteractionFeedbackBrushMaterial();
	void EnsureInteractionFeedbackMaterialInstance(const FWacomFirstPersonCardInteractionFeedbackView& View);
	UMaterialInterface* ResolveInteractionFeedbackMaterial(
		const FWacomFirstPersonCardInteractionFeedbackView& View,
		bool& bOutUsesOverrideMaterial,
		bool& bOutUsesBrushMaterial) const;
	bool ResolveDrawRevealCardBodyUVRect(
		FLinearColor& OutMin,
		FLinearColor& OutMax) const;
	void ApplyPendingCardViewData();
};
