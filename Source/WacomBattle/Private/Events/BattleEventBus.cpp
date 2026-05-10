// Copyright Wacom. All Rights Reserved.

#include "Events/BattleEventBus.h"

void FBattleEventBus::Emit(FBattleEvent Event)
{
	Event.Sequence = NextSequence++;
	Pending.Add(MoveTemp(Event));
}

TArray<FBattleEvent> FBattleEventBus::Consume()
{
	TArray<FBattleEvent> Out = MoveTemp(Pending);
	Pending.Reset();
	return Out;
}

void FBattleEventBus::Reset()
{
	Pending.Reset();
	NextSequence = 0;
}
