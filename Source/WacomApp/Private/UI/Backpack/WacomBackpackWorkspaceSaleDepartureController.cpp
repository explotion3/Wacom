// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceSaleDepartureController.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UI/Backpack/WacomDeckCardWidget.h"

namespace
{
FWacomFirstPersonCardSurfaceEffectView BuildSaleSurfaceView(
	const FWacomFirstPersonCardSurfaceDepartureTickResult& PlaybackView,
	const FWacomFirstPersonCardPlayedDissolveStyleData& Style)
{
	FWacomFirstPersonCardSurfaceEffectView View;
	View.PlayedDissolve.bActive =
		PlaybackView.Kind == EWacomFirstPersonCardSurfaceDepartureKind::ExhaustDissolve;
	View.PlayedDissolve.bReducedMotion = PlaybackView.bReducedMotion;
	View.PlayedDissolve.Amount = PlaybackView.Amount;
	View.PlayedDissolve.TimeSeconds = PlaybackView.TimeSeconds;
	View.PlayedDissolve.Seed = PlaybackView.Seed;
	View.PlayedDissolve.Style = Style;
	return View;
}
}

bool FWacomBackpackWorkspaceSaleDepartureController::IsStyleValid(
	const FWacomFirstPersonCardPlayedDissolveStyleData& Style)
{
	return Style.SurfaceEffectMaterial != nullptr
		&& Style.NoiseTexture != nullptr
		&& Style.DurationSeconds > KINDA_SMALL_NUMBER;
}

float FWacomBackpackWorkspaceSaleDepartureController::AllocateSeed(FGuid InstanceId)
{
	uint32 Candidate = GetTypeHash(InstanceId);
	float Seed = static_cast<float>(
		static_cast<double>(Candidate) / MAX_uint32);
	while (UsedSeeds.Contains(Seed))
	{
		++Candidate;
		Seed = static_cast<float>(
			static_cast<double>(Candidate) / MAX_uint32);
	}
	UsedSeeds.Add(Seed);
	return Seed;
}

uint32 FWacomBackpackWorkspaceSaleDepartureController::MixRandomBits(uint32 Value)
{
	// A compact avalanche mix keeps scheduling deterministic for a committed
	// batch while making neighboring carry positions visually unrelated.
	Value ^= Value >> 16;
	Value *= 0x7feb352dU;
	Value ^= Value >> 15;
	Value *= 0x846ca68bU;
	Value ^= Value >> 16;
	return Value;
}

bool FWacomBackpackWorkspaceSaleDepartureController::Enqueue(
	UWacomDeckCardWidget& Card,
	FGuid InstanceId,
	const FWacomFirstPersonCardPlayedDissolveStyleData& Style,
	bool bSimplifiedMotion)
{
	if (!InstanceId.IsValid() || ContainsInstanceId(InstanceId) || !IsStyleValid(Style))
	{
		return false;
	}

	TUniquePtr<FEntry> Entry = MakeUnique<FEntry>();
	Entry->Card.Reset(&Card);
	Entry->InstanceId = InstanceId;
	Entry->Style = Style;
	Entry->Seed = AllocateSeed(InstanceId);
	Entry->bSimplifiedMotion = bSimplifiedMotion;
	PendingEntries.Add(MoveTemp(Entry));
	return true;
}

