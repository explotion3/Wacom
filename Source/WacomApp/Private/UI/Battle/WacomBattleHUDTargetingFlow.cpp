// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDTargetingFlow.h"

#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleHUDCommandFlow.h"

#include "Cards/CardDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Snapshots/EnemySnapshot.h"
#include "Types/WacomEnums.h"
#include "Types/WacomInteractionTargetTypes.h"

void FWacomBattleHUDTargetingFlow::HandleCardClicked(UBattleHUD& HUD, const FGuid& CardInstanceId)
{
	HUD.HideCardDetailPanel();

	if (!HUD.CanSubmitPlayerActionCommand())
	{
		return;
	}

	UBattleSession* Session = HUD.GetSession();
	if (!Session)
	{
		return;
	}

	if (HUD.UIState == EBattleUIState::TargetSelect && CardInstanceId == HUD.PendingTargetingCardId)
	{
		CancelTargetSelect(HUD);
		return;
	}

	const FBattleSnapshot Snap = Session->BuildSnapshot();
	const FHandCardSnapshot* Card = nullptr;
	for (const FHandCardSnapshot& Candidate : Snap.Hand.Cards)
	{
		if (Candidate.InstanceId == CardInstanceId)
		{
			Card = &Candidate;
			break;
		}
	}

	if (!Card || !Card->Definition || !Card->bIsPlayable)
	{
		return;
	}

	switch (Card->Definition->TargetMode)
	{
	case ECardTargetMode::None:
	case ECardTargetMode::Self:
	case ECardTargetMode::AllEnemyParts:
		FWacomBattleHUDCommandFlow::SubmitPlayCard(HUD, CardInstanceId, FGuid());
		break;

	case ECardTargetMode::SingleEnemyPart:
		HUD.PendingTargetingCardId = CardInstanceId;
		HUD.SetUIState(EBattleUIState::TargetSelect);
		break;

	case ECardTargetMode::HandCard:
	default:
		break;
	}
}

void FWacomBattleHUDTargetingFlow::HandleEnemyPartClicked(
	UBattleHUD& HUD,
	const FWacomInteractionTargetHandle& TargetHandle)
{
	HUD.HideCardDetailPanel();

	if (HUD.UIState != EBattleUIState::TargetSelect || !HUD.PendingTargetingCardId.IsValid())
	{
		return;
	}
	if (!HUD.CanSubmitPlayerActionCommand())
	{
		return;
	}

	const FGuid CardId = HUD.PendingTargetingCardId;

	FWacomBattleHUDCommandFlow::SubmitPlayCardOnWorldTarget(HUD, CardId, TargetHandle);
}

void FWacomBattleHUDTargetingFlow::CancelTargetSelect(UBattleHUD& HUD)
{
	HUD.HideCardDetailPanel();

	if (HUD.UIState != EBattleUIState::TargetSelect)
	{
		return;
	}

	HUD.PendingTargetingCardId.Invalidate();
	HUD.SetUIState(EBattleUIState::Idle);
}

FBattleTargetSelectionView FWacomBattleHUDTargetingFlow::BuildTargetSelectionView(const UBattleHUD& HUD)
{
	FBattleTargetSelectionView View;
	View.bIsTargetSelecting = HUD.UIState == EBattleUIState::TargetSelect && HUD.PendingTargetingCardId.IsValid();
	View.PendingCardInstanceId = View.bIsTargetSelecting ? HUD.PendingTargetingCardId : FGuid();

	const UBattleSession* Session = HUD.GetSession();
	if (!Session)
	{
		return View;
	}

	const FBattleSnapshot Snap = Session->BuildSnapshot();
	int32 TargetablePartCapacity = 0;
	for (const FEnemySnapshot& Enemy : Snap.Enemies)
	{
		TargetablePartCapacity += Enemy.Parts.Num();
	}
	View.TargetableParts.Reserve(TargetablePartCapacity);
	for (const FEnemySnapshot& Enemy : Snap.Enemies)
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

void FWacomBattleHUDTargetingFlow::ClearTargetSelection(UBattleHUD& HUD)
{
	if (HUD.UIState == EBattleUIState::TargetSelect || HUD.PendingTargetingCardId.IsValid())
	{
		HUD.PendingTargetingCardId.Invalidate();
		HUD.SetUIState(EBattleUIState::Idle);
	}
}
