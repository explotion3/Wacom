// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/DebugBattleHUD.h"

#include "CommonTextBlock.h"
#include "Snapshots/BattleSnapshot.h"
#include "Cards/CardDefinition.h"
#include "Enemies/EnemyPartDefinition.h"

namespace
{
	const TCHAR* ZoneToString(EHandZone Zone)
	{
		switch (Zone)
		{
		case EHandZone::Left:  return TEXT("L");
		case EHandZone::Both:  return TEXT("B");
		case EHandZone::Right: return TEXT("R");
		default:               return TEXT("-");
		}
	}

	const TCHAR* PhaseToString(EBattlePhase Phase)
	{
		switch (Phase)
		{
		case EBattlePhase::Setup:        return TEXT("Setup");
		case EBattlePhase::TurnStart:    return TEXT("TurnStart");
		case EBattlePhase::PlayerAction: return TEXT("PlayerAction");
		case EBattlePhase::TurnEnd:      return TEXT("TurnEnd");
		case EBattlePhase::PendingKnockdownChoice: return TEXT("PendingKnockdownChoice");
		case EBattlePhase::BattleEnd:    return TEXT("BattleEnd");
		default:                          return TEXT("None");
		}
	}
}

void UDebugBattleHUD::NativeRefreshFromSnapshot(const FBattleSnapshot& Snap)
{
	if (!SnapshotText) { return; }

	FString Msg;
	Msg += FString::Printf(TEXT("== Turn %d | %s | Wait=%d | Ver=%d =="),
		Snap.TurnNumber, PhaseToString(Snap.Phase), Snap.CurrentWaitValue, Snap.Version);
	Msg += LINE_TERMINATOR;

	Msg += FString::Printf(TEXT("Player HP %d/%d  Shield %d"),
		Snap.Player.CurrentHp, Snap.Player.MaxHp, Snap.Player.Shield);
	Msg += LINE_TERMINATOR;

	Msg += FString::Printf(TEXT("Enemy InitSum=%d  AllDestroyed=%d"),
		Snap.Enemy.InitiativeSum, (int32)Snap.Enemy.bAllPartsDestroyed);
	Msg += LINE_TERMINATOR;

	for (int32 i = 0; i < Snap.Enemy.Parts.Num(); ++i)
	{
		const auto& P = Snap.Enemy.Parts[i];
		Msg += FString::Printf(TEXT("  Part[%d] HP %d/%d  Init %d  Shield %d  Destroyed=%d  Intent=%s Init=%d Resist=%d"),
			i, P.CurrentHp, P.MaxHp, P.CurrentInitiative, P.Shield, (int32)P.bDestroyed,
			*P.CurrentIntent.DisplayName.ToString(),
			P.CurrentIntent.Initiative,
			P.CurrentIntent.ResistanceValue);
		Msg += LINE_TERMINATOR;
	}

	Msg += FString::Printf(TEXT("Hand: %d cards (Normal=%d/%d, L=%d R=%d)"),
		Snap.Hand.Cards.Num(), Snap.Hand.NormalCardCount, Snap.Hand.NormalCardLimit,
		(int32)Snap.Hand.bLeftHandPresent, (int32)Snap.Hand.bRightHandPresent);
	Msg += LINE_TERMINATOR;

	for (int32 i = 0; i < Snap.Hand.Cards.Num(); ++i)
	{
		const auto& C = Snap.Hand.Cards[i];
		const FString Name = C.Definition ? C.Definition->CardId.ToString() : TEXT("?");
		Msg += FString::Printf(TEXT("  [%d] %-20s  Cost=%d  Zone=%s  Anchor=%d  Playable=%d"),
			i + 1, *Name, C.RuntimeCost, ZoneToString(C.Zone),
			(int32)C.bIsHandAnchor, (int32)C.bIsPlayable);
		Msg += LINE_TERMINATOR;
	}

	Msg += FString::Printf(TEXT("Pile: Draw=%d  Discard=%d  Played=%d  Exhaust=%d"),
		Snap.PileCounts.DrawCount,
		Snap.PileCounts.DiscardCount,
		Snap.PileCounts.PlayedCount,
		Snap.PileCounts.ExhaustCount);

	SnapshotText->SetText(FText::FromString(Msg));
}