void FWacomBackpackWorkspaceSaleDepartureController::RandomizePendingTail(
	int32 FirstPendingIndex)
{
	const int32 SafeFirstIndex = FMath::Clamp(
		FirstPendingIndex,
		0,
		PendingEntries.Num());
	const int32 BatchSize = PendingEntries.Num() - SafeFirstIndex;
	if (BatchSize <= 0)
	{
		return;
	}

	++RandomBatchSequence;
	const uint32 BatchSalt = MixRandomBits(
		RandomBatchSequence * 0x9e3779b9U
		^ static_cast<uint32>(BatchSize));
	TArray<FGuid> OriginalOrder;
	OriginalOrder.Reserve(BatchSize);
	for (int32 Index = SafeFirstIndex; Index < PendingEntries.Num(); ++Index)
	{
		FEntry& Entry = *PendingEntries[Index];
		OriginalOrder.Add(Entry.InstanceId);
		const uint32 IdentityHash = GetTypeHash(Entry.InstanceId);
		Entry.RandomOrderKey = MixRandomBits(IdentityHash ^ BatchSalt);
		const uint32 StaggerBits = MixRandomBits(
			Entry.RandomOrderKey ^ 0xa511e9b3U);
		const float Unit = static_cast<float>(
			static_cast<double>(StaggerBits) / MAX_uint32);
		Entry.LaunchIntervalUnit = Unit;
	}

	for (int32 Index = SafeFirstIndex + 1;
		Index < PendingEntries.Num();
		++Index)
	{
		int32 Cursor = Index;
		while (Cursor > SafeFirstIndex
			&& PendingEntries[Cursor]->RandomOrderKey
				< PendingEntries[Cursor - 1]->RandomOrderKey)
		{
			PendingEntries.Swap(Cursor, Cursor - 1);
			--Cursor;
		}
	}

	if (BatchSize > 1)
	{
		bool bMatchesOriginalOrder = true;
		for (int32 Offset = 0; Offset < BatchSize; ++Offset)
		{
			if (PendingEntries[SafeFirstIndex + Offset]->InstanceId
				!= OriginalOrder[Offset])
			{
				bMatchesOriginalOrder = false;
				break;
			}
		}
		if (bMatchesOriginalOrder)
		{
			// A random permutation may legitimately equal its input, but the
			// authored effect promises a visibly shuffled departure order.
			TUniquePtr<FEntry> First =
				MoveTemp(PendingEntries[SafeFirstIndex]);
			PendingEntries.RemoveAt(
				SafeFirstIndex,
				1,
				EAllowShrinking::No);
			PendingEntries.Insert(
				MoveTemp(First),
				SafeFirstIndex + BatchSize - 1);
		}
	}
}

float FWacomBackpackWorkspaceSaleDepartureController::ResolveLaunchIntervalSeconds(
	const FEntry& Entry) const
{
	return Entry.bSimplifiedMotion
		? FMath::Lerp(
			SimplifiedMotionMinimumLaunchIntervalSeconds,
			SimplifiedMotionMaximumLaunchIntervalSeconds,
			Entry.LaunchIntervalUnit)
		: FMath::Lerp(
			FullMotionMinimumLaunchIntervalSeconds,
			FullMotionMaximumLaunchIntervalSeconds,
			Entry.LaunchIntervalUnit);
}

bool FWacomBackpackWorkspaceSaleDepartureController::HasEntryWaitingForReadiness()
	const
{
	return ActiveEntries.ContainsByPredicate(
		[](const TUniquePtr<FEntry>& Entry)
		{
			return Entry && !Entry->bPlaybackStarted;
		});
}

void FWacomBackpackWorkspaceSaleDepartureController::LaunchNextEntry()
{
	if (PendingEntries.IsEmpty()
		|| ActiveEntries.Num() >= MaximumConcurrentCards)
	{
		return;
	}

	const bool bWasIdle = ActiveEntries.IsEmpty();
	TUniquePtr<FEntry> Entry = MoveTemp(PendingEntries[0]);
	PendingEntries.RemoveAt(0, 1, EAllowShrinking::No);
	PrepareEntry(*Entry, bWasIdle);
	ActiveEntries.Add(MoveTemp(Entry));
	NextLaunchDelayRemainingSeconds = -1.0f;
	MaximumObservedRealtimeCardCount = FMath::Max(
		MaximumObservedRealtimeCardCount,
		ActiveEntries.Num());
}

