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
#include "UI/Battle/WacomBattleHUDResultApplicator.h"
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

	const FBattleResolution Resolution = Session->ResolveCommand(Command);
	FWacomBattleCommandPresentationContext PresentationContext;
	PresentationContext.SourceSession = Session;
	PresentationContext.CombatLogContext = MoveTemp(LogContext);
	PresentationContext.PreCommandSnapshot = PreCommandSnapshot;
	PresentationContext.PlayCardCommit.Emplace(FWacomBattlePlayCardCommitPresentation{
		CardId,
		TargetPart ? TargetPart->Identity : FBattlePartSlotIdentity(),
		PresentationTargetWidgetPosition});
	Runtime.GetResultApplicator().ApplyCommandResolution(PresentationContext, Resolution);
}

void FWacomBattleHUDCommandController::SubmitPlayCardOnWorldTarget(
	const FGuid& CardId,
	const FWacomInteractionTargetHandle& TargetHandle,
	const TOptional<FVector2D>& PresentationTargetWidgetPosition)
{
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

	const FBattleResolution Resolution = Session->ResolveCommand(
		FBattleCommand::MakePlayCardOnEnemyPartKey(CardId, Validation.ResolvedPartKey));
	FWacomBattleCommandPresentationContext PresentationContext;
	PresentationContext.SourceSession = Session;
	PresentationContext.CombatLogContext = MoveTemp(LogContext);
	PresentationContext.PreCommandSnapshot = PreCommandSnapshot;
	PresentationContext.PlayCardCommit.Emplace(FWacomBattlePlayCardCommitPresentation{
		CardId,
		FBattlePartSlotIdentity::FromEnemyPartKey(Validation.ResolvedPartKey),
		PresentationTargetWidgetPosition});
	Runtime.GetResultApplicator().ApplyCommandResolution(PresentationContext, Resolution);
}

void FWacomBattleHUDCommandController::SubmitPlayCardOnHandCard(
	const FGuid& CardId,
	const FGuid& TargetCardId,
	const TOptional<FVector2D>& PresentationTargetWidgetPosition)
{
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

	const FBattleResolution Resolution = Session->ResolveCommand(
		FBattleCommand::MakePlayCardOnHandCard(CardId, TargetCardId));
	FWacomBattleCommandPresentationContext PresentationContext;
	PresentationContext.SourceSession = Session;
	PresentationContext.CombatLogContext = MoveTemp(LogContext);
	PresentationContext.PreCommandSnapshot = PreCommandSnapshot;
	PresentationContext.PlayCardCommit.Emplace(FWacomBattlePlayCardCommitPresentation{
		CardId,
		FBattlePartSlotIdentity(),
		PresentationTargetWidgetPosition});
	Runtime.GetResultApplicator().ApplyCommandResolution(PresentationContext, Resolution);
}

void FWacomBattleHUDCommandController::SubmitWait()
{
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

	const FBattleResolution Resolution = Session->ResolveCommand(FBattleCommand::MakeWait());
	FWacomBattleCommandPresentationContext PresentationContext;
	PresentationContext.SourceSession = Session;
	PresentationContext.CombatLogContext = LogContext;
	PresentationContext.PreCommandSnapshot = PreCommandSnapshot;
	Runtime.GetResultApplicator().ApplyCommandResolution(PresentationContext, Resolution);
}

void FWacomBattleHUDCommandController::SubmitEndTurn()
{
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

	const FBattleResolution Resolution = Session->ResolveCommand(FBattleCommand::MakeEndTurn());
	FWacomBattleCommandPresentationContext PresentationContext;
	PresentationContext.SourceSession = Session;
	PresentationContext.CombatLogContext = LogContext;
	PresentationContext.PreCommandSnapshot = PreCommandSnapshot;
	Runtime.GetResultApplicator().ApplyCommandResolution(PresentationContext, Resolution);
}

bool FWacomBattleHUDCommandController::TrySubmitKnockdownChoice(
	EKnockdownChoice Choice)
{
	if (Choice == EKnockdownChoice::None)
	{
		return false;
	}

	UBattleSession* Session = Runtime.GetSession();
	if (!Session)
	{
		return false;
	}

	const FBattleSnapshot PreCommandSnapshot = Session->BuildSnapshot();
	const FWacomBattleCombatLogCommandContext LogContext =
		UWacomBattleCombatLogBuilder::BuildKnockdownChoiceCommandContext(PreCommandSnapshot, Choice);

	const FBattleResolution Resolution = Session->ResolveCommand(FBattleCommand::MakeKnockdownChoice(Choice));
	FWacomBattleCommandPresentationContext PresentationContext;
	PresentationContext.SourceSession = Session;
	PresentationContext.CombatLogContext = LogContext;
	PresentationContext.PreCommandSnapshot = PreCommandSnapshot;
	Runtime.GetResultApplicator().ApplyCommandResolution(PresentationContext, Resolution);
	return Resolution.IsOk();
}
