// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundWave.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

namespace WacomFirstPersonCardInteractionFeedbackSpec
{
	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}

	FWacomFirstPersonCardLayerSlotView MakeSlotView()
	{
		FWacomFirstPersonCardLayerSlotView View;
		View.Entry.CardInstanceId = FGuid::NewGuid();
		View.Entry.bIsPlayable = true;
		View.Entry.InteractionIntent = EWacomFirstPersonCardInteractionIntent::CommitNoTarget;
		View.bProjected = true;
		View.RenderScale = 1.0f;
		View.RenderOpacity = 1.0f;
		return View;
	}

	FWacomFirstPersonCardInteractionFeedbackConfig MakeReducedConfig()
	{
		FWacomFirstPersonCardInteractionFeedbackConfig Config;
		Config.bEnabled = true;
		Config.bReduceInteractionMotion = true;
		Config.PressedScale = 0.8f;
		Config.PressedTranslationYPixels = 12.0f;
		Config.PressedInDurationSeconds = 0.045f;
		Config.PressedOutDurationSeconds = 0.08f;
		Config.DenyDuration = 0.2f;
		Config.DenyShakePixels = 16.0f;
		Config.DenyColor = FLinearColor::Red;
		Config.DenyOpacity = 0.5f;
		return Config;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardInteractionFeedbackReducedMotionTest,
	"Wacom.UI.FirstPersonCardLayer.InteractionFeedback.ReducedMotionKeepsSemanticDenyCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardInteractionFeedbackReducedMotionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardInteractionFeedbackSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PlayerController = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget =
		NewObject<UWacomFirstPersonCardLayerSlotWidget>(PlayerController);
	if (!TestNotNull(TEXT("Player controller"), PlayerController)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*SlotWidget,
		WacomFirstPersonCardInteractionFeedbackSpec::MakeReducedConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(
		WacomFirstPersonCardInteractionFeedbackSpec::MakeSlotView());
	const FWidgetTransform AuthoredTransform = SlotWidget->GetRenderTransform();

	TestTrue(TEXT("Press is still accepted"),
		FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 0.1f);
	TestEqual(TEXT("Reduced motion suppresses pressed envelope"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).PressedFeedbackAmount, 0.0f);
	TestEqual(TEXT("Reduced motion preserves authored scale"),
		SlotWidget->GetRenderTransform().Scale, AuthoredTransform.Scale);
	TestEqual(TEXT("Reduced motion preserves authored translation"),
		SlotWidget->GetRenderTransform().Translation, AuthoredTransform.Translation);

	FWacomFirstPersonCardLayerTestAccess::RequestMouseUp(*SlotWidget);
	FWacomFirstPersonCardLayerTestAccess::TriggerDenyFeedback(*SlotWidget);
	const FWacomFirstPersonCardSlotAutomationTestView InitialDenyView =
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget);
	TestEqual(TEXT("Semantic corner cue remains enabled"),
		InitialDenyView.InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::Deny);
	TestTrue(TEXT("Semantic corner cue remains visible"),
		InitialDenyView.InteractionCueAmount > 0.0f);

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 0.025f);
	TestEqual(TEXT("Reduced motion suppresses deny shake"),
		SlotWidget->GetRenderTransform().Translation.X,
		AuthoredTransform.Translation.X);

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 0.25f);
	TestEqual(TEXT("Deny cue clears after its authored duration"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::None);

	PlayerController->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardInvalidTargetPreviewTest,
	"Wacom.UI.FirstPersonCardLayer.InteractionFeedback.InvalidTargetPreviewRequiresResolvedTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardInvalidTargetPreviewTest::RunTest(const FString& Parameters)
{
	UWacomFirstPersonCardLayerSlotWidget* Slot =
		NewObject<UWacomFirstPersonCardLayerSlotWidget>();
	FWacomFirstPersonCardInteractionFeedbackConfig Config;
	Config.bEnabled = true;
	Config.InvalidTargetPreviewEnterDuration = 0.08f;
	Config.InvalidTargetPreviewExitDuration = 0.06f;
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*Slot, Config);
	Slot->SetCardLayerInteractionEnabled(true);
	Slot->SetSlotViewImmediate(WacomFirstPersonCardInteractionFeedbackSpec::MakeSlotView());
	FWacomFirstPersonCardLayerTestAccess::SetGestureState(
		*Slot,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);

	Slot->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle(),
		false,
		EWacomFirstPersonCardDragTargetFeedbackState::Invalid);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 0.10f);
	TestFalse(
		TEXT("Blank invalid area does not show the source warning"),
		FWacomFirstPersonCardLayerTestAccess::View(*Slot).bInvalidTargetPreviewActive);

	const FWacomInteractionTargetHandle InvalidTarget =
		FWacomInteractionTargetHandle::ForWorldTarget(
			FGuid::NewGuid(),
			Slot,
			FVector::ZeroVector,
			FVector2D(700.0f, 260.0f));
	Slot->SetCardDragFeedbackTarget(
		InvalidTarget,
		false,
		EWacomFirstPersonCardDragTargetFeedbackState::Invalid);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 0.08f);
	const FWacomFirstPersonCardSlotAutomationTestView InvalidView =
		FWacomFirstPersonCardLayerTestAccess::View(*Slot);
	TestTrue(TEXT("Resolved invalid target activates the warning"),
		InvalidView.bInvalidTargetPreviewActive);
	TestEqual(TEXT("Invalid target uses the preview cue"),
		InvalidView.InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::InvalidPreview);
	TestTrue(TEXT("Preview reaches its authored amount"),
		InvalidView.InvalidTargetPreviewAmount > 0.99f);

	Slot->SetCardDragFeedbackTarget(
		InvalidTarget,
		false,
		EWacomFirstPersonCardDragTargetFeedbackState::Invalid);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 0.01f);
	TestTrue(TEXT("Equivalent target refresh does not restart the warning"),
		FWacomFirstPersonCardLayerTestAccess::View(*Slot).InvalidTargetPreviewAmount > 0.99f);

	Slot->SetCardDragFeedbackTarget(
		InvalidTarget,
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 0.08f);
	TestEqual(TEXT("Valid target fades the warning out"),
		FWacomFirstPersonCardLayerTestAccess::View(*Slot).InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDirectionalDenyTest,
	"Wacom.UI.FirstPersonCardLayer.InteractionFeedback.DirectionalDenyAndSound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDirectionalDenyTest::RunTest(const FString& Parameters)
{
	UWacomFirstPersonCardLayerSlotWidget* Slot =
		NewObject<UWacomFirstPersonCardLayerSlotWidget>();
	FWacomFirstPersonCardInteractionFeedbackConfig Config;
	Config.bEnabled = true;
	Config.DenyDuration = 0.20f;
	Config.DenyShakePixels = 10.0f;
	Config.DenyCompressScale = 0.97f;
	Config.DenySound = NewObject<USoundWave>(Slot);
	Config.DenySoundPitchMultiplier = 1.0f;
	Config.DenySoundPitchVariation = 0.03f;
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*Slot, Config);
	Slot->SetCardLayerInteractionEnabled(true);
	Slot->SetSlotViewImmediate(WacomFirstPersonCardInteractionFeedbackSpec::MakeSlotView());

	FWacomFirstPersonCardLayerTestAccess::TriggerDenyFeedback(*Slot);
	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Slot);
	TestTrue(TEXT("Deny begins immediately"), View.bDenyFeedbackActive);
	TestEqual(TEXT("Deny sound is requested once"), View.DenySoundRequestCount, 1);
	TestTrue(TEXT("Deny pitch remains inside the authored variation"),
		View.LastDenySoundPitchMultiplier >= 0.97f
		&& View.LastDenySoundPitchMultiplier <= 1.03f);

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 0.025f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Slot);
	TestTrue(TEXT("Deny compresses before recoil"), View.RenderTransform.Scale.X < 1.0f);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 0.065f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Slot);
	TestTrue(TEXT("Deny recoils opposite the default release direction"),
		View.RenderTransform.Translation.Y > 0.0f);
	TestEqual(TEXT("Ticking the same Deny never repeats its sound"),
		View.DenySoundRequestCount,
		1);

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 0.20f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Slot);
	TestFalse(TEXT("Deny completes"), View.bDenyFeedbackActive);
	TestTrue(TEXT("Deny restores scale"),
		FMath::IsNearlyEqual(View.RenderTransform.Scale.X, 1.0f, 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardInteractionConfigRefreshTest,
	"Wacom.UI.FirstPersonCardLayer.InteractionFeedback.ConfigRefreshDoesNotReactivateCompletedFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardInteractionConfigRefreshTest::RunTest(const FString& Parameters)
{
	UWacomFirstPersonCardLayerSlotWidget* Slot =
		NewObject<UWacomFirstPersonCardLayerSlotWidget>();
	FWacomFirstPersonCardInteractionFeedbackConfig Config;
	Config.bEnabled = true;
	Config.DenyDuration = 0.20f;
	Config.PlayCommitDuration = 0.12f;
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*Slot, Config);
	Slot->SetCardLayerInteractionEnabled(true);
	Slot->SetSlotViewImmediate(WacomFirstPersonCardInteractionFeedbackSpec::MakeSlotView());

	FWacomFirstPersonCardLayerTestAccess::TriggerDenyFeedback(*Slot);
	Slot->TriggerCommitFeedback();
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 0.25f);
	TestFalse(TEXT("Deny has completed before the config refresh"),
		FWacomFirstPersonCardLayerTestAccess::View(*Slot).bDenyFeedbackActive);
	TestFalse(TEXT("Commit has completed before the config refresh"),
		FWacomFirstPersonCardLayerTestAccess::View(*Slot).bCommitFeedbackActive);

	Config.DenyDuration = 0.40f;
	Config.PlayCommitDuration = 0.30f;
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*Slot, Config);
	TestFalse(TEXT("Increasing Deny duration does not reactivate completed feedback"),
		FWacomFirstPersonCardLayerTestAccess::View(*Slot).bDenyFeedbackActive);
	TestFalse(TEXT("Increasing Commit duration does not reactivate completed feedback"),
		FWacomFirstPersonCardLayerTestAccess::View(*Slot).bCommitFeedbackActive);
	return true;
}
