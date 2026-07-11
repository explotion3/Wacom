// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "Widgets/SVirtualWindow.h"

namespace WacomFirstPersonCardLayerInputCaptureSpec
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

	FWacomFirstPersonCardLayerSlotView MakeInteractiveSlot(const FGuid& CardInstanceId)
	{
		const FVector2D Position(400.0f, 300.0f);
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.bIsPlayable = true;
		Slot.Entry.InteractionIntent = EWacomFirstPersonCardInteractionIntent::DragToDropTarget;
		Slot.ScreenPosition = Position;
		Slot.WidgetPosition = Position;
		Slot.SnappedWidgetPosition = Position;
		Slot.InputHitCenter = Position;
		Slot.InputHitScale = 1.0f;
		Slot.InputHitAngleDegrees = 0.0f;
		Slot.InputHitOrder = 0;
		Slot.RenderScale = 1.0f;
		Slot.RenderOpacity = 1.0f;
		Slot.bProjected = true;
		return Slot;
	}

	class FScopedVirtualWindowRegistration
	{
	public:
		FScopedVirtualWindowRegistration(
			FSlateApplication& InApplication,
			const TSharedRef<SWidget>& Content)
			: Application(InApplication)
			, Window(SNew(SVirtualWindow).Size(FVector2D(1280.0f, 720.0f)))
		{
			Window->SetContent(Content);
			Application.RegisterVirtualWindow(Window);
			Window->SlatePrepass(1.0f);
		}

		~FScopedVirtualWindowRegistration()
		{
			Application.ReleaseAllPointerCapture();
			Application.UnregisterVirtualWindow(Window);
		}

	private:
		FSlateApplication& Application;
		TSharedRef<SVirtualWindow> Window;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDisableInteractionReleasesMouseCaptureTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.DisableInteractionReleasesSlateMouseCapture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDisableInteractionReleasesMouseCaptureTest::RunTest(
	const FString& Parameters)
{
	using namespace WacomFirstPersonCardLayerInputCaptureSpec;

	if (!TestTrue(TEXT("Slate application is initialized"), FSlateApplication::IsInitialized()))
	{
		return false;
	}

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({ MakeInteractiveSlot(CardId) });
	UWacomFirstPersonCardLayerSlotWidget* Slot = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Interactive slot"), Slot))
	{
		PC->Destroy();
		return false;
	}
	FWacomFirstPersonCardSlotMotionConfig Motion;
	Motion.bEnabled = false;
	Slot->SetSlotMotionConfig(Motion);
	Slot->SetSlotViewImmediate(MakeInteractiveSlot(CardId));
	Slot->SetCardLayerInteractionEnabled(true);

	TSharedRef<SWidget> LayerSlateWidget = Layer->TakeWidget();
	TSharedRef<SWidget> SlotSlateWidget = Slot->TakeWidget();
	FSlateApplication& SlateApplication = FSlateApplication::Get();
	SlateApplication.ReleaseAllPointerCapture();
	{
		FScopedVirtualWindowRegistration WindowRegistration(SlateApplication, LayerSlateWidget);

		FWidgetPath SlotPath;
		if (!TestTrue(
			TEXT("Virtual window exposes a Slate path to the card slot"),
			SlateApplication.GeneratePathToWidgetUnchecked(
				SlotSlateWidget,
				SlotPath,
				EVisibility::All)))
		{
			PC->Destroy();
			return false;
		}

		const TOptional<FArrangedWidget> ArrangedSlot = SlotPath.FindArrangedWidget(SlotSlateWidget);
		if (!TestTrue(TEXT("Slate path contains arranged card slot"), ArrangedSlot.IsSet()))
		{
			PC->Destroy();
			return false;
		}

		const FVector2D PointerPosition(400.0f, 300.0f);
		if (!TestEqual(
			TEXT("Production card pointer route requests capture"),
			FWacomFirstPersonCardLayerTestAccess::RequestPressRouteActionAtWidgetPosition(
				*Layer,
				PointerPosition),
			EWacomFirstPersonCardPointerRouteAction::CaptureMouse))
		{
			PC->Destroy();
			return false;
		}
		const FReply MouseDownReply = FReply::Handled().CaptureMouse(SlotSlateWidget);
		const FPointerEvent MouseDownEvent(
			0,
			0,
			PointerPosition,
			PointerPosition,
			TSet<FKey>(),
			EKeys::LeftMouseButton,
			0.0f,
			FModifierKeysState());

		SlateApplication.ProcessReply(
			SlotPath,
			MouseDownReply,
			&SlotPath,
			&MouseDownEvent,
			MouseDownEvent.GetUserIndex());
		if (!TestTrue(TEXT("Slate grants mouse capture to the pressed card slot"),
			SlotSlateWidget->HasMouseCapture()))
		{
			PC->Destroy();
			return false;
		}

		Layer->SetCardLayerInteractionEnabled(false);
		TestFalse(
			TEXT("Programmatically disabling the card layer immediately releases its Slate mouse capture"),
			SlotSlateWidget->HasMouseCapture());
	}

	PC->Destroy();
	return true;
}
