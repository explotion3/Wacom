// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Input/Events.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/BattleSceneTargetClickTestAccess.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "UI/Foundation/WacomGameViewportClient.h"
#include "UI/GameViewportClientTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardInputRouterSpec
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

	FWacomFirstPersonCardLayerSlotView MakeDualFaceSlot(
		const FGuid& CardInstanceId,
		const FVector2D& Position)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.CardViewData.Name = FText::FromString(TEXT("战斗面"));
		Slot.Entry.AlternateFaceCardViewData.Name =
			FText::FromString(TEXT("探索面"));
		Slot.Entry.DefaultFaceContext = EWacomCardFaceContext::Battle;
		Slot.Entry.bHasAlternateFace = true;
		Slot.Entry.bAllowLockedFaceInspection = true;
		Slot.Entry.bIsPlayable = true;
		Slot.Entry.InteractionIntent =
			EWacomFirstPersonCardInteractionIntent::DragToDropTarget;
		Slot.ScreenPosition = Position;
		Slot.WidgetPosition = Position;
		Slot.SnappedWidgetPosition = Position;
		Slot.InputHitCenter = Position;
		Slot.InputHitScale = 1.0f;
		Slot.RenderScale = 1.0f;
		Slot.RenderOpacity = 1.0f;
		Slot.bProjected = true;
		return Slot;
	}

	FWacomFirstPersonCardDragConfig MakeInspectConfig()
	{
		FWacomFirstPersonCardDragConfig Config;
		Config.CardInspectHoldDelaySeconds = 0.05f;
		Config.CardDragStartThresholdPixels = 10.0f;
		Config.CardInspectScrubHandPaddingPixels = FVector2D(40.0f, 60.0f);
		return Config;
	}

	FPointerEvent MakeMouseEvent(
		const FKey& EffectingButton,
		const bool bPressed,
		const FVector2D& ScreenPosition)
	{
		TSet<FKey> PressedButtons;
		if (bPressed)
		{
			PressedButtons.Add(EffectingButton);
		}
		return FPointerEvent(
			0,
			ScreenPosition,
			ScreenPosition,
			PressedButtons,
			EffectingButton,
			0.0f,
			FModifierKeysState());
	}

	bool EnterLockedInspection(
		UWacomFirstPersonCardLayerWidget& Layer,
		UWacomFirstPersonCardLayerSlotWidget& Slot,
		const FVector2D& CardPosition)
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(Layer, 1.0f);
		if (!FWacomFirstPersonCardLayerTestAccess::RequestPressAtWidgetPosition(
				Layer,
				CardPosition))
		{
			return false;
		}
		FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(
			Slot,
			0.06f,
			CardPosition);
		FWacomFirstPersonCardLayerTestAccess::RequestReleaseRouteActionAtWidgetPosition(
			Layer,
			CardPosition);
		return Layer.IsLockedCardInspectionActive();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardInputRouterArbitrationSpec,
	"Wacom.UI.FirstPersonCardInputRouter.ArbitrationAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardInputRouterArbitrationSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardInputRouterSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World)
		|| !TestTrue(
			TEXT("Slate application is initialized"),
			FSlateApplication::IsInitialized()))
	{
		return false;
	}

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
		|| !TestNotNull(TEXT("Viewport client"), ViewportClient))
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
	if (!TestNotNull(TEXT("Card anchor"), Anchor)
		|| !TestNotNull(TEXT("Card layer"), Layer))
	{
		Cleanup();
		return false;
	}

	const FVector2D CardPosition(500.0f, 650.0f);
	const FGuid CardInstanceId = FGuid::NewGuid();
	Layer->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(
		*Layer,
		MakeInspectConfig());
	FWacomFirstPersonCardLayerTestAccess::SetViewportSizeOverride(
		*Layer,
		FVector2D(1000.0f, 1000.0f));
	Layer->SetCardSlots({
		MakeDualFaceSlot(CardInstanceId, CardPosition)
	});
	FWacomFirstPersonCardLayerTestAccess::SetCardLayer(*Anchor, Layer);
	UWacomFirstPersonCardLayerSlotWidget* Slot = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Card slot"), Slot))
	{
		Cleanup();
		return false;
	}

	FWacomGameViewportClientTestAccess::SetRouteOverrides(
		*ViewportClient,
		true,
		PC);
	FWacomGameViewportClientTestAccess::RegisterInputPreProcessor(*ViewportClient);
	const FPointerEvent SlateRightDown = MakeMouseEvent(
		EKeys::RightMouseButton,
		true,
		FVector2D(500.0f, 180.0f));

	TestTrue(
		TEXT("Keyboard shortcut drag starts for PlayerController adapter"),
		Anchor->TryStartFirstPersonCardDragGesture(
			CardInstanceId,
			FVector2D(500.0f, 180.0f)));
	TestTrue(
		TEXT("PlayerController right mouse consumes shortcut cancellation"),
		FWacomBattleSceneTargetClickTestAccess::InputKey(
			PC,
			EKeys::RightMouseButton,
			IE_Pressed));
	TestFalse(
		TEXT("PlayerController adapter cancels shortcut drag"),
		Anchor->IsFirstPersonCardDragGestureActive());

	TestTrue(
		TEXT("Keyboard shortcut drag restarts for Slate adapter"),
		Anchor->TryStartFirstPersonCardDragGesture(
			CardInstanceId,
			FVector2D(500.0f, 180.0f)));
	TestTrue(
		TEXT("Slate preprocessor consumes shortcut cancellation"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonDown(
			*ViewportClient,
			SlateRightDown));
	TestFalse(
		TEXT("Slate adapter cancels shortcut drag"),
		Anchor->IsFirstPersonCardDragGestureActive());

	TestTrue(
		TEXT("Keyboard shortcut drag restarts for viewport reroute"),
		Anchor->TryStartFirstPersonCardDragGesture(
			CardInstanceId,
			FVector2D(500.0f, 180.0f)));
	TestTrue(
		TEXT("Viewport reroute consumes shortcut cancellation"),
		FWacomGameViewportClientTestAccess::DispatchReroutedInput(
			*ViewportClient,
			EKeys::RightMouseButton,
			IE_Pressed));
	TestFalse(
		TEXT("Viewport reroute cancels shortcut drag"),
		Anchor->IsFirstPersonCardDragGestureActive());

	TestTrue(
		TEXT("Keyboard shortcut drag restarts for Enhanced Input callback"),
		Anchor->TryStartFirstPersonCardDragGesture(
			CardInstanceId,
			FVector2D(500.0f, 180.0f)));
	FWacomBattleSceneTargetClickTestAccess::PressWaitShortcut(PC);
	TestFalse(
		TEXT("Wait callback cancels formal drag before command routing"),
		Anchor->IsFirstPersonCardDragGestureActive());

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	TestTrue(
		TEXT("Mouse press starts a non-shortcut gesture"),
		FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(
			*Slot,
			CardPosition));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(
		*Slot,
		0.0f,
		CardPosition + FVector2D(0.0f, -80.0f));
	TestTrue(
		TEXT("Mouse drag is active"),
		Anchor->IsFirstPersonCardDragGestureActive());
	TestFalse(
		TEXT("PlayerController right mouse does not cancel mouse drag"),
		FWacomBattleSceneTargetClickTestAccess::InputKey(
			PC,
			EKeys::RightMouseButton,
			IE_Pressed));
	TestFalse(
		TEXT("Slate right mouse does not claim mouse drag"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonDown(
			*ViewportClient,
			SlateRightDown));
	FWacomGameViewportClientTestAccess::DispatchReroutedInput(
		*ViewportClient,
		EKeys::RightMouseButton,
		IE_Pressed);
	TestTrue(
		TEXT("All right-mouse adapters preserve mouse-origin drag"),
		Anchor->IsFirstPersonCardDragGestureActive());
	Anchor->CancelFirstPersonCardDragGesture(false);

	FWacomBattleSceneTargetClickTestAccess::PressPlayCardShortcut(PC, 1);
	TestFalse(
		TEXT("Number shortcut does not start the non-Battle hand"),
		Anchor->IsFirstPersonCardDragGestureActive());

	TestTrue(
		TEXT("Dual-faced card enters locked inspection"),
		EnterLockedInspection(*Layer, *Slot, CardPosition));
	TestTrue(
		TEXT("PlayerController Tab starts face toggle"),
		FWacomBattleSceneTargetClickTestAccess::InputKey(
			PC,
			EKeys::Tab,
			IE_Pressed));
	TestFalse(
		TEXT("Repeated Tab is rejected while flip playback is active"),
		FWacomBattleSceneTargetClickTestAccess::InputKey(
			PC,
			EKeys::Tab,
			IE_Pressed));
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 0.22f);
	TestTrue(
		TEXT("PlayerController gamepad B closes locked inspection"),
		FWacomBattleSceneTargetClickTestAccess::InputKey(
			PC,
			EKeys::Gamepad_FaceButton_Right,
			IE_Pressed));
	TestFalse(
		TEXT("Gamepad close leaves no locked inspection"),
		Anchor->IsFirstPersonCardLockedInspectionActive());

	TestTrue(
		TEXT("Locked inspection re-enters for blank-click routing"),
		EnterLockedInspection(*Layer, *Slot, CardPosition));
	const FPointerEvent BlankDown = MakeMouseEvent(
		EKeys::LeftMouseButton,
		true,
		FVector2D(40.0f, 40.0f));
	const FPointerEvent BlankUp = MakeMouseEvent(
		EKeys::LeftMouseButton,
		false,
		FVector2D(40.0f, 40.0f));
	TestTrue(
		TEXT("Slate blank press closes locked inspection"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonDown(
			*ViewportClient,
			BlankDown));
	TestTrue(
		TEXT("Matching Slate release is consumed exactly once"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonUp(
			*ViewportClient,
			BlankUp));
	TestFalse(
		TEXT("Repeated Slate release is not consumed"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonUp(
			*ViewportClient,
			BlankUp));

	TestTrue(
		TEXT("Locked inspection re-enters before interaction disable"),
		EnterLockedInspection(*Layer, *Slot, CardPosition));
	TestTrue(
		TEXT("Blank press arms matching release"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonDown(
			*ViewportClient,
			BlankDown));
	Layer->SetCardLayerInteractionEnabled(false);
	TestFalse(
		TEXT("Interaction disable clears stale release reservation"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonUp(
			*ViewportClient,
			BlankUp));
	Layer->SetCardLayerInteractionEnabled(true);

	TestTrue(
		TEXT("Locked inspection re-enters before Pawn teardown"),
		EnterLockedInspection(*Layer, *Slot, CardPosition));
	PC->UnPossess();
	TestFalse(
		TEXT("Pawn teardown clears active locked inspection"),
		Layer->IsLockedCardInspectionActive());
	TestFalse(
		TEXT("Pawn teardown leaves no stale release reservation"),
		Layer->ConsumePendingLockedInspectionPointerRelease());

	Cleanup();
	return true;
}

#endif
