// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardPresentationPrewarmController.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogWacomCardPresentationPrewarm, Log, All);

namespace
{
	void NormalizePaths(TArray<FSoftObjectPath>& Paths)
	{
		Paths.RemoveAll([](const FSoftObjectPath& Path)
		{
			return Path.IsNull();
		});
		Paths.Sort([](const FSoftObjectPath& A, const FSoftObjectPath& B)
		{
			return A.ToString() < B.ToString();
		});
		for (int32 Index = Paths.Num() - 1; Index > 0; --Index)
		{
			if (Paths[Index] == Paths[Index - 1])
			{
				Paths.RemoveAt(Index, 1, EAllowShrinking::No);
			}
		}
	}

	void FindMissingAssets(
		const TArray<FSoftObjectPath>& Paths,
		TArray<FSoftObjectPath>& OutMissingAssets)
	{
		OutMissingAssets.Reset();
		for (const FSoftObjectPath& Path : Paths)
		{
			if (!Path.ResolveObject())
			{
				OutMissingAssets.Add(Path);
			}
		}
	}

	FString JoinPaths(const TArray<FSoftObjectPath>& Paths)
	{
		TArray<FString> Strings;
		Strings.Reserve(Paths.Num());
		for (const FSoftObjectPath& Path : Paths)
		{
			Strings.Add(Path.ToString());
		}
		return FString::Join(Strings, TEXT(", "));
	}
}

struct FWacomFirstPersonCardPresentationPrewarmController::FActiveLoadState
{
	FName ScopeId = NAME_None;
	uint32 Generation = 0;
	EWacomFirstPersonCardPresentationPrewarmState State =
		EWacomFirstPersonCardPresentationPrewarmState::Inactive;
	TArray<FSoftObjectPath> RequiredVisualAssets;
	TArray<FSoftObjectPath> OptionalAudioAssets;
	TSharedPtr<FStreamableHandle> RequiredHandle;
	TSharedPtr<FStreamableHandle> OptionalHandle;
	float TimeoutSeconds = 1.5f;
	float ElapsedSeconds = 0.0f;
	int32 MissingRequiredAssetCount = 0;
	int32 MissingOptionalAssetCount = 0;
	TArray<FSoftObjectPath> MissingRequiredAssets;
	TArray<FSoftObjectPath> MissingOptionalAssets;
	bool bRequiredLoadCompleted = false;
	bool bOptionalLoadCompleted = false;
	bool bGateResolved = false;
	bool bGateResolutionPending = false;
	bool bGateResolvedByTimeout = false;
	bool bTimeoutWarningLogged = false;
	bool bRequiredFailureWarningLogged = false;
	bool bOptionalFailureWarningLogged = false;
};

FWacomFirstPersonCardPresentationPrewarmController::
FWacomFirstPersonCardPresentationPrewarmController() = default;

FWacomFirstPersonCardPresentationPrewarmController::
~FWacomFirstPersonCardPresentationPrewarmController()
{
	Reset();
}

