// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardLayerRetainedFeedbackSpec
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

	FWacomFirstPersonCardLayerSlotView MakeSlot(
		const FGuid& CardInstanceId,
		const FVector2D& Position)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.Zone = EHandZone::Both;
		Slot.Entry.bIsPlayable = true;
		Slot.ScreenPosition = Position;
		Slot.WidgetPosition = Position;
		Slot.SnappedWidgetPosition = Position;
		Slot.InputHitCenter = Position;
		Slot.InputHitScale = 1.0f;
		Slot.InputHitOrder = 0;
		Slot.RenderScale = 1.0f;
		Slot.RenderOpacity = 1.0f;
		Slot.AnchorWidgetPosition = FVector2D(500.0f, 500.0f);
		Slot.bProjected = true;
		return Slot;
	}

	FWacomFirstPersonCardLayerFeedbackHint MakeRetainedHint(
		const FGuid& CardInstanceId,
		int32 SequenceIndex = 0,
		int32 SequenceCount = 1)
	{
		FWacomFirstPersonCardLayerFeedbackHint Hint;
		Hint.CardInstanceId = CardInstanceId;
		Hint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::Retained;
		Hint.SequenceIndex = SequenceIndex;
		Hint.SequenceCount = SequenceCount;
		return Hint;
	}

	FWacomFirstPersonCardSlotFeedbackConfig MakeRetainedFeedbackConfig()
	{
		FWacomFirstPersonCardSlotFeedbackConfig Config;
		Config.bEnabled = true;
		Config.bEnableRetainedFeedback = true;
		Config.RetainedFeedbackDuration = 0.24f;
		Config.RetainedFeedbackStaggerSeconds = 0.05f;
		Config.RetainedFeedbackLiftPixels = 16.0f;
		Config.RetainedFeedbackScale = 1.05f;
		Config.RetainedFeedbackColor = FLinearColor(1.0f, 0.84f, 0.34f, 1.0f);
		Config.RetainedFeedbackOpacity = 0.4f;
		Config.RetainedFeedbackZOrderBoost = 200;
		return Config;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerRetainedFeedbackPlaybackTest,
	"Wacom.UI.FirstPersonCardLayer.RetainedFeedback.RetainedHintPlaysPulseAndSurvivesRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRetainedFeedbackPlaybackTest::RunTest(const FString& Parameters)
{
	using namespace WacomFirstPersonCardLayerRetainedFeedbackSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	const FWacomFirstPersonCardSlotFeedbackConfig FeedbackConfig = MakeRetainedFeedbackConfig();
	Layer->SetSlotFeedbackConfig(FeedbackConfig);

	const FGuid CardId = FGuid::NewGuid();
	const FVector2D InitialPosition(160.0f, 280.0f);
	Layer->SetCardFeedbackHints({ MakeRetainedHint(CardId, 0, 1) });
	Layer->SetCardSlots({ MakeSlot(CardId, InitialPosition) });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Retained slot widget"), SlotWidget))
	{
		const FWacomFirstPersonCardSlotAutomationTestView InitialView =
			FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget);
		TestTrue(TEXT("Retained feedback starts active"), InitialView.bRetainedFeedbackActive);
		TestEqual(
			TEXT("Retained feedback uses retained interaction kind"),
			InitialView.InteractionFeedbackKind,
			EWacomFirstPersonCardInteractionFeedbackKind::Retained);
		TestTrue(TEXT("Retained overlay is visible"), InitialView.FeedbackOverlayOpacity > 0.0f);
		TestEqual(TEXT("Retained hint is consumed"), FWacomFirstPersonCardLayerTestAccess::View(*Layer).PendingFeedbackHintCardIds.Num(), 0);

		const FWidgetTransform InitialTransform = SlotWidget->GetRenderTransform();
		TestTrue(TEXT("Retained pulse lifts the card"), InitialTransform.Translation.Y < 0.0f);
		TestTrue(TEXT("Retained pulse scales the card"), InitialTransform.Scale.X > 1.0f);
		TestEqual(TEXT("Retained pulse boosts z order"), Layer->GetCardZOrderAt(0), FeedbackConfig.RetainedFeedbackZOrderBoost);

		const FVector2D RefreshedPosition(172.0f, 280.0f);
		Layer->SetCardSlots({ MakeSlot(CardId, RefreshedPosition) });
		const FWacomFirstPersonCardSlotAutomationTestView RefreshView =
			FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget);
		TestTrue(TEXT("Ordinary refresh preserves retained feedback"), RefreshView.bRetainedFeedbackActive);
		TestEqual(TEXT("Ordinary refresh updates target slot"), SlotWidget->GetSlotView().ScreenPosition, RefreshedPosition);

		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.3f);
		const FWacomFirstPersonCardSlotAutomationTestView FinishedView =
			FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget);
		TestFalse(TEXT("Retained feedback completes"), FinishedView.bRetainedFeedbackActive);
		TestEqual(
			TEXT("Retained interaction feedback clears"),
			FinishedView.InteractionFeedbackKind,
			EWacomFirstPersonCardInteractionFeedbackKind::None);
		TestEqual(TEXT("Retained overlay clears"), FinishedView.FeedbackOverlayOpacity, 0.0f);
		TestEqual(TEXT("Retained z boost clears"), Layer->GetCardZOrderAt(0), 0);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerRetainedFeedbackWaitsForProjectedSlotTest,
	"Wacom.UI.FirstPersonCardLayer.RetainedFeedback.PendingRetainedHintWaitsForProjectedSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRetainedFeedbackWaitsForProjectedSlotTest::RunTest(const FString& Parameters)
{
	using namespace WacomFirstPersonCardLayerRetainedFeedbackSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	Layer->SetSlotFeedbackConfig(MakeRetainedFeedbackConfig());

	const FGuid CardId = FGuid::NewGuid();
	const FVector2D Position(240.0f, 320.0f);
	FWacomFirstPersonCardLayerSlotView UnprojectedSlot = MakeSlot(CardId, Position);
	UnprojectedSlot.bProjected = false;

	Layer->SetCardFeedbackHints({ MakeRetainedHint(CardId, 0, 1) });
	Layer->SetCardSlots({ UnprojectedSlot });
	TestEqual(
		TEXT("Unprojected retained hint remains pending"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).PendingFeedbackHintCardIds.Num(),
		1);

	if (UWacomFirstPersonCardLayerSlotWidget* UnprojectedWidget = Layer->GetSlotWidgetAt(0))
	{
		TestFalse(
			TEXT("Unprojected retained slot does not start feedback"),
			FWacomFirstPersonCardLayerTestAccess::View(*UnprojectedWidget).bRetainedFeedbackActive);
	}

	Layer->SetCardSlots({ MakeSlot(CardId, Position) });
	UWacomFirstPersonCardLayerSlotWidget* ProjectedWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Projected retained slot widget"), ProjectedWidget))
	{
		TestTrue(
			TEXT("Projected retained slot starts feedback"),
			FWacomFirstPersonCardLayerTestAccess::View(*ProjectedWidget).bRetainedFeedbackActive);
		TestEqual(
			TEXT("Projected retained hint is consumed"),
			FWacomFirstPersonCardLayerTestAccess::View(*Layer).PendingFeedbackHintCardIds.Num(),
			0);
	}

	PC->Destroy();
	return true;
}

#endif
