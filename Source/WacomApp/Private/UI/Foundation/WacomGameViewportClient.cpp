// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomGameViewportClient.h"

#include "CommonButtonBase.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/WacomPlayerController.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Layout/WidgetPath.h"
#include "Types/ReflectionMetadata.h"
#include "UI/Foundation/WacomMenuButtonWidget.h"
#include "UI/Menus/WacomMainMenuScreen.h"
#include "UI/Settings/WacomSettingsOptionRow.h"
#include "UI/Card/WacomFirstPersonCardInputRouter.h"
#include "Widgets/SViewport.h"

class FWacomFirstPersonCardInputPreprocessor final : public IInputProcessor
{
public:
	explicit FWacomFirstPersonCardInputPreprocessor(
		UWacomGameViewportClient& InViewportClient)
		: ViewportClient(&InViewportClient)
	{
	}

	virtual bool HandleMouseButtonDownEvent(
		FSlateApplication& SlateApp,
		const FPointerEvent& PointerEvent) override
	{
		UWacomGameViewportClient* ViewportClientPtr = ViewportClient.Get();
		return ViewportClientPtr
			&& ViewportClientPtr->HandlePreprocessedMouseButtonDown(
				SlateApp,
				PointerEvent);
	}

	virtual bool HandleMouseButtonUpEvent(
		FSlateApplication& SlateApp,
		const FPointerEvent& PointerEvent) override
	{
		UWacomGameViewportClient* ViewportClientPtr = ViewportClient.Get();
		return ViewportClientPtr
			&& ViewportClientPtr->HandlePreprocessedMouseButtonUp(
				SlateApp,
				PointerEvent);
	}

	virtual void Tick(
		const float /*DeltaTime*/,
		FSlateApplication& /*SlateApp*/,
		TSharedRef<ICursor> /*Cursor*/) override
	{
	}

private:
	TWeakObjectPtr<UWacomGameViewportClient> ViewportClient;
};

void UWacomGameViewportClient::Init(
	FWorldContext& WorldContext,
	UGameInstance* OwningGameInstance,
	bool bCreateNewAudioDevice)
{
	Super::Init(WorldContext, OwningGameInstance, bCreateNewAudioDevice);
	RegisterFirstPersonCardInputPreProcessor();
}

void UWacomGameViewportClient::DetachViewportClient()
{
	UnregisterFirstPersonCardInputPreProcessor();
	Super::DetachViewportClient();
}

void UWacomGameViewportClient::BeginDestroy()
{
	UnregisterFirstPersonCardInputPreProcessor();
	Super::BeginDestroy();
}

TOptional<bool> UWacomGameViewportClient::QueryShowFocus(
	EFocusCause InFocusCause) const
{
	if (FSlateApplication::IsInitialized()
		&& HasProjectOwnedFocusPresentation(
			FSlateApplication::Get().GetUserFocusedWidget(/*UserIndex*/0)))
	{
		return false;
	}

	return Super::QueryShowFocus(InFocusCause);
}

bool UWacomGameViewportClient::HasProjectOwnedFocusPresentation(
	const TSharedPtr<SWidget>& FocusedWidget)
{
	for (TSharedPtr<SWidget> Widget = FocusedWidget;
		Widget.IsValid();
		Widget = Widget->GetParentWidget())
	{
		const TSharedPtr<FReflectionMetaData> ReflectionMetaData =
			Widget->GetMetaData<FReflectionMetaData>();
		if (ReflectionMetaData.IsValid()
			&& ReflectionMetaData->SourceObject.IsValid()
			&& ReflectionMetaData->SourceObject->IsA<UWacomSettingsOptionRow>())
		{
			return true;
		}

		const TSharedPtr<FCommonButtonMetaData> ButtonMetaData =
			Widget->GetMetaData<FCommonButtonMetaData>();
		const UCommonButtonBase* CommonButton = ButtonMetaData.IsValid()
			? ButtonMetaData->OwningCommonButton.Get()
			: nullptr;
		if (CommonButton)
		{
			return CommonButton->IsA<UWacomMainMenuButtonWidget>()
				|| CommonButton->IsA<UWacomMenuButtonWidget>();
		}
	}

	return false;
}

void UWacomGameViewportClient::RegisterFirstPersonCardInputPreProcessor()
{
	if (FirstPersonCardInputPreProcessor.IsValid()
		|| !FSlateApplication::IsInitialized())
	{
		return;
	}

	FirstPersonCardInputPreProcessor =
		MakeShared<FWacomFirstPersonCardInputPreprocessor>(*this);
	FSlateApplication::Get().RegisterInputPreProcessor(
		FirstPersonCardInputPreProcessor,
		EInputPreProcessorType::Game);
}

void UWacomGameViewportClient::UnregisterFirstPersonCardInputPreProcessor()
{
	if (!FirstPersonCardInputPreProcessor.IsValid())
	{
		return;
	}

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(
			FirstPersonCardInputPreProcessor);
	}
	FirstPersonCardInputPreProcessor.Reset();
}

bool UWacomGameViewportClient::HandlePreprocessedMouseButtonDown(
	FSlateApplication& SlateApp,
	const FPointerEvent& PointerEvent)
{
	if (!IsPointerEventInsideOwnGameViewport(SlateApp, PointerEvent))
	{
		return false;
	}
	const FKey Button = PointerEvent.GetEffectingButton();
	if (Button != EKeys::LeftMouseButton
		&& Button != EKeys::RightMouseButton)
	{
		return false;
	}
	return TryRoutePreprocessedInput(
		PointerEvent.GetInputDeviceId(),
		Button,
		IE_Pressed,
		FVector2D(PointerEvent.GetScreenSpacePosition()));
}

