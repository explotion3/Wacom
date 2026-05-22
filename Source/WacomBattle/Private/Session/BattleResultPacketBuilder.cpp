// Copyright Wacom. All Rights Reserved.

#include "Session/BattleResultPacketBuilder.h"

#include "Core/BattleState.h"

FBattleResultPacket FBattleResultPacketBuilder::Build(const FBattleState& State)
{
	FBattleResultPacket Packet;
	Packet.Outcome                 = State.Outcome;
	Packet.bCrossedHighHpThreshold = State.bCrossedHighHpThreshold;
	Packet.bCrossedLowHpThreshold  = State.bCrossedLowHpThreshold;
	Packet.bMutualDestruction      = State.bMutualDestruction;
	Packet.KnockdownExpGains       = State.PendingKnockdownExpGains;
	Packet.KnockdownChoices        = State.PendingKnockdownChoices;
	Packet.GainedCards             = State.PendingGainedCards;
	Packet.DestroyedPartIds        = State.DestroyedPartIds;

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
