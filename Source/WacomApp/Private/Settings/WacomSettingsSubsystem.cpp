// Copyright Wacom. All Rights Reserved.

#include "Settings/WacomSettingsSubsystem.h"

#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Settings/WacomGameUserSettings.h"
#include "Settings/WacomLocalSettingsDefaults.h"
#include "Settings/WacomSettingsDeveloperSettings.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

#define LOCTEXT_NAMESPACE "WacomSettingsSubsystem"

DEFINE_LOG_CATEGORY_STATIC(LogWacomLocalSettings, Log, All);

namespace
{
	bool IsGameAudioWorld(const UWorld* World)
	{
		return World
			&& (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
	}

	bool HasVideoModeChanged(
		const FWacomLocalSettingsSnapshot& Before,
		const FWacomLocalSettingsSnapshot& After)
	{
		return Before.ScreenResolution != After.ScreenResolution
			|| Before.WindowMode != After.WindowMode;
	}
}

void UWacomSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PostWorldInitializationHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(
		this,
		&UWacomSettingsSubsystem::HandlePostWorldInitialization);

	if (UWacomGameUserSettings* Settings = GetSettings())
	{
		Settings->LoadSettings(false);
		Settings->ApplyNonResolutionSettings();
		ApplyRuntimeSettings(EWacomRuntimeSettingsChangeReason::Startup);
	}
	else
	{
		UE_LOG(LogWacomLocalSettings, Error,
			TEXT("[LocalSettings] GameUserSettingsClassName must be UWacomGameUserSettings."));
	}
}

void UWacomSettingsSubsystem::Deinitialize()
{
	ClearPendingVideoConfirmation();
	ClearActiveEdit();
	if (PostWorldInitializationHandle.IsValid())
	{
		FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitializationHandle);
		PostWorldInitializationHandle.Reset();
	}
	RuntimeSettingsChangedNative.Clear();
	Super::Deinitialize();
}

FWacomSettingsEditSession UWacomSettingsSubsystem::BeginEdit()
{
	FWacomSettingsEditSession Session;
	if (bHasActiveEdit || IsVideoModeConfirmationPending())
	{
		UE_LOG(LogWacomLocalSettings, Warning, TEXT("Reject overlapping edit transaction."));
		return Session;
	}

	UWacomGameUserSettings* Settings = GetSettings();
	if (!Settings)
	{
		return Session;
	}

	bHasActiveEdit = true;
	ActiveEditToken = FGuid::NewGuid();
	ActiveEditBaseline = Settings->MakeSnapshot();
	Session.Token = ActiveEditToken;
	Session.Snapshot = ActiveEditBaseline;
	return Session;
}

FWacomSettingsOperationResult UWacomSettingsSubsystem::Preview(
	const FGuid& Token,
	const FWacomLocalSettingsSnapshot& Draft)
{
	if (!IsActiveToken(Token))
	{
		return FWacomSettingsOperationResult::Failure(
			LOCTEXT("InvalidPreviewToken", "设置编辑已失效，无法预览。"));
	}

	UWacomGameUserSettings* Settings = GetSettings();
	if (!Settings)
	{
		return FWacomSettingsOperationResult::Failure(
			LOCTEXT("MissingSettingsPreview", "本地设置服务不可用。"));
	}

	Settings->SetPreviewableFieldsFromSnapshot(Draft);
	ApplyRuntimeSettings(EWacomRuntimeSettingsChangeReason::Preview);
	return FWacomSettingsOperationResult::Success();
}

FWacomSettingsOperationResult UWacomSettingsSubsystem::Apply(
	const FGuid& Token,
	const FWacomLocalSettingsSnapshot& Draft)
{
	if (!IsActiveToken(Token))
	{
		return FWacomSettingsOperationResult::Failure(
			LOCTEXT("InvalidApplyToken", "设置编辑已失效，无法应用。"));
	}

	UWacomGameUserSettings* Settings = GetSettings();
	if (!Settings)
	{
		return FWacomSettingsOperationResult::Failure(
			LOCTEXT("MissingSettingsApply", "本地设置服务不可用。"));
	}

	FWacomLocalSettingsSnapshot SanitizedDraft = Draft;
	SanitizedDraft.Sanitize();
	const bool bNeedsVideoConfirmation = HasVideoModeChanged(ActiveEditBaseline, SanitizedDraft);
	Settings->SetFromSnapshot(SanitizedDraft);

	if (!bNeedsVideoConfirmation)
	{
#if WITH_AUTOMATION_TESTS
		++PersistenceRequestCountForTest;
		if (!bSuppressEngineApplicationAndPersistenceForTest)
#endif
		{
			Settings->ApplySettings(false);
		}
		ClearActiveEdit();
		ApplyRuntimeSettings(EWacomRuntimeSettingsChangeReason::Applied);
		return FWacomSettingsOperationResult::Success();
	}

	// Applying these separately is intentional: UGameUserSettings::ApplySettings would save
	// the unconfirmed video mode before the user has proved it is usable.
	PendingVideoModeBaseline = ActiveEditBaseline;
#if WITH_AUTOMATION_TESTS
	if (!bSuppressEngineApplicationAndPersistenceForTest)
#endif
	{
		Settings->ApplyResolutionSettings(false);
		Settings->ApplyNonResolutionSettings();
	}
	PendingVideoModeToken = Token;
	PendingVideoModeDeadlineSeconds = FPlatformTime::Seconds() + VideoModeConfirmationSeconds;
	ClearActiveEdit();
	VideoConfirmationTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UWacomSettingsSubsystem::TickVideoModeConfirmation),
		0.1f);
	ApplyRuntimeSettings(EWacomRuntimeSettingsChangeReason::Applied);
	return FWacomSettingsOperationResult::Success(true);
}

