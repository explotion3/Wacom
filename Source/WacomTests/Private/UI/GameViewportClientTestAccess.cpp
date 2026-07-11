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

void FWacomGameViewportClientTestAccess::SetRouteOverrides(
	UWacomGameViewportClient& ViewportClient,
	TOptional<bool> bPointerInsideViewport,
	AWacomPlayerController* PlayerController)
{
	ViewportClient.PointerInsideViewportOverrideForAutomation =
		bPointerInsideViewport;
	ViewportClient.PlayerControllerOverrideForAutomation = PlayerController;
}

#endif
