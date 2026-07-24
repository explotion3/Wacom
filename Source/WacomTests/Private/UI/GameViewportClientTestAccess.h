// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AWacomPlayerController;
class SWidget;
class UWacomGameViewportClient;
struct FPointerEvent;
struct FKeyEvent;

#if WITH_AUTOMATION_TESTS

struct FWacomGameViewportClientTestAccess
{
	static void RegisterInputPreProcessor(UWacomGameViewportClient& ViewportClient);
	static void UnregisterInputPreProcessor(UWacomGameViewportClient& ViewportClient);
	static bool IsInputPreProcessorRegistered(
		const UWacomGameViewportClient& ViewportClient);
	static const void* InputPreProcessorAddress(
		const UWacomGameViewportClient& ViewportClient);
	static bool DispatchMouseButtonDown(
		UWacomGameViewportClient& ViewportClient,
		const FPointerEvent& PointerEvent);
	static bool DispatchMouseButtonUp(
		UWacomGameViewportClient& ViewportClient,
		const FPointerEvent& PointerEvent);
	static bool DispatchReroutedInput(
		UWacomGameViewportClient& ViewportClient,
		FKey Key,
		EInputEvent Event);
	static bool DispatchKeyDown(
		UWacomGameViewportClient& ViewportClient,
		const FKeyEvent& KeyEvent);
	static void SetRouteOverrides(
		UWacomGameViewportClient& ViewportClient,
		TOptional<bool> bPointerInsideViewport,
		AWacomPlayerController* PlayerController);
	static void SetPreUiInputRouteOverride(
		UWacomGameViewportClient& ViewportClient,
		TOptional<bool> RouteResult);
	static const TArray<FKey>& GetPreUiInputRouteKeys(
		const UWacomGameViewportClient& ViewportClient);
	static const TArray<EInputEvent>& GetPreUiInputRouteEvents(
		const UWacomGameViewportClient& ViewportClient);
	static bool HasProjectOwnedFocusPresentation(
		const TSharedPtr<SWidget>& FocusedWidget);
};

#endif
