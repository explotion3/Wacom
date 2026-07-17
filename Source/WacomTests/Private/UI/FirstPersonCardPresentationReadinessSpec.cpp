// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Materials/MaterialInstanceConstant.h"
#include "Sound/SoundWave.h"
#include "UI/Card/WacomFirstPersonCardDrawRevealStyle.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "../../../WacomApp/Private/UI/Card/WacomFirstPersonCardPresentationReadinessGate.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardPresentationReadinessSpec
{
	FWacomFirstPersonCardLayerSlotView MakeSlot(const FGuid& CardInstanceId)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.Zone = EHandZone::Both;
		Slot.Entry.bIsPlayable = true;
		Slot.ScreenPosition = FVector2D(420.0f, 360.0f);
		Slot.WidgetPosition = Slot.ScreenPosition;
		Slot.SnappedWidgetPosition = Slot.ScreenPosition;
		Slot.InputHitCenter = Slot.ScreenPosition;
		Slot.InputHitScale = 1.0f;
		Slot.RenderScale = 1.0f;
		Slot.RenderOpacity = 1.0f;
		Slot.bProjected = true;
		return Slot;
	}

	UWacomFirstPersonCardLayerSlotWidget* MakeDrawWidget()
	{
		UWacomFirstPersonCardLayerSlotWidget* Widget =
			NewObject<UWacomFirstPersonCardLayerSlotWidget>();
		FWacomFirstPersonCardSlotMotionConfig MotionConfig;
		MotionConfig.bEnabled = true;
		MotionConfig.EnterOpacity = 1.0f;
		Widget->SetSlotMotionConfig(MotionConfig);

		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.DrawReveal.bEnabled = true;
		VisualConfig.DrawReveal.Style.SurfaceEffectMaterialInstance =
			NewObject<UMaterialInstanceConstant>();
		Widget->SetSlotVisualConfig(VisualConfig);
		return Widget;
	}

	UWacomFirstPersonCardLayerSlotWidget* MakeHandTargetWidget()
	{
		UWacomFirstPersonCardLayerSlotWidget* Widget =
			NewObject<UWacomFirstPersonCardLayerSlotWidget>();
		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.HandTargetImpact.bEnabled = true;
		VisualConfig.HandTargetImpact.Style.SurfaceEffectMaterialInstance =
			NewObject<UMaterialInstanceConstant>();
		VisualConfig.HandTargetImpact.Style.PreviewFadeInSeconds = 0.10f;
		VisualConfig.HandTargetImpact.Style.CommitDurationSeconds = 0.29f;
		Widget->SetSlotVisualConfig(VisualConfig);
		Widget->SetSlotViewImmediate(MakeSlot(FGuid::NewGuid()));
		return Widget;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardPresentationReadinessGateStateTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationReadiness.MaterialPaintAndTimeout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardPresentationReadinessGateStateTest::RunTest(
	const FString& /*Parameters*/)
{
	FWacomFirstPersonCardPresentationReadinessGate SurfaceGate;
	SurfaceGate.Begin(7);
	TestEqual(
		TEXT("A new gate waits for a material"),
		SurfaceGate.Poll(0.10f, false, false),
		EWacomFirstPersonCardPresentationReadinessPollResult::Waiting);
	TestEqual(
		TEXT("A material without Paint is not ready"),
		SurfaceGate.Poll(0.10f, true, false),
		EWacomFirstPersonCardPresentationReadinessPollResult::Waiting);
	TestEqual(
		TEXT("The first real Paint produces one start edge"),
		SurfaceGate.Poll(0.10f, true, true),
		EWacomFirstPersonCardPresentationReadinessPollResult::BecameReady);
	TestEqual(
		TEXT("Subsequent frames remain ready without another edge"),
		SurfaceGate.Poll(1.0f, true, true),
		EWacomFirstPersonCardPresentationReadinessPollResult::Ready);

	FWacomFirstPersonCardPresentationReadinessGate CostGate;
	FWacomFirstPersonCardPresentationReadinessGate BadgeGate;
	CostGate.Begin(11);
	BadgeGate.Begin(12);
	TestEqual(
		TEXT("Cost can become ready while Badge remains pending"),
		CostGate.Poll(0.10f, true, true),
		EWacomFirstPersonCardPresentationReadinessPollResult::BecameReady);
	TestEqual(
		TEXT("Badge retains its independent generation"),
		BadgeGate.Poll(0.10f, true, false),
		EWacomFirstPersonCardPresentationReadinessPollResult::Waiting);

	FWacomFirstPersonCardPresentationReadinessGate TimeoutGate;
	TimeoutGate.Begin(21);
	TestEqual(
		TEXT("A cold resource times out before the command watchdog"),
		TimeoutGate.Poll(0.76f, false, false),
		EWacomFirstPersonCardPresentationReadinessPollResult::Failed);
	TimeoutGate.Reset();
	TestEqual(
		TEXT("Reset cancels the generation"),
		TimeoutGate.GetState(),
		EWacomFirstPersonCardPresentationReadinessState::Inactive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardPresentationReadinessDrawStartTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationReadiness.DrawWaitsForPaintAndStartsAtZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardPresentationReadinessDrawStartTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardPresentationReadinessSpec;

	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeDrawWidget();
	FWacomFirstPersonCardTransitionMotionProfile Profile;
	Profile.DurationSeconds = 1.0f;
	Profile.TransitionKind = EWacomFirstPersonCardSlotTransitionKind::Drawn;
	Profile.StartSound = NewObject<USoundWave>();
	Widget->BeginSlotMotionWithEnterProfile(
		MakeSlot(FGuid::NewGuid()),
		true,
		Profile);

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotionWithoutPresentationPaint(
		*Widget,
		0.40f);
	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("The pending gate blocks the command phase"), Widget->HasActivePresentationPlayback());
	TestTrue(TEXT("Playback reports that readiness freezes it"), View.bPlaybackFrozenForReadiness);
	TestTrue(TEXT("Enter elapsed time remains at zero before Paint"), FMath::IsNearlyZero(View.EnterTransitionElapsedSeconds));
	TestTrue(TEXT("Draw reveal remains on its waiting back view"), View.bDrawRevealWaiting);
	TestEqual(TEXT("No start sound is consumed before Paint"), View.EnterTransitionSoundRequestCount, 0);

	FWacomFirstPersonCardLayerTestAccess::AcknowledgePendingPresentationPaint(*Widget);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotionWithoutPresentationPaint(
		*Widget,
		1.0f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("The ready edge releases the freeze"), View.bPlaybackFrozenForReadiness);
	TestTrue(TEXT("A large first Delta does not advance Enter"), FMath::IsNearlyZero(View.EnterTransitionElapsedSeconds));
	TestFalse(TEXT("Draw starts on the ready edge"), View.bDrawRevealWaiting);
	TestEqual(TEXT("The start sound request is consumed exactly once"), View.EnterTransitionSoundRequestCount, 1);

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotionWithoutPresentationPaint(
		*Widget,
		0.25f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Only the next Tick advances authored time"), FMath::IsNearlyEqual(View.EnterTransitionElapsedSeconds, 0.25f));
	TestTrue(TEXT("Reveal follows the same normalized Enter progress"), FMath::IsNearlyEqual(View.DrawRevealProgress, 0.25f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardPresentationReadinessPreviewPrewarmTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationReadiness.HandTargetPreviewPrewarmsCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardPresentationReadinessPreviewPrewarmTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardPresentationReadinessSpec;

	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeHandTargetWidget();
	Widget->SetCardDragTargetFocusFeedback(
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget,
		true);
	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	const uint32 PreviewGeneration = View.SurfaceReadinessGeneration;
	TestTrue(TEXT("Preview installs a surface preparation generation"), PreviewGeneration != 0);
	TestFalse(TEXT("Preview preparation is non-blocking"), Widget->HasActivePresentationPlayback());

	FWacomFirstPersonCardLayerTestAccess::AcknowledgePendingPresentationPaint(*Widget);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotionWithoutPresentationPaint(*Widget, 0.0f);
	Widget->TriggerHandTargetImpactFeedback();
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestEqual(
		TEXT("Commit reuses the preview generation after its real Paint"),
		View.SurfaceReadinessGeneration,
		PreviewGeneration);
	TestTrue(TEXT("Commit remains a blocking semantic playback"), Widget->HasActivePresentationPlayback());
	Widget->ForceCompletePresentationPlayback();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardPresentationReadinessTimeoutFallbackTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationReadiness.DrawTimeoutFallsBackWithoutLateSound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardPresentationReadinessTimeoutFallbackTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardPresentationReadinessSpec;

	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeDrawWidget();
	FWacomFirstPersonCardTransitionMotionProfile Profile;
	Profile.DurationSeconds = 1.0f;
	Profile.TransitionKind = EWacomFirstPersonCardSlotTransitionKind::Drawn;
	Profile.StartSound = NewObject<USoundWave>();
	Widget->BeginSlotMotionWithEnterProfile(MakeSlot(FGuid::NewGuid()), true, Profile);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotionWithoutPresentationPaint(*Widget, 0.76f);
	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestEqual(TEXT("Timeout is recorded once"), View.PresentationReadinessTimeoutCount, 1);
	TestEqual(TEXT("Timeout selects one fallback"), View.PresentationReadinessFallbackCount, 1);
	TestFalse(TEXT("Failed reveal is removed"), View.bDrawRevealPlaybackActive);
	TestTrue(TEXT("Fallback keeps the spatial Enter at its origin"), FMath::IsNearlyZero(View.EnterTransitionElapsedSeconds));

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotionWithoutPresentationPaint(*Widget, 0.10f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("The original spatial Enter resumes after fallback"), FMath::IsNearlyEqual(View.EnterTransitionElapsedSeconds, 0.10f));
	TestEqual(TEXT("A timed-out sound is not played late"), View.EnterTransitionSoundRequestCount, 0);

	UWacomFirstPersonCardLayerSlotWidget* CancelledWidget = MakeDrawWidget();
	CancelledWidget->BeginSlotMotionWithEnterProfile(
		MakeSlot(FGuid::NewGuid()),
		true,
		Profile);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotionWithoutPresentationPaint(
		*CancelledWidget,
		0.10f);
	CancelledWidget->ForceCompletePresentationPlayback();
	const FWacomFirstPersonCardSlotAutomationTestView CancelledView =
		FWacomFirstPersonCardLayerTestAccess::View(*CancelledWidget);
	TestEqual(TEXT("ForceComplete cancels the pending generation"), CancelledView.SurfaceReadinessGeneration, 0u);
	TestFalse(TEXT("ForceComplete removes the waiting reveal"), CancelledView.bDrawRevealPlaybackActive);
	TestFalse(TEXT("ForceComplete leaves no blocking playback"), CancelledWidget->HasActivePresentationPlayback());
	return true;
}

#endif
