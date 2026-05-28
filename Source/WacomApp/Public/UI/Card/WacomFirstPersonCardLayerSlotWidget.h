// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "WacomFirstPersonCardLayerSlotWidget.generated.h"

class UOverlay;
class UWacomCardView;

DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerSlotInteractionNative, const FGuid&, const FWacomFirstPersonCardLayerSlotView&);

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardSlotMotionConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float MotionSpeed = 26.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float OpacitySpeed = 18.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D EnterOffsetPixels = FVector2D(0.0f, 48.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float EnterOpacity = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D ExitOffsetPixels = FVector2D(0.0f, 36.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ExitDuration = 0.16f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ResetDistancePixels = 420.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnableEventAwareTransitions = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D DrawnEnterOffsetPixels = FVector2D(0.0f, 96.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D GainedEnterOffsetPixels = FVector2D(0.0f, -120.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D PlayedExitOffsetPixels = FVector2D(0.0f, -120.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D DiscardedExitOffsetPixels = FVector2D(0.0f, 120.0f);
};

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
	void SetSlotViewImmediate(const FWacomFirstPersonCardLayerSlotView& InSlotView);
	void BeginSlotMotion(const FWacomFirstPersonCardLayerSlotView& InTargetSlotView, bool bTreatAsNewSlot);
	void BeginSlotMotionWithEnterOffset(
		const FWacomFirstPersonCardLayerSlotView& InTargetSlotView,
		bool bTreatAsNewSlot,
		const TOptional<FVector2D>& EnterOffsetOverride);
	void BeginExitMotion(const FWacomFirstPersonCardLayerSlotView& InExitTargetSlotView);
	void BeginExitMotionWithOffset(
		const FWacomFirstPersonCardLayerSlotView& InExitTargetSlotView,
		const TOptional<FVector2D>& ExitOffsetOverride);
	void SetSlotMotionConfig(const FWacomFirstPersonCardSlotMotionConfig& InConfig);
	void SetCardLayerInteractionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	UWacomCardView* GetCardView() const { return CardView; }

	const FWacomFirstPersonCardLayerSlotView& GetSlotView() const { return CurrentSlotView; }
	const FWacomFirstPersonCardLayerSlotView& GetVisualSlotView() const { return VisualSlotView; }
	const FString& GetSlotMotionKey() const { return SlotMotionKey; }
	void SetSlotMotionKey(const FString& InKey) { SlotMotionKey = InKey; }

	bool IsExitingForFirstPersonLayer() const { return bIsExitingForFirstPersonLayer; }
	bool IsExitMotionFinished() const;
	bool WantsSlotMotionTick() const { return bWantsSlotMotionTick; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsHoveredForFirstPersonLayer() const { return bIsHoveredForFirstPersonLayer; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsCardLayerInteractionEnabled() const { return bCardLayerInteractionEnabled; }

#if WITH_AUTOMATION_TESTS
	bool RequestHoverForTest();
	void RequestUnhoverForTest();
	bool RequestClickForTest();
	void TickSlotMotionForTest(float DeltaTime);
#endif

	FWacomFirstPersonCardLayerSlotInteractionNative OnCardClickedNative;
	FWacomFirstPersonCardLayerSlotInteractionNative OnCardHoveredNative;
	FWacomFirstPersonCardLayerSlotInteractionNative OnCardUnhoveredNative;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
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

	UPROPERTY(Transient)
	FWacomFirstPersonCardLayerSlotView TargetSlotView;

	UPROPERTY(Transient)
	FWacomFirstPersonCardLayerSlotView VisualSlotView;

	FWacomFirstPersonCardSlotMotionConfig SlotMotionConfig;
	FString SlotMotionKey;
	float ExitMotionElapsedSeconds = 0.0f;
	bool bCardLayerInteractionEnabled = false;
	bool bIsHoveredForFirstPersonLayer = false;
	bool bHasVisualSlotView = false;
	bool bIsExitingForFirstPersonLayer = false;
	bool bWantsSlotMotionTick = false;

	void EnsureCardView();
	void ApplyCurrentSlotView();
	void ApplyVisualSlotView();
	void ApplySlotViewToWidget(const FWacomFirstPersonCardLayerSlotView& SlotView);
	bool CanInteractWithCurrentSlot() const;
	bool CanClickCurrentSlot() const;
	void SetHoveredForFirstPersonLayer(bool bHovered);
	void UpdateVisibilityForInteractionMode();
	void SetTickEnabledForMotion(bool bEnabled);
	static FWacomFirstPersonCardLayerSlotView LerpSlotView(
		const FWacomFirstPersonCardLayerSlotView& From,
		const FWacomFirstPersonCardLayerSlotView& To,
		float MotionAlpha,
		float OpacityAlpha);
};
