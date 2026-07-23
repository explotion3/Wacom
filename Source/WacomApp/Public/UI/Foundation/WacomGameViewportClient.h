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
 * CommonUI-compatible viewport input owner for Wacom-wide pre-UI gestures.
 *
 * Mouse button presses can be skipped by FSceneViewport::InputKey while the
 * viewport uses NoCapture. A scoped Slate input preprocessor therefore owns the
 * primary shortcut-drag cancel path; CommonUI rerouting remains a fallback.
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
	bool TryRouteWorldShopPointerInput(
		FInputDeviceId DeviceId,
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
	TOptional<bool> WorldShopPointerRouteResultOverrideForAutomation;
	TArray<EInputEvent> WorldShopPointerRouteEventsForAutomation;
#endif

	friend class FWacomFirstPersonCardInputPreprocessor;
	friend struct FWacomGameViewportClientTestAccess;
};
