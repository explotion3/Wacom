// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDEventFlow.h"

#include "UI/Battle/BattleHUD.h"

#include "Events/BattleEvent.h"
#include "Session/BattleSession.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattleHUDCombatLogController.h"

namespace
{
	const TCHAR* HUDEventTypeToString(EBattleEventType Type)
	{
		switch (Type)
		{
		case EBattleEventType::BattleStarted:             return TEXT("BattleStarted");
		case EBattleEventType::TurnStarted:               return TEXT("TurnStarted");
		case EBattleEventType::CardsDrawn:                return TEXT("CardsDrawn");
		case EBattleEventType::HandZoneChanged:           return TEXT("HandZoneChanged");
		case EBattleEventType::CardPlayed:                return TEXT("CardPlayed");
		case EBattleEventType::InitiativeHit:             return TEXT("InitiativeHit");
		case EBattleEventType::ResistanceResolved:        return TEXT("ResistanceResolved");
		case EBattleEventType::PerfectReleaseResolved:    return TEXT("PerfectReleaseResolved");
		case EBattleEventType::DamageDealt:               return TEXT("DamageDealt");
		case EBattleEventType::StatusApplied:             return TEXT("StatusApplied");
		case EBattleEventType::InitiativePushed:          return TEXT("InitiativePushed");
		case EBattleEventType::WaitPerformed:             return TEXT("WaitPerformed");
		case EBattleEventType::EnemyPartActed:            return TEXT("EnemyPartActed");
		case EBattleEventType::EnemyPartHpEmptied:        return TEXT("EnemyPartHpEmptied");
		case EBattleEventType::EnemyKnockdown:            return TEXT("EnemyKnockdown");
		case EBattleEventType::KnockdownChoiceRequested:  return TEXT("KnockdownChoiceRequested");
		case EBattleEventType::KnockdownChoiceMade:       return TEXT("KnockdownChoiceMade");
		case EBattleEventType::TurnEnded:                 return TEXT("TurnEnded");
		case EBattleEventType::PassiveTriggered:          return TEXT("PassiveTriggered");
		case EBattleEventType::HandLimitDiscarded:        return TEXT("HandLimitDiscarded");
		case EBattleEventType::CardDiscarded:             return TEXT("CardDiscarded");
		case EBattleEventType::CardExhausted:             return TEXT("CardExhausted");
		case EBattleEventType::CardGained:                return TEXT("CardGained");
		case EBattleEventType::BattleEnded:               return TEXT("BattleEnded");
		default:                                          return TEXT("?");
		}
	}

	void LogRawBattleEvents(const TArray<FBattleEvent>& Events)
	{
		for (const FBattleEvent& Event : Events)
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("[BattleHUD] [#%d] %-22s Amount=%d Count=%d Actor=%s Card=%s Tag=%s"),
				Event.Sequence,
				HUDEventTypeToString(Event.Type),
				Event.Amount,
				Event.Count,
				*Event.ActorInstanceId.ToString(EGuidFormats::Short),
				*Event.CardInstanceId.ToString(EGuidFormats::Short),
				*Event.Tag.ToString());
		}
	}
}

void FWacomBattleHUDEventFlow::ConsumeAndLogEvents(UBattleHUD& HUD)
{
	UBattleSession* Session = HUD.GetSession();
	if (!Session)
	{
		return;
	}

	const TArray<FBattleEvent> Events = Session->ConsumeEvents();
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FWacomBattleCombatLogCommandContext SystemContext =
		UWacomBattleCombatLogBuilder::BuildSystemCommandContext(Snapshot);
	LogRawBattleEvents(Events);
	HUD.StoreFirstPersonCardTransitionEvents(Events);
	HUD.GetCombatLogController().AppendBlock(SystemContext, Events, Snapshot, Snapshot);
	HUD.EnqueueBattlePresentationEvents(Events, INDEX_NONE);
}

void FWacomBattleHUDEventFlow::ConsumeAndLogEvents(
	UBattleHUD& HUD,
	const FWacomBattleCombatLogCommandContext& CommandContext,
	const FBattleSnapshot& PreCommandSnapshot,
	const FBattleSnapshot& PostCommandSnapshot)
{
	UBattleSession* Session = HUD.GetSession();
	if (!Session)
	{
		return;
	}

	const TArray<FBattleEvent> Events = Session->ConsumeEvents();
	LogRawBattleEvents(Events);
	HUD.StoreFirstPersonCardTransitionEvents(Events);
	HUD.GetCombatLogController().AppendBlock(
		CommandContext,
		Events,
		PreCommandSnapshot,
		PostCommandSnapshot);
	const int32 PresentationStackEntryId =
		CommandContext.CommandKind == EWacomBattleCombatLogCommandKind::PlayCard
			? HUD.AppendBattlePresentationStackEntry(CommandContext, PreCommandSnapshot)
			: INDEX_NONE;
	HUD.EnqueueBattlePresentationEvents(Events, PresentationStackEntryId);
}
