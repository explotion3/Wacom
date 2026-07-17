// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Actors/BattleTriggerActor.h"

/**
 * ExitBattle 的双信号屏障。
 *
 * 镜头返回与 ExitBattle 后置工作都完成后，先统一退役已解决 Encounter 的
 * Trigger/Host，再执行 Journey Summary 或普通 Run 表现恢复回调。
 */
class FExitBattlePostRunBarrierState
{
public:
	explicit FExitBattlePostRunBarrierState(TFunction<void()>&& InOnReady)
		: OnReady(MoveTemp(InOnReady))
	{
	}

	void MarkReturnCompleted()
	{
		bReturnCompleted = true;
		TryComplete();
	}

	void MarkExitBattlePostRunReady()
	{
		bExitBattlePostRunReady = true;
		TryComplete();
	}

	void SetResolvedEncounterTrigger(ABattleTriggerActor* InTrigger)
	{
		WeakResolvedEncounterTrigger = InTrigger;
	}

private:
	void TryComplete()
	{
		if (!bReturnCompleted || !bExitBattlePostRunReady || bCompleted)
		{
			return;
		}

		bCompleted = true;
		if (ABattleTriggerActor* Trigger = WeakResolvedEncounterTrigger.Get())
		{
			Trigger->CompleteResolvedEncounterSceneRetirement();
		}
		if (OnReady)
		{
			OnReady();
		}
	}

	TFunction<void()> OnReady;
	TWeakObjectPtr<ABattleTriggerActor> WeakResolvedEncounterTrigger;
	bool bReturnCompleted = false;
	bool bExitBattlePostRunReady = false;
	bool bCompleted = false;
};
