// Copyright Wacom. All Rights Reserved.

#include "Session/BattleResultPacketBuilder.h"

#include "Core/BattleState.h"
#include "Runtime/RuntimeEnemyPart.h"

FBattleResultPacket FBattleResultPacketBuilder::Build(const FBattleState& State)
{
	FBattleResultPacket Packet;
	Packet.Outcome                 = State.Outcome;
	Packet.EncounterId             = State.Enemy.EncounterId;
	Packet.bCrossedHighHpThreshold = State.bCrossedHighHpThreshold;
	Packet.bCrossedLowHpThreshold  = State.bCrossedLowHpThreshold;
	Packet.bMutualDestruction      = State.bMutualDestruction;
	Packet.KnockdownExpGains       = State.PendingKnockdownExpGains;
	Packet.KnockdownChoices        = State.PendingKnockdownChoices;
	Packet.GainedCards             = State.PendingGainedCards;
	Packet.PersistentCardMutations.Reserve(State.Cards.AllCards.Num());
	for (const FRuntimeCardInstance& Card : State.Cards.AllCards)
	{
		if (!Card.SourceRunInstanceId.IsValid())
		{
			continue;
		}
		FBattlePersistentCardMutation Mutation;
		Mutation.SourceRunInstanceId = Card.SourceRunInstanceId;
		Mutation.PersistentModifiers = Card.PersistentModifiers;
		Packet.PersistentCardMutations.Add(MoveTemp(Mutation));
	}
	Packet.DestroyedParts          = State.DestroyedParts;
	Packet.DestroyedPartKeys.Reserve(State.DestroyedParts.Num());
	for (const FBattlePartSlotIdentity& Identity : State.DestroyedParts)
	{
		Packet.DestroyedPartKeys.AddUnique(Identity.ToEnemyPartKey());
	}

	Packet.EnemyResults.Reserve(State.Enemy.EnemySlots.Num());
	for (const FEnemySlotState& EnemySlot : State.Enemy.EnemySlots)
	{
		FBattleEnemyResult EnemyResult;
		EnemyResult.EncounterId = EnemySlot.EncounterId;
		EnemyResult.EnemySlotId = EnemySlot.EnemySlotId;
		EnemyResult.Definition = EnemySlot.Definition;
		for (const FGuid& PartInstanceId : EnemySlot.PartInstanceIds)
		{
			const int32* PartIndex = State.Enemy.PartIndexById.Find(PartInstanceId);
			if (!PartIndex || !State.Enemy.Parts.IsValidIndex(*PartIndex))
			{
				continue;
			}
			const FRuntimeEnemyPart& Part = State.Enemy.Parts[*PartIndex];
			if (Part.bDestroyed)
			{
				EnemyResult.DestroyedParts.AddUnique(Part.Identity);
				EnemyResult.DestroyedPartKeys.AddUnique(Part.Identity.ToEnemyPartKey());
			}
		}
		Packet.EnemyResults.Add(MoveTemp(EnemyResult));
	}

	for (const FKnockdownChoice& Choice : State.PendingKnockdownChoices)
	{
		if (Choice.Choice == EKnockdownChoice::Withdraw)
		{
			Packet.bWithdrawn = true;
			break;
		}
	}

	return Packet;
}
