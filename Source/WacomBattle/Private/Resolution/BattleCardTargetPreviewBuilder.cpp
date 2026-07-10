// Copyright Wacom. All Rights Reserved.

#include "Resolution/BattleCardTargetPreviewBuilder.h"

#include "Commands/PlayCardEvaluation.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Effects/CardEffectMagnitudeEvaluator.h"
#include "Effects/ConditionResolver.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"

namespace
{
	bool IsHandAnchor(const FBattleState& State, const FGuid& CardInstanceId)
	{
		return CardInstanceId.IsValid()
			&& (CardInstanceId == State.Cards.LeftHandInstanceId
				|| CardInstanceId == State.Cards.RightHandInstanceId);
	}

	bool IsNormalHandCardTarget(const FBattleState& State, const FGuid& CardInstanceId)
	{
		const FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardInstanceId);
		return Card
			&& Card->Location == ECardLocation::Hand
			&& !IsHandAnchor(State, CardInstanceId);
	}

	int32 ComputeRuntimeCostWithModifierDelta(const FRuntimeCardInstance& Card, int32 ModifierDelta)
	{
		const int32 BaseCost = Card.Definition ? Card.Definition->BaseCost : 0;
		return FMath::Max(0, BaseCost + Card.RuntimeCostModifier + ModifierDelta);
	}

	void ApplyHandCardCostPreviewToAggregate(
		FBattleCardTargetPreview& Preview,
		const FBattleCardTargetPreviewEffect& EffectPreview)
	{
		if (!EffectPreview.bHasTargetHandCardCostPreview)
		{
			return;
		}

		Preview.bHasTargetHandCardCostPreview = true;
		Preview.TargetHandCardRuntimeCostAfter =
			EffectPreview.TargetHandCardRuntimeCostAfter;
	}

	void ApplyHandCardActionPreviewToAggregate(
		FBattleCardTargetPreview& Preview,
		const FBattleCardTargetPreviewEffect& EffectPreview)
	{
		Preview.bWouldDiscardTargetHandCard |= EffectPreview.bWouldDiscardTargetHandCard;
		Preview.bWouldExhaustTargetHandCard |= EffectPreview.bWouldExhaustTargetHandCard;
		if (EffectPreview.bWouldGainTargetHandCardKeyword)
		{
			Preview.bWouldGainTargetHandCardKeyword = true;
			Preview.TargetHandCardKeyword = EffectPreview.TargetHandCardKeyword;
		}
	}

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
		Preview.TargetHandCardRuntimeCostBefore = FBattleRules::ComputeRuntimeCost(*TargetCard);
		Preview.TargetHandCardRuntimeCostAfter = Preview.TargetHandCardRuntimeCostBefore;
	}

	FBattleCardTargetPreviewEffect MakeSkippedEffectPreview(
		int32 EffectIndex,
		const FCardEffect& Effect,
		EWacomBattleCardPreviewEffectSkipReason Reason)
	{
		FBattleCardTargetPreviewEffect EffectPreview;
		EffectPreview.EffectIndex = EffectIndex;
		EffectPreview.EffectType = Effect.EffectType;
		EffectPreview.Target = Effect.Target;
		EffectPreview.bSkipped = true;
		EffectPreview.SkipReason = Reason;
		return EffectPreview;
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

	const FGuid SelectedPartId = Preview.TargetEnemyPartInstanceId;
	const FGuid SelectedHandCardId = Preview.TargetHandCardInstanceId;
	const FRuntimeCardInstance* TargetHandCard =
		SelectedHandCardId.IsValid() ? FBattleRules::FindCard(State, SelectedHandCardId) : nullptr;
	int32 TargetHandCardModifierDelta = 0;

	Preview.Effects.Reserve(Definition->Effects.Num());

	for (int32 EffectIndex = 0; EffectIndex < Definition->Effects.Num(); ++EffectIndex)
	{
		const FCardEffect& Effect = Definition->Effects[EffectIndex];
		if (!FConditionResolver::Evaluate(
			State,
			Effect.Condition,
			Candidate.SourceCardInstanceId,
			SelectedPartId))
		{
			Preview.Effects.Add(MakeSkippedEffectPreview(
				EffectIndex,
				Effect,
				EWacomBattleCardPreviewEffectSkipReason::ConditionFailed));
			continue;
		}

		if (Effect.Target == WacomTags::Target_SelectedHandCard && !SelectedHandCardId.IsValid())
		{
			Preview.Effects.Add(MakeSkippedEffectPreview(
				EffectIndex,
				Effect,
				EWacomBattleCardPreviewEffectSkipReason::InvalidTarget));
			continue;
		}

		FBattleCardTargetPreviewEffect EffectPreview;
		EffectPreview.EffectIndex = EffectIndex;
		EffectPreview.EffectType = Effect.EffectType;
		EffectPreview.Target = Effect.Target;
		EffectPreview.Magnitude = FCardEffectMagnitudeEvaluator::ComputeFinalMagnitude(
				State,
				Effect,
				Preview.SourceCardRuntimeCost,
				SelectedPartId,
				Candidate.SourceCardInstanceId);
		EffectPreview.bHasMagnitude = true;

		if (Effect.Target == WacomTags::Target_SelectedHandCard && TargetHandCard)
		{
			EffectPreview.bTargetsSelectedHandCard = true;

			if (Effect.EffectType == WacomTags::Effect_Card_AddCost
				|| Effect.EffectType == WacomTags::Effect_Card_ReduceCost)
			{
				EffectPreview.bHasTargetHandCardCostPreview = true;
				EffectPreview.TargetHandCardRuntimeCostBefore =
					ComputeRuntimeCostWithModifierDelta(*TargetHandCard, TargetHandCardModifierDelta);

				const int32 Delta = Effect.EffectType == WacomTags::Effect_Card_AddCost
					? EffectPreview.Magnitude
					: -EffectPreview.Magnitude;
				TargetHandCardModifierDelta += Delta;
				EffectPreview.TargetHandCardRuntimeCostAfter =
					ComputeRuntimeCostWithModifierDelta(*TargetHandCard, TargetHandCardModifierDelta);
				ApplyHandCardCostPreviewToAggregate(Preview, EffectPreview);
			}
			else if (Effect.EffectType == WacomTags::Effect_Card_DiscardSelected)
			{
				if (IsNormalHandCardTarget(State, SelectedHandCardId))
				{
					EffectPreview.bWouldDiscardTargetHandCard = true;
				}
				else
				{
					EffectPreview.bSkipped = true;
					EffectPreview.SkipReason =
						EWacomBattleCardPreviewEffectSkipReason::UnsupportedTarget;
				}
			}
			else if (Effect.EffectType == WacomTags::Effect_Card_ExhaustSelected)
			{
				if (IsNormalHandCardTarget(State, SelectedHandCardId))
				{
					EffectPreview.bWouldExhaustTargetHandCard = true;
				}
				else
				{
					EffectPreview.bSkipped = true;
					EffectPreview.SkipReason =
						EWacomBattleCardPreviewEffectSkipReason::UnsupportedTarget;
				}
			}
			else if (Effect.EffectType == WacomTags::Effect_GainKeyword && Effect.TargetZone.IsValid())
			{
				EffectPreview.bWouldGainTargetHandCardKeyword = true;
				EffectPreview.TargetHandCardKeyword = Effect.TargetZone;
			}
		}

		ApplyHandCardActionPreviewToAggregate(Preview, EffectPreview);
		Preview.Effects.Add(MoveTemp(EffectPreview));
	}

	return Preview;
}
