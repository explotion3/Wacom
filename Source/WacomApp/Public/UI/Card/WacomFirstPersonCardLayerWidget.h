// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
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
	void SetSlotMotionConfig(const FWacomFirstPersonCardSlotMotionConfig& InConfig);
	void ClearSlotMotionState();
	void SetCardSlots(const TArray<FWacomFirstPersonCardLayerSlotView>& InSlots);
	void SetStaticCardSlots(const TArray<FWacomFirstPersonCardLayerSlotView>& InSlots);
	void SetCardLayerInteractionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	int32 GetCardViewCount() const { return SlotWidgets.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	int32 GetOutgoingCardViewCount() const { return OutgoingSlotWidgets.Num(); }

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

#if WITH_AUTOMATION_TESTS
	void TickSlotMotionForTest(float DeltaTime);
	UWacomFirstPersonCardLayerSlotWidget* FindSlotWidgetByKeyForTest(const FString& SlotKey) const;
	UWacomFirstPersonCardLayerSlotWidget* GetOutgoingSlotWidgetAtForTest(int32 Index) const;
#endif

	FWacomFirstPersonCardLayerInteractionNative OnCardClickedNative;
	FWacomFirstPersonCardLayerInteractionNative OnCardHoveredNative;
	FWacomFirstPersonCardLayerInteractionNative OnCardUnhoveredNative;
	FWacomFirstPersonCardLayerInteractionNative OnHoveredCardSlotUpdatedNative;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>> SlotWidgets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>> OutgoingSlotWidgets;

	UPROPERTY(Transient)
	TArray<FWacomFirstPersonCardLayerSlotView> LastSlots;

	UPROPERTY(Transient)
	TSubclassOf<UWacomCardView> CardViewClass;

	FWacomFirstPersonCardSlotMotionConfig SlotMotionConfig;
	bool bCardLayerInteractionEnabled = false;

	UWacomFirstPersonCardLayerSlotWidget* CreateSlotWidget();
	void ApplyLayerVisibility();
	void BindSlotWidget(UWacomFirstPersonCardLayerSlotWidget* SlotWidget);
	void UnbindSlotWidget(UWacomFirstPersonCardLayerSlotWidget* SlotWidget);
	void RemoveOutgoingFinishedSlots();
	void RemoveUntrackedSlotChildren();
	FString MakeSlotMotionKey(const FWacomFirstPersonCardLayerSlotView& SlotView) const;
	void HandleSlotClicked(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleSlotHovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleSlotUnhovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
};
