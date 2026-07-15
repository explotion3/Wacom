// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomRetainedCardViewWidget.generated.h"

class URetainerBox;
class UWacomCardView;

/**
 * Passive retained-card wrapper for scaled, mostly static card faces.
 *
 * The inner CardView remains the authored layout and data renderer. This wrapper
 * only caches that layout to a RetainerBox and explicitly redraws it when the
 * card data changes; it owns no gameplay input, fake-3D or transition state.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomRetainedCardViewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Card View")
	void SetCardViewData(const FWacomCardViewData& InData);

	UFUNCTION(BlueprintPure, Category = "Wacom|Card View")
	UWacomCardView* GetInnerCardView() const { return CardView; }

	/** Requests one retained redraw without enabling a per-frame render phase. */
	void RequestCardFaceRender();
	/** CommonUI layer transitions use pass-through rendering so inherited fade alpha is not baked into the static cache. */
	void SetRetainedRenderingEnabled(bool bEnabled);
	bool IsRetainedRenderingEnabled() const { return bRetainedRenderingEnabled; }
	bool IsSurfaceFoilEnabled() const { return bEnableSurfaceFoil; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URetainerBox> CardFaceRetainer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomCardView> CardView;

private:
	FWacomCardViewData PendingCardViewData;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Card View|Surface", meta = (ToolTip = "是否允许内层卡面显示动态 SurfaceFoilOverlay。背包静态 Retainer 默认关闭，避免缓存冻结的流光；只有持续刷新或不使用静态 Retainer 的场景才应开启。"))
	bool bEnableSurfaceFoil = false;
	bool bRetainedRenderingEnabled = true;

	void EnsureFallbackWidgetTree();
	void ApplySurfaceFoilPolicy();
	void ApplyRetainedRenderingPolicy();
	void ApplyPendingCardViewData();
};
