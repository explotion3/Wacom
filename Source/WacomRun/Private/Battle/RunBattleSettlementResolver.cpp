// Copyright Wacom. All Rights Reserved.

#include "Battle/RunBattleSettlementResolver.h"

#include "Cards/CardDefinition.h"
#include "Deck/RunDeckRules.h"
#include "RunState.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	int32 PopulateWithdrawnBattleProgressSnapshot(
		const FBattleResultPacket& Packet,
		FBattleProgressSnapshot& OutSnapshot)
	{
		OutSnapshot.DestroyedPartKeys.Reset();
		OutSnapshot.DestroyedParts.Reset();
		OutSnapshot.DestroyedPartKeys.Reserve(
			Packet.DestroyedPartKeys.Num() + Packet.DestroyedParts.Num());

		for (const FBattleEnemyPartKey& DestroyedPartKey : Packet.DestroyedPartKeys)
		{
			if (DestroyedPartKey.IsValidKey())
			{
				OutSnapshot.DestroyedPartKeys.AddUnique(DestroyedPartKey);
			}
		}

		if (OutSnapshot.DestroyedPartKeys.Num() == 0)
		{
			for (const FBattlePartSlotIdentity& DestroyedPart : Packet.DestroyedParts)
			{
				if (DestroyedPart.IsValidSlot())
				{
					OutSnapshot.DestroyedPartKeys.AddUnique(DestroyedPart.ToEnemyPartKey());
				}
			}
		}

		// Legacy fallback only: new packets should persist keys, not duplicate the
		// internal identity projection into Run progress.
		if (OutSnapshot.DestroyedPartKeys.Num() == 0)
		{
			OutSnapshot.DestroyedParts = Packet.DestroyedParts;
		}

		return OutSnapshot.DestroyedPartKeys.Num() > 0
			? OutSnapshot.DestroyedPartKeys.Num()
			: OutSnapshot.DestroyedParts.Num();
	}

	FString GetPartKeyDebugString(const FBattleEnemyPartKey& PartKey)
	{
		return PartKey.IsValidKey() ? PartKey.ToDebugString() : FString(TEXT("<invalid>"));
	}

	bool GrantCard(FRunState& State, UCardDefinition* Card)
	{
		if (!Card)
		{
			return false;
		}
		FCardInstance Instance;
		Instance.Definition = Card;
		Instance.InstanceId = FGuid::NewGuid();
		if (!Instance.InstanceId.IsValid())
		{
			return false;
		}
		State.Backpack.Add(Instance);
		FRunDeckRules::EnsureSpecialZoneEntryFor(State, Instance);
		FRunDeckRules::RecomputeBurden(State, true);
		return true;
	}

	void AddExperience(FRunState& State, const int32 Amount)
	{
		if (Amount <= 0)
		{
			return;
		}
		State.ExperienceCurrent = FMath::Max(0, State.ExperienceCurrent + Amount);
		const int32 Capacity = FMath::Max(1, State.ExperienceCapacity);
		while (State.ExperienceCurrent >= Capacity)
		{
			State.ExperienceCurrent -= Capacity;
			State.AcquiredSkills.Add(WacomTags::SkillSlot_Placeholder);
		}
	}
}

