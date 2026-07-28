// Copyright Wacom. All Rights Reserved.

#include "Resolution/BattleCardTargetPreviewBuilder.h"

#include "Commands/PlayCardEvaluation.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Effects/Semantics/BattleEffectSemanticsModule.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Types/WacomInteractionTargetTypes.h"

#include "Cards/CardDefinition.h"

namespace
{
	void FillHandCardTarget(
		const FBattleState& State,
		const FGuid& TargetCardInstanceId,
		FBattleCardTargetPreview& Preview)
	{
		const FRuntimeCardInstance* TargetCard = FBattleRules::FindCard(State, TargetCardInstanceId);
		if (!TargetCard || TargetCard->Location != ECardLocation::Hand)
		{
			return;
		}

		Preview.TargetKind = EWacomBattleCardPreviewTargetKind::HandCard;
		Preview.TargetHandCardInstanceId = TargetCard->InstanceId;
		Preview.TargetHandCardRuntimeCostBefore =
			FBattleRules::ComputeRuntimeCost(State, *TargetCard);
		Preview.TargetHandCardRuntimeCostAfter = Preview.TargetHandCardRuntimeCostBefore;
	}
}

FBattleCardTargetPreview FBattleCardTargetPreviewBuilder::Build(
	const FBattleState& State,
	const FGuid& CardInstanceId,
	const FWacomInteractionTargetHandle& Target)
{
	return Build(
		State,
		FPlayCardEvaluator::EvaluatePreviewCandidate(State, CardInstanceId, Target));
}

FBattleCardTargetPreview FBattleCardTargetPreviewBuilder::Build(
	const FBattleState& State,
	const FPlayCardPreviewCandidate& Candidate)
{
	FBattleCardTargetPreview Preview;
	Preview.Validation = Candidate.Validation;
	if (!Candidate.bCanPreview || !Preview.Validation.bCanTarget)
	{
		return Preview;
	}
	if (Candidate.EvaluatedStateVersion != State.StateVersion)
	{
		Preview.Validation.bCanTarget = false;
		Preview.Validation.RejectReason = EWacomBattleTargetRejectReason::SourceCardInvalid;
		Preview.Validation.DebugSummary += TEXT(" StalePlayCardPreviewCandidate");
		return Preview;
	}

	const FRuntimeCardInstance* SourceCard =
		FBattleRules::FindCard(State, Candidate.SourceCardInstanceId);
	const UCardDefinition* Definition = Candidate.SourceDefinition;
	if (!SourceCard || !Definition || SourceCard->Definition != Definition)
	{
		Preview.Validation.bCanTarget = false;
		Preview.Validation.RejectReason = EWacomBattleTargetRejectReason::SourceCardInvalid;
		Preview.Validation.DebugSummary += TEXT(" StalePlayCardSourceFacts");
		return Preview;
	}

	Preview.bHasPreview = true;
	Preview.SourceCardInstanceId = Candidate.SourceCardInstanceId;
	Preview.SourceCardRuntimeCost = Candidate.RuntimeCost;
	Preview.bSourceCardSwift = Candidate.bSwift;

	const FPlayCardTargetFacts& EnemyTarget = Candidate.ExecutionTarget.HasEnemyPart()
		? Candidate.ExecutionTarget
		: Candidate.FocusTarget;
	if (EnemyTarget.HasEnemyPart())
	{
		Preview.TargetKind = EWacomBattleCardPreviewTargetKind::EnemyPart;
		Preview.TargetEnemyPartInstanceId = EnemyTarget.EnemyPartInstanceId;
		Preview.TargetEnemyPartIdentity = EnemyTarget.EnemyPartIdentity;
		Preview.TargetEnemyPartKey = EnemyTarget.EnemyPartKey;
	}
	else if (Candidate.ExecutionTarget.HasHandCard())
	{
		FillHandCardTarget(
			State,
			Candidate.ExecutionTarget.HandCardInstanceId,
			Preview);
	}

	FBattleEffectSemanticsModule::ProjectCardChain(
		State,
		Definition->ResolveEffects(SourceCard->UpgradeTier),
		FCardEffectChainBindings{
			Candidate.RuntimeCost,
			Candidate.SourceCardInstanceId,
			Preview.TargetEnemyPartInstanceId,
			Preview.TargetHandCardInstanceId },
		Preview);

	return Preview;
}
