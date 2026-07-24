// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CommonGameViewportClient.h"
#include "WacomGameViewportClient.generated.h"

class AWacomPlayerController;
class FSlateApplication;
class IInputProcessor;
class SWidget;
struct FPointerEvent;
struct FWacomGameViewportClientTestAccess;

/**
 * CommonUI-compatible viewport adapter for Wacom-wide pre-UI gestures.
 *
 * Mouse button presses can be skipped by FSceneViewport::InputKey while the
 * viewport uses NoCapture. A scoped Slate input preprocessor forwards the
 * limited pointer subset to PlayerController's native pre-UI route; policy and
 * gesture truth remain outside the ViewportClient.
 */
UCLASS(Within = Engine, transient, config = Engine)
class WACOMAPP_API UWacomGameViewportClient : public UCommonGameViewportClient
{
	GENERATED_BODY()

public:
	virtual void Init(
		FWorldContext& WorldContext,
		UGameInstance* OwningGameInstance,
		bool bCreateNewAudioDevice = true) override;
	virtual void DetachViewportClient() override;
	virtual void BeginDestroy() override;
	virtual TOptional<bool> QueryShowFocus(EFocusCause InFocusCause) const override;

	virtual void HandleRerouteInput(
		FInputDeviceId DeviceId,
		FKey Key,
		EInputEvent EventType,
		FReply& Reply) override;

private:
	void RegisterFirstPersonCardInputPreProcessor();
	void UnregisterFirstPersonCardInputPreProcessor();
	bool HandlePreprocessedMouseButtonDown(
		FSlateApplication& SlateApp,
		const FPointerEvent& PointerEvent);
	bool HandlePreprocessedMouseButtonUp(
		FSlateApplication& SlateApp,
		const FPointerEvent& PointerEvent);
	bool TryRoutePreprocessedInput(
		FInputDeviceId DeviceId,
		FKey Key,
		EInputEvent Event,
		const TOptional<FVector2D>& AbsoluteScreenPosition);
	bool TryRouteReroutedInput(
		FInputDeviceId DeviceId,
		FKey Key,
		EInputEvent Event);
	bool IsPointerEventInsideOwnGameViewport(
		FSlateApplication& SlateApp,
		const FPointerEvent& PointerEvent) const;
	AWacomPlayerController* ResolveWacomPlayerController(FInputDeviceId DeviceId) const;
	static bool HasProjectOwnedFocusPresentation(
		const TSharedPtr<SWidget>& FocusedWidget);

	TSharedPtr<IInputProcessor> FirstPersonCardInputPreProcessor;

#if WITH_AUTOMATION_TESTS
	TOptional<bool> PointerInsideViewportOverrideForAutomation;
	TWeakObjectPtr<AWacomPlayerController> PlayerControllerOverrideForAutomation;
	TOptional<bool> PreUiInputRouteResultOverrideForAutomation;
	TArray<FKey> PreUiInputRouteKeysForAutomation;
	TArray<EInputEvent> PreUiInputRouteEventsForAutomation;
#endif

	friend class FWacomFirstPersonCardInputPreprocessor;
	friend struct FWacomGameViewportClientTestAccess;
};
