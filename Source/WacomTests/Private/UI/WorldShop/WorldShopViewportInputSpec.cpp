// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "UI/Foundation/WacomGameViewportClient.h"
#include "UI/GameViewportClientTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomWorldShopViewportInputSpec
{
	FPointerEvent MakeMouseEvent(FKey EffectingButton, bool bPressed)
	{
		TSet<FKey> PressedButtons;
		if (bPressed)
		{
			PressedButtons.Add(EffectingButton);
		}
		return FPointerEvent(
			0,
			FVector2D(500.0f, 180.0f),
			FVector2D(500.0f, 180.0f),
			PressedButtons,
			EffectingButton,
			0.0f,
			FModifierKeysState());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopViewportInputSpec,
	"Wacom.UI.WorldShop.Input.NoCaptureSlatePreprocessorRoutesPressAndRelease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopViewportInputSpec::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Slate application is initialized"), FSlateApplication::IsInitialized()))
	{
		return false;
	}
	UWacomGameViewportClient* ViewportClient = NewObject<UWacomGameViewportClient>(GEngine);
	if (!TestNotNull(TEXT("Wacom viewport client"), ViewportClient))
	{
		return false;
	}

	FWacomGameViewportClientTestAccess::SetRouteOverrides(
		*ViewportClient,
		true,
		nullptr);
	FWacomGameViewportClientTestAccess::SetWorldShopPointerRouteOverride(
		*ViewportClient,
		true);
	FWacomGameViewportClientTestAccess::RegisterInputPreProcessor(*ViewportClient);

	const FPointerEvent LeftDown =
		WacomWorldShopViewportInputSpec::MakeMouseEvent(EKeys::LeftMouseButton, true);
	const FPointerEvent LeftUp =
		WacomWorldShopViewportInputSpec::MakeMouseEvent(EKeys::LeftMouseButton, false);
	TestTrue(TEXT("left press is consumed before Slate under NoCapture"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonDown(
			*ViewportClient,
			LeftDown));
	TestTrue(TEXT("left release is consumed and closes the virtual pointer gesture"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonUp(
			*ViewportClient,
			LeftUp));

	const TArray<EInputEvent>& RoutedEvents =
		FWacomGameViewportClientTestAccess::GetWorldShopPointerRouteEvents(*ViewportClient);
	TestEqual(TEXT("press and release are routed exactly once"), RoutedEvents.Num(), 2);
	if (RoutedEvents.Num() == 2)
	{
		TestEqual(TEXT("first event is press"), RoutedEvents[0], IE_Pressed);
		TestEqual(TEXT("second event is release"), RoutedEvents[1], IE_Released);
	}

	FWacomGameViewportClientTestAccess::SetWorldShopPointerRouteOverride(
		*ViewportClient,
		TOptional<bool>());
	FWacomGameViewportClientTestAccess::SetRouteOverrides(
		*ViewportClient,
		TOptional<bool>(),
		nullptr);
	FWacomGameViewportClientTestAccess::UnregisterInputPreProcessor(*ViewportClient);
	return true;
}

#endif
