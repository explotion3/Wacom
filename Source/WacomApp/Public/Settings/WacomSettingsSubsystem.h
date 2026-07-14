// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Settings/WacomLocalSettingsTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WacomSettingsSubsystem.generated.h"

class UWacomGameUserSettings;

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FWacomRuntimeSettingsChangedNative,
	const FWacomLocalSettingsSnapshot&,
	EWacomRuntimeSettingsChangeReason);

/**
 * Owns one local-settings edit transaction and the safe video-mode confirmation window.
 * It never reads or writes journey/player SaveGame data.
 */
UCLASS()
class WACOMAPP_API UWacomSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr float VideoModeConfirmationSeconds = 15.0f;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FWacomSettingsEditSession BeginEdit();
	FWacomSettingsOperationResult Preview(
		const FGuid& Token,
		const FWacomLocalSettingsSnapshot& Draft);
	FWacomSettingsOperationResult Apply(
		const FGuid& Token,
		const FWacomLocalSettingsSnapshot& Draft);
	FWacomSettingsOperationResult Cancel(const FGuid& Token);
	FWacomSettingsOperationResult ConfirmVideoMode(const FGuid& Token);
	FWacomSettingsOperationResult RevertVideoMode(const FGuid& Token);

	FWacomLocalSettingsSnapshot GetCurrentSnapshot() const;
	FWacomLocalSettingsSnapshot GetDefaultSnapshot() const;
	FWacomScreenResolutionOptions GetScreenResolutionOptions(
		EWindowMode::Type WindowMode,
		FIntPoint CurrentResolution) const;
	bool HasActiveEdit() const { return bHasActiveEdit; }
	bool IsVideoModeConfirmationPending() const { return PendingVideoModeToken.IsValid(); }
	float GetRemainingVideoModeConfirmationSeconds() const;

	FWacomRuntimeSettingsChangedNative& OnRuntimeSettingsChangedNative()
	{
		return RuntimeSettingsChangedNative;
	}

private:
#if WITH_AUTOMATION_TESTS
	friend struct FWacomSettingsSubsystemTestAccess;
#endif

	UWacomGameUserSettings* GetSettings() const;
	bool IsActiveToken(const FGuid& Token) const;
	bool IsPendingVideoToken(const FGuid& Token) const;
	void ApplyRuntimeSettings(EWacomRuntimeSettingsChangeReason Reason);
	void ApplyAudioSettingsToWorld(UWorld* World, const FWacomLocalSettingsSnapshot& Snapshot);
	void HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues IVS);
	bool TickVideoModeConfirmation(float DeltaTime);
	void ClearActiveEdit();
	void ClearPendingVideoConfirmation();

	bool bHasActiveEdit = false;
	FGuid ActiveEditToken;
	FWacomLocalSettingsSnapshot ActiveEditBaseline;

	FGuid PendingVideoModeToken;
	FWacomLocalSettingsSnapshot PendingVideoModeBaseline;
	double PendingVideoModeDeadlineSeconds = 0.0;
	FTSTicker::FDelegateHandle VideoConfirmationTickerHandle;
	FDelegateHandle PostWorldInitializationHandle;
	bool bLoggedMissingAudioAssets = false;

	FWacomRuntimeSettingsChangedNative RuntimeSettingsChangedNative;

#if WITH_AUTOMATION_TESTS
	UWacomGameUserSettings* SettingsOverrideForTest = nullptr;
	bool bSuppressEngineApplicationAndPersistenceForTest = false;
	int32 PersistenceRequestCountForTest = 0;
	bool bHasScreenResolutionEnvironmentOverrideForTest = false;
	FIntPoint DesktopResolutionOverrideForTest = FIntPoint::ZeroValue;
	FIntPoint WindowWorkAreaOverrideForTest = FIntPoint::ZeroValue;
	TArray<FIntPoint> FullscreenResolutionsOverrideForTest;
#endif
};