FWacomSettingsOperationResult UWacomSettingsSubsystem::Cancel(const FGuid& Token)
{
	if (!IsActiveToken(Token))
	{
		return FWacomSettingsOperationResult::Failure(
			LOCTEXT("InvalidCancelToken", "设置编辑已失效，无法取消。"));
	}

	UWacomGameUserSettings* Settings = GetSettings();
	if (!Settings)
	{
		return FWacomSettingsOperationResult::Failure(
			LOCTEXT("MissingSettingsCancel", "本地设置服务不可用。"));
	}
	Settings->SetPreviewableFieldsFromSnapshot(ActiveEditBaseline);
	ClearActiveEdit();
	ApplyRuntimeSettings(EWacomRuntimeSettingsChangeReason::PreviewCancelled);
	return FWacomSettingsOperationResult::Success();
}

FWacomSettingsOperationResult UWacomSettingsSubsystem::ConfirmVideoMode(const FGuid& Token)
{
	if (!IsPendingVideoToken(Token))
	{
		return FWacomSettingsOperationResult::Failure(
			LOCTEXT("InvalidConfirmToken", "视频模式确认已失效。"));
	}

	UWacomGameUserSettings* Settings = GetSettings();
	if (!Settings)
	{
		return FWacomSettingsOperationResult::Failure(
			LOCTEXT("MissingSettingsConfirm", "本地设置服务不可用。"));
	}
#if WITH_AUTOMATION_TESTS
	++PersistenceRequestCountForTest;
	if (!bSuppressEngineApplicationAndPersistenceForTest)
#endif
	{
		Settings->ConfirmVideoMode();
		Settings->SaveSettings();
	}
	ClearPendingVideoConfirmation();
	ApplyRuntimeSettings(EWacomRuntimeSettingsChangeReason::VideoConfirmed);
	return FWacomSettingsOperationResult::Success();
}

FWacomSettingsOperationResult UWacomSettingsSubsystem::RevertVideoMode(const FGuid& Token)
{
	if (!IsPendingVideoToken(Token))
	{
		return FWacomSettingsOperationResult::Failure(
			LOCTEXT("InvalidRevertToken", "视频模式确认已失效。"));
	}

	UWacomGameUserSettings* Settings = GetSettings();
	if (!Settings)
	{
		return FWacomSettingsOperationResult::Failure(
			LOCTEXT("MissingSettingsRevert", "本地设置服务不可用。"));
	}
#if WITH_AUTOMATION_TESTS
	++PersistenceRequestCountForTest;
	if (bSuppressEngineApplicationAndPersistenceForTest)
	{
		FWacomLocalSettingsSnapshot Reverted = Settings->MakeSnapshot();
		Reverted.ScreenResolution = PendingVideoModeBaseline.ScreenResolution;
		Reverted.WindowMode = PendingVideoModeBaseline.WindowMode;
		Settings->SetFromSnapshot(Reverted);
	}
	else
#endif
	{
		Settings->RevertVideoMode();
		Settings->ApplyResolutionSettings(false);
		Settings->SaveSettings();
	}
	ClearPendingVideoConfirmation();
	ApplyRuntimeSettings(EWacomRuntimeSettingsChangeReason::VideoReverted);
	return FWacomSettingsOperationResult::Success();
}

FWacomLocalSettingsSnapshot UWacomSettingsSubsystem::GetCurrentSnapshot() const
{
	if (const UWacomGameUserSettings* Settings = GetSettings())
	{
		return Settings->MakeSnapshot();
	}
	return FWacomLocalSettingsSnapshot();
}

FWacomLocalSettingsSnapshot UWacomSettingsSubsystem::GetDefaultSnapshot() const
{
	return FWacomLocalSettingsDefaults::Build(GetSettings());
}

TArray<FIntPoint> UWacomSettingsSubsystem::GetSupportedScreenResolutions() const
{
	TArray<FIntPoint> Resolutions;
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
	Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return A.X == B.X ? A.Y < B.Y : A.X < B.X;
	});
	return Resolutions;
}

