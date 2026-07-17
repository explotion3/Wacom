// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
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