void FWacomBackpackWorkspaceSaleDepartureController::PrepareEntry(
	FEntry& Entry,
	bool bSoundOwner)
{
	Entry.bSoundOwner = bSoundOwner;
	Entry.bPlaybackStarted = false;

	FWacomFirstPersonCardSurfaceDeparturePlaybackConfig Config;
	Config.Kind = EWacomFirstPersonCardSurfaceDepartureKind::ExhaustDissolve;
	Config.DurationSeconds = Entry.bSimplifiedMotion
		? SimplifiedMotionDurationSeconds
		: Entry.Style.DurationSeconds;
	Config.ConfirmHoldSeconds = Entry.bSimplifiedMotion
		? 0.0f
		: Entry.Style.ConfirmHoldSeconds;
	Config.Seed = Entry.Seed;
	Config.bReducedMotion = Entry.bSimplifiedMotion;
	if (Entry.bSoundOwner)
	{
		Config.StartSound = Entry.Style.StartSound;
		Config.SoundVolumeMultiplier = Entry.Style.StartSoundVolumeMultiplier;
		Config.SoundPitchMultiplier = Entry.Style.StartSoundPitchMultiplier;
		Config.SoundPitchVariation = Entry.Style.StartSoundPitchVariation;
	}
	Entry.Playback.Begin(Config);

	if (UWacomDeckCardWidget* Card = Entry.Card.Get())
	{
		Card->ApplyBackpackSaleSurfaceView(BuildSaleSurfaceView(
			Entry.Playback.BuildView(),
			Entry.Style));
		Entry.PreparationGeneration =
			Card->BeginBackpackSaleSurfacePreparation();
		Entry.Readiness.Begin(
			Entry.PreparationGeneration,
			Entry.PreparationGeneration != 0
				&& Card->IsBackpackSaleSurfaceMaterialReady(
					Entry.PreparationGeneration)
				&& Card->IsBackpackSaleSurfacePainted(
					Entry.PreparationGeneration));
		Card->SetBackpackSaleSurfaceRealtime(true);
		Card->RequestBackpackCardFaceRender();
	}
	else
	{
		Entry.Readiness.Begin(0);
	}
}

void FWacomBackpackWorkspaceSaleDepartureController::FinishEntry(
	FEntry& Entry,
	bool bFailed)
{
	if (UWacomDeckCardWidget* Card = Entry.Card.Get())
	{
		Card->CancelBackpackSaleSurfacePreparation();
		Card->SetBackpackSaleSurfaceRealtime(false);
		Card->ClearBackpackSaleSurfaceView();
		Card->RemoveFromParent();
	}
	if (bFailed)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Backpack sale dissolve readiness failed for card %s; removing the committed sale visual safely."),
			*Entry.InstanceId.ToString());
	}
	else
	{
		++CompletedCardCount;
	}
}

