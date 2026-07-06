// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

namespace WacomFirstPersonCardLayerInspectScrubSpec
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
		int32 Index,
		const FVector2D& Position)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = Index;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.bIsPlayable = true;
		Slot.Entry.InteractionIntent =
			EWacomFirstPersonCardInteractionIntent::CommitNoTarget;
		Slot.ScreenPosition = Position;
		Slot.WidgetPosition = Position;
		Slot.SnappedWidgetPosition = Position;
		Slot.InputHitCenter = Position;
		Slot.InputHitScale = 1.0f;
		Slot.InputHitAngleDegrees = 0.0f;
		Slot.InputHitOrder = Index;
		Slot.RenderScale = 1.0f;
		Slot.RenderOpacity = 1.0f;
		Slot.ZOrder = Index;
		Slot.bProjected = true;
		return Slot;
	}

	FWacomFirstPersonCardDragConfig MakeScrubDragConfig()
	{
		FWacomFirstPersonCardDragConfig Config;
		Config.CardInspectHoldDelaySeconds = 0.1f;
		Config.CardDragStartThresholdPixels = 10.0f;
		Config.CardInspectScrubHandPaddingPixels = FVector2D(32.0f, 48.0f);
		Config.NoTargetCardDragOutCommitDistancePixels = 140.0f;
		return Config;
	}

	class FLayerDragReceiver
	{
	public:
		int32 StartedCount = 0;
		int32 UpdatedCount = 0;
		int32 ReleasedCount = 0;
		int32 CancelledCount = 0;
		FGuid LastCardId;
		FWacomFirstPersonCardDragView LastDragView;

		void HandleStarted(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView)
		{
			++StartedCount;
			LastCardId = CardInstanceId;
			LastDragView = DragView;
		}

		void HandleUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView)
		{
			++UpdatedCount;
			LastCardId = CardInstanceId;
			LastDragView = DragView;
		}

		void HandleReleased(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView)
		{
			++ReleasedCount;
			LastCardId = CardInstanceId;
			LastDragView = DragView;
		}

		void HandleCancelled(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView)
		{
			++CancelledCount;
			LastCardId = CardInstanceId;
			LastDragView = DragView;
		}
	};

	void BindReceiver(UWacomFirstPersonCardLayerWidget& Layer, FLayerDragReceiver& Receiver)
	{
		Layer.OnCardDragStartedNative.AddRaw(&Receiver, &FLayerDragReceiver::HandleStarted);
		Layer.OnCardDragUpdatedNative.AddRaw(&Receiver, &FLayerDragReceiver::HandleUpdated);
		Layer.OnCardDragReleasedNative.AddRaw(&Receiver, &FLayerDragReceiver::HandleReleased);
		Layer.OnCardDragCancelledNative.AddRaw(&Receiver, &FLayerDragReceiver::HandleCancelled);
	}

	void UnbindReceiver(UWacomFirstPersonCardLayerWidget& Layer, FLayerDragReceiver& Receiver)
	{
		Layer.OnCardDragStartedNative.RemoveAll(&Receiver);
		Layer.OnCardDragUpdatedNative.RemoveAll(&Receiver);
		Layer.OnCardDragReleasedNative.RemoveAll(&Receiver);
		Layer.OnCardDragCancelledNative.RemoveAll(&Receiver);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInspectScrubKeepsPreviewInHandTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.ScrubInsideHandKeepsPreviewAndTransfersSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInspectScrubKeepsPreviewInHandTest::RunTest(const FString& Parameters)
{
	using namespace WacomFirstPersonCardLayerInspectScrubSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	const FVector2D SourcePosition(400.0f, 600.0f);
	const FVector2D GapPosition(550.0f, 600.0f);
	const FVector2D TargetPosition(700.0f, 600.0f);
	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();

	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardDragConfig(MakeScrubDragConfig());
	FWacomFirstPersonCardLayerTestAccess::SetViewportSizeOverride(*Layer, FVector2D(1000.0f, 1000.0f));
	Layer->SetCardSlots({
		MakeSlot(SourceCardId, 0, SourcePosition),
		MakeSlot(TargetCardId, 1, TargetPosition)
	});

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	FLayerDragReceiver Receiver;
	BindReceiver(*Layer, Receiver);

	TestTrue(TEXT("Press source captures gesture"),
		FWacomFirstPersonCardLayerTestAccess::RequestPressAtWidgetPosition(*Layer, SourcePosition));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.12f, SourcePosition);
	TestEqual(TEXT("Source enters inspect"),
		SourceWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Inspecting);
	TestEqual(TEXT("Initial inspect source"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CardInstanceId,
		SourceCardId);

	TestTrue(TEXT("Gap move inside scrub is handled"),
		FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerMovedAtWidgetPosition(
			*Layer,
			*SourceWidget,
			GapPosition));
	TestEqual(TEXT("Gap inside hand keeps source inspecting"),
		SourceWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Inspecting);
	TestEqual(TEXT("Gap keeps previous preview source"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CardInstanceId,
		SourceCardId);

	TestTrue(TEXT("Target body move transfers inspect"),
		FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerMovedAtWidgetPosition(
			*Layer,
			*SourceWidget,
			TargetPosition));
	TestEqual(TEXT("Source silently returns idle"),
		SourceWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Idle);
	TestEqual(TEXT("Target becomes inspect source"),
		TargetWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Inspecting);
	TestEqual(TEXT("Layer current drag source switches to target"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CardInstanceId,
		TargetCardId);
	TestEqual(TEXT("Transfer starts target inspect without cancelling source"),
		Receiver.CancelledCount,
		0);

	TestTrue(TEXT("Gap after transfer stays on target"),
		FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerMovedAtWidgetPosition(
			*Layer,
			*SourceWidget,
			GapPosition));
	TestEqual(TEXT("Target remains inspecting in gap"),
		TargetWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Inspecting);
	TestEqual(TEXT("Gap keeps target preview source"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CardInstanceId,
		TargetCardId);

	TestEqual(TEXT("Release inside scrub releases capture"),
		FWacomFirstPersonCardLayerTestAccess::RequestReleaseRouteActionAtWidgetPosition(*Layer, GapPosition),
		EWacomFirstPersonCardPointerRouteAction::ReleaseMouseCapture);
	TestEqual(TEXT("Release stays neutral inspect"),
		Receiver.LastDragView.GestureState,
		EWacomFirstPersonCardGestureState::Inspecting);
	TestEqual(TEXT("Neutral release uses target source"),
		Receiver.LastCardId,
		TargetCardId);
	TestEqual(TEXT("Target returns idle after neutral release"),
		TargetWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Idle);
	TestFalse(TEXT("Neutral release does not deny"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bDenyFeedbackActive);
	TestEqual(TEXT("No cancel during scrub flow"), Receiver.CancelledCount, 0);

	UnbindReceiver(*Layer, Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInspectScrubDragSourceTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.ScrubbedPreviewBecomesDragSourceWhenLeavingHand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInspectScrubDragSourceTest::RunTest(const FString& Parameters)
{
	using namespace WacomFirstPersonCardLayerInspectScrubSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	const FVector2D SourcePosition(400.0f, 600.0f);
	const FVector2D TargetPosition(700.0f, 600.0f);
	const FVector2D OutsideHandPosition(700.0f, 190.0f);
	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();

	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardDragConfig(MakeScrubDragConfig());
	FWacomFirstPersonCardLayerTestAccess::SetViewportSizeOverride(*Layer, FVector2D(1000.0f, 1000.0f));
	Layer->SetCardSlots({
		MakeSlot(SourceCardId, 0, SourcePosition),
		MakeSlot(TargetCardId, 1, TargetPosition)
	});

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	TestTrue(TEXT("Press source captures gesture"),
		FWacomFirstPersonCardLayerTestAccess::RequestPressAtWidgetPosition(*Layer, SourcePosition));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.12f, SourcePosition);
	TestTrue(TEXT("Move to target transfers inspect"),
		FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerMovedAtWidgetPosition(
			*Layer,
			*SourceWidget,
			TargetPosition));
	TestEqual(TEXT("Target is current inspect source before leaving hand"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CardInstanceId,
		TargetCardId);

	TestTrue(TEXT("Outside hand move is handled by active source"),
		FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerMovedAtWidgetPosition(
			*Layer,
			*SourceWidget,
			OutsideHandPosition));
	TestEqual(TEXT("Source card remains idle"),
		SourceWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Idle);
	TestEqual(TEXT("Scrubbed target becomes armed drag source"),
		TargetWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::ArmedForCommit);

	const FWacomFirstPersonCardLayerAutomationTestView LayerView =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("Layer drag source is scrubbed target"),
		LayerView.CurrentDragView.CardInstanceId,
		TargetCardId);
	TestEqual(TEXT("Layer drag state follows target"),
		LayerView.CurrentDragView.GestureState,
		EWacomFirstPersonCardGestureState::ArmedForCommit);

	FWacomFirstPersonCardLayerTestAccess::RequestReleaseAtWidgetPosition(*Layer, OutsideHandPosition);
	PC->Destroy();
	return true;
}
