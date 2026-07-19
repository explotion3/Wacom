// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomEnemySceneRuntimeAutomationTestView.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Actors/WacomBattleEnemyActor.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemySceneRuntimeComponent.h"

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
	return Runtime && Runtime->SyncPartFromBattleSnapshot(Part, Snapshot);
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

#endif
