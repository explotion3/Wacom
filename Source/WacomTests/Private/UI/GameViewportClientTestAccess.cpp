// Copyright Wacom. All Rights Reserved.

#include "UI/GameViewportClientTestAccess.h"

#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/WacomPlayerController.h"
#include "UI/Foundation/WacomGameViewportClient.h"

#if WITH_AUTOMATION_TESTS

void FWacomGameViewportClientTestAccess::RegisterInputPreProcessor(
	UWacomGameViewportClient& ViewportClient)
{
	ViewportClient.RegisterFirstPersonCardInputPreProcessor();
}

void FWacomGameViewportClientTestAccess::UnregisterInputPreProcessor(
	UWacomGameViewportClient& ViewportClient)
{
	ViewportClient.UnregisterFirstPersonCardInputPreProcessor();
}

bool FWacomGameViewportClientTestAccess::IsInputPreProcessorRegistered(
	const UWacomGameViewportClient& ViewportClient)
{
	return FSlateApplication::IsInitialized()
		&& ViewportClient.FirstPersonCardInputPreProcessor.IsValid()
		&& FSlateApplication::Get().FindInputPreProcessor(
			ViewportClient.FirstPersonCardInputPreProcessor,
			EInputPreProcessorType::Game) != INDEX_NONE;
}

const void* FWacomGameViewportClientTestAccess::InputPreProcessorAddress(
	const UWacomGameViewportClient& ViewportClient)
{
	return ViewportClient.FirstPersonCardInputPreProcessor.Get();
}

bool FWacomGameViewportClientTestAccess::DispatchMouseButtonDown(
	UWacomGameViewportClient& ViewportClient,
	const FPointerEvent& PointerEvent)
{
	return FSlateApplication::IsInitialized()
		&& ViewportClient.FirstPersonCardInputPreProcessor.IsValid()
		&& ViewportClient.FirstPersonCardInputPreProcessor
			->HandleMouseButtonDownEvent(
				FSlateApplication::Get(),
				PointerEvent);
}

bool FWacomGameViewportClientTestAccess::DispatchMouseButtonUp(
	UWacomGameViewportClient& ViewportClient,
	const FPointerEvent& PointerEvent)
{
	return FSlateApplication::IsInitialized()
		&& ViewportClient.FirstPersonCardInputPreProcessor.IsValid()
		&& ViewportClient.FirstPersonCardInputPreProcessor
			->HandleMouseButtonUpEvent(
				FSlateApplication::Get(),
				PointerEvent);
}

bool FWacomGameViewportClientTestAccess::DispatchReroutedInput(
	UWacomGameViewportClient& ViewportClient,
	FKey Key,
	EInputEvent Event)
{
	return ViewportClient.TryRouteReroutedInput(
		FInputDeviceId::CreateFromInternalId(0),
		Key,
		Event);
}

void FWacomGameViewportClientTestAccess::SetRouteOverrides(
	UWacomGameViewportClient& ViewportClient,
	TOptional<bool> bPointerInsideViewport,
	AWacomPlayerController* PlayerController)
{
	ViewportClient.PointerInsideViewportOverrideForAutomation =
		bPointerInsideViewport;
	ViewportClient.PlayerControllerOverrideForAutomation = PlayerController;
}

void FWacomGameViewportClientTestAccess::SetPreUiInputRouteOverride(
	UWacomGameViewportClient& ViewportClient,
	TOptional<bool> RouteResult)
{
	ViewportClient.PreUiInputRouteResultOverrideForAutomation = RouteResult;
	ViewportClient.PreUiInputRouteKeysForAutomation.Reset();
	ViewportClient.PreUiInputRouteEventsForAutomation.Reset();
}

const TArray<FKey>& FWacomGameViewportClientTestAccess::GetPreUiInputRouteKeys(
	const UWacomGameViewportClient& ViewportClient)
{
	return ViewportClient.PreUiInputRouteKeysForAutomation;
}

const TArray<EInputEvent>& FWacomGameViewportClientTestAccess::GetPreUiInputRouteEvents(
	const UWacomGameViewportClient& ViewportClient)
{
	return ViewportClient.PreUiInputRouteEventsForAutomation;
}

bool FWacomGameViewportClientTestAccess::HasProjectOwnedFocusPresentation(
	const TSharedPtr<SWidget>& FocusedWidget)
{
	return UWacomGameViewportClient::HasProjectOwnedFocusPresentation(FocusedWidget);
}

#endif
