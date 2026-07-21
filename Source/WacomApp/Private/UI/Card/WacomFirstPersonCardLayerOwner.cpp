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
	if (Input.bCanConsumeFeedbackHints && Input.ConsumeFeedbackHints)
	{
		WidgetRef->SetCardFeedbackHints(Input.ConsumeFeedbackHints());
	}
	WidgetRef->SetCardSlots(Input.Slots);
	if (Input.bCanConsumePileTransferHints && Input.ConsumePileTransferHints)
	{
		// Outgoing slots must exist before card-slot sourced glyph transfers resolve their origins.
		WidgetRef->SetPileTransferHints(Input.ConsumePileTransferHints());
	}
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
		|| !AreSlotRuntimeConfigsEquivalent(
			LastAppliedSlotRuntimeConfig,
			Config.SlotRuntimeConfig)
		|| bLastAppliedLogDiagnostics != Config.bLogSlotMotionDiagnostics
		|| bLastAppliedInteractionEnabled != Config.bInteractionEnabled
		|| bLastAppliedPresentationVisible != Config.bPresentationVisible;
	if (bConfigChanged)
	{
		ApplyConfig(LayerWidget, Config);
	}
}

void FWacomFirstPersonCardLayerOwner::ApplyConfig(
	UWacomFirstPersonCardLayerWidget& LayerWidget,
	const FWacomFirstPersonCardLayerOwnerConfig& Config)
{
	LayerWidget.SetSlotRuntimeConfig(Config.SlotRuntimeConfig);
	LayerWidget.SetPileTransferConfig(Config.PileTransferConfig);
	LayerWidget.SetLogSlotMotionDiagnostics(Config.bLogSlotMotionDiagnostics);
	LayerWidget.SetCardViewClass(Config.CardViewClass);
	LayerWidget.SetCardLayerInteractionEnabled(Config.bInteractionEnabled);
	LayerWidget.SetCardLayerPresentationVisible(Config.bPresentationVisible);

	bHasAppliedConfig = true;
	LastAppliedConfigHash = Config.ConfigHash;
	LastAppliedCardViewClass = Config.CardViewClass.Get();
	LastAppliedSlotRuntimeConfig = Config.SlotRuntimeConfig;
	bLastAppliedLogDiagnostics = Config.bLogSlotMotionDiagnostics;
	bLastAppliedInteractionEnabled = Config.bInteractionEnabled;
	bLastAppliedPresentationVisible = Config.bPresentationVisible;
#if WITH_AUTOMATION_TESTS
	++ConfigApplyCountForTest;
#endif
}

void FWacomFirstPersonCardLayerOwner::ResetConfigState()
{
	bHasAppliedConfig = false;
	LastAppliedConfigHash = 0;
	LastAppliedCardViewClass.Reset();
	LastAppliedSlotRuntimeConfig = FWacomFirstPersonCardSlotRuntimeConfig();
	bLastAppliedLogDiagnostics = false;
	bLastAppliedInteractionEnabled = false;
	bLastAppliedPresentationVisible = true;
}
