// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomGameViewportClient.h"

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/WacomPlayerController.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Layout/WidgetPath.h"
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
	if (PointerEvent.GetEffectingButton() != EKeys::RightMouseButton
		|| !IsPointerEventInsideOwnGameViewport(SlateApp, PointerEvent))
	{
		return false;
	}

	AWacomPlayerController* PlayerController =
		ResolveWacomPlayerController(PointerEvent.GetInputDeviceId());
	return PlayerController
		&& PlayerController->TryCancelFirstPersonCardKeyboardShortcutDrag();
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
	if (Key == EKeys::RightMouseButton && EventType == IE_Pressed)
	{
		AWacomPlayerController* PlayerController =
			ResolveWacomPlayerController(DeviceId);
		if (PlayerController
			&& PlayerController->TryCancelFirstPersonCardKeyboardShortcutDrag())
		{
			Reply = FReply::Handled();
			return;
		}
	}

	Super::HandleRerouteInput(DeviceId, Key, EventType, Reply);
}