uint32 FWacomFirstPersonCardPresentationPrewarmController::Begin(
	const FWacomFirstPersonCardPresentationPrewarmRequest& Request)
{
	Reset();

	TSharedRef<FActiveLoadState> State = MakeShared<FActiveLoadState>();
	State->ScopeId = Request.ScopeId;
	State->Generation = ++NextGeneration;
	State->RequiredVisualAssets = Request.RequiredVisualAssets;
	State->OptionalAudioAssets = Request.OptionalAudioAssets;
	NormalizePaths(State->RequiredVisualAssets);
	NormalizePaths(State->OptionalAudioAssets);
	State->TimeoutSeconds = FMath::Max(0.01f, Request.TimeoutSeconds);
	State->State = EWacomFirstPersonCardPresentationPrewarmState::Loading;
	ActiveState = State;

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	const TWeakPtr<FActiveLoadState> WeakState(State);
#if WITH_AUTOMATION_TESTS
	if (Request.bDeferRequiredCompletionForTest)
	{
		// Intentionally left loading so the timeout/generation contract can be tested
		// without relying on disk or StreamableManager timing.
	}
	else
#endif
	if (State->RequiredVisualAssets.IsEmpty())
	{
		State->bRequiredLoadCompleted = true;
	}
	else
	{
		State->RequiredHandle = StreamableManager.RequestAsyncLoad(
			State->RequiredVisualAssets,
			FStreamableDelegate::CreateLambda([WeakState]()
			{
				if (const TSharedPtr<FActiveLoadState> Pinned = WeakState.Pin())
				{
					Pinned->bRequiredLoadCompleted = true;
				}
			}),
			FStreamableManager::AsyncLoadHighPriority,
			false,
			false,
			FString::Printf(TEXT("WacomCardPrewarm.Required.%s"), *State->ScopeId.ToString()));
		if (!State->RequiredHandle)
		{
			State->bRequiredLoadCompleted = true;
		}
	}

	if (State->OptionalAudioAssets.IsEmpty())
	{
		State->bOptionalLoadCompleted = true;
	}
	else
	{
		State->OptionalHandle = StreamableManager.RequestAsyncLoad(
			State->OptionalAudioAssets,
			FStreamableDelegate::CreateLambda([WeakState]()
			{
				if (const TSharedPtr<FActiveLoadState> Pinned = WeakState.Pin())
				{
					Pinned->bOptionalLoadCompleted = true;
				}
			}),
			FStreamableManager::DefaultAsyncLoadPriority,
			false,
			false,
			FString::Printf(TEXT("WacomCardPrewarm.Optional.%s"), *State->ScopeId.ToString()));
		if (!State->OptionalHandle)
		{
			State->bOptionalLoadCompleted = true;
		}
	}

	Tick(0.0f);
	return State->Generation;
}

void FWacomFirstPersonCardPresentationPrewarmController::Tick(float DeltaTime)
{
	if (!ActiveState)
	{
		return;
	}

	FActiveLoadState& State = *ActiveState;
	if (State.State == EWacomFirstPersonCardPresentationPrewarmState::Loading)
	{
		State.ElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	}

	if (State.bOptionalLoadCompleted && State.MissingOptionalAssetCount == 0)
	{
		FindMissingAssets(State.OptionalAudioAssets, State.MissingOptionalAssets);
		State.MissingOptionalAssetCount = State.MissingOptionalAssets.Num();
		if (State.MissingOptionalAssetCount > 0 && !State.bOptionalFailureWarningLogged)
		{
			State.bOptionalFailureWarningLogged = true;
			UE_LOG(
				LogWacomCardPresentationPrewarm,
				Warning,
				TEXT("Optional card presentation audio was not loaded; entry remains unblocked (Scope=%s Generation=%u MissingOptional=%d Paths=[%s])."),
				*State.ScopeId.ToString(),
				State.Generation,
				State.MissingOptionalAssetCount,
				*JoinPaths(State.MissingOptionalAssets));
		}
	}

	if (State.bRequiredLoadCompleted
		&& (State.State == EWacomFirstPersonCardPresentationPrewarmState::Loading
			|| State.State == EWacomFirstPersonCardPresentationPrewarmState::TimedOut))
	{
		FindMissingAssets(State.RequiredVisualAssets, State.MissingRequiredAssets);
		State.MissingRequiredAssetCount = State.MissingRequiredAssets.Num();
		const bool bRequiredAssetsReady = State.MissingRequiredAssetCount == 0;
		State.State = bRequiredAssetsReady
			? EWacomFirstPersonCardPresentationPrewarmState::Ready
			: EWacomFirstPersonCardPresentationPrewarmState::Failed;
		if (!State.bGateResolved)
		{
			State.bGateResolved = true;
			State.bGateResolutionPending = true;
		}
		if (!bRequiredAssetsReady && !State.bRequiredFailureWarningLogged)
		{
			State.bRequiredFailureWarningLogged = true;
			UE_LOG(
				LogWacomCardPresentationPrewarm,
				Warning,
				TEXT("Card presentation prewarm failed (Scope=%s Generation=%u MissingRequired=%d Paths=[%s])."),
				*State.ScopeId.ToString(),
				State.Generation,
				State.MissingRequiredAssetCount,
				*JoinPaths(State.MissingRequiredAssets));
		}
	}

	if (State.State == EWacomFirstPersonCardPresentationPrewarmState::Loading
		&& State.ElapsedSeconds >= State.TimeoutSeconds)
	{
		State.State = EWacomFirstPersonCardPresentationPrewarmState::TimedOut;
		State.bGateResolved = true;
		State.bGateResolutionPending = true;
		State.bGateResolvedByTimeout = true;
		if (!State.bTimeoutWarningLogged)
		{
			State.bTimeoutWarningLogged = true;
			UE_LOG(
				LogWacomCardPresentationPrewarm,
				Warning,
				TEXT("Card presentation prewarm timed out (Scope=%s Generation=%u Required=%d Timeout=%.2fs)."),
				*State.ScopeId.ToString(),
				State.Generation,
				State.RequiredVisualAssets.Num(),
				State.TimeoutSeconds);
		}
	}
}

