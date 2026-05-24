// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDEventFlow.h"

#include "UI/Battle/BattleHUD.h"

#include "Events/BattleEvent.h"
#include "Session/BattleSession.h"
#include "UI/Battle/BattleEventLogPanel.h"
#include "UI/Battle/EventToast.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"
#include "UI/Battle/WacomKnockdownChoiceDialog.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomUITags.h"

#include "CommonActivatableWidget.h"

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
		case EBattleEventType::BattleEnded:               return TEXT("BattleEnded");
		default:                                          return TEXT("?");
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
	for (const FBattleEvent& Event : Events)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[BattleHUD] [#%d] %-22s Amount=%d Count=%d Actor=%s Card=%s Tag=%s"),
			Event.Sequence,
			HUDEventTypeToString(Event.Type),
			Event.Amount,
			Event.Count,
			*Event.ActorInstanceId.ToString(EGuidFormats::Short),
			*Event.CardInstanceId.ToString(EGuidFormats::Short),
			*Event.Tag.ToString());
	}

	if (HUD.EventToast)
	{
		HUD.EventToast->EnqueueEvents(Events);
	}

	AppendBattleEventLogEntries(HUD, Events);

	for (const FBattleEvent& Event : Events)
	{
		if (Event.Type != EBattleEventType::KnockdownChoiceRequested)
		{
			continue;
		}

		UGameInstance* GameInstance = HUD.GetGameInstance();
		UWacomGameUIManagerSubsystem* UIManager =
			GameInstance ? GameInstance->GetSubsystem<UWacomGameUIManagerSubsystem>() : nullptr;
		if (!UIManager)
		{
			continue;
		}

		UCommonActivatableWidget* Pushed = UIManager->PushContentToLayer(
			WacomUITags::UI_Layer_Modal.GetTag(),
			UWacomKnockdownChoiceDialog::StaticClass());
		UWacomKnockdownChoiceDialog* Dialog = Cast<UWacomKnockdownChoiceDialog>(Pushed);
		if (!Dialog)
		{
			continue;
		}

		UBattleSession* CurrentSession = HUD.GetSession();
		if (!CurrentSession)
		{
			continue;
		}

		const FKnockdownChoiceView ChoiceView = CurrentSession->BuildPendingKnockdownChoiceView();
		if (!ChoiceView.bHasPendingChoice)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[BattleHUD] KnockdownChoiceRequested received but no pending choice view"));
			continue;
		}

		Dialog->SetContext(&HUD, ChoiceView);
	}
}

void FWacomBattleHUDEventFlow::AppendBattleEventLogEntries(UBattleHUD& HUD, const TArray<FBattleEvent>& Events)
{
	TArray<FBattleEventPresentationView> VisibleEntries;
	for (const FBattleEvent& Event : Events)
	{
		FBattleEventPresentationView View = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		if (!View.bShouldDisplay)
		{
			continue;
		}

		HUD.BattleEventLogHistory.Add(View);
		VisibleEntries.Add(MoveTemp(View));
	}

	if (VisibleEntries.IsEmpty())
	{
		return;
	}

	TrimBattleEventLogHistory(HUD);
	SyncBattleEventLogPanel(HUD);
}

void FWacomBattleHUDEventFlow::TrimBattleEventLogHistory(UBattleHUD& HUD)
{
	const int32 SafeMaxEntries = FMath::Max(1, HUD.BattleEventLogMaxEntries);
	if (HUD.BattleEventLogHistory.Num() > SafeMaxEntries)
	{
		HUD.BattleEventLogHistory.RemoveAt(0, HUD.BattleEventLogHistory.Num() - SafeMaxEntries);
	}
}

void FWacomBattleHUDEventFlow::SyncBattleEventLogPanel(UBattleHUD& HUD)
{
	if (!HUD.EventLogPanel)
	{
		return;
	}

	HUD.EventLogPanel->MaxEntries = FMath::Max(1, HUD.BattleEventLogMaxEntries);
	HUD.EventLogPanel->SetEventLogEntries(HUD.BattleEventLogHistory);
}