void FWacomBackpackWorkspaceSaleDepartureController::Tick(
	float DeltaSeconds,
	UObject* WorldContext)
{
	if (ActiveEntries.IsEmpty()
		&& !PendingEntries.IsEmpty()
		&& NextLaunchDelayRemainingSeconds < 0.0f)
	{
		LaunchNextEntry();
	}

	const float SafeDeltaSeconds = FMath::Clamp(DeltaSeconds, 0.0f, 0.1f);
	bool bLaunchDelayArmedThisFrame = false;
	for (int32 Index = ActiveEntries.Num() - 1; Index >= 0; --Index)
	{
		FEntry& Entry = *ActiveEntries[Index];
		UWacomDeckCardWidget* Card = Entry.Card.Get();
		if (!Card)
		{
			FinishEntry(Entry, true);
			ActiveEntries.RemoveAt(Index, 1, EAllowShrinking::No);
			continue;
		}

		Card->RefreshBackpackSaleSurfacePreparation(
			Entry.PreparationGeneration);
		const EWacomFirstPersonCardPresentationReadinessPollResult ReadinessResult =
			Entry.Readiness.Poll(
				SafeDeltaSeconds,
				Card->IsBackpackSaleSurfaceMaterialReady(
					Entry.PreparationGeneration),
				Card->IsBackpackSaleSurfacePainted(
					Entry.PreparationGeneration));
		if (ReadinessResult
			== EWacomFirstPersonCardPresentationReadinessPollResult::Failed)
		{
			FinishEntry(Entry, true);
			ActiveEntries.RemoveAt(Index, 1, EAllowShrinking::No);
			continue;
		}
		if (ReadinessResult
				!= EWacomFirstPersonCardPresentationReadinessPollResult::Ready
			&& ReadinessResult
				!= EWacomFirstPersonCardPresentationReadinessPollResult::BecameReady)
		{
			continue;
		}

		if (!Entry.bPlaybackStarted)
		{
			Entry.bPlaybackStarted = true;
			if (!PendingEntries.IsEmpty()
				&& NextLaunchDelayRemainingSeconds < 0.0f)
			{
				NextLaunchDelayRemainingSeconds =
					ResolveLaunchIntervalSeconds(*PendingEntries[0]);
				bLaunchDelayArmedThisFrame = true;
			}
		}

		const FWacomFirstPersonCardSurfaceDepartureTickResult PlaybackView =
			Entry.Playback.Tick(SafeDeltaSeconds);
		if (const TOptional<FWacomFirstPersonCardSurfaceDepartureSoundRequest> SoundRequest =
				Entry.Playback.ConsumePendingSoundRequest();
			SoundRequest.IsSet())
		{
			const FWacomFirstPersonCardSurfaceDepartureSoundRequest& Request =
				SoundRequest.GetValue();
			if (USoundBase* Sound = Request.Sound.Get(); Sound && WorldContext)
			{
				UGameplayStatics::PlaySound2D(
					WorldContext,
					Sound,
					Request.VolumeMultiplier,
					Request.PitchMultiplier);
			}
		}
		Card->ApplyBackpackSaleSurfaceView(BuildSaleSurfaceView(
			PlaybackView,
			Entry.Style));
		if (PlaybackView.bCompleted)
		{
			FinishEntry(Entry, false);
			ActiveEntries.RemoveAt(Index, 1, EAllowShrinking::No);
		}
	}

	if (!PendingEntries.IsEmpty()
		&& NextLaunchDelayRemainingSeconds < 0.0f
		&& !HasEntryWaitingForReadiness())
	{
		if (ActiveEntries.IsEmpty())
		{
			LaunchNextEntry();
		}
		else
		{
			NextLaunchDelayRemainingSeconds =
				ResolveLaunchIntervalSeconds(*PendingEntries[0]);
			bLaunchDelayArmedThisFrame = true;
		}
	}

	if (!PendingEntries.IsEmpty()
		&& NextLaunchDelayRemainingSeconds >= 0.0f
		&& !bLaunchDelayArmedThisFrame)
	{
		NextLaunchDelayRemainingSeconds = FMath::Max(
			0.0f,
			NextLaunchDelayRemainingSeconds - SafeDeltaSeconds);
		if (NextLaunchDelayRemainingSeconds <= 0.0f
			&& ActiveEntries.Num() < MaximumConcurrentCards)
		{
			// One scheduler decision launches exactly one card. Any remaining
			// elapsed time is intentionally discarded so a slow frame cannot
			// collapse several authored stagger events into a visible group.
			LaunchNextEntry();
		}
	}

	if (PendingEntries.IsEmpty())
	{
		NextLaunchDelayRemainingSeconds = -1.0f;
	}
}

void FWacomBackpackWorkspaceSaleDepartureController::SetRetainedRenderingEnabled(
	bool bEnabled)
{
	const auto Apply = [bEnabled](const TArray<TUniquePtr<FEntry>>& Entries)
	{
		for (const TUniquePtr<FEntry>& Entry : Entries)
		{
			if (Entry)
			{
				if (UWacomDeckCardWidget* Card = Entry->Card.Get())
				{
					Card->SetBackpackCardFaceRetainedRenderingEnabled(bEnabled);
				}
			}
		}
	};
	Apply(PendingEntries);
	Apply(ActiveEntries);
}

