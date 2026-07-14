// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AWacomPlayerController;
class SWidget;
class UWacomGameViewportClient;
struct FPointerEvent;

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
	static void SetRouteOverrides(
		UWacomGameViewportClient& ViewportClient,
		TOptional<bool> bPointerInsideViewport,
		AWacomPlayerController* PlayerController);
	static bool HasProjectOwnedFocusPresentation(
		const TSharedPtr<SWidget>& FocusedWidget);
};

#endif
