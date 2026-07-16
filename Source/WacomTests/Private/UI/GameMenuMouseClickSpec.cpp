// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "UI/Foundation/WacomMenuButtonWidget.h"
#include "UI/Map/WacomRunMapScreen.h"
#include "UI/Map/WacomRunMapScreenTypes.h"
#include "UI/Menus/WacomPauseMenuScreen.h"
#include "UI/RunMapScreenTestAccess.h"
#include "Widgets/SVirtualWindow.h"

namespace WacomGameMenuMouseClickSpec
{
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
			FSlateWindowElementList ElementList(Window);
			Window->PaintWindow(
				Application.GetCurrentTime(),
				Application.GetDeltaTime(),
				ElementList,
				FWidgetStyle(),
				true);
		}

		~FScopedVirtualWindowRegistration()
		{
			Application.ReleaseAllPointerCapture();
			Application.UnregisterVirtualWindow(Window);
		}

		const TSharedRef<SVirtualWindow>& GetWindow() const
		{
			return Window;
		}

	private:
		FSlateApplication& Application;
		TSharedRef<SVirtualWindow> Window;
	};

	bool RouteMouseClick(
		FAutomationTestBase& Test,
		FSlateApplication& Application,
		const TSharedRef<SVirtualWindow>& Window,
		UWidget& Target)
	{
		const TSharedRef<SWidget> TargetSlateWidget = Target.TakeWidget();
		FWidgetPath TargetPath;
		if (!Test.TestTrue(
			TEXT("Virtual window exposes a Slate path to the target button"),
			Application.GeneratePathToWidgetUnchecked(
				TargetSlateWidget,
				TargetPath,
				EVisibility::All)))
		{
			return false;
		}

		const TOptional<FArrangedWidget> ArrangedTarget =
			TargetPath.FindArrangedWidget(TargetSlateWidget);
		if (!Test.TestTrue(TEXT("Target button has arranged Slate geometry"), ArrangedTarget.IsSet()))
		{
			return false;
		}

		const FVector2D PointerPosition = ArrangedTarget->Geometry.GetAbsolutePosition()
			+ ArrangedTarget->Geometry.GetAbsoluteSize() * 0.5f;
		const FWidgetPath HitPath = Application.LocateWindowUnderMouse(
			PointerPosition,
			{ Window });
		Test.AddInfo(FString::Printf(TEXT("Slate hit path: %s"), *HitPath.ToString()));
		if (!Test.TestTrue(TEXT("Target button participates in the Slate hit-test path"),
			HitPath.IsValid() && HitPath.ContainsWidget(&TargetSlateWidget.Get())))
		{
			return false;
		}

		TSet<FKey> PressedButtons;
		PressedButtons.Add(EKeys::LeftMouseButton);
		const FPointerEvent MouseMove(
			0,
			0,
			PointerPosition,
			PointerPosition - FVector2D(1.0f, 0.0f),
			TSet<FKey>(),
			EKeys::Invalid,
			0.0f,
			FModifierKeysState());
		const FPointerEvent MouseDown(
			0,
			0,
			PointerPosition,
			PointerPosition,
			PressedButtons,
			EKeys::LeftMouseButton,
			0.0f,
			FModifierKeysState());
		const FPointerEvent MouseUp(
			0,
			0,
			PointerPosition,
			PointerPosition,
			TSet<FKey>(),
			EKeys::LeftMouseButton,
			0.0f,
			FModifierKeysState());

		Application.RoutePointerMoveEvent(HitPath, MouseMove, false);
		Test.TestTrue(TEXT("Pointer move establishes button hover"),
			CastChecked<UWacomMenuButtonWidget>(&Target)->IsHovered());
		const FReply DownReply = Application.RoutePointerDownEvent(HitPath, MouseDown);
		Test.TestTrue(TEXT("Mouse route establishes button pressed state"),
			CastChecked<UWacomMenuButtonWidget>(&Target)->IsPressed());
		Test.TestTrue(TEXT("Mouse down grants Slate capture"), Application.HasAnyMouseCaptor());
		const FReply UpReply = Application.RoutePointerUpEvent(HitPath, MouseUp);
		Test.TestTrue(TEXT("Mouse down is handled by the button path"), DownReply.IsEventHandled());
		Test.TestTrue(TEXT("Mouse up is handled by the button path"), UpReply.IsEventHandled());
		return DownReply.IsEventHandled() && UpReply.IsEventHandled();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomGameMenuNativeButtonsReceiveSlateMouseClicksTest,
	"Wacom.UI.GameMenu.PointerRouting.NativeButtonsReceiveSlateMouseClicks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomGameMenuNativeButtonsReceiveSlateMouseClicksTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomGameMenuMouseClickSpec;
	if (!TestTrue(TEXT("Slate application is initialized"), FSlateApplication::IsInitialized()))
	{
		return false;
	}

	FSlateApplication& Application = FSlateApplication::Get();
	Application.ReleaseAllPointerCapture();

	UWacomPauseMenuScreen* PauseMenu = NewObject<UWacomPauseMenuScreen>();
	const TSharedRef<SWidget> PauseSlateWidget = PauseMenu->TakeWidget();
	UWacomMenuButtonWidget* ResumeButton = Cast<UWacomMenuButtonWidget>(
		PauseMenu->GetWidgetFromName(TEXT("ResumeButton")));
	if (!TestNotNull(TEXT("Pause menu exposes its native Resume button"), ResumeButton))
	{
		return false;
	}
	int32 PauseClickCount = 0;
	ResumeButton->OnClicked().AddLambda([&PauseClickCount]() { ++PauseClickCount; });
	{
		FScopedVirtualWindowRegistration Window(Application, PauseSlateWidget);
		RouteMouseClick(*this, Application, Window.GetWindow(), *ResumeButton);
	}
	TestEqual(TEXT("Pause Resume action receives one real Slate mouse click"), PauseClickCount, 1);

	UWidgetBlueprintGeneratedClass* ScreenClass = LoadObject<UWidgetBlueprintGeneratedClass>(
		nullptr,
		TEXT("/Game/Wacom/UI/Map/WBP_RunMapScreen.WBP_RunMapScreen_C"));
	if (!TestNotNull(TEXT("Authored Run Map screen class"), ScreenClass))
	{
		return false;
	}

	UWacomRunMapScreen* MapScreen = NewObject<UWacomRunMapScreen>(
		GetTransientPackage(),
		ScreenClass);
	FWacomRunMapScreenTestAccess::BuildAndConstruct(*MapScreen);
	FWacomRunMapScreenViewData ViewData;
	ViewData.StateVersion = 1;
	ViewData.bIsAvailable = true;
	FWacomRunMapNodeViewData Node;
	Node.Handle = { TEXT("Floor.Test"), TEXT("Node.Destination") };
	Node.bCanSelect = true;
	Node.bCanTravel = true;
	ViewData.Nodes.Add(Node);
	ViewData.SelectedNode = Node.Handle;
	ViewData.DefaultFocusNode = Node.Handle;
	MapScreen->ApplyViewData(ViewData);

	UWacomMenuButtonWidget* TravelButton = Cast<UWacomMenuButtonWidget>(
		MapScreen->GetWidgetFromName(TEXT("TravelButton")));
	if (!TestNotNull(TEXT("Authored Run Map exposes Travel button"), TravelButton))
	{
		FWacomRunMapScreenTestAccess::Destruct(*MapScreen);
		return false;
	}
	int32 MapClickCount = 0;
	TravelButton->OnClicked().AddLambda([&MapClickCount]() { ++MapClickCount; });
	{
		FScopedVirtualWindowRegistration Window(Application, MapScreen->TakeWidget());
		RouteMouseClick(*this, Application, Window.GetWindow(), *TravelButton);
	}
	TestEqual(TEXT("Run Map Travel action receives one real Slate mouse click"), MapClickCount, 1);
	FWacomRunMapScreenTestAccess::Destruct(*MapScreen);

	return !HasAnyErrors();
}

#endif
