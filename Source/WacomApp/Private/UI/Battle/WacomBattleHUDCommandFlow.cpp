// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDCommandFlow.h"

#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattleHUDEventFlow.h"
#include "UI/Battle/WacomBattleHUDTargetingFlow.h"

#include "Commands/BattleCommand.h"
#include "Resolution/BattleTargetValidationResult.h"
#include "Session/BattleSession.h"
#include "Types/WacomInteractionTargetTypes.h"

void FWacomBattleHUDCommandFlow::SubmitPlayCard(UBattleHUD& HUD, const FGuid& CardId, const FGuid& TargetPartId)
{
	HUD.HideCardDetailPanel();

	UBattleSession* Session = HUD.GetSession();
	if (!Session)
	{
		return;
	}
	if (!HUD.CanSubmitPlayerActionCommand())
	{
		return;
	}

	const FBattleSnapshot PreCommandSnapshot = Session->BuildSnapshot();
	const FWacomBattleCombatLogCommandContext LogContext =
		UWacomBattleCombatLogBuilder::BuildPlayCardCommandContext(
			PreCommandSnapshot,
			CardId,
			TargetPartId,
			FGuid());

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
	AfterCommand(HUD, LogContext, PreCommandSnapshot);
}

void FWacomBattleHUDCommandFlow::SubmitPlayCardOnWorldTarget(
	UBattleHUD& HUD,
	const FGuid& CardId,
	const FWacomInteractionTargetHandle& TargetHandle)
{
	HUD.HideCardDetailPanel();

	UBattleSession* Session = HUD.GetSession();
	if (!Session)
	{
		return;
	}
	if (!HUD.CanSubmitPlayerActionCommand())
	{
		return;
	}

	const FWacomBattleTargetValidationResult Validation =
		Session->ValidateTargetWithCard(CardId, TargetHandle);
	if (!Validation.bCanTarget)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleHUD] PlayCard world target rejected by validation: %s"),
			*Validation.DebugSummary);
		return;
	}

	const FGuid ResolvedPartId = Validation.ResolvedPartInstanceId.IsValid()
		? Validation.ResolvedPartInstanceId
		: TargetHandle.WorldTargetId;
	const FBattleSnapshot PreCommandSnapshot = Session->BuildSnapshot();
	const FWacomBattleCombatLogCommandContext LogContext =
		UWacomBattleCombatLogBuilder::BuildPlayCardCommandContext(
			PreCommandSnapshot,
			CardId,
			ResolvedPartId,
			FGuid());

	FBattleCommand Command = FBattleCommand::MakePlayCard(CardId, TargetHandle.WorldTargetId);
	Command.TargetEnemySlotId = TargetHandle.EnemySlotId;
	Command.TargetPartSlotId = TargetHandle.PartSlotId;

	const FWacomStatus Status = Session->SubmitCommand(Command);
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] PlayCardOnWorldTarget failed, code=%d detail=%s"),
			(int32)Status.Code, *Status.Detail.ToString());
		return;
	}

	HUD.RecordFirstPersonPlayCommit(CardId, ResolvedPartId);
	HUD.PendingTargetingCardId.Invalidate();
	HUD.SetUIState(EBattleUIState::Idle);
	AfterCommand(HUD, LogContext, PreCommandSnapshot);
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
	if (!HUD.CanSubmitPlayerActionCommand())
	{
		return;
	}

	const FBattleSnapshot PreCommandSnapshot = Session->BuildSnapshot();
	const FWacomBattleCombatLogCommandContext LogContext =
		UWacomBattleCombatLogBuilder::BuildPlayCardCommandContext(
			PreCommandSnapshot,
			CardId,
			FGuid(),
			TargetCardId);

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
	AfterCommand(HUD, LogContext, PreCommandSnapshot);
}

void FWacomBattleHUDCommandFlow::SubmitWait(UBattleHUD& HUD)
{
	HUD.HideCardDetailPanel();

	if (HUD.HasBattlePresentationStackEntries())
	{
		HUD.QueuePendingTurnBoundaryCommand(UBattleHUD::ETurnBoundaryCommand::Wait);
		return;
	}

	if (!HUD.CanSubmitPlayerActionCommand())
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

	const FBattleSnapshot PreCommandSnapshot = Session->BuildSnapshot();
	const FWacomBattleCombatLogCommandContext LogContext =
		UWacomBattleCombatLogBuilder::BuildWaitCommandContext(PreCommandSnapshot);

	const FWacomStatus Status = Session->SubmitCommand(FBattleCommand::MakeWait());
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] Wait failed, code=%d"), (int32)Status.Code);
		return;
	}

	AfterCommand(HUD, LogContext, PreCommandSnapshot);
}

void FWacomBattleHUDCommandFlow::SubmitEndTurn(UBattleHUD& HUD)
{
	HUD.HideCardDetailPanel();

	if (HUD.HasBattlePresentationStackEntries())
	{
		HUD.QueuePendingTurnBoundaryCommand(UBattleHUD::ETurnBoundaryCommand::EndTurn);
		return;
	}

	if (!HUD.CanSubmitPlayerActionCommand())
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

	const FBattleSnapshot PreCommandSnapshot = Session->BuildSnapshot();
	const FWacomBattleCombatLogCommandContext LogContext =
		UWacomBattleCombatLogBuilder::BuildEndTurnCommandContext(PreCommandSnapshot);

	const FWacomStatus Status = Session->SubmitCommand(FBattleCommand::MakeEndTurn());
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] EndTurn failed, code=%d"), (int32)Status.Code);
		return;
	}

	AfterCommand(HUD, LogContext, PreCommandSnapshot);
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

	const FBattleSnapshot PreCommandSnapshot = Session->BuildSnapshot();
	const FWacomBattleCombatLogCommandContext LogContext =
		UWacomBattleCombatLogBuilder::BuildKnockdownChoiceCommandContext(PreCommandSnapshot, Choice);

	const FWacomStatus Status = Session->SubmitCommand(FBattleCommand::MakeKnockdownChoice(Choice));
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] KnockdownChoice failed, code=%d detail=%s"),
			(int32)Status.Code, *Status.Detail.ToString());
		return;
	}

	AfterCommand(HUD, LogContext, PreCommandSnapshot);
}

void FWacomBattleHUDCommandFlow::AfterCommand(UBattleHUD& HUD)
{
	UBattleSession* Session = HUD.GetSession();
	if (!Session)
	{
		return;
	}

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FWacomBattleCombatLogCommandContext SystemContext =
		UWacomBattleCombatLogBuilder::BuildSystemCommandContext(Snapshot);
	AfterCommand(HUD, SystemContext, Snapshot);
}

void FWacomBattleHUDCommandFlow::AfterCommand(
	UBattleHUD& HUD,
	const FWacomBattleCombatLogCommandContext& LogContext,
	const FBattleSnapshot& PreCommandSnapshot)
{
	HUD.HideCardDetailPanel();

	UBattleSession* Session = HUD.GetSession();
	if (!Session)
	{
		return;
	}

	const FBattleSnapshot PostCommandSnapshot = Session->BuildSnapshot();
	FWacomBattleHUDEventFlow::ConsumeAndLogEvents(
		HUD,
		LogContext,
		PreCommandSnapshot,
		PostCommandSnapshot);
	HUD.RefreshFromSnapshot(PostCommandSnapshot);
}
