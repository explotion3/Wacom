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
	bool bHasFake3DSurfaceRetainer = false;
	bool bFake3DEffectMaterialReady = false;
	bool bUsingSurfaceEffectMaterial = false;
	bool bBaseSurfaceEffectMaterialCached = false;
	bool bRetainerCaptureRootUsesIndependentClipping = false;
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
	bool bLastInteractionFeedbackUsedOverrideMaterial = false;
	bool bLastInteractionFeedbackUsedBrushMaterial = false;
	bool bBaseSurfaceEffectMaterialCached = false;

	void EnsureFallbackWidgetTree();
	void ConfigureRetainerCaptureRootClipping();
	void CacheBaseSurfaceEffectMaterial();
	void RestoreBaseSurfaceEffectMaterial();
	void EnsureSurfaceEffectMaterialInstance(UMaterialInterface* Material);
	void ApplyCardDepthParameters(UMaterialInstanceDynamic& Material) const;
	void ApplyCardUseEffectParameters(
		UMaterialInstanceDynamic& Material,
		const FWacomFirstPersonCardUseEffectView& View) const;
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
	void ApplyPendingCardViewData();
};
