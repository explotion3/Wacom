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

FBattleEventBus FBattleEventBus::BeginTransaction() const
{
	FBattleEventBus Transaction;
	Transaction.NextSequence = NextSequence;
	return Transaction;
}

void FBattleEventBus::CommitTransactionSequence(const FBattleEventBus& Transaction)
{
	NextSequence = Transaction.NextSequence;
}
