// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"

struct FWacomBattlePresentationTargetEntry
{
	TWeakObjectPtr<UObject> Owner;
	TFunction<void(const FWacomBattlePresentationTargetCue&)> Handler;
};

/**
 * BattleHUD 私有表现目标表。
 *
 * V1 只按敌方部位实例 ID 分发轻量 TargetCue；当前注册者是 2D EnemyPartWidget，
 * 后续场景敌人 Actor/Component 可注册到同一表而不让表现队列依赖具体 UI 类型。
 */
class FWacomBattlePresentationTargetRegistry
{
public:
	void Register(
		const FGuid& PartInstanceId,
		UObject* Owner,
		TFunction<void(const FWacomBattlePresentationTargetCue&)> Handler)
	{
		if (!PartInstanceId.IsValid() || !IsValid(Owner) || !Handler)
		{
			return;
		}

		RemoveStaleTargets();

		FWacomBattlePresentationTargetEntry Entry;
		Entry.Owner = Owner;
		Entry.Handler = MoveTemp(Handler);
		TargetsByPartId.Add(PartInstanceId, MoveTemp(Entry));
	}

	void UnregisterOwner(const UObject* Owner)
	{
		if (!Owner)
		{
			return;
		}

		for (TMap<FGuid, FWacomBattlePresentationTargetEntry>::TIterator It(TargetsByPartId); It; ++It)
		{
			if (!It.Value().Owner.IsValid() || It.Value().Owner.Get() == Owner)
			{
				It.RemoveCurrent();
			}
		}
	}

	void Clear()
	{
		TargetsByPartId.Reset();
	}

	bool ContainsOwner(const UObject* Owner) const
	{
		if (!Owner)
		{
			return false;
		}

		for (const TPair<FGuid, FWacomBattlePresentationTargetEntry>& Pair : TargetsByPartId)
		{
			if (Pair.Value.Owner.IsValid() && Pair.Value.Owner.Get() == Owner && Pair.Value.Handler)
			{
				return true;
			}
		}
		return false;
	}

	bool PlayCue(const FWacomBattlePresentationTargetCue& Cue)
	{
		if (!Cue.TargetPartInstanceId.IsValid())
		{
			return false;
		}

		FWacomBattlePresentationTargetEntry* Entry = TargetsByPartId.Find(Cue.TargetPartInstanceId);
		if (!Entry)
		{
			RemoveStaleTargets();
			return false;
		}

		if (!Entry->Owner.IsValid() || !Entry->Handler)
		{
			TargetsByPartId.Remove(Cue.TargetPartInstanceId);
			return false;
		}

		Entry->Handler(Cue);
		return true;
	}

	int32 Num() const
	{
		int32 Count = 0;
		for (const TPair<FGuid, FWacomBattlePresentationTargetEntry>& Pair : TargetsByPartId)
		{
			if (Pair.Value.Owner.IsValid() && Pair.Value.Handler)
			{
				++Count;
			}
		}
		return Count;
	}

private:
	void RemoveStaleTargets()
	{
		for (TMap<FGuid, FWacomBattlePresentationTargetEntry>::TIterator It(TargetsByPartId); It; ++It)
		{
			if (!It.Value().Owner.IsValid() || !It.Value().Handler)
			{
				It.RemoveCurrent();
			}
		}
	}

	TMap<FGuid, FWacomBattlePresentationTargetEntry> TargetsByPartId;
};
