// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

#if WITH_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardReleaseReentrancyTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.AcceptedReleaseSurvivesSynchronousRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardReleaseReentrancyTest::RunTest(const FString& /*Parameters*/)
{
	UWacomFirstPersonCardLayerSlotWidget* Slot =
		NewObject<UWacomFirstPersonCardLayerSlotWidget>();
	FWacomFirstPersonCardInteractionFeedbackConfig FeedbackConfig;
	FeedbackConfig.bEnabled = true;
	FeedbackConfig.PressedOutDurationSeconds = 0.08f;
	FeedbackConfig.DenyDuration = 0.10f;
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*Slot, FeedbackConfig);
	Slot->SetCardLayerInteractionEnabled(true);

	FWacomFirstPersonCardLayerSlotView SlotView;
	SlotView.Entry.CardInstanceId = FGuid::NewGuid();
	SlotView.Entry.bIsPlayable = true;
	SlotView.Entry.InteractionIntent =
		EWacomFirstPersonCardInteractionIntent::AimWorldTarget;
	SlotView.ScreenPosition = FVector2D(400.0f, 500.0f);
	SlotView.WidgetPosition = SlotView.ScreenPosition;
	SlotView.SnappedWidgetPosition = SlotView.ScreenPosition;
	SlotView.bProjected = true;
	Slot->SetSlotViewImmediate(SlotView);
	Slot->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerTestAccess::SetGestureState(
		*Slot,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);

	const FWacomInteractionTargetHandle ValidTarget =
		FWacomInteractionTargetHandle::ForWorldTarget(
			FGuid::NewGuid(),
			Slot,
			FVector::ZeroVector,
			FVector2D(700.0f, 260.0f));
	Slot->SetCardDragFeedbackTarget(
		ValidTarget,
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	Slot->OnCardDragReleasedNative.AddLambda(
		[Slot](const FGuid&, const FWacomFirstPersonCardDragView&)
		{
			// Mimic the synchronous HUD refresh performed by a successful command.
			Slot->SetCardLayerInteractionEnabled(false);
		});

	TestTrue(
		TEXT("Release route handles the accepted drag"),
		FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(
			*Slot,
			FVector2D(700.0f, 260.0f)));
	const FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Slot);
	TestFalse(TEXT("Synchronous refresh cannot create a false Deny"), View.bDenyFeedbackActive);
	TestEqual(
		TEXT("Accepted release has no optimistic cue"),
		View.InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardReleaseRequiresResolvedInvalidTargetTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.ReleaseDenyRequiresResolvedInvalidTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardReleaseRequiresResolvedInvalidTargetTest::RunTest(
	const FString& /*Parameters*/)
{
	auto MakeSlot = []()
	{
		UWacomFirstPersonCardLayerSlotWidget* Slot =
			NewObject<UWacomFirstPersonCardLayerSlotWidget>();
		FWacomFirstPersonCardInteractionFeedbackConfig FeedbackConfig;
		FeedbackConfig.bEnabled = true;
		FeedbackConfig.DenyDuration = 0.20f;
		FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*Slot, FeedbackConfig);
		Slot->SetCardLayerInteractionEnabled(true);
		FWacomFirstPersonCardLayerSlotView SlotView;
		SlotView.Entry.CardInstanceId = FGuid::NewGuid();
		SlotView.Entry.bIsPlayable = true;
		SlotView.Entry.InteractionIntent =
			EWacomFirstPersonCardInteractionIntent::AimWorldTarget;
		SlotView.ScreenPosition = FVector2D(400.0f, 500.0f);
		SlotView.WidgetPosition = SlotView.ScreenPosition;
		SlotView.SnappedWidgetPosition = SlotView.ScreenPosition;
		SlotView.bProjected = true;
		Slot->SetSlotViewImmediate(SlotView);
		Slot->SetCardLayerInteractionEnabled(true);
		FWacomFirstPersonCardLayerTestAccess::SetGestureState(
			*Slot,
			EWacomFirstPersonCardGestureState::AimingTargetedCard);
		return Slot;
	};

	UWacomFirstPersonCardLayerSlotWidget* BlankSlot = MakeSlot();
	BlankSlot->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle(),
		false,
		EWacomFirstPersonCardDragTargetFeedbackState::Invalid);
	TestTrue(TEXT("Blank release is handled"),
		FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(
			*BlankSlot,
			FVector2D(700.0f, 260.0f)));
	TestFalse(TEXT("Blank release is neutral"),
		FWacomFirstPersonCardLayerTestAccess::View(*BlankSlot).bDenyFeedbackActive);

	UWacomFirstPersonCardLayerSlotWidget* InvalidSlot = MakeSlot();
	const FWacomInteractionTargetHandle InvalidTarget =
		FWacomInteractionTargetHandle::ForWorldTarget(
			FGuid::NewGuid(),
			InvalidSlot,
			FVector::ZeroVector,
			FVector2D(700.0f, 260.0f));
	InvalidSlot->SetCardDragFeedbackTarget(
		InvalidTarget,
		false,
		EWacomFirstPersonCardDragTargetFeedbackState::Invalid);
	TestTrue(TEXT("Resolved invalid release is handled"),
		FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(
			*InvalidSlot,
			FVector2D(700.0f, 260.0f)));
	const FWacomFirstPersonCardSlotAutomationTestView InvalidView =
		FWacomFirstPersonCardLayerTestAccess::View(*InvalidSlot);
	TestTrue(TEXT("Resolved invalid release triggers Deny"), InvalidView.bDenyFeedbackActive);
	TestEqual(TEXT("Deny keeps source-card decorative cues disabled"),
		InvalidView.InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::None);
	return true;
}

#endif
