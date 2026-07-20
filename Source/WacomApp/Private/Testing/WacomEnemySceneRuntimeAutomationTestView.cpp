// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomEnemySceneRuntimeAutomationTestView.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Actors/WacomBattleEnemyActor.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemySceneRuntimeComponent.h"
#include "Snapshots/BattleSnapshot.h"

void FWacomEnemySceneRuntimeAutomationTestView::InitializeBinding(
	AWacomBattleEnemyActor& Host,
	FName EncounterId,
	FName EnemySlotId)
{
	if (UWacomBattleEnemySceneRuntimeComponent* Runtime = Host.GetEnemySceneRuntimeComponent())
	{
		Runtime->InitializeRuntimeSceneBinding(EncounterId, EnemySlotId);
	}
}

bool FWacomEnemySceneRuntimeAutomationTestView::SyncPart(
	AWacomBattleEnemyActor& Host,
	UWacomBattleEnemyPartComponent& Part,
	const FBattleSnapshot& Snapshot)
{
	UWacomBattleEnemySceneRuntimeComponent* Runtime = Host.GetEnemySceneRuntimeComponent();
	if (!Runtime)
	{
		return false;
	}
	const FEnemyPartSnapshot* Match = nullptr;
	for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
	{
		if (Enemy.EncounterId != Snapshot.EncounterId
			|| Enemy.EnemySlotId != Host.GetEffectiveEnemySlotId())
		{
			continue;
		}
		Match = Enemy.Parts.FindByPredicate([&Part](const FEnemyPartSnapshot& Candidate)
		{
			return Candidate.PartSlotId == Part.PartSlotId;
		});
		break;
	}
	return Runtime->ApplyPartSnapshotFacts(Part, Match, false, TEXT("Automation"));
}

void FWacomEnemySceneRuntimeAutomationTestView::SetRegisteredAndTargetable(
	AWacomBattleEnemyActor& Host,
	UWacomBattleEnemyPartComponent& Part,
	bool bRegistered,
	bool bTargetable)
{
	if (UWacomBattleEnemySceneRuntimeComponent* Runtime = Host.GetEnemySceneRuntimeComponent())
	{
		Runtime->SetPartRegisteredWithHUD(Part, bRegistered);
		Runtime->SetPartTargetable(Part, bTargetable, NAME_None);
	}
}

void FWacomEnemySceneRuntimeAutomationTestView::PlayAction(
	AWacomBattleEnemyActor& Host,
	UWacomBattleEnemyPartComponent& Part,
	FName IntentId,
	FWacomBattleEnemyActionPlaybackCallbacks&& Callbacks)
{
	if (UWacomBattleEnemySceneRuntimeComponent* Runtime = Host.GetEnemySceneRuntimeComponent())
	{
		Runtime->PlayPartActionAnimation(Part, IntentId, MoveTemp(Callbacks));
		return;
	}
	Callbacks.CompleteImmediately();
}

void FWacomEnemySceneRuntimeAutomationTestView::CancelAction(
	AWacomBattleEnemyActor& Host,
	UWacomBattleEnemyPartComponent& Part)
{
	if (UWacomBattleEnemySceneRuntimeComponent* Runtime = Host.GetEnemySceneRuntimeComponent())
	{
		Runtime->CancelPartActionAnimation(Part);
	}
}

void FWacomEnemySceneRuntimeAutomationTestView::SetHoverPrediction(
	AWacomBattleEnemyActor& Host,
	UWacomBattleEnemyPartComponent& Part,
	const FWacomBattleEnemyPartDragPredictionDebugInput& PredictionInput)
{
	if (UWacomBattleEnemySceneRuntimeComponent* Runtime = Host.GetEnemySceneRuntimeComponent())
	{
		Runtime->SetPartHoverProbeState(
			Part,
			Part.BuildWorldTargetHandle(),
			TEXT("AutomationHover"),
			PredictionInput);
	}
}

void FWacomEnemySceneRuntimeAutomationTestView::ClearHoverPrediction(
	AWacomBattleEnemyActor& Host,
	UWacomBattleEnemyPartComponent& Part)
{
	if (UWacomBattleEnemySceneRuntimeComponent* Runtime = Host.GetEnemySceneRuntimeComponent())
	{
		Runtime->ClearPartHoverProbeState(Part, TEXT("AutomationClear"));
	}
}

void FWacomEnemySceneRuntimeAutomationTestView::SetDragTargetPreview(
	AWacomBattleEnemyActor& Host,
	UWacomBattleEnemyPartComponent& Part,
	EWacomFirstPersonCardDragTargetFeedbackState PreviewState)
{
	if (UWacomBattleEnemySceneRuntimeComponent* Runtime = Host.GetEnemySceneRuntimeComponent())
	{
		Runtime->SetPartDragTargetPreviewState(
			Part,
			PreviewState,
			FWacomBattleEnemyPartDragPredictionDebugInput());
	}
}

void FWacomEnemySceneRuntimeAutomationTestView::ClearDragTargetPreview(
	AWacomBattleEnemyActor& Host,
	UWacomBattleEnemyPartComponent& Part)
{
	if (UWacomBattleEnemySceneRuntimeComponent* Runtime = Host.GetEnemySceneRuntimeComponent())
	{
		Runtime->ClearPartDragTargetPreviewState(Part);
	}
}

FName FWacomEnemySceneRuntimeAutomationTestView::GetDesiredTargetPreviewKind(
	const UWacomBattleEnemyPartComponent& Part)
{
	return Part.GetRuntimeDebugView().TargetPreviewKind;
}

#endif