void FWacomFirstPersonCardPresentationPrewarmController::Reset()
{
	if (!ActiveState)
	{
		return;
	}
	ActiveState->State = EWacomFirstPersonCardPresentationPrewarmState::Cancelled;
	if (ActiveState->RequiredHandle && !ActiveState->RequiredHandle->HasLoadCompleted())
	{
		ActiveState->RequiredHandle->CancelHandle();
	}
	if (ActiveState->OptionalHandle && !ActiveState->OptionalHandle->HasLoadCompleted())
	{
		ActiveState->OptionalHandle->CancelHandle();
	}
	ActiveState->RequiredHandle.Reset();
	ActiveState->OptionalHandle.Reset();
	ActiveState.Reset();
}

bool FWacomFirstPersonCardPresentationPrewarmController::IsGateResolved() const
{
	return ActiveState && ActiveState->bGateResolved;
}

bool FWacomFirstPersonCardPresentationPrewarmController::ConsumeGateResolution(
	uint32& OutGeneration,
	EWacomFirstPersonCardPresentationPrewarmState& OutState)
{
	if (!ActiveState || !ActiveState->bGateResolutionPending)
	{
		return false;
	}
	ActiveState->bGateResolutionPending = false;
	OutGeneration = ActiveState->Generation;
	OutState = ActiveState->State;
	return true;
}

uint32 FWacomFirstPersonCardPresentationPrewarmController::GetGeneration() const
{
	return ActiveState ? ActiveState->Generation : 0;
}

FWacomFirstPersonCardPresentationPrewarmDebugView
FWacomFirstPersonCardPresentationPrewarmController::BuildDebugView() const
{
	FWacomFirstPersonCardPresentationPrewarmDebugView View;
	if (!ActiveState)
	{
		return View;
	}
	View.State = ActiveState->State;
	View.Generation = ActiveState->Generation;
	View.RequiredAssetCount = ActiveState->RequiredVisualAssets.Num();
	View.OptionalAssetCount = ActiveState->OptionalAudioAssets.Num();
	View.MissingRequiredAssetCount = ActiveState->MissingRequiredAssetCount;
	View.MissingOptionalAssetCount = ActiveState->MissingOptionalAssetCount;
	View.MissingRequiredAssets = ActiveState->MissingRequiredAssets;
	View.MissingOptionalAssets = ActiveState->MissingOptionalAssets;
	View.ElapsedSeconds = ActiveState->ElapsedSeconds;
	View.bGateResolved = ActiveState->bGateResolved;
	View.bGateResolvedByTimeout = ActiveState->bGateResolvedByTimeout;
	View.bRequiredLoadCompleted = ActiveState->bRequiredLoadCompleted;
	View.bOptionalLoadCompleted = ActiveState->bOptionalLoadCompleted;
	return View;
}
