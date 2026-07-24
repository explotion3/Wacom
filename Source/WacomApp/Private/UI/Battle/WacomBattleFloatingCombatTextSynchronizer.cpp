// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleFloatingCombatTextSynchronizer.h"

#include "Events/BattleEvent.h"

namespace
{
	FWacomBattleFloatingCombatTextTarget ResolveTarget(const FBattleEvent& Event)
	{
		return Event.ActorEnemyPartKey.IsValidKey()
			? FWacomBattleFloatingCombatTextTarget::EnemyPart(
				Event.ActorEnemyPartKey)
			: FWacomBattleFloatingCombatTextTarget::Player();
	}
}

void FWacomBattleFloatingCombatTextSynchronizer::Stage(
	const uint64 TransactionId,
	const TArray<FBattleEvent>& Events)
{
	if (TransactionId == 0)
	{
		return;
	}

	FPendingTransaction& Transaction = PendingTransactions.FindOrAdd(TransactionId);
	Transaction.Groups.Reset();
	for (const FBattleEvent& Event : Events)
	{
		TArray<FWacomBattleFloatingCombatTextRow> Rows = BuildRows(Event);
		if (Rows.IsEmpty())
		{
			continue;
		}

		FPendingGroup& Group = Transaction.Groups.AddDefaulted_GetRef();
		Group.EventSequence = Event.Sequence;
		Group.Rows = MoveTemp(Rows);
	}
}

TArray<FWacomBattleFloatingCombatTextEmission>
FWacomBattleFloatingCombatTextSynchronizer::ApplyProgress(
	const FWacomBattlePresentationProgress& Progress,
	bool& bOutFlushedRemainder)
{
	bOutFlushedRemainder = false;
	if (Progress.PresentationTransactionId == 0
		|| !PendingTransactions.Contains(Progress.PresentationTransactionId))
	{
		return {};
	}

	switch (Progress.Kind)
	{
	case EWacomBattlePresentationProgressKind::PhaseEventsReached:
	{
		TSet<int32> ReachedSequences;
		for (const int32 EventSequence : Progress.EventSequences)
		{
			ReachedSequences.Add(EventSequence);
		}
		return ReleaseMatching(
			Progress.PresentationTransactionId,
			[&ReachedSequences](const int32 Sequence)
			{
				return ReachedSequences.Contains(Sequence);
			});
	}

	case EWacomBattlePresentationProgressKind::EnemyActionImpact:
		return ReleaseMatching(
			Progress.PresentationTransactionId,
			[&Progress](const int32 Sequence)
			{
				return MatchesRange(
					Sequence,
					Progress.FirstEventSequence,
					Progress.LastEventSequence);
			});

	case EWacomBattlePresentationProgressKind::PlanCompleted:
		return Flush(Progress.PresentationTransactionId, bOutFlushedRemainder);

	case EWacomBattlePresentationProgressKind::PlanCancelled:
		if (Progress.CancelPolicy == EWacomBattlePresentationCancelPolicy::DiscardPending)
		{
			Discard(Progress.PresentationTransactionId);
			return {};
		}
		return Flush(Progress.PresentationTransactionId, bOutFlushedRemainder);

	default:
		return {};
	}
}

void FWacomBattleFloatingCombatTextSynchronizer::Discard(const uint64 TransactionId)
{
	PendingTransactions.Remove(TransactionId);
}

void FWacomBattleFloatingCombatTextSynchronizer::Clear()
{
	PendingTransactions.Reset();
}