bool UWacomGameViewportClient::HandlePreprocessedMouseButtonUp(
	FSlateApplication& SlateApp,
	const FPointerEvent& PointerEvent)
{
	if (PointerEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return false;
	}
	// Release must remain routable after the pointer leaves the viewport so a
	// pre-UI owner can clear its paired press without leaking into the world.
	return TryRoutePreprocessedInput(
		PointerEvent.GetInputDeviceId(),
		EKeys::LeftMouseButton,
		IE_Released,
		FVector2D(PointerEvent.GetScreenSpacePosition()));
}

bool UWacomGameViewportClient::TryRoutePreprocessedInput(
	FInputDeviceId DeviceId,
	FKey Key,
	EInputEvent Event,
	const TOptional<FVector2D>& AbsoluteScreenPosition)
{
#if WITH_AUTOMATION_TESTS
	if (PreUiInputRouteResultOverrideForAutomation.IsSet())
	{
		PreUiInputRouteKeysForAutomation.Add(Key);
		PreUiInputRouteEventsForAutomation.Add(Event);
		return PreUiInputRouteResultOverrideForAutomation.GetValue();
	}
#endif

	AWacomPlayerController* PlayerController = ResolveWacomPlayerController(DeviceId);
	if (!PlayerController)
	{
		return false;
	}
	FWacomFirstPersonCardInputEvent Input;
	Input.Key = Key;
	Input.Event = Event;
	Input.Adapter = EWacomFirstPersonCardInputAdapter::SlatePreprocessor;
	Input.AbsoluteScreenPosition = AbsoluteScreenPosition;
	return PlayerController->TryRoutePreUiInput(Input);
}

bool UWacomGameViewportClient::TryRouteReroutedInput(
	FInputDeviceId DeviceId,
	FKey Key,
	EInputEvent Event)
{
	AWacomPlayerController* PlayerController = ResolveWacomPlayerController(DeviceId);
	if (!PlayerController)
	{
		return false;
	}
	FWacomFirstPersonCardInputEvent Input;
	Input.Key = Key;
	Input.Event = Event;
	Input.Adapter = EWacomFirstPersonCardInputAdapter::ViewportReroute;
	if (FSlateApplication::IsInitialized())
	{
		Input.AbsoluteScreenPosition = FSlateApplication::Get().GetCursorPos();
	}
	return PlayerController->TryRoutePreUiInput(Input);
}

bool UWacomGameViewportClient::IsPointerEventInsideOwnGameViewport(
	FSlateApplication& SlateApp,
	const FPointerEvent& PointerEvent) const
{
#if WITH_AUTOMATION_TESTS
	if (PointerInsideViewportOverrideForAutomation.IsSet())
	{
		return PointerInsideViewportOverrideForAutomation.GetValue();
	}
#endif

	const TSharedPtr<SViewport> ViewportWidget = GetGameViewportWidget();
	if (!ViewportWidget.IsValid())
	{
		return false;
	}

	const FWidgetPath WidgetsUnderPointer = SlateApp.LocateWindowUnderMouse(
		PointerEvent.GetScreenSpacePosition(),
		SlateApp.GetInteractiveTopLevelWindows(),
		/*bIgnoreEnabledStatus*/ false,
		PointerEvent.GetUserIndex());
	return WidgetsUnderPointer.IsValid()
		&& WidgetsUnderPointer.ContainsWidget(ViewportWidget.Get());
}

AWacomPlayerController* UWacomGameViewportClient::ResolveWacomPlayerController(
	FInputDeviceId DeviceId) const
{
#if WITH_AUTOMATION_TESTS
	if (AWacomPlayerController* PlayerControllerOverride =
		PlayerControllerOverrideForAutomation.Get())
	{
		return PlayerControllerOverride;
	}
#endif

	UGameInstance* OwningGameInstance = GetGameInstance();
	ULocalPlayer* LocalPlayer = OwningGameInstance
		? OwningGameInstance->FindLocalPlayerFromDeviceId(DeviceId)
		: nullptr;
	if (!LocalPlayer && OwningGameInstance)
	{
		LocalPlayer = OwningGameInstance->GetFirstGamePlayer();
	}

	AWacomPlayerController* PlayerController = LocalPlayer
		? Cast<AWacomPlayerController>(LocalPlayer->PlayerController)
		: nullptr;
	if (!PlayerController && GetWorld())
	{
		PlayerController = Cast<AWacomPlayerController>(
			GetWorld()->GetFirstPlayerController());
	}
	return PlayerController;
}

void UWacomGameViewportClient::HandleRerouteInput(
	FInputDeviceId DeviceId,
	FKey Key,
	EInputEvent EventType,
	FReply& Reply)
{
	const bool bLeftPointerEvent =
		Key == EKeys::LeftMouseButton
		&& (EventType == IE_Pressed || EventType == IE_Released);
	const bool bRightPointerCancel =
		Key == EKeys::RightMouseButton
		&& EventType == IE_Pressed;
	if (bLeftPointerEvent || bRightPointerCancel)
	{
		if (TryRouteReroutedInput(DeviceId, Key, EventType))
		{
			Reply = FReply::Handled();
			return;
		}
	}

	Super::HandleRerouteInput(DeviceId, Key, EventType, Reply);
}
