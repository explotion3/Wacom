// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDCommandFlow.h"

#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattleHUDEventFlow.h"
#include "UI/Battle/WacomBattleHUDTargetingFlow.h"

#include "Commands/BattleCommand.h"
#include "Resolution/BattleTargetValidationResult.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Snapshots/EnemySnapshot.h"
#include "Types/WacomInteractionTargetTypes.h"

namespace
{
	const FEnemyPartSnapshot* FindEnemyPartByInstanceId(const FBattleSnapshot& Snapshot, const FGuid& PartInstanceId)
	{
		for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
		{
			for (const FEnemyPartSnapshot& Part : Enemy.Parts)
			{
				if (Part.InstanceId == PartInstanceId)
				{
					return &Part;
				}
			}
		}
		return nullptr;
	}
}

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
const FEnemyPartSnapshot* TargetPart = TargetPartId.IsValid()
? FindEnemyPartByInstanceId(PreCommandSnapshot, TargetPartId)
: nullptr;
const FWacomBattleCombatLogCommandContext LogContext =
UWacomBattleCombatLogBuilder::BuildPlayCardCommandContext(
PreCommandSnapshot,
CardId,
TargetPart ? TargetPart->Identity : FBattlePartSlotIdentity(),
FGuid());

const FBattleCommand Command = TargetPart
		? FBattleCommand::MakePlayCardOnEnemyPartKey(CardId, TargetPart->PartKey)
		: FBattleCommand::MakePlayCard(CardId);

	const FWacomStatus Status = Session->SubmitCommand(Command);
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] PlayCard failed, code=%d detail=%s"),
			(int32)Status.Code, *Status.Detail.ToString());
		return;
	}

	HUD.RecordFirstPersonPlayCommit(CardId, TargetPart ? TargetPart->Identity : FBattlePartSlotIdentity());
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
FBattlePartSlotIdentity::FromEnemyPartKey(Validation.ResolvedPartKey),
FGuid());

	const FBattleCommand Command =
		FBattleCommand::MakePlayCardOnEnemyPartKey(CardId, Validation.ResolvedPartKey);

	const FWacomStatus Status = Session->SubmitCommand(Command);
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] PlayCardOnWorldTarget failed, code=%d detail=%s"),
			(int32)Status.Code, *Status.Detail.ToString());
		return;
	}

HUD.RecordFirstPersonPlayCommit(CardId, FBattlePartSlotIdentity::FromEnemyPartKey(Validation.ResolvedPartKey));
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
			FBattlePartSlotIdentity(),
			TargetCardId);

	const FWacomStatus Status = Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(CardId, TargetCardId));
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] PlayCardOnHandCard failed, code=%d detail=%s"),
			(int32)Status.Code, *Status.Detail.ToString());
		return;
	}

		HUD.RecordFirstPersonPlayCommit(CardId, FBattlePartSlotIdentity());
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
