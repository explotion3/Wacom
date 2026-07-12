// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class APlayerController;
class UWacomFirstPersonCardViewWidget;
class UWacomFirstPersonCardLayerWidget;

struct FWacomFirstPersonCardLayerOwnerConfig
{
	TSubclassOf<UWacomFirstPersonCardLayerWidget> LayerWidgetClass;
	TSubclassOf<UWacomFirstPersonCardViewWidget> CardViewClass;
	int32 ZOrder = 0;
	uint32 ConfigHash = 0;
	FWacomFirstPersonCardSlotMotionConfig SlotMotionConfig;
	FWacomFirstPersonCardSlotVisualConfig SlotVisualConfig;
	FWacomFirstPersonCardSlotFeedbackConfig SlotFeedbackConfig;
	FWacomFirstPersonCardDragConfig CardDragConfig;
	FWacomFirstPersonCardPileTransferConfig PileTransferConfig;
	bool bLogSlotMotionDiagnostics = false;
	bool bInteractionEnabled = false;
};

struct FWacomFirstPersonCardLayerOwnerUpdateInput
{
	APlayerController* PlayerController = nullptr;
	FWacomFirstPersonCardLayerOwnerConfig Config;
	TArray<FWacomFirstPersonCardLayerSlotView> Slots;
	FWacomFirstPersonCardPresentationAnchorSet PresentationAnchors;
	TFunction<UWacomFirstPersonCardLayerWidget*(
		APlayerController*,
		TSubclassOf<UWacomFirstPersonCardLayerWidget>)> CreateLayerWidget;
	TFunction<void(UWacomFirstPersonCardLayerWidget*)> BindLayerWidget;
	TFunction<void(UWacomFirstPersonCardLayerWidget*, int32)> AddLayerWidgetToViewport;
	TFunction<TArray<FWacomFirstPersonCardLayerTransitionHint>()> ConsumeTransitionHints;
	TFunction<TArray<FWacomFirstPersonCardLayerFeedbackHint>()> ConsumeFeedbackHints;
	TFunction<TArray<FWacomFirstPersonCardPileTransferHint>()> ConsumePileTransferHints;
	bool bCanConsumeTransitionHints = true;
	bool bCanConsumeFeedbackHints = true;
	bool bCanConsumePileTransferHints = true;
};

class FWacomFirstPersonCardLayerOwner
{
public:
	bool Update(
		const FWacomFirstPersonCardLayerOwnerUpdateInput& Input,
		TObjectPtr<UWacomFirstPersonCardLayerWidget>& WidgetRef);

	void Remove(
		TObjectPtr<UWacomFirstPersonCardLayerWidget>& WidgetRef,
		const TFunction<void(UWacomFirstPersonCardLayerWidget*)>& UnbindLayerWidget);

#if WITH_AUTOMATION_TESTS
	int32 GetConfigApplyCountForTest() const { return ConfigApplyCountForTest; }
#endif

private:
	void ApplyConfigIfNeeded(
		UWacomFirstPersonCardLayerWidget& LayerWidget,
		const FWacomFirstPersonCardLayerOwnerConfig& Config);
	void ApplyConfig(
		UWacomFirstPersonCardLayerWidget& LayerWidget,
		const FWacomFirstPersonCardLayerOwnerConfig& Config);
	void ResetConfigState();

	bool bHasAppliedConfig = false;
	uint32 LastAppliedConfigHash = 0;
	TWeakObjectPtr<UClass> LastAppliedCardViewClass;
	FWacomFirstPersonCardDragConfig LastAppliedCardDragConfig;
	bool bLastAppliedLogDiagnostics = false;
	bool bLastAppliedInteractionEnabled = false;
#if WITH_AUTOMATION_TESTS
	int32 ConfigApplyCountForTest = 0;
#endif
};
