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
	FWacomFirstPersonCardSlotFeedbackConfig FeedbackConfig;
	FeedbackConfig.bEnabled = true;
	FeedbackConfig.ConfirmDuration = 0.10f;
	FeedbackConfig.DenyDuration = 0.10f;
	Slot->SetSlotFeedbackConfig(FeedbackConfig);
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
	TestTrue(TEXT("Accepted release keeps confirm semantics"), View.bConfirmFeedbackActive);
	return true;
}

#endif
