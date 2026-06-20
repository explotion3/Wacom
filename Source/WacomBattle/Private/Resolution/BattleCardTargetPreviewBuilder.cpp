// Copyright Wacom. All Rights Reserved.

#include "Resolution/BattleCardTargetPreviewBuilder.h"

#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Effects/CardEffectMagnitudeEvaluator.h"
#include "Effects/ConditionResolver.h"
#include "Resolution/BattleTargetResolver.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"

namespace
{
	bool HasEffectiveKeyword(const FRuntimeCardInstance& Card, const FGameplayTag& Keyword)
	{
		return (Card.Definition && Card.Definition->Keywords.HasTagExact(Keyword))
			|| Card.TemporaryKeywords.HasTagExact(Keyword);
	}

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

	void FillEnemyPartTarget(
		const FBattleState& State,
		const FWacomInteractionTargetHandle& Target,
		FBattleCardTargetPreview& Preview)
	{
		EWacomBattleTargetRejectReason RejectReason = EWacomBattleTargetRejectReason::None;
		const FRuntimeEnemyPart* Part =
			FBattleTargetResolver::ResolveWorldEnemyPartTarget(State, Target, RejectReason);
		if (!Part)
		{
			return;
		}

		Preview.TargetKind = EWacomBattleCardPreviewTargetKind::EnemyPart;
		Preview.TargetEnemyPartInstanceId = Part->InstanceId;
		Preview.TargetEnemyPartIdentity = Part->Identity;
		Preview.TargetEnemyPartKey = Part->Identity.ToEnemyPartKey();
	}

	void FillHandCardTarget(
		const FBattleState& State,
		const FWacomInteractionTargetHandle& Target,
		FBattleCardTargetPreview& Preview)
	{
		const FRuntimeCardInstance* TargetCard = FBattleRules::FindCard(State, Target.CardInstanceId);
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
	FBattleCardTargetPreview Preview;
	Preview.Validation = FBattleTargetResolver::ValidateTargetWithCard(State, CardInstanceId, Target);
	if (!Preview.Validation.bCanTarget)
	{
		return Preview;
	}

	const FRuntimeCardInstance* SourceCard = FBattleRules::FindCard(State, CardInstanceId);
	if (!SourceCard || SourceCard->Location != ECardLocation::Hand || !SourceCard->Definition)
	{
		Preview.Validation.bCanTarget = false;
		Preview.Validation.RejectReason = EWacomBattleTargetRejectReason::SourceCardInvalid;
		return Preview;
	}

	Preview.bHasPreview = true;
	Preview.SourceCardInstanceId = SourceCard->InstanceId;
	Preview.SourceCardRuntimeCost = FBattleRules::ComputeRuntimeCost(*SourceCard);
	Preview.bSourceCardSwift = HasEffectiveKeyword(*SourceCard, WacomTags::Card_Keyword_Swift);

	if (Target.TargetKind == EWacomInteractionTargetKind::World)
	{
		FillEnemyPartTarget(State, Target, Preview);
	}
	else if (Target.TargetKind == EWacomInteractionTargetKind::Card)
	{
		FillHandCardTarget(State, Target, Preview);
	}

	const FGuid SelectedPartId = Preview.TargetEnemyPartInstanceId;
	const FGuid SelectedHandCardId = Preview.TargetHandCardInstanceId;
	const FRuntimeCardInstance* TargetHandCard =
		SelectedHandCardId.IsValid() ? FBattleRules::FindCard(State, SelectedHandCardId) : nullptr;
	int32 TargetHandCardModifierDelta = 0;

	const UCardDefinition* Definition = SourceCard->Definition;
	Preview.Effects.Reserve(Definition->Effects.Num());

	for (int32 EffectIndex = 0; EffectIndex < Definition->Effects.Num(); ++EffectIndex)
	{
		const FCardEffect& Effect = Definition->Effects[EffectIndex];
		if (!FConditionResolver::Evaluate(State, Effect.Condition, CardInstanceId, SelectedPartId))
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
			CardInstanceId);
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
