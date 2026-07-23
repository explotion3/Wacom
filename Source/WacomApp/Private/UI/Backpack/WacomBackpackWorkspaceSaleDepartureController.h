// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../Card/WacomFirstPersonCardPresentationReadinessGate.h"
#include "../Card/WacomFirstPersonCardSurfaceDeparturePlayback.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "UObject/StrongObjectPtr.h"

class UObject;
class UWacomDeckCardWidget;

/**
 * App-private owner for sold-card surface departures.
 *
 * Rules have already committed before an entry arrives here. This controller
 * only retains the original card Widget, gates its Retainer material, and
 * removes the visual when the FIFO departure completes.
 */
class FWacomBackpackWorkspaceSaleDepartureController
{
public:
	static constexpr int32 MaximumConcurrentCards = 4;
	static constexpr float FullMotionStaggerSeconds = 0.035f;
	static constexpr float SimplifiedMotionStaggerSeconds = 0.02f;
	static constexpr float SimplifiedMotionDurationSeconds = 0.12f;
	static constexpr float MinimumStaggerScale = 0.65f;
	static constexpr float MaximumStaggerScale = 1.35f;

	bool Enqueue(
		UWacomDeckCardWidget& Card,
		FGuid InstanceId,
		const FWacomFirstPersonCardPlayedDissolveStyleData& Style,
		bool bSimplifiedMotion);
	/** Randomizes only the newly appended tail, preserving older sale batches. */
	void RandomizePendingTail(int32 FirstPendingIndex);
	void Tick(float DeltaSeconds, UObject* WorldContext);
	void SetRetainedRenderingEnabled(bool bEnabled);
	void Reset(bool bRemoveWidgets);

	bool HasWork() const
	{
		return !PendingEntries.IsEmpty() || !ActiveEntries.IsEmpty();
	}
	bool HasActiveGroup() const { return !ActiveEntries.IsEmpty(); }
	bool ContainsCard(const UWacomDeckCardWidget* Card) const;
	bool ContainsInstanceId(FGuid InstanceId) const;
	int32 GetQueuedCardCount() const { return PendingEntries.Num(); }
	int32 GetActiveCardCount() const { return ActiveEntries.Num(); }
	int32 GetRealtimeCardCount() const { return ActiveEntries.Num(); }
	int32 GetMaximumObservedRealtimeCardCount() const
	{
		return MaximumObservedRealtimeCardCount;
	}
	int32 GetCompletedCardCount() const { return CompletedCardCount; }

#if WITH_AUTOMATION_TESTS
	TArray<FGuid> GetPendingInstanceIdsForTest() const;
	TArray<FGuid> GetActiveInstanceIdsForTest() const;
	TMap<FGuid, float> GetSeedsForTest() const;
	TMap<FGuid, float> GetActiveStartDelaysForTest() const;
	TArray<UWacomDeckCardWidget*> GetActiveCardsForTest() const;
	void ForceActiveReadinessForTest();
#endif

private:
	struct FEntry
	{
		TStrongObjectPtr<UWacomDeckCardWidget> Card;
		FGuid InstanceId;
		FWacomFirstPersonCardPlayedDissolveStyleData Style;
		FWacomFirstPersonCardSurfaceDeparturePlayback Playback;
		FWacomFirstPersonCardPresentationReadinessGate Readiness;
		uint32 PreparationGeneration = 0;
		float StartDelayRemainingSeconds = 0.0f;
		float Seed = 0.0f;
		float StaggerScale = 1.0f;
		uint32 RandomOrderKey = 0;
		bool bSimplifiedMotion = false;
		bool bGroupSoundOwner = false;
	};

	TArray<TUniquePtr<FEntry>> PendingEntries;
	TArray<TUniquePtr<FEntry>> ActiveEntries;
	TSet<float> UsedSeeds;
	int32 MaximumObservedRealtimeCardCount = 0;
	int32 CompletedCardCount = 0;
	uint32 RandomBatchSequence = 0;

	static bool IsStyleValid(
		const FWacomFirstPersonCardPlayedDissolveStyleData& Style);
	static uint32 MixRandomBits(uint32 Value);
	float AllocateSeed(FGuid InstanceId);
	void FillAvailableSlots();
	void PrepareEntry(
		FEntry& Entry,
		float StartDelaySeconds,
		bool bSoundOwner);
	void FinishEntry(FEntry& Entry, bool bFailed);
};
