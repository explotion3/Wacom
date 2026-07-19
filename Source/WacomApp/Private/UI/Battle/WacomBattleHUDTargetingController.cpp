// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDTargetingController.h"

#include "Enemies/EnemyPartDefinition.h"
#include "GameplayTagContainer.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Snapshots/EnemySnapshot.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"

FWacomBattleHUDTargetingController::FWacomBattleHUDTargetingController(
	FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
{
}

void FWacomBattleHUDTargetingController::HandleEnemyPartClicked(
	const FWacomInteractionTargetHandle& TargetHandle)
{
	Runtime.HideCardDetailPanel();

	if (Runtime.GetUIState() != EBattleUIState::TargetSelect
		|| !Runtime.GetPendingTargetingCardId().IsValid())
	{
		return;
	}
	if (!Runtime.CanSubmitPlayerActionCommand())
	{
		return;
	}

	Runtime.SubmitPlayCardOnWorldTarget(Runtime.GetPendingTargetingCardId(), TargetHandle);
}

void FWacomBattleHUDTargetingController::CancelTargetSelect()
{
	Runtime.HideCardDetailPanel();

	if (Runtime.GetUIState() != EBattleUIState::TargetSelect)
	{
		return;
	}

	Runtime.ClearPendingTargetingCardId();
	Runtime.SetUIState(EBattleUIState::Idle);
}

FBattleTargetSelectionView FWacomBattleHUDTargetingController::BuildTargetSelectionView() const
{
	const UBattleSession* Session = Runtime.GetSession();
	return Session
		? BuildTargetSelectionView(Session->BuildSnapshot())
		: FBattleTargetSelectionView();
}

FBattleTargetSelectionView FWacomBattleHUDTargetingController::BuildTargetSelectionView(
	const FBattleSnapshot& Snapshot) const
{
	FBattleTargetSelectionView View;
	View.bIsTargetSelecting =
		Runtime.GetUIState() == EBattleUIState::TargetSelect
		&& Runtime.GetPendingTargetingCardId().IsValid();
	View.PendingCardInstanceId = View.bIsTargetSelecting ? Runtime.GetPendingTargetingCardId() : FGuid();

	const UBattleSession* Session = Runtime.GetSession();
	if (!Session)
	{
		return View;
	}
	int32 TargetablePartCapacity = 0;
	for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
	{
		TargetablePartCapacity += Enemy.Parts.Num();
	}
	View.TargetableParts.Reserve(TargetablePartCapacity);
	for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
	{
		for (const FEnemyPartSnapshot& Part : Enemy.Parts)
		{
			FBattleTargetablePartView PartView;
			PartView.PartInstanceId = Part.InstanceId;
			if (Part.Definition)
			{
				PartView.PartId = Part.Definition->PartId;
				PartView.PartName = Part.Definition->DisplayName.IsEmpty()
					? FText::FromName(Part.Definition->PartId)
					: Part.Definition->DisplayName;
			}

			if (!View.bIsTargetSelecting)
			{
				PartView.bTargetable = false;
				PartView.DisabledReason = FName(TEXT("NotTargetSelecting"));
			}
			else if (Part.bDestroyed)
			{
				PartView.bTargetable = false;
				PartView.DisabledReason = FName(TEXT("PartDestroyed"));
			}
			else
			{
				const FWacomInteractionTargetHandle Handle = FWacomInteractionTargetHandle::ForWorldTarget(
					Part.InstanceId,
					nullptr,
					FVector::ZeroVector,
					FVector2D::ZeroVector,
					FGameplayTag(),
					Part.Definition ? Part.Definition->PartId : NAME_None,
					Part.EncounterId,
					Part.EnemySlotId,
					Part.PartSlotId);
				if (Session->ValidateTargetWithCard(View.PendingCardInstanceId, Handle).bCanTarget)
				{
					PartView.bTargetable = true;
					PartView.DisabledReason = NAME_None;
				}
				else
				{
					PartView.bTargetable = false;
					PartView.DisabledReason = FName(TEXT("NotValidTargetForCard"));
				}
			}

			View.TargetableParts.Add(PartView);
		}
	}

	return View;
}

void FWacomBattleHUDTargetingController::ClearTargetSelection()
{
	if (Runtime.GetUIState() == EBattleUIState::TargetSelect
		|| Runtime.GetPendingTargetingCardId().IsValid())
	{
		Runtime.ClearPendingTargetingCardId();
		Runtime.SetUIState(EBattleUIState::Idle);
	}
}
