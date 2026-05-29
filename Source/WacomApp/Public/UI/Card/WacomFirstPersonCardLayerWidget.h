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

struct FWacomFirstPersonCardLayerResolvedTransitionHint
{
	EWacomFirstPersonCardSlotTransitionKind TransitionKind = EWacomFirstPersonCardSlotTransitionKind::Default;
	bool bPlayCommitFeedback = false;
	bool bHasPlayedExitTargetWidgetPosition = false;
	FVector2D PlayedExitTargetWidgetPosition = FVector2D::ZeroVector;
};

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
	void SetSlotFeedbackConfig(const FWacomFirstPersonCardSlotFeedbackConfig& InConfig);
	void ClearSlotMotionState();
	void SetCardTransitionHints(const TArray<FWacomFirstPersonCardLayerTransitionHint>& InHints);
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

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer|Debug")
	FWacomFirstPersonCardLayerMotionDebugView GetSlotMotionDebugView() const { return LastMotionDebugView; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer|Debug")
	FString GetSlotMotionDebugSummary() const;

	void SetLogSlotMotionDiagnostics(bool bEnabled) { bLogSlotMotionDiagnostics = bEnabled; }

#if WITH_AUTOMATION_TESTS
	void TickSlotMotionForTest(float DeltaTime);
	UWacomFirstPersonCardLayerSlotWidget* FindSlotWidgetByKeyForTest(const FString& SlotKey) const;
	UWacomFirstPersonCardLayerSlotWidget* GetOutgoingSlotWidgetAtForTest(int32 Index) const;
	const FWacomFirstPersonCardSlotMotionConfig& GetSlotMotionConfigForTest() const { return SlotMotionConfig; }
	const FWacomFirstPersonCardSlotFeedbackConfig& GetSlotFeedbackConfigForTest() const { return SlotFeedbackConfig; }
	TSubclassOf<UWacomCardView> GetCardViewClassForTest() const { return CardViewClass; }
	void AddUntrackedSlotChildForTest();
	void SetViewportSizeOverrideForTest(const FVector2D& WidgetViewportSize);
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
	FWacomFirstPersonCardSlotFeedbackConfig SlotFeedbackConfig;
	FWacomFirstPersonCardLayerMotionDebugView LastMotionDebugView;
	TMap<FString, FWacomFirstPersonCardLayerResolvedTransitionHint> PendingTransitionHintsByKey;
	bool bCardLayerInteractionEnabled = false;
	bool bLogSlotMotionDiagnostics = false;
#if WITH_AUTOMATION_TESTS
	TOptional<FVector2D> WidgetViewportSizeOverrideForTest;
#endif

	UWacomFirstPersonCardLayerSlotWidget* CreateSlotWidget();
	void ApplyLayerVisibility();
	void BindSlotWidget(UWacomFirstPersonCardLayerSlotWidget* SlotWidget);
	void UnbindSlotWidget(UWacomFirstPersonCardLayerSlotWidget* SlotWidget);
	int32 RemoveOutgoingFinishedSlots();
	int32 RemoveUntrackedSlotChildren();
	void EnforceOutgoingSlotLimit();
	void RepairSlotMotionInvariants();
	void RefreshSlotMotionDebugCounts();
	int32 CountRootCanvasSlotChildren() const;
	int32 CountMotionTickSlots() const;
	void ReportSlotMotionDiagnosticsIfNeeded();
	FString MakeSlotMotionKey(const FWacomFirstPersonCardLayerSlotView& SlotView) const;
	TOptional<FWacomFirstPersonCardTransitionMotionProfile> GetEnterProfileForTransition(
		EWacomFirstPersonCardSlotTransitionKind TransitionKind,
		const FWacomFirstPersonCardLayerSlotView& TargetSlotView) const;
	TOptional<FWacomFirstPersonCardTransitionMotionProfile> GetExitProfileForTransition(
		const FWacomFirstPersonCardLayerResolvedTransitionHint& TransitionHint,
		const FWacomFirstPersonCardLayerSlotView& VisualSlotView) const;
	bool ResolveViewportAnchorPosition(
		const FVector2D& NormalizedViewportAnchor,
		FVector2D& OutWidgetPosition) const;
	void HandleSlotClicked(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleSlotHovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleSlotUnhovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
};