bool FRunBattleSettlementResolver::Resolve(
	FRunState& State,
	const FBattleResultPacket& Packet,
	const FWacomMapNodeHandle& EncounterNode)
{
	// 1) Outcome 主分支
	switch (Packet.Outcome)
	{
	case EBattleOutcome::Victory:
		if (Packet.bWithdrawn)
		{
			// 撤离：节点不算完成。
			// 持久化破坏部位列表，下次进入同一战斗 Trigger 时维持破坏态。
			int32 PersistedDestroyedPartCount = 0;
			if (EncounterNode.IsValid())
			{
				FBattleProgressSnapshot Snapshot;
				PersistedDestroyedPartCount = PopulateWithdrawnBattleProgressSnapshot(Packet, Snapshot);
				State.BattleProgress.Add(EncounterNode, MoveTemp(Snapshot));
			}
			UE_LOG(LogTemp, Display,
				TEXT("[RunSession] Battle withdrawn (Node=%s/%s, %d parts persisted destroyed)"),
				*EncounterNode.FloorId.ToString(),
				*EncounterNode.NodeId.ToString(),
				PersistedDestroyedPartCount);
		}
		else
		{
			// 真胜利：清理该 Trigger 的撤离进度；永久完成状态由 GameMode.MarkTriggerDestroyed 写入。
			if (EncounterNode.IsValid())
			{
				State.BattleProgress.Remove(EncounterNode);
			}
			UE_LOG(LogTemp, Display,
				TEXT("[RunSession] Battle victory (Node=%s/%s)"),
				*EncounterNode.FloorId.ToString(),
				*EncounterNode.NodeId.ToString());
		}
		break;

	case EBattleOutcome::Defeat:
		State.bRunActive = false;
		UE_LOG(LogTemp, Display, TEXT("[RunSession] Battle defeat, run ended"));
		break;

	case EBattleOutcome::Undetermined:
	default:
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] OnBattleFinished with Undetermined outcome, ignored"));
		// 未定结果不做战外结算（疲劳 / 伤口都不加），直接返回。
		return false;
	}

	// 2) 战外结算压力。
	// 疲劳：每场战斗后 +1%（无论胜败）。
	State.Pressure.Add(EWacomPressureType::Fatigue, 1);

	// 伤口阈值跨越。
	if (Packet.bCrossedHighHpThreshold)
	{
		State.Pressure.Add(EWacomPressureType::Wound, 1);
	}
	if (Packet.bCrossedLowHpThreshold)
	{
		State.Pressure.Add(EWacomPressureType::Wound, 5);
	}
	// 同归于尽：+10% 伤口；不影响 bRunActive（Outcome 已是 Victory）。
	if (Packet.bMutualDestruction)
	{
		State.Pressure.Add(EWacomPressureType::Wound, 10);
		UE_LOG(LogTemp, Display,
			TEXT("[RunSession] Mutual destruction: Wound +10%%, total Wound=%d"),
				State.Pressure.Get(EWacomPressureType::Wound));
	}

	// 3) 经验结算。
	// Defeat 不结算：Run 都结束了发了无意义。
	// Victory（含同归于尽）正常结算。
	if (Packet.Outcome == EBattleOutcome::Victory)
	{
		int32 TotalExp = 0;
		for (const FKnockdownExpGain& Gain : Packet.KnockdownExpGains)
		{
			TotalExp += Gain.ExpAmount;
		}
		if (TotalExp > 0)
		{
			AddExperience(State, TotalExp);
			UE_LOG(LogTemp, Display,
				TEXT("[RunSession] Exp granted: %d (from %d destroyed parts)"),
				TotalExp, Packet.KnockdownExpGains.Num());
		}
	}

	// 4) 战斗中获得的卡牌结算。
	// Victory 包括撤离；Defeat / Undetermined 不结算。
	if (Packet.Outcome == EBattleOutcome::Victory)
	{
		for (const FBattleGainedCard& GainedCard : Packet.GainedCards)
		{
			if (!GainedCard.Definition)
			{
				continue;
			}
			if (!GrantCard(State, GainedCard.Definition.Get()))
			{
				return false;
			}
			UE_LOG(LogTemp, Display,
				TEXT("[RunSession] Gained card from battle: Card=%s, SourcePartKey=%s, Choice=%d"),
				*GetNameSafe(GainedCard.Definition),
				*GetPartKeyDebugString(GainedCard.SourcePartKey),
				static_cast<int32>(GainedCard.SourceChoice));
		}
	}

	// 5) 击倒事件玩家选择记账。当前只打日志；后续可由 RunEvent 按 Choice 衔接分支。
	for (const FKnockdownChoice& Choice : Packet.KnockdownChoices)
	{
		const TCHAR* ChoiceName = TEXT("?");
		switch (Choice.Choice)
		{
		case EKnockdownChoice::Aid:      ChoiceName = TEXT("Aid"); break;
		case EKnockdownChoice::Destroy:  ChoiceName = TEXT("Destroy"); break;
		case EKnockdownChoice::Withdraw: ChoiceName = TEXT("Withdraw"); break;
		default: break;
		}
		UE_LOG(LogTemp, Display,
			TEXT("[RunSession] KnockdownChoice: PartKey=%s, Choice=%s"),
			*GetPartKeyDebugString(Choice.PartKey), ChoiceName);
	}

	return true;
}
