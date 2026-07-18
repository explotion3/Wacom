// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FStreamableHandle;

enum class EWacomFirstPersonCardPresentationPrewarmState : uint8
{
	Inactive,
	Loading,
	Ready,
	TimedOut,
	Failed,
	Cancelled
};

struct FWacomFirstPersonCardPresentationPrewarmRequest
{
	FName ScopeId = NAME_None;
	TArray<FSoftObjectPath> RequiredVisualAssets;
	TArray<FSoftObjectPath> OptionalAudioAssets;
	float TimeoutSeconds = 1.5f;
#if WITH_AUTOMATION_TESTS
	bool bDeferRequiredCompletionForTest = false;
#endif
};

struct FWacomFirstPersonCardPresentationPrewarmDebugView
{
	EWacomFirstPersonCardPresentationPrewarmState State =
		EWacomFirstPersonCardPresentationPrewarmState::Inactive;
	uint32 Generation = 0;
	int32 RequiredAssetCount = 0;
	int32 OptionalAssetCount = 0;
	int32 MissingRequiredAssetCount = 0;
	int32 MissingOptionalAssetCount = 0;
	TArray<FSoftObjectPath> MissingRequiredAssets;
	TArray<FSoftObjectPath> MissingOptionalAssets;
	float ElapsedSeconds = 0.0f;
	bool bGateResolved = false;
	bool bGateResolvedByTimeout = false;
	bool bRequiredLoadCompleted = false;
	bool bOptionalLoadCompleted = false;
};

/**
 * App-private async residency controller shared by Battle and Run card sources.
 * The controller owns its streamable handles until Reset so loaded presentation
 * resources remain resident for the source lifetime.
 */
class WACOMAPP_API FWacomFirstPersonCardPresentationPrewarmController
{
public:
	FWacomFirstPersonCardPresentationPrewarmController();
	~FWacomFirstPersonCardPresentationPrewarmController();

	uint32 Begin(const FWacomFirstPersonCardPresentationPrewarmRequest& Request);
	void Tick(float DeltaTime);
	void Reset();

	bool IsGateResolved() const;
	bool ConsumeGateResolution(
		uint32& OutGeneration,
		EWacomFirstPersonCardPresentationPrewarmState& OutState);
	uint32 GetGeneration() const;
	FWacomFirstPersonCardPresentationPrewarmDebugView BuildDebugView() const;

private:
	struct FActiveLoadState;
	TSharedPtr<FActiveLoadState> ActiveState;
	uint32 NextGeneration = 0;
};
