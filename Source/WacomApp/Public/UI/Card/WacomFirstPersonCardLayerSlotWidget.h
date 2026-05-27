// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "WacomFirstPersonCardLayerSlotWidget.generated.h"

class UOverlay;
class UWacomCardView;

DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerSlotInteractionNative, const FGuid&, const FWacomFirstPersonCardLayerSlotView&);

/**
 * Single visual card slot inside the first-person card layer.
 *
 * This widget owns pointer interaction only. It does not submit battle commands;
 * callers decide what to do with the forwarded card instance id.
 */
UCLASS()
class WACOMAPP_API UWacomFirstPersonCardLayerSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetCardViewClass(TSubclassOf<UWacomCardView> InCardViewClass);
	void SetSlotView(const FWacomFirstPersonCardLayerSlotView& InSlotView);
	void SetCardLayerInteractionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	UWacomCardView* GetCardView() const { return CardView; }

	const FWacomFirstPersonCardLayerSlotView& GetSlotView() const { return CurrentSlotView; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsHoveredForFirstPersonLayer() const { return bIsHoveredForFirstPersonLayer; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsCardLayerInteractionEnabled() const { return bCardLayerInteractionEnabled; }

#if WITH_AUTOMATION_TESTS
	bool RequestHoverForTest();
	void RequestUnhoverForTest();
	bool RequestClickForTest();
#endif

	FWacomFirstPersonCardLayerSlotInteractionNative OnCardClickedNative;
	FWacomFirstPersonCardLayerSlotInteractionNative OnCardHoveredNative;
	FWacomFirstPersonCardLayerSlotInteractionNative OnCardUnhoveredNative;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UOverlay> RootOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UWacomCardView> CardView;

	UPROPERTY(Transient)
	TSubclassOf<UWacomCardView> CardViewClass;

	UPROPERTY(Transient)
	FWacomFirstPersonCardLayerSlotView CurrentSlotView;

	bool bCardLayerInteractionEnabled = false;
	bool bIsHoveredForFirstPersonLayer = false;

	void EnsureCardView();
	void ApplyCurrentSlotView();
	bool CanInteractWithCurrentSlot() const;
	bool CanClickCurrentSlot() const;
	void SetHoveredForFirstPersonLayer(bool bHovered);
	void UpdateVisibilityForInteractionMode();
};