float UWacomSettingsSubsystem::GetRemainingVideoModeConfirmationSeconds() const
{
	if (!IsVideoModeConfirmationPending())
	{
		return 0.0f;
	}
	return static_cast<float>(FMath::Max(0.0, PendingVideoModeDeadlineSeconds - FPlatformTime::Seconds()));
}

UWacomGameUserSettings* UWacomSettingsSubsystem::GetSettings() const
{
#if WITH_AUTOMATION_TESTS
	if (SettingsOverrideForTest)
	{
		return SettingsOverrideForTest;
	}
#endif
	return GEngine ? Cast<UWacomGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

bool UWacomSettingsSubsystem::IsActiveToken(const FGuid& Token) const
{
	return bHasActiveEdit && Token.IsValid() && Token == ActiveEditToken;
}

bool UWacomSettingsSubsystem::IsPendingVideoToken(const FGuid& Token) const
{
	return Token.IsValid() && Token == PendingVideoModeToken;
}

void UWacomSettingsSubsystem::ApplyRuntimeSettings(EWacomRuntimeSettingsChangeReason Reason)
{
	const FWacomLocalSettingsSnapshot Snapshot = GetCurrentSnapshot();
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			ApplyAudioSettingsToWorld(Context.World(), Snapshot);
		}
	}
	RuntimeSettingsChangedNative.Broadcast(Snapshot, Reason);
}

void UWacomSettingsSubsystem::ApplyAudioSettingsToWorld(
	UWorld* World,
	const FWacomLocalSettingsSnapshot& Snapshot)
{
	if (!IsGameAudioWorld(World) || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	const UWacomSettingsDeveloperSettings* AssetSettings = GetDefault<UWacomSettingsDeveloperSettings>();
	USoundMix* SoundMix = AssetSettings->UserSettingsSoundMix.LoadSynchronous();
	USoundClass* Master = AssetSettings->MasterSoundClass.LoadSynchronous();
	USoundClass* Music = AssetSettings->MusicSoundClass.LoadSynchronous();
	USoundClass* SFX = AssetSettings->SFXSoundClass.LoadSynchronous();
	USoundClass* UI = AssetSettings->UISoundClass.LoadSynchronous();
	if (!SoundMix || !Master || !Music || !SFX || !UI)
	{
		if (!bLoggedMissingAudioAssets)
		{
			bLoggedMissingAudioAssets = true;
			UE_LOG(LogWacomLocalSettings, Warning,
				TEXT("[LocalSettings] Audio routing assets are incomplete; volume preview is deferred."));
		}
		return;
	}

	UGameplayStatics::SetBaseSoundMix(World, SoundMix);
	UGameplayStatics::SetSoundMixClassOverride(World, SoundMix, Master, Snapshot.MasterVolume, 1.0f, 0.0f, true);
	UGameplayStatics::SetSoundMixClassOverride(World, SoundMix, Music, Snapshot.MusicVolume, 1.0f, 0.0f, true);
	UGameplayStatics::SetSoundMixClassOverride(World, SoundMix, SFX, Snapshot.SFXVolume, 1.0f, 0.0f, true);
	UGameplayStatics::SetSoundMixClassOverride(World, SoundMix, UI, Snapshot.UISoundVolume, 1.0f, 0.0f, true);
}

void UWacomSettingsSubsystem::HandlePostWorldInitialization(
	UWorld* World,
	const UWorld::InitializationValues /*IVS*/)
{
	ApplyAudioSettingsToWorld(World, GetCurrentSnapshot());
}

bool UWacomSettingsSubsystem::TickVideoModeConfirmation(float /*DeltaTime*/)
{
	if (!IsVideoModeConfirmationPending())
	{
		VideoConfirmationTickerHandle.Reset();
		return false;
	}
	if (FPlatformTime::Seconds() < PendingVideoModeDeadlineSeconds)
	{
		return true;
	}

	const FGuid ExpiredToken = PendingVideoModeToken;
	RevertVideoMode(ExpiredToken);
	return false;
}

void UWacomSettingsSubsystem::ClearActiveEdit()
{
	bHasActiveEdit = false;
	ActiveEditToken.Invalidate();
	ActiveEditBaseline = FWacomLocalSettingsSnapshot();
}

void UWacomSettingsSubsystem::ClearPendingVideoConfirmation()
{
	if (VideoConfirmationTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(VideoConfirmationTickerHandle);
		VideoConfirmationTickerHandle.Reset();
	}
	PendingVideoModeToken.Invalidate();
	PendingVideoModeBaseline = FWacomLocalSettingsSnapshot();
	PendingVideoModeDeadlineSeconds = 0.0;
}

#undef LOCTEXT_NAMESPACE
