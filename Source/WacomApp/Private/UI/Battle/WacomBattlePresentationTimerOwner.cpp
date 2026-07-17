// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattlePresentationTimerOwner.h"

#include "Engine/World.h"
#include "TimerManager.h"

FWacomBattlePresentationTimerKey FWacomBattlePresentationTimerKey::EventQueueAdvance()
{
	return {
		EWacomBattlePresentationTimerKind::EventQueueAdvance,
		INDEX_NONE,
	};
}

FWacomBattlePresentationTimerKey FWacomBattlePresentationTimerKey::PresentationPlanPoll()
{
	return {
		EWacomBattlePresentationTimerKind::PresentationPlanPoll,
		INDEX_NONE,
	};
}

FWacomBattlePresentationTimerKey FWacomBattlePresentationTimerKey::StackEntryExit(
	int32 EntryId)
{
	return {
		EWacomBattlePresentationTimerKind::StackEntryExit,
		EntryId,
	};
}

FWacomBattlePresentationTimerOwner::~FWacomBattlePresentationTimerOwner()
{
	AbandonAllWithoutWorldAccess();
}

bool FWacomBattlePresentationTimerOwner::ScheduleOnce(
	UWorld* World,
	const FWacomBattlePresentationTimerKey& Key,
	float DelaySeconds,
	TFunction<void()>&& Callback)
{
	Cancel(Key);
	if (!IsValid(World) || !Callback)
	{
		return false;
	}

	uint64 Serial = ++NextSerial;
	if (Serial == 0)
	{
		Serial = ++NextSerial;
	}

	FTimerEntry& Entry = Entries.Add(Key);
	Entry.World = World;
	Entry.Serial = Serial;
	Entry.Callback = MoveTemp(Callback);

	const TWeakPtr<FWacomBattlePresentationTimerOwner> WeakThis = AsShared();
	World->GetTimerManager().SetTimer(
		Entry.Handle,
		FTimerDelegate::CreateLambda([WeakThis, Key, Serial]()
		{
			if (const TSharedPtr<FWacomBattlePresentationTimerOwner> Pinned = WeakThis.Pin())
			{
				Pinned->HandleTimerFired(Key, Serial);
			}
		}),
		FMath::Max(0.01f, DelaySeconds),
		false);
	return true;
}

void FWacomBattlePresentationTimerOwner::Cancel(
	const FWacomBattlePresentationTimerKey& Key)
{
	FTimerEntry Entry;
	if (!Entries.RemoveAndCopyValue(Key, Entry))
	{
		return;
	}

	if (UWorld* World = Entry.World.Get())
	{
		World->GetTimerManager().ClearTimer(Entry.Handle);
	}
}

void FWacomBattlePresentationTimerOwner::CancelKind(
	EWacomBattlePresentationTimerKind Kind)
{
	TArray<FWacomBattlePresentationTimerKey> Keys;
	for (const TPair<FWacomBattlePresentationTimerKey, FTimerEntry>& Pair : Entries)
	{
		if (Pair.Key.Kind == Kind)
		{
			Keys.Add(Pair.Key);
		}
	}

	for (const FWacomBattlePresentationTimerKey& Key : Keys)
	{
		Cancel(Key);
	}
}

void FWacomBattlePresentationTimerOwner::CancelAll()
{
	TArray<FWacomBattlePresentationTimerKey> Keys;
	Entries.GenerateKeyArray(Keys);
	for (const FWacomBattlePresentationTimerKey& Key : Keys)
	{
		Cancel(Key);
	}
}

void FWacomBattlePresentationTimerOwner::AbandonWithoutWorldAccess(
	const FWacomBattlePresentationTimerKey& Key)
{
	Entries.Remove(Key);
}

void FWacomBattlePresentationTimerOwner::AbandonAllWithoutWorldAccess()
{
	Entries.Reset();
}

void FWacomBattlePresentationTimerOwner::HandleTimerFired(
	const FWacomBattlePresentationTimerKey& Key,
	uint64 ExpectedSerial)
{
	FTimerEntry* Entry = Entries.Find(Key);
	if (!Entry || Entry->Serial != ExpectedSerial)
	{
		return;
	}

	TFunction<void()> Callback = MoveTemp(Entry->Callback);
	Entries.Remove(Key);
	if (Callback)
	{
		Callback();
	}
}
