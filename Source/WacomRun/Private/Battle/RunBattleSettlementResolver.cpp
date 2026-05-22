// Copyright Wacom. All Rights Reserved.

#include "Battle/RunBattleSettlementResolver.h"

#include "Cards/CardDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "RunState.h"

bool FRunBattleSettlementResolver::Resolve(
	FRunState& State,
	const FBattleResultPacket& Packet,
	UEnemyDefinition* EnemyDef,
	FName TriggerPersistentId,
	const FCallbacks& Callbacks)
{
	// 1) Outcome 主分支
	switch (Packet.Outcome)
	{
	case EBattleOutcome::Victory:
		if (Packet.bWithdrawn)
		{
			// 撤离：敌人不进 DefeatedEnemies、节点不算完成。
			// 持久化破坏部位列表，下次进入同一战斗 Trigger 时维持破坏态。
			if (!TriggerPersistentId.IsNone())
			{
				FBattleProgressSnapshot Snapshot;
				Snapshot.DestroyedPartIds = Packet.DestroyedPartIds;
				State.BattleProgress.Add(TriggerPersistentId, MoveTemp(Snapshot));
			}
			UE_LOG(LogTemp, Display,
				TEXT("[RunSession] Battle withdrawn from %s (Trigger=%s, %d parts persisted destroyed)"),
				*GetNameSafe(EnemyDef),
				*TriggerPersistentId.ToString(),
				Packet.DestroyedPartIds.Num());
		}
		else
		{
			// 真胜利：进 DefeatedEnemies + 清理该 Trigger 的进度（一次性完成）
			if (EnemyDef)
			{
				State.DefeatedEnemies.AddUnique(EnemyDef);
			}
			if (!TriggerPersistentId.IsNone())
			{
				State.BattleProgress.Remove(TriggerPersistentId);
			}
			UE_LOG(LogTemp, Display,
				TEXT("[RunSession] Battle victory against %s (%d total defeated)"),
				*GetNameSafe(EnemyDef),
				State.DefeatedEnemies.Num());
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
	Callbacks.AddPressure(EWacomPressureType::Fatigue, 1);

	// 伤口阈值跨越。
	if (Packet.bCrossedHighHpThreshold)
	{
		Callbacks.AddPressure(EWacomPressureType::Wound, 1);
	}
	if (Packet.bCrossedLowHpThreshold)
	{
		Callbacks.AddPressure(EWacomPressureType::Wound, 5);
	}
	// 同归于尽：+10% 伤口；不影响 bRunActive（Outcome 已是 Victory）。
	if (Packet.bMutualDestruction)
	{
		Callbacks.AddPressure(EWacomPressureType::Wound, 10);
		UE_LOG(LogTemp, Display,
			TEXT("[RunSession] Mutual destruction: Wound +10%%, total Wound=%d"),
			Callbacks.GetPressureValue(EWacomPressureType::Wound));
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
			Callbacks.AddExperience(TotalExp);
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
			Callbacks.AcquireCardToRun(GainedCard.Definition.Get());
			UE_LOG(LogTemp, Display,
				TEXT("[RunSession] Gained card from battle: Card=%s, Part=%s, Choice=%d"),
				*GetNameSafe(GainedCard.Definition),
				*GainedCard.SourcePartId.ToString(),
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
			TEXT("[RunSession] KnockdownChoice: Part=%s, Choice=%s"),
			*Choice.PartId.ToString(), ChoiceName);
	}

	return true;
}
