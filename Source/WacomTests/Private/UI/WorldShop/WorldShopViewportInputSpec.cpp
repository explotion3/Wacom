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

	FKeyEvent MakeKeyEvent(FKey Key)
	{
		return FKeyEvent(
			Key,
			FModifierKeysState(),
			0,
			false,
			0,
			0);
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
	FWacomGameViewportClientTestAccess::SetPreUiInputRouteOverride(
		*ViewportClient,
		true);
	FWacomGameViewportClientTestAccess::RegisterInputPreProcessor(*ViewportClient);

	const FPointerEvent LeftDown =
		WacomWorldShopViewportInputSpec::MakeMouseEvent(EKeys::LeftMouseButton, true);
	const FPointerEvent LeftUp =
		WacomWorldShopViewportInputSpec::MakeMouseEvent(EKeys::LeftMouseButton, false);

	FWacomGameViewportClientTestAccess::SetRouteOverrides(
		*ViewportClient,
		false,
		nullptr);
	TestFalse(TEXT("an editor-side release without an owned press is not consumed"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonUp(
			*ViewportClient,
			LeftUp));
	TestEqual(
		TEXT("an editor-side release is never forwarded to the pre-UI route"),
		FWacomGameViewportClientTestAccess::GetPreUiInputRouteEvents(
			*ViewportClient).Num(),
		0);

	FWacomGameViewportClientTestAccess::SetRouteOverrides(
		*ViewportClient,
		true,
		nullptr);
	TestTrue(TEXT("left press is consumed before Slate under NoCapture"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonDown(
			*ViewportClient,
			LeftDown));
	FWacomGameViewportClientTestAccess::SetRouteOverrides(
		*ViewportClient,
		false,
		nullptr);
	TestTrue(TEXT("an owned release stays routed after leaving the game viewport"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonUp(
			*ViewportClient,
			LeftUp));

	const TArray<FKey>& RoutedKeys =
		FWacomGameViewportClientTestAccess::GetPreUiInputRouteKeys(*ViewportClient);
	const TArray<EInputEvent>& RoutedEvents =
		FWacomGameViewportClientTestAccess::GetPreUiInputRouteEvents(*ViewportClient);
	TestEqual(TEXT("press and release are routed exactly once"), RoutedEvents.Num(), 2);
	TestEqual(TEXT("press and release retain their key identity"), RoutedKeys.Num(), 2);
	if (RoutedEvents.Num() == 2 && RoutedKeys.Num() == 2)
	{
		TestEqual(TEXT("first key is left mouse"), RoutedKeys[0], EKeys::LeftMouseButton);
		TestEqual(TEXT("second key is left mouse"), RoutedKeys[1], EKeys::LeftMouseButton);
		TestEqual(TEXT("first event is press"), RoutedEvents[0], IE_Pressed);
		TestEqual(TEXT("second event is release"), RoutedEvents[1], IE_Released);
	}

	FWacomGameViewportClientTestAccess::SetPreUiInputRouteOverride(
		*ViewportClient,
		TOptional<bool>());
	FWacomGameViewportClientTestAccess::SetRouteOverrides(
		*ViewportClient,
		TOptional<bool>(),
		nullptr);
	FWacomGameViewportClientTestAccess::UnregisterInputPreProcessor(*ViewportClient);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopViewportEscapeInputSpec,
	"Wacom.UI.WorldShop.Input.NoCaptureSlatePreprocessorRoutesScopedEscape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopViewportEscapeInputSpec::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Slate application is initialized"), FSlateApplication::IsInitialized()))
	{
		return false;
	}
	UWacomGameViewportClient* ViewportClient =
		NewObject<UWacomGameViewportClient>(GEngine);
	if (!TestNotNull(TEXT("Wacom viewport client"), ViewportClient))
	{
		return false;
	}

	FWacomGameViewportClientTestAccess::SetRouteOverrides(
		*ViewportClient,
		true,
		nullptr);
	FWacomGameViewportClientTestAccess::SetPreUiInputRouteOverride(
		*ViewportClient,
		true);
	FWacomGameViewportClientTestAccess::RegisterInputPreProcessor(*ViewportClient);

	TestTrue(
		TEXT("Escape is routed while the game viewport owns keyboard context"),
		FWacomGameViewportClientTestAccess::DispatchKeyDown(
			*ViewportClient,
			WacomWorldShopViewportInputSpec::MakeKeyEvent(EKeys::Escape)));
	TestFalse(
		TEXT("unrelated keyboard keys remain available to the normal input path"),
		FWacomGameViewportClientTestAccess::DispatchKeyDown(
			*ViewportClient,
			WacomWorldShopViewportInputSpec::MakeKeyEvent(EKeys::A)));

	const TArray<FKey>& RoutedKeys =
		FWacomGameViewportClientTestAccess::GetPreUiInputRouteKeys(
			*ViewportClient);
	TestEqual(TEXT("only Escape reached the World Shop route"), RoutedKeys.Num(), 1);
	if (RoutedKeys.Num() == 1)
	{
		TestEqual(TEXT("routed key is Escape"), RoutedKeys[0], EKeys::Escape);
	}

	FWacomGameViewportClientTestAccess::SetRouteOverrides(
		*ViewportClient,
		false,
		nullptr);
	TestFalse(
		TEXT("Escape outside this game viewport is not consumed"),
		FWacomGameViewportClientTestAccess::DispatchKeyDown(
			*ViewportClient,
			WacomWorldShopViewportInputSpec::MakeKeyEvent(EKeys::Escape)));

	FWacomGameViewportClientTestAccess::SetPreUiInputRouteOverride(
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
