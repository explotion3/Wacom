// Copyright Wacom. All Rights Reserved.

#include "UI/Settings/WacomSettingsScreenFlow.h"

#include "GameFramework/PlayerController.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/Settings/WacomSettingsScreen.h"

bool FWacomSettingsScreenFlow::Open(APlayerController& PlayerController)
{
	UGameInstance* GameInstance = PlayerController.GetGameInstance();
	UWacomGameUIManagerSubsystem* UIManager = GameInstance
		? GameInstance->GetSubsystem<UWacomGameUIManagerSubsystem>()
		: nullptr;
	UWacomSettingsSubsystem* Settings = GameInstance
		? GameInstance->GetSubsystem<UWacomSettingsSubsystem>()
		: nullptr;
	if (!UIManager || !Settings)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[SettingsScreenFlow] Open rejected: missing UIManager or SettingsSubsystem"));
		return false;
	}

	const FGameplayTag GameMenuLayer = WacomUITags::UI_Layer_GameMenu.GetTag();
	if (UIManager->HasPendingAsyncPushToLayer(GameMenuLayer)
		|| Settings->HasActiveEdit()
		|| Settings->IsVideoModeConfirmationPending())
	{
		UE_LOG(LogTemp, Display,
			TEXT("[SettingsScreenFlow] Open ignored: settings screen is opening or already owns a transaction"));
		return false;
	}

	UIManager->EnsurePrimaryLayout(&PlayerController);
	if (!UIManager->GetPrimaryLayout())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[SettingsScreenFlow] Open rejected: PrimaryLayout is unavailable"));
		return false;
	}

	TWeakObjectPtr<APlayerController> WeakPlayer(&PlayerController);
	TWeakObjectPtr<UWacomSettingsSubsystem> WeakSettings(Settings);
	FWacomAsyncWidgetPushRequest Request;
	Request.LayerTag = GameMenuLayer;
	Request.WidgetTag = WacomUITags::UI_Widget_SettingsScreen.GetTag();
	Request.FallbackClass = UWacomSettingsScreen::StaticClass();
	Request.OwningPlayer = &PlayerController;
	Request.bLogMissingEntry = true;
	Request.CanPush = [WeakPlayer, WeakSettings]()
	{
		const UWacomSettingsSubsystem* CurrentSettings = WeakSettings.Get();
		return WeakPlayer.IsValid()
			&& CurrentSettings
			&& !CurrentSettings->HasActiveEdit()
			&& !CurrentSettings->IsVideoModeConfirmationPending();
	};
	Request.OnComplete = [](const FWacomAsyncWidgetPushResult& Result)
	{
		if (!Result.bSucceeded)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[SettingsScreenFlow] Push failed: %s"),
				*Result.FailureReason.ToString());
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("[SettingsScreenFlow] Settings screen opened: %s"),
			*GetNameSafe(Result.PushedWidget));
	};
	UIManager->PushRegisteredWidgetToLayerAsync(MoveTemp(Request));
	return true;
}
