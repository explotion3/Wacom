// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Input/Events.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "UI/GameViewportClientTestAccess.h"
#include "UI/Foundation/WacomGameViewportClient.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardShortcutCancelSpec
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

	FWacomFirstPersonCardLayerSlotView MakeSlot(const FGuid& CardInstanceId)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.Zone = EHandZone::Both;
		Slot.Entry.bIsPlayable = true;
		Slot.Entry.InteractionIntent =
			EWacomFirstPersonCardInteractionIntent::DragToDropTarget;
		Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
		Slot.WidgetPosition = Slot.ScreenPosition;
		Slot.SnappedWidgetPosition = Slot.ScreenPosition;
		Slot.InputHitCenter = Slot.ScreenPosition;
		Slot.InputHitScale = 1.0f;
		Slot.RenderScale = 1.0f;
		Slot.RenderOpacity = 1.0f;
		Slot.bProjected = true;
		return Slot;
	}

	FPointerEvent MakeMouseButtonDownEvent(
		FKey EffectingButton,
		const FVector2D& ScreenPosition = FVector2D(500.0f, 180.0f))
	{
		TSet<FKey> PressedButtons;
		PressedButtons.Add(EffectingButton);
		return FPointerEvent(
			0,
			ScreenPosition,
			ScreenPosition,
			PressedButtons,
			EffectingButton,
			0.0f,
			FModifierKeysState());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardShortcutRightClickCancelTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.ShortcutRightClickCancelsWithoutAffectingMouseDrag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardShortcutRightClickCancelTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardShortcutCancelSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	if (!TestTrue(TEXT("Slate application is initialized"), FSlateApplication::IsInitialized()))
	{
		return false;
	}
	TestTrue(
		TEXT("Project uses the Wacom CommonUI viewport input seam"),
		GEngine
			&& GEngine->GameViewportClientClass
				== UWacomGameViewportClient::StaticClass());

	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(
		AWacomPlayerCharacter::StaticClass(),
		FTransform::Identity);
	UWacomGameViewportClient* ViewportClient =
		NewObject<UWacomGameViewportClient>(GEngine);
	if (!TestNotNull(TEXT("Player controller"), PC)
		|| !TestNotNull(TEXT("Player character"), Character)
		|| !TestNotNull(TEXT("Wacom game viewport client"), ViewportClient))
	{
		return false;
	}

	auto Cleanup = [PC, Character, ViewportClient]()
	{
		if (ViewportClient)
		{
			FWacomGameViewportClientTestAccess::SetRouteOverrides(
				*ViewportClient,
				TOptional<bool>(),
				nullptr);
			FWacomGameViewportClientTestAccess::UnregisterInputPreProcessor(
				*ViewportClient);
		}
		if (PC)
		{
			PC->UnPossess();
			PC->Destroy();
		}
		if (Character)
		{
			Character->Destroy();
		}
	};

	PC->Possess(Character);
	UWacomFirstPersonCardAnchorComponent* Anchor =
		Character->GetFirstPersonCardAnchorComponent();
	UWacomFirstPersonCardLayerWidget* Layer =
		NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("First-person card anchor"), Anchor)
		|| !TestNotNull(TEXT("First-person card layer"), Layer))
	{
		Cleanup();
		return false;
	}

	Layer->SetCardLayerInteractionEnabled(true);
	const FGuid CardInstanceId = FGuid::NewGuid();
	const FWacomFirstPersonCardLayerSlotView Slot = MakeSlot(CardInstanceId);
	Layer->SetCardSlots({ Slot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	FWacomFirstPersonCardLayerTestAccess::SetCardLayer(*Anchor, Layer);
	FWacomGameViewportClientTestAccess::SetRouteOverrides(
		*ViewportClient,
		true,
		PC);
	FWacomGameViewportClientTestAccess::RegisterInputPreProcessor(*ViewportClient);
	TestTrue(
		TEXT("Slate input preprocessor is registered in the Game bucket"),
		FWacomGameViewportClientTestAccess::IsInputPreProcessorRegistered(
			*ViewportClient));
	const void* RegisteredProcessor =
		FWacomGameViewportClientTestAccess::InputPreProcessorAddress(
			*ViewportClient);
	FWacomGameViewportClientTestAccess::RegisterInputPreProcessor(*ViewportClient);
	TestTrue(
		TEXT("Repeated registration reuses the same input processor"),
		FWacomGameViewportClientTestAccess::InputPreProcessorAddress(
			*ViewportClient) == RegisteredProcessor);

	int32 CancelBroadcastCount = 0;
	Anchor->OnFirstPersonCardLayerDragCancelled.AddLambda(
		[&CancelBroadcastCount](
			const FGuid& /*CardInstanceId*/,
			const FWacomFirstPersonCardDragView& /*DragView*/)
		{
			++CancelBroadcastCount;
		});
	const FPointerEvent RightMouseDown = MakeMouseButtonDownEvent(
		EKeys::RightMouseButton);
	const FPointerEvent LeftMouseDown = MakeMouseButtonDownEvent(
		EKeys::LeftMouseButton);

	TestTrue(
		TEXT("Keyboard shortcut drag starts"),
		Anchor->TryStartFirstPersonCardDragGesture(
			CardInstanceId,
			FVector2D(500.0f, 180.0f)));
	TestTrue(
		TEXT("Anchor identifies the active shortcut drag"),
		Anchor->IsFirstPersonCardKeyboardShortcutDragGestureActive());
	TestFalse(
		TEXT("Non-right mouse input is not consumed by the Slate preprocessor"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonDown(
			*ViewportClient,
			LeftMouseDown));
	TestTrue(
		TEXT("Non-right mouse input keeps the shortcut drag active"),
		Anchor->IsFirstPersonCardKeyboardShortcutDragGestureActive());

	FWacomGameViewportClientTestAccess::SetRouteOverrides(
		*ViewportClient,
		false,
		PC);
	TestFalse(
		TEXT("Right mouse outside the owning viewport is not consumed"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonDown(
			*ViewportClient,
			RightMouseDown));
	TestTrue(
		TEXT("Outside-viewport right mouse keeps the shortcut drag active"),
		Anchor->IsFirstPersonCardKeyboardShortcutDragGestureActive());
	FWacomGameViewportClientTestAccess::SetRouteOverrides(
		*ViewportClient,
		true,
		PC);
	TestTrue(
		TEXT("Slate right mouse press is consumed while cancelling shortcut drag"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonDown(
			*ViewportClient,
			RightMouseDown));
	TestFalse(
		TEXT("Right mouse press cancels the shortcut drag"),
		Anchor->IsFirstPersonCardDragGestureActive());
	TestEqual(
		TEXT("Shortcut cancellation broadcasts once"),
		CancelBroadcastCount,
		1);
	TestFalse(
		TEXT("Repeated right mouse after cancellation is not consumed"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonDown(
			*ViewportClient,
			RightMouseDown));
	TestEqual(
		TEXT("Repeated right mouse does not rebroadcast cancellation"),
		CancelBroadcastCount,
		1);

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Card slot widget"), SlotWidget))
	{
		Cleanup();
		return false;
	}
	TestTrue(
		TEXT("Mouse press starts after shortcut cancel"),
		FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(
			*SlotWidget,
			Slot.ScreenPosition));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(
		*SlotWidget,
		0.0f,
		Slot.ScreenPosition + FVector2D(0.0f, -80.0f));
	TestTrue(
		TEXT("Mouse drag is active"),
		Anchor->IsFirstPersonCardDragGestureActive());
	TestFalse(
		TEXT("Mouse drag is not identified as a keyboard shortcut drag"),
		Anchor->IsFirstPersonCardKeyboardShortcutDragGestureActive());
	TestFalse(
		TEXT("Slate right mouse press is not consumed for a mouse-origin drag"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonDown(
			*ViewportClient,
			RightMouseDown));
	TestTrue(
		TEXT("Right mouse press does not cancel a mouse-origin drag"),
		Anchor->IsFirstPersonCardDragGestureActive());

	Anchor->CancelFirstPersonCardDragGesture(false);
	ViewportClient->DetachViewportClient();
	TestFalse(
		TEXT("DetachViewportClient removes the input preprocessor"),
		FWacomGameViewportClientTestAccess::IsInputPreProcessorRegistered(
			*ViewportClient));
	TestFalse(
		TEXT("Unregistered input preprocessor cannot consume input"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonDown(
			*ViewportClient,
			RightMouseDown));
	Cleanup();
	return true;
}

#endif
