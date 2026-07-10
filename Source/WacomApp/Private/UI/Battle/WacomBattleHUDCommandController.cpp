// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDCommandController.h"

#include "Commands/BattleCommand.h"
#include "GameplayTagContainer.h"
#include "Resolution/BattleTargetValidationResult.h"
#include "Runtime/BattlePartSlotIdentity.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Snapshots/EnemySnapshot.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"
#include "UI/Battle/WacomBattleHUDTargetingController.h"

namespace
{
	const FEnemyPartSnapshot* FindEnemyPartByInstanceId(
		const FBattleSnapshot& Snapshot,
		const FGuid& PartInstanceId)
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

	FWacomInteractionTargetHandle BuildWorldTargetHandleFromPart(
		const FEnemyPartSnapshot& Part,
		UObject* SourceObject)
	{
		return FWacomInteractionTargetHandle::ForWorldTarget(
			Part.InstanceId,
			SourceObject,
			FVector::ZeroVector,
			FVector2D::ZeroVector,
			FGameplayTag(),
			NAME_None,
			Part.EncounterId,
			Part.EnemySlotId,
			Part.PartSlotId);
	}
}

FWacomBattleHUDCommandController::FWacomBattleHUDCommandController(
	FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
{
}

void FWacomBattleHUDCommandController::SubmitPlayCard(
	const FGuid& CardId,
	const FGuid& TargetPartId,
	const TOptional<FVector2D>& PresentationTargetWidgetPosition)
{
	Runtime.HideCardDetailPanel();

	UBattleSession* Session = Runtime.GetSession();
	if (!Session || !Runtime.CanSubmitPlayerActionCommand())
	{
		return;
	}

	const FBattleSnapshot PreCommandSnapshot = Session->BuildSnapshot();
	const FEnemyPartSnapshot* TargetPart = TargetPartId.IsValid()
		? FindEnemyPartByInstanceId(PreCommandSnapshot, TargetPartId)
		: nullptr;
	FWacomBattleCombatLogCommandContext LogContext =
		UWacomBattleCombatLogBuilder::BuildPlayCardCommandContext(
			PreCommandSnapshot,
			CardId,
			TargetPart ? TargetPart->Identity : FBattlePartSlotIdentity(),
			FGuid());
	if (TargetPart)
	{
		LogContext.CardTargetPreview =
			Session->BuildCardTargetPreview(CardId, BuildWorldTargetHandleFromPart(*TargetPart, Runtime.Host().AsObject()));
	}

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

	Runtime.RecordFirstPersonPlayCommit(
		CardId,
		TargetPart ? TargetPart->Identity : FBattlePartSlotIdentity(),
		PresentationTargetWidgetPosition);
	Runtime.ClearPendingTargetingCardId();
	Runtime.SetUIState(EBattleUIState::Idle);
	AfterCommand(LogContext, PreCommandSnapshot);
}

void FWacomBattleHUDCommandController::SubmitPlayCardOnWorldTarget(
	const FGuid& CardId,
	const FWacomInteractionTargetHandle& TargetHandle,
	const TOptional<FVector2D>& PresentationTargetWidgetPosition)
{
	Runtime.HideCardDetailPanel();

	UBattleSession* Session = Runtime.GetSession();
	if (!Session || !Runtime.CanSubmitPlayerActionCommand())
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

	const FBattleSnapshot PreCommandSnapshot = Session->BuildSnapshot();
	FWacomBattleCombatLogCommandContext LogContext =
		UWacomBattleCombatLogBuilder::BuildPlayCardCommandContext(
			PreCommandSnapshot,
			CardId,
			FBattlePartSlotIdentity::FromEnemyPartKey(Validation.ResolvedPartKey),
			FGuid());
	LogContext.CardTargetPreview = Session->BuildCardTargetPreview(CardId, TargetHandle);

	const FWacomStatus Status = Session->SubmitCommand(
		FBattleCommand::MakePlayCardOnEnemyPartKey(CardId, Validation.ResolvedPartKey));
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] PlayCardOnWorldTarget failed, code=%d detail=%s"),
			(int32)Status.Code, *Status.Detail.ToString());
		return;
	}

	Runtime.RecordFirstPersonPlayCommit(
		CardId,
		FBattlePartSlotIdentity::FromEnemyPartKey(Validation.ResolvedPartKey),
		PresentationTargetWidgetPosition);
	Runtime.ClearPendingTargetingCardId();
	Runtime.SetUIState(EBattleUIState::Idle);
	AfterCommand(LogContext, PreCommandSnapshot);
}