TArray<FWacomBattleFloatingCombatTextRow>
FWacomBattleFloatingCombatTextSynchronizer::BuildRows(const FBattleEvent& Event)
{
	TArray<FWacomBattleFloatingCombatTextRow> Rows;
	if (Event.Sequence <= 0)
	{
		return Rows;
	}

	if (Event.Type == EBattleEventType::DamageDealt)
	{
		int32 ChannelIndex = 0;
		if (Event.DamageResolution.ShieldAbsorbed > 0)
		{
			FWacomBattleFloatingCombatTextRow& Shield = Rows.AddDefaulted_GetRef();
			Shield.EventSequence = Event.Sequence;
			Shield.ChannelIndex = ChannelIndex++;
			Shield.Kind = EWacomBattleFloatingCombatTextKind::ShieldAbsorbed;
			Shield.Target = ResolveTarget(Event);
			Shield.Amount = Event.DamageResolution.ShieldAbsorbed;
			Shield.bShieldBroken = Event.DamageResolution.ShieldAfter <= 0;
		}

		if (Event.Amount > 0)
		{
			FWacomBattleFloatingCombatTextRow& Damage = Rows.AddDefaulted_GetRef();
			Damage.EventSequence = Event.Sequence;
			Damage.ChannelIndex = ChannelIndex;
			Damage.Kind = Event.DamageResolution.bCritical
				? EWacomBattleFloatingCombatTextKind::CriticalDamage
				: Event.DamageResolution.Kind == EBattleDamageKind::Periodic
					? EWacomBattleFloatingCombatTextKind::PeriodicDamage
					: EWacomBattleFloatingCombatTextKind::HpDamage;
			Damage.Target = ResolveTarget(Event);
			Damage.Amount = Event.Amount;
			Damage.IconTag = Damage.Kind == EWacomBattleFloatingCombatTextKind::PeriodicDamage
				? Event.Tag
				: FGameplayTag();
		}
		return Rows;
	}

	if (Event.Type == EBattleEventType::ShieldChanged && Event.Amount != 0)
	{
		FWacomBattleFloatingCombatTextRow& Shield = Rows.AddDefaulted_GetRef();
		Shield.EventSequence = Event.Sequence;
		Shield.ChannelIndex = 0;
		Shield.Kind = EWacomBattleFloatingCombatTextKind::ShieldChanged;
		Shield.Target = ResolveTarget(Event);
		Shield.Amount = Event.Amount;
	}
	return Rows;
}

bool FWacomBattleFloatingCombatTextSynchronizer::MatchesRange(
	const int32 Sequence,
	const int32 First,
	const int32 Last)
{
	return Sequence > 0 && First > 0 && Last >= First
		&& Sequence >= First && Sequence <= Last;
}

TArray<FWacomBattleFloatingCombatTextEmission>
FWacomBattleFloatingCombatTextSynchronizer::ReleaseMatching(
	const uint64 TransactionId,
	TFunctionRef<bool(int32)> Predicate)
{
	FPendingTransaction* Transaction = PendingTransactions.Find(TransactionId);
	if (!Transaction)
	{
		return {};
	}

	FWacomBattleFloatingCombatTextEmission Emission;
	Emission.TransactionId = TransactionId;
	for (FPendingGroup& Group : Transaction->Groups)
	{
		if (Group.bReleased || !Predicate(Group.EventSequence))
		{
			continue;
		}
		Group.bReleased = true;
		Emission.Rows.Append(Group.Rows);
	}

	TArray<FWacomBattleFloatingCombatTextEmission> Result;
	if (!Emission.Rows.IsEmpty())
	{
		Result.Add(MoveTemp(Emission));
	}
	return Result;
}

TArray<FWacomBattleFloatingCombatTextEmission>
FWacomBattleFloatingCombatTextSynchronizer::Flush(
	const uint64 TransactionId,
	bool& bOutFlushedRemainder)
{
	FPendingTransaction* Transaction = PendingTransactions.Find(TransactionId);
	if (!Transaction)
	{
		bOutFlushedRemainder = false;
		return {};
	}

	FWacomBattleFloatingCombatTextEmission Emission;
	Emission.TransactionId = TransactionId;
	for (FPendingGroup& Group : Transaction->Groups)
	{
		if (Group.bReleased)
		{
			continue;
		}
		Group.bReleased = true;
		Emission.Rows.Append(Group.Rows);
	}
	bOutFlushedRemainder = !Emission.Rows.IsEmpty();
	PendingTransactions.Remove(TransactionId);

	TArray<FWacomBattleFloatingCombatTextEmission> Result;
	if (!Emission.Rows.IsEmpty())
	{
		Result.Add(MoveTemp(Emission));
	}
	return Result;
}
