// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardLayerOwner.h"

#include "WacomFirstPersonCardLayerConfigUtils.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"

bool FWacomFirstPersonCardLayerOwner::Update(
	const FWacomFirstPersonCardLayerOwnerUpdateInput& Input,
	TObjectPtr<UWacomFirstPersonCardLayerWidget>& WidgetRef)
{
	if (!Input.PlayerController || !Input.CreateLayerWidget || !Input.AddLayerWidgetToViewport)
	{
		return false;
	}

	if (!WidgetRef)
	{
		TSubclassOf<UWacomFirstPersonCardLayerWidget> LayerClass = Input.Config.LayerWidgetClass;
		if (!LayerClass)
		{
			LayerClass = UWacomFirstPersonCardLayerWidget::StaticClass();
		}

		WidgetRef = Input.CreateLayerWidget(Input.PlayerController, LayerClass);
		if (WidgetRef)
		{
			WidgetRef->SetVisibility(ESlateVisibility::HitTestInvisible);
			ApplyConfig(*WidgetRef, Input.Config);
			if (Input.BindLayerWidget)
			{
				Input.BindLayerWidget(WidgetRef);
			}
			Input.AddLayerWidgetToViewport(WidgetRef, Input.Config.ZOrder);
		}
	}

	if (!WidgetRef)
	{
		return false;
	}

	ApplyConfigIfNeeded(*WidgetRef, Input.Config);
	WidgetRef->SetPresentationAnchors(Input.PresentationAnchors);

	if (Input.bCanConsumeTransitionHints && Input.ConsumeTransitionHints)
	{
		const TArray<FWacomFirstPersonCardLayerTransitionHint> TransitionHints =
			Input.ConsumeTransitionHints();
		WidgetRef->SetCardTransitionHints(TransitionHints);
	}

	WidgetRef->SetCardSlots(Input.Slots);
	return true;
}

void FWacomFirstPersonCardLayerOwner::Remove(
	TObjectPtr<UWacomFirstPersonCardLayerWidget>& WidgetRef,
	const TFunction<void(UWacomFirstPersonCardLayerWidget*)>& UnbindLayerWidget)
{
	if (WidgetRef)
	{
		WidgetRef->ClearSlotMotionState();
		if (UnbindLayerWidget)
		{
			UnbindLayerWidget(WidgetRef);
		}
		WidgetRef->RemoveFromParent();
		WidgetRef = nullptr;
	}

	ResetConfigState();
}

void FWacomFirstPersonCardLayerOwner::ApplyConfigIfNeeded(
	UWacomFirstPersonCardLayerWidget& LayerWidget,
	const FWacomFirstPersonCardLayerOwnerConfig& Config)
{
	const bool bConfigChanged =
		!bHasAppliedConfig
		|| LastAppliedConfigHash != Config.ConfigHash
		|| LastAppliedCardViewClass.Get() != Config.CardViewClass.Get()
		|| !AreCardDragConfigsEquivalent(LastAppliedCardDragConfig, Config.CardDragConfig)
		|| bLastAppliedLogDiagnostics != Config.bLogSlotMotionDiagnostics
		|| bLastAppliedInteractionEnabled != Config.bInteractionEnabled;
	if (bConfigChanged)
	{
		ApplyConfig(LayerWidget, Config);
	}
}

void FWacomFirstPersonCardLayerOwner::ApplyConfig(
	UWacomFirstPersonCardLayerWidget& LayerWidget,
	const FWacomFirstPersonCardLayerOwnerConfig& Config)
{
	LayerWidget.SetSlotMotionConfig(Config.SlotMotionConfig);
	LayerWidget.SetSlotVisualConfig(Config.SlotVisualConfig);
	LayerWidget.SetSlotFeedbackConfig(Config.SlotFeedbackConfig);
	LayerWidget.SetCardDragConfig(Config.CardDragConfig);
	LayerWidget.SetLogSlotMotionDiagnostics(Config.bLogSlotMotionDiagnostics);
	LayerWidget.SetCardViewClass(Config.CardViewClass);
	LayerWidget.SetCardLayerInteractionEnabled(Config.bInteractionEnabled);

	bHasAppliedConfig = true;
	LastAppliedConfigHash = Config.ConfigHash;
	LastAppliedCardViewClass = Config.CardViewClass.Get();
	LastAppliedCardDragConfig = Config.CardDragConfig;
	bLastAppliedLogDiagnostics = Config.bLogSlotMotionDiagnostics;
	bLastAppliedInteractionEnabled = Config.bInteractionEnabled;
#if WITH_AUTOMATION_TESTS
	++ConfigApplyCountForTest;
#endif
}

void FWacomFirstPersonCardLayerOwner::ResetConfigState()
{
	bHasAppliedConfig = false;
	LastAppliedConfigHash = 0;
	LastAppliedCardViewClass.Reset();
	LastAppliedCardDragConfig = FWacomFirstPersonCardDragConfig();
	bLastAppliedLogDiagnostics = false;
	bLastAppliedInteractionEnabled = false;
}
