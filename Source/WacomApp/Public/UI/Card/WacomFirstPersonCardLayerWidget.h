// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "WacomFirstPersonCardLayerWidget.generated.h"

class UCanvasPanel;
class UWacomCardView;
class UWacomFirstPersonCardLayerSlotWidget;

DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerInteractionNative, const FGuid&, const FWacomFirstPersonCardLayerSlotView&);

/**
 * HUD card layer driven by first-person card anchor projection.
 * Renders UWacomCardView-compatible card face widgets, may opt into slot-level
 * hover/click intent, and never owns battle or Run commands.
 */
UCLASS()
class WACOMAPP_API UWacomFirstPersonCardLayerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetCardViewClass(TSubclassOf<UWacomCardView> InCardViewClass);
	void SetCardSlots(const TArray<FWacomFirstPersonCardLayerSlotView>& InSlots);
	void SetStaticCardSlots(const TArray<FWacomFirstPersonCardLayerSlotView>& InSlots);
	void SetCardLayerInteractionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	int32 GetCardViewCount() const { return SlotWidgets.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	UWacomCardView* GetCardViewAt(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	UWacomFirstPersonCardLayerSlotWidget* GetSlotWidgetAt(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsCardSlotVisible(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	FWidgetTransform GetCardRenderTransformAt(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	float GetCardRenderOpacityAt(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	int32 GetCardZOrderAt(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsCardLayerInteractionEnabled() const { return bCardLayerInteractionEnabled; }

	FWacomFirstPersonCardLayerInteractionNative OnCardClickedNative;
	FWacomFirstPersonCardLayerInteractionNative OnCardHoveredNative;
	FWacomFirstPersonCardLayerInteractionNative OnCardUnhoveredNative;
	FWacomFirstPersonCardLayerInteractionNative OnHoveredCardSlotUpdatedNative;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>> SlotWidgets;

	UPROPERTY(Transient)
	TArray<FWacomFirstPersonCardLayerSlotView> LastSlots;

	UPROPERTY(Transient)
	TSubclassOf<UWacomCardView> CardViewClass;

	bool bCardLayerInteractionEnabled = false;

	void EnsureSlotWidgetCount(int32 DesiredCount);
	UWacomFirstPersonCardLayerSlotWidget* CreateSlotWidget(int32 Index);
	void ApplyLayerVisibility();
	void BindSlotWidget(UWacomFirstPersonCardLayerSlotWidget* SlotWidget);
	void HandleSlotClicked(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleSlotHovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleSlotUnhovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
};
