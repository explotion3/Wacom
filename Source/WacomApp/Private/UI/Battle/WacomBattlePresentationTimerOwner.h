// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWorld;

enum class EWacomBattlePresentationTimerKind : uint8
{
	EventQueueAdvance,
	PresentationPlanPoll,
	StackEntryExit,
};

struct FWacomBattlePresentationTimerKey
{
	EWacomBattlePresentationTimerKind Kind =
		EWacomBattlePresentationTimerKind::EventQueueAdvance;
	int32 StackEntryId = INDEX_NONE;

	static FWacomBattlePresentationTimerKey EventQueueAdvance();
	static FWacomBattlePresentationTimerKey PresentationPlanPoll();
	static FWacomBattlePresentationTimerKey StackEntryExit(int32 EntryId);

	bool operator==(const FWacomBattlePresentationTimerKey& Other) const
	{
		return Kind == Other.Kind && StackEntryId == Other.StackEntryId;
	}

	friend uint32 GetTypeHash(const FWacomBattlePresentationTimerKey& Key)
	{
		return HashCombine(
			GetTypeHash(static_cast<uint8>(Key.Kind)),
			GetTypeHash(Key.StackEntryId));
	}
};

/**
 * Owns every TimerManager registration used by Battle presentation orchestration.
 *
 * TimerManager delegates only retain a weak reference to this owner plus a key and
 * serial. The business callback remains inside the matching entry, so destroying
 * the owner makes every outstanding delegate inert even when its World is no
 * longer available for explicit cancellation.
 */
class FWacomBattlePresentationTimerOwner
	: public TSharedFromThis<FWacomBattlePresentationTimerOwner>
{
public:
	~FWacomBattlePresentationTimerOwner();

	bool ScheduleOnce(
		UWorld* World,
		const FWacomBattlePresentationTimerKey& Key,
		float DelaySeconds,
		TFunction<void()>&& Callback);
	void Cancel(const FWacomBattlePresentationTimerKey& Key);
	void CancelKind(EWacomBattlePresentationTimerKind Kind);
	void CancelAll();
	void AbandonWithoutWorldAccess(const FWacomBattlePresentationTimerKey& Key);
	void AbandonAllWithoutWorldAccess();

private:
	struct FTimerEntry
	{
		TWeakObjectPtr<UWorld> World;
		FTimerHandle Handle;
		uint64 Serial = 0;
		TFunction<void()> Callback;
	};

	TMap<FWacomBattlePresentationTimerKey, FTimerEntry> Entries;
	uint64 NextSerial = 0;

	void HandleTimerFired(
		const FWacomBattlePresentationTimerKey& Key,
		uint64 ExpectedSerial);
};
