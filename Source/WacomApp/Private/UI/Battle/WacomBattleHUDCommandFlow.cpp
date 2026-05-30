// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDCommandFlow.h"

#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleHUDEventFlow.h"
#include "UI/Battle/WacomBattleHUDTargetingFlow.h"

#include "Commands/BattleCommand.h"
#include "Session/BattleSession.h"

void FWacomBattleHUDCommandFlow::SubmitPlayCard(UBattleHUD& HUD, const FGuid& CardId, const FGuid& TargetPartId)
{
	HUD.HideCardDetailPanel();

	UBattleSession* Session = HUD.GetSession();
	if (!Session)
	{
		return;
	}
	if (HUD.IsBattlePresentationQueueBusy())
	{
		return;
	}

	const FWacomStatus Status = Session->SubmitCommand(FBattleCommand::MakePlayCard(CardId, TargetPartId));
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] PlayCard failed, code=%d detail=%s"),
			(int32)Status.Code, *Status.Detail.ToString());
		return;
	}

	HUD.RecordFirstPersonPlayCommit(CardId, TargetPartId);
	HUD.PendingTargetingCardId.Invalidate();
	HUD.SetUIState(EBattleUIState::Idle);
	AfterCommand(HUD);
}

void FWacomBattleHUDCommandFlow::SubmitPlayCardOnHandCard(
	UBattleHUD& HUD,
	const FGuid& CardId,
	const FGuid& TargetCardId)
{
	HUD.HideCardDetailPanel();

	UBattleSession* Session = HUD.GetSession();
	if (!Session)
	{
		return;
	}
	if (HUD.IsBattlePresentationQueueBusy())
	{
		return;
	}

	const FWacomStatus Status = Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(CardId, TargetCardId));
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] PlayCardOnHandCard failed, code=%d detail=%s"),
			(int32)Status.Code, *Status.Detail.ToString());
		return;
	}

	HUD.RecordFirstPersonPlayCommit(CardId, FGuid());
	HUD.PendingTargetingCardId.Invalidate();
	HUD.SetUIState(EBattleUIState::Idle);
	AfterCommand(HUD);
}

void FWacomBattleHUDCommandFlow::SubmitWait(UBattleHUD& HUD)
{
	HUD.HideCardDetailPanel();

	if (HUD.UIState == EBattleUIState::BattleEnd
		|| HUD.UIState == EBattleUIState::Resolving
		|| HUD.IsBattlePresentationQueueBusy())
	{
		return;
	}

	if (HUD.UIState == EBattleUIState::TargetSelect)
	{
		FWacomBattleHUDTargetingFlow::ClearTargetSelection(HUD);
	}

	UBattleSession* Session = HUD.GetSession();
	if (!Session)
	{
		return;
	}

	const FWacomStatus Status = Session->SubmitCommand(FBattleCommand::MakeWait());
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] Wait failed, code=%d"), (int32)Status.Code);
		return;
	}

	AfterCommand(HUD);
}

void FWacomBattleHUDCommandFlow::SubmitEndTurn(UBattleHUD& HUD)
{
	HUD.HideCardDetailPanel();

	if (HUD.UIState == EBattleUIState::BattleEnd
		|| HUD.UIState == EBattleUIState::Resolving
		|| HUD.IsBattlePresentationQueueBusy())
	{
		return;
	}

	if (HUD.UIState == EBattleUIState::TargetSelect)
	{
		FWacomBattleHUDTargetingFlow::ClearTargetSelection(HUD);
	}

	UBattleSession* Session = HUD.GetSession();
	if (!Session)
	{
		return;
	}
	if (HUD.IsBattlePresentationQueueBusy())
	{
		return;
	}

	const FWacomStatus Status = Session->SubmitCommand(FBattleCommand::MakeEndTurn());
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] EndTurn failed, code=%d"), (int32)Status.Code);
		return;
	}

	AfterCommand(HUD);
}

void FWacomBattleHUDCommandFlow::SubmitKnockdownChoice(UBattleHUD& HUD, EKnockdownChoice Choice)
{
	HUD.HideCardDetailPanel();

	if (Choice == EKnockdownChoice::None)
	{
		return;
	}

	UBattleSession* Session = HUD.GetSession();
	if (!Session)
	{
		return;
	}

	const FWacomStatus Status = Session->SubmitCommand(FBattleCommand::MakeKnockdownChoice(Choice));
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] KnockdownChoice failed, code=%d detail=%s"),
			(int32)Status.Code, *Status.Detail.ToString());
		return;
	}

	AfterCommand(HUD);
}

void FWacomBattleHUDCommandFlow::AfterCommand(UBattleHUD& HUD)
{
	HUD.HideCardDetailPanel();
	FWacomBattleHUDEventFlow::ConsumeAndLogEvents(HUD);

	UBattleSession* Session = HUD.GetSession();
	if (!Session)
	{
		return;
	}

	HUD.RefreshFromSnapshot(Session->BuildSnapshot());
}
