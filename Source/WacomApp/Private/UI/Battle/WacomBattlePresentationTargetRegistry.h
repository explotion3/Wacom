// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"

class FWacomBattlePresentationTargetRegistry
{
public:
	void Register(const FBattlePartSlotIdentity& TargetPartKey, UObject* Owner,
		TFunction<void(const FWacomBattlePresentationTargetCue&)> Handler)
	{
		for (int32 i = Handlers.Num() - 1; i >= 0; --i)
		{
			if (Handlers[i].TargetPartKey == TargetPartKey)
			{
				Handlers.RemoveAt(i);
			}
		}

		FEntry& Entry = Handlers.AddDefaulted_GetRef();
		Entry.TargetPartKey = TargetPartKey;
		Entry.Owner = Owner;
		Entry.Handler = MoveTemp(Handler);
	}

	void UnregisterOwner(const UObject* Owner)
	{
		for (int32 i = Handlers.Num() - 1; i >= 0; --i)
		{
			if (Handlers[i].Owner.Get() == Owner)
			{
				Handlers.RemoveAt(i);
			}
		}
	}

	bool ContainsOwner(const UObject* Owner) const
	{
		for (const FEntry& Entry : Handlers)
		{
			if (Entry.Owner.Get() == Owner)
			{
				return true;
			}
		}
		return false;
	}

	void PlayCue(const FWacomBattlePresentationTargetCue& Cue) const
	{
		for (const FEntry& Entry : Handlers)
		{
			if (Entry.TargetPartKey == Cue.TargetPartKey && Entry.Handler)
			{
				Entry.Handler(Cue);
				break;
			}
		}
	}

	void Clear() { Handlers.Empty(); }

	int32 Num() const { return Handlers.Num(); }

private:
	struct FEntry
	{
		FBattlePartSlotIdentity TargetPartKey;
		TWeakObjectPtr<UObject> Owner;
		TFunction<void(const FWacomBattlePresentationTargetCue&)> Handler;
	};

	TArray<FEntry> Handlers;
};