void FWacomBackpackWorkspaceSaleDepartureController::Reset(bool bRemoveWidgets)
{
	const auto ResetEntries = [bRemoveWidgets](TArray<TUniquePtr<FEntry>>& Entries)
	{
		for (TUniquePtr<FEntry>& Entry : Entries)
		{
			if (!Entry)
			{
				continue;
			}
			if (UWacomDeckCardWidget* Card = Entry->Card.Get())
			{
				Card->CancelBackpackSaleSurfacePreparation();
				Card->SetBackpackSaleSurfaceRealtime(false);
				Card->ClearBackpackSaleSurfaceView();
				if (bRemoveWidgets)
				{
					Card->RemoveFromParent();
				}
			}
		}
		Entries.Reset();
	};
	ResetEntries(ActiveEntries);
	ResetEntries(PendingEntries);
	UsedSeeds.Reset();
	MaximumObservedRealtimeCardCount = 0;
	CompletedCardCount = 0;
	RandomBatchSequence = 0;
	NextLaunchDelayRemainingSeconds = -1.0f;
}

bool FWacomBackpackWorkspaceSaleDepartureController::ContainsCard(
	const UWacomDeckCardWidget* Card) const
{
	const auto Contains = [Card](const TArray<TUniquePtr<FEntry>>& Entries)
	{
		return Entries.ContainsByPredicate(
			[Card](const TUniquePtr<FEntry>& Entry)
			{
				return Entry && Entry->Card.Get() == Card;
			});
	};
	return Contains(PendingEntries) || Contains(ActiveEntries);
}

bool FWacomBackpackWorkspaceSaleDepartureController::ContainsInstanceId(
	FGuid InstanceId) const
{
	const auto Contains = [InstanceId](const TArray<TUniquePtr<FEntry>>& Entries)
	{
		return Entries.ContainsByPredicate(
			[InstanceId](const TUniquePtr<FEntry>& Entry)
			{
				return Entry && Entry->InstanceId == InstanceId;
			});
	};
	return Contains(PendingEntries) || Contains(ActiveEntries);
}

#if WITH_AUTOMATION_TESTS
TArray<FGuid>
FWacomBackpackWorkspaceSaleDepartureController::GetPendingInstanceIdsForTest() const
{
	TArray<FGuid> Result;
	for (const TUniquePtr<FEntry>& Entry : PendingEntries)
	{
		if (Entry)
		{
			Result.Add(Entry->InstanceId);
		}
	}
	return Result;
}

TArray<FGuid>
FWacomBackpackWorkspaceSaleDepartureController::GetActiveInstanceIdsForTest() const
{
	TArray<FGuid> Result;
	for (const TUniquePtr<FEntry>& Entry : ActiveEntries)
	{
		if (Entry)
		{
			Result.Add(Entry->InstanceId);
		}
	}
	return Result;
}

TMap<FGuid, float>
FWacomBackpackWorkspaceSaleDepartureController::GetSeedsForTest() const
{
	TMap<FGuid, float> Result;
	const auto Append = [&Result](const TArray<TUniquePtr<FEntry>>& Entries)
	{
		for (const TUniquePtr<FEntry>& Entry : Entries)
		{
			if (Entry)
			{
				Result.Add(Entry->InstanceId, Entry->Seed);
			}
		}
	};
	Append(PendingEntries);
	Append(ActiveEntries);
	return Result;
}

TArray<UWacomDeckCardWidget*>
FWacomBackpackWorkspaceSaleDepartureController::GetActiveCardsForTest() const
{
	TArray<UWacomDeckCardWidget*> Result;
	for (const TUniquePtr<FEntry>& Entry : ActiveEntries)
	{
		if (Entry)
		{
			if (UWacomDeckCardWidget* Card = Entry->Card.Get())
			{
				Result.Add(Card);
			}
		}
	}
	return Result;
}

void FWacomBackpackWorkspaceSaleDepartureController::ForceActiveReadinessForTest()
{
	for (TUniquePtr<FEntry>& Entry : ActiveEntries)
	{
		if (Entry)
		{
			Entry->Readiness.Begin(
				Entry->PreparationGeneration != 0
					? Entry->PreparationGeneration
					: 1,
				true);
		}
	}
}
#endif