void FWacomBattleHUDCommandController::SubmitPlayCardOnHandCard(
	const FGuid& CardId,
	const FGuid& TargetCardId,
	const TOptional<FVector2D>& PresentationTargetWidgetPosition)
{
	Runtime.HideCardDetailPanel();

	UBattleSession* Session = Runtime.GetSession();
	if (!Session || !Runtime.CanSubmitPlayerActionCommand())
	{
		return;
	}

	const FBattleSnapshot PreCommandSnapshot = Session->BuildSnapshot();
	FWacomBattleCombatLogCommandContext LogContext =
		UWacomBattleCombatLogBuilder::BuildPlayCardCommandContext(
			PreCommandSnapshot,
			CardId,
			FBattlePartSlotIdentity(),
			TargetCardId);
	LogContext.CardTargetPreview = Session->BuildCardTargetPreview(
		CardId,
		FWacomInteractionTargetHandle::ForCardTarget(TargetCardId, Runtime.Host().AsObject()));

	const FWacomStatus Status = Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(CardId, TargetCardId));
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] PlayCardOnHandCard failed, code=%d detail=%s"),
			(int32)Status.Code, *Status.Detail.ToString());
		return;
	}

	Runtime.RecordFirstPersonPlayCommit(
		CardId,
		FBattlePartSlotIdentity(),
		PresentationTargetWidgetPosition);
	Runtime.ClearPendingTargetingCardId();
	Runtime.SetUIState(EBattleUIState::Idle);
	AfterCommand(LogContext, PreCommandSnapshot);
}

void FWacomBattleHUDCommandController::SubmitWait()
{
	Runtime.HideCardDetailPanel();

	if (Runtime.HasBattlePresentationStackEntries())
	{
		Runtime.QueuePendingTurnBoundaryCommand(EWacomBattleHUDTurnBoundaryCommand::Wait);
		return;
	}

	if (!Runtime.CanSubmitPlayerActionCommand())
	{
		return;
	}

	if (Runtime.GetUIState() == EBattleUIState::TargetSelect)
	{
		Runtime.GetTargetingController().ClearTargetSelection();
	}

	UBattleSession* Session = Runtime.GetSession();
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

	AfterCommand(LogContext, PreCommandSnapshot);
}

void FWacomBattleHUDCommandController::SubmitEndTurn()
{
	Runtime.HideCardDetailPanel();

	if (Runtime.HasBattlePresentationStackEntries())
	{
		Runtime.QueuePendingTurnBoundaryCommand(EWacomBattleHUDTurnBoundaryCommand::EndTurn);
		return;
	}

	if (!Runtime.CanSubmitPlayerActionCommand())
	{
		return;
	}

	if (Runtime.GetUIState() == EBattleUIState::TargetSelect)
	{
		Runtime.GetTargetingController().ClearTargetSelection();
	}

	UBattleSession* Session = Runtime.GetSession();
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

	AfterCommand(LogContext, PreCommandSnapshot);
}

void FWacomBattleHUDCommandController::SubmitKnockdownChoice(
	EKnockdownChoice Choice)
{
	Runtime.HideCardDetailPanel();

	if (Choice == EKnockdownChoice::None)
	{
		return;
	}

	UBattleSession* Session = Runtime.GetSession();
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

	AfterCommand(LogContext, PreCommandSnapshot);
}

void FWacomBattleHUDCommandController::AfterCommand()
{
	UBattleSession* Session = Runtime.GetSession();
	if (!Session)
	{
		return;
	}

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FWacomBattleCombatLogCommandContext SystemContext =
		UWacomBattleCombatLogBuilder::BuildSystemCommandContext(Snapshot);
	AfterCommand(SystemContext, Snapshot);
}

void FWacomBattleHUDCommandController::AfterCommand(
	const FWacomBattleCombatLogCommandContext& LogContext,
	const FBattleSnapshot& PreCommandSnapshot)
{
	Runtime.HideCardDetailPanel();

	UBattleSession* Session = Runtime.GetSession();
	if (!Session)
	{
		return;
	}

	const FBattleSnapshot PostCommandSnapshot = Session->BuildSnapshot();
	const bool bPresentationHandled =
		Runtime.ConsumeAndLogEvents(
			LogContext,
			PreCommandSnapshot,
			PostCommandSnapshot);
	if (bPresentationHandled)
	{
		return;
	}
	Runtime.NativeRefreshFromSnapshot(PostCommandSnapshot);
}
