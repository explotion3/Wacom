// Copyright Wacom. All Rights Reserved.

#include "Commands/KnockdownChoiceAvailability.h"

#include "Cards/CardDefinition.h"
#include "Core/BattleState.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Runtime/RuntimeEnemyPart.h"

namespace
{
	void PopulateRewardSummary(
		const UEnemyPartDefinition& PartDefinition,
		EKnockdownChoice Choice,
		FKnockdownChoiceOptionView& OutOption)
	{
		const UCardDefinition* RewardCard =
			PartDefinition.ResolveKnockdownRewardCard(Choice);
		if (!RewardCard)
		{
			return;
		}

		OutOption.bHasRewardCard = true;
		OutOption.RewardCardId = RewardCard->CardId;
		OutOption.RewardCardName = RewardCard->DisplayName.IsEmpty()
			? FText::FromName(RewardCard->CardId)
			: RewardCard->DisplayName;
	}
}

const FName FKnockdownChoiceAvailability::Reason_None(TEXT("None"));
const FName FKnockdownChoiceAvailability::Reason_NoLivingEnemyPart(TEXT("NoLivingEnemyPart"));
const FName FKnockdownChoiceAvailability::Reason_LeftHandMissing(TEXT("LeftHandMissing"));
const FName FKnockdownChoiceAvailability::Reason_RightHandMissing(TEXT("RightHandMissing"));

bool FKnockdownChoiceAvailability::HasAnyLivingEnemyPart(const FBattleState& State)
{
	for (const FRuntimeEnemyPart& Part : State.Enemy.Parts)
	{
		if (!Part.bDestroyed && Part.CurrentHp > 0)
		{
			return true;
		}
	}
	return false;
}

FKnockdownChoiceView FKnockdownChoiceAvailability::BuildView(const FBattleState& State)
{
	FKnockdownChoiceView View;

	View.AidOption.Choice = EKnockdownChoice::Aid;
	View.WithdrawOption.Choice = EKnockdownChoice::Withdraw;
	View.DestroyOption.Choice = EKnockdownChoice::Destroy;

	View.AidOption.DisabledReason = Reason_None;
	View.WithdrawOption.DisabledReason = Reason_None;
	View.DestroyOption.DisabledReason = Reason_None;

	if (State.PendingKnockdownEvents.Num() <= 0)
	{
		return View;
	}

	const FBattleState::FPendingKnockdownEvent& Event = State.PendingKnockdownEvents[0];
	View.bHasPendingChoice = true;
	View.PartInstanceId = Event.PartInstanceId;
	View.PartId = Event.PartId;
	View.Identity = Event.Identity;

	for (const FRuntimeEnemyPart& Part : State.Enemy.Parts)
	{
		if (Part.InstanceId != Event.PartInstanceId || !Part.Definition)
		{
			continue;
		}

		View.PartName = Part.Definition->DisplayName.IsEmpty()
			? FText::FromName(Part.Definition->PartId)
			: Part.Definition->DisplayName;
		PopulateRewardSummary(
			*Part.Definition,
			EKnockdownChoice::Aid,
			View.AidOption);
		PopulateRewardSummary(
			*Part.Definition,
			EKnockdownChoice::Destroy,
			View.DestroyOption);
		break;
	}

	if (View.PartName.IsEmpty())
	{
		View.PartName = Event.PartId.IsNone()
			? NSLOCTEXT("WacomBattle", "Knockdown.UnknownPart", "敌方部位")
			: FText::FromName(Event.PartId);
	}

	View.AidOption.bAvailable = Event.bLeftHandAvailable;
	if (!View.AidOption.bAvailable)
	{
		View.AidOption.DisabledReason = Reason_LeftHandMissing;
	}

	const bool bWithdrawAvailable = HasAnyLivingEnemyPart(State);
	View.WithdrawOption.bAvailable = bWithdrawAvailable;
	if (!View.WithdrawOption.bAvailable)
	{
		View.WithdrawOption.DisabledReason = Reason_NoLivingEnemyPart;
	}

	View.DestroyOption.bAvailable = Event.bRightHandAvailable;
	if (!View.DestroyOption.bAvailable)
	{
		View.DestroyOption.DisabledReason = Reason_RightHandMissing;
	}

	return View;
}

int32 FKnockdownChoiceAvailability::BuildLegacyEventMask(const FKnockdownChoiceView& View)
{
	int32 Mask = 0;
	if (View.AidOption.bAvailable) { Mask |= 1; }
	if (View.DestroyOption.bAvailable) { Mask |= 2; }
	if (View.WithdrawOption.bAvailable) { Mask |= 4; }
	return Mask;
}

bool FKnockdownChoiceAvailability::IsChoiceAvailable(const FKnockdownChoiceView& View, EKnockdownChoice Choice)
{
	switch (Choice)
	{
	case EKnockdownChoice::Aid:
		return View.AidOption.bAvailable;
	case EKnockdownChoice::Withdraw:
		return View.WithdrawOption.bAvailable;
	case EKnockdownChoice::Destroy:
		return View.DestroyOption.bAvailable;
	default:
		return false;
	}
}

FName FKnockdownChoiceAvailability::GetDisabledReason(const FKnockdownChoiceView& View, EKnockdownChoice Choice)
{
	switch (Choice)
	{
	case EKnockdownChoice::Aid:
		return View.AidOption.DisabledReason;
	case EKnockdownChoice::Withdraw:
		return View.WithdrawOption.DisabledReason;
	case EKnockdownChoice::Destroy:
		return View.DestroyOption.DisabledReason;
	default:
		return Reason_None;
	}
}
