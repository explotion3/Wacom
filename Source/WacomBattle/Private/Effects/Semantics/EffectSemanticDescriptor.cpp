// Copyright Wacom. All Rights Reserved.

#include "Effects/Semantics/EffectSemanticDescriptor.h"

#include "Cards/BattleCardRuntimeStateModule.h"
#include "Cards/CardEffect.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Resolution/BattleCardTargetPreview.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	bool IsLiteralMagnitudeSource(const FGameplayTag& Source)
	{
		return !Source.IsValid() || Source == WacomTags::Magnitude_Source_Literal;
	}

	bool IsSingleEnemyTargetAllowed(
		FWacomBattleRuleContentContract::ECardEffectContext Context,
		ECardTargetMode CardTargetMode)
	{
		using EContext = FWacomBattleRuleContentContract::ECardEffectContext;
		return Context == EContext::PerfectRelease
			|| ((Context == EContext::MainEffect || Context == EContext::ZoneHookOnPlay)
				&& CardTargetMode == ECardTargetMode::SingleEnemyPart);
	}

	bool IsActorTarget(const FGameplayTag& Target)
	{
		return Target == WacomTags::Target_Player
			|| Target == WacomTags::Target_Self
			|| Target == WacomTags::Target_SingleEnemyPart
			|| Target == WacomTags::Target_AllEnemyParts;
	}

	bool IsNormalHandCardTarget(const FBattleState& State, const FGuid& CardId)
	{
		const FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardId);
		return Card
			&& Card->Location == ECardLocation::Hand
			&& CardId != State.Cards.LeftHandInstanceId
			&& CardId != State.Cards.RightHandInstanceId;
	}

	EEffectTargetPlanKind BuildGenericCardTargetPlan(const FGameplayTag& Target)
	{
		if (Target == WacomTags::Target_Player || Target == WacomTags::Target_Self)
		{
			return EEffectTargetPlanKind::Player;
		}
		if (Target == WacomTags::Target_SingleEnemyPart) return EEffectTargetPlanKind::SelectedEnemyPart;
		if (Target == WacomTags::Target_AllEnemyParts) return EEffectTargetPlanKind::AllEnemyParts;
		if (Target == WacomTags::Target_SelectedHandCard) return EEffectTargetPlanKind::SelectedHandCard;
		if (Target == WacomTags::Target_LastShuffledCard) return EEffectTargetPlanKind::LastShuffledCard;
		if (Target == WacomTags::Target_RandomHandCard) return EEffectTargetPlanKind::RandomHandCard;
		if (Target == WacomTags::Target_ZoneHandCard) return EEffectTargetPlanKind::ZoneHandCard;
		return EEffectTargetPlanKind::None;
	}
}

bool FEffectSemanticDescriptor::SupportsCardTarget(
	const FGameplayTag& Target,
	FWacomBattleRuleContentContract::ECardEffectContext Context,
	ECardTargetMode CardTargetMode) const
{
	switch (CardTargetPolicy)
	{
	case EEffectCardTargetPolicy::Actor:
		return Target == WacomTags::Target_SingleEnemyPart
			? IsSingleEnemyTargetAllowed(Context, CardTargetMode)
			: IsActorTarget(Target);
	case EEffectCardTargetPolicy::PlayerOrSelf:
		return Target == WacomTags::Target_Player || Target == WacomTags::Target_Self;
	case EEffectCardTargetPolicy::EnemyPart:
		return Target == WacomTags::Target_AllEnemyParts
			|| (Target == WacomTags::Target_SingleEnemyPart
				&& IsSingleEnemyTargetAllowed(Context, CardTargetMode));
	case EEffectCardTargetPolicy::ShuffleRandom:
		return Target == WacomTags::Target_RandomHandCard;
	case EEffectCardTargetPolicy::ShuffleFromBoth:
		return Target == WacomTags::Target_ZoneHandCard;
	case EEffectCardTargetPolicy::ShuffleSelf:
		return Target == WacomTags::Target_Self;
	case EEffectCardTargetPolicy::CardCost:
		return Target == WacomTags::Target_Self
			|| Target == WacomTags::Target_AllHandCards
			|| Target == WacomTags::Target_LastShuffledCard
			|| (Target == WacomTags::Target_SelectedHandCard
				&& CardTargetMode == ECardTargetMode::HandCard);
	case EEffectCardTargetPolicy::SelectedHandCard:
		return Target == WacomTags::Target_SelectedHandCard
			&& CardTargetMode == ECardTargetMode::HandCard;
	case EEffectCardTargetPolicy::SimpleSelf:
		return !Target.IsValid()
			|| Target == WacomTags::Target_Player
			|| Target == WacomTags::Target_Self;
	case EEffectCardTargetPolicy::LastShuffledOrSelectedHandCard:
		return Target == WacomTags::Target_LastShuffledCard
			|| (Target == WacomTags::Target_SelectedHandCard
				&& CardTargetMode == ECardTargetMode::HandCard);
	default:
		return false;
	}
}

bool FEffectSemanticDescriptor::SupportsEnemyIntentTarget(const FGameplayTag& Target) const
{
	switch (IntentTargetPolicy)
	{
	case EIntentActorTargetPolicy::Player: return Target == WacomTags::Target_Player;
	case EIntentActorTargetPolicy::Self: return Target == WacomTags::Target_Self;
	case EIntentActorTargetPolicy::PlayerOrSelf:
		return Target == WacomTags::Target_Player || Target == WacomTags::Target_Self;
	default: return false;
	}
}

bool FEffectSemanticDescriptor::SupportsCardMagnitudeSource(const FGameplayTag& Source) const
{
	return IsLiteralMagnitudeSource(Source)
		|| (bSupportsRuntimeCostMagnitude && Source == WacomTags::Magnitude_Source_RuntimeCost)
		|| (bSupportsTargetStatusMagnitude && Source == WacomTags::Magnitude_Source_TargetStatusStacks);
}

EEffectTargetPlanKind FEffectSemanticDescriptor::BuildCardTargetPlan(const FGameplayTag& Target) const
{
	if ((CardTargetPolicy == EEffectCardTargetPolicy::ShuffleSelf
			|| CardTargetPolicy == EEffectCardTargetPolicy::CardCost)
		&& Target == WacomTags::Target_Self)
	{
		return EEffectTargetPlanKind::SourceCard;
	}
	return BuildGenericCardTargetPlan(Target);
}

FEffectParameters FEffectSemanticDescriptor::DecodeCardParameters(const FCardEffect& Effect) const
{
	switch (ParameterRole)
	{
	case EEffectParameterRole::CardLocation:
	{
		FEffectParameters Parameters;
		Parameters.Emplace<FDrawSourceEffectParameters>();
		FDrawSourceEffectParameters& Draw = Parameters.Get<FDrawSourceEffectParameters>();
		Draw.SourceLocation = Effect.TargetZone == WacomTags::CardLocation_Discard
			? ECardLocation::Discard
			: Effect.TargetZone == WacomTags::CardLocation_Exhaust
				? ECardLocation::Exhaust
				: ECardLocation::Draw;
		return Parameters;
	}
	case EEffectParameterRole::HandZone:
	{
		FEffectParameters Parameters;
		Parameters.Emplace<FHandZoneEffectParameters>();
		FHandZoneEffectParameters& HandZone = Parameters.Get<FHandZoneEffectParameters>();
		HandZone.Zone = Effect.TargetZone == WacomTags::HandZone_Left
			? EHandZone::Left
			: Effect.TargetZone == WacomTags::HandZone_Both
				? EHandZone::Both
				: Effect.TargetZone == WacomTags::HandZone_Right
					? EHandZone::Right
					: EHandZone::None;
		return Parameters;
	}
	case EEffectParameterRole::CardKeyword:
	{
		FEffectParameters Parameters;
		Parameters.Emplace<FKeywordEffectParameters>();
		Parameters.Get<FKeywordEffectParameters>().Keyword = Effect.TargetZone;
		return Parameters;
	}
	case EEffectParameterRole::StackStatus:
	{
		FEffectParameters Parameters;
		Parameters.Emplace<FStatusEffectParameters>();
		Parameters.Get<FStatusEffectParameters>().Status = Effect.TargetZone;
		return Parameters;
	}
	default:
		return FEffectParameters{};
	}
}

void FEffectSemanticDescriptor::ProjectTargetPreview(
	const FEffectProjectionContext& Context,
	FEffectProjectionScratch& Scratch,
	FBattleCardTargetPreviewEffect& OutEffect) const
{
	if (Context.Effect.Target != WacomTags::Target_SelectedHandCard || !Context.TargetHandCard)
	{
		return;
	}

	switch (ProjectionPolicy)
	{
	case EEffectProjectionPolicy::AddCardCost:
	case EEffectProjectionPolicy::ReduceCardCost:
		OutEffect.bHasTargetHandCardCostPreview = true;
		OutEffect.TargetHandCardRuntimeCostBefore =
			FBattleCardRuntimeStateModule::EvaluateCostWithRuntimeModifierDelta(
				Context.State,
				*Context.TargetHandCard,
				Scratch.SelectedHandCardCostModifierDelta).EffectiveCost;
		Scratch.SelectedHandCardCostModifierDelta +=
			ProjectionPolicy == EEffectProjectionPolicy::AddCardCost
				? Context.Magnitude
				: -Context.Magnitude;
		OutEffect.TargetHandCardRuntimeCostAfter =
			FBattleCardRuntimeStateModule::EvaluateCostWithRuntimeModifierDelta(
				Context.State,
				*Context.TargetHandCard,
				Scratch.SelectedHandCardCostModifierDelta).EffectiveCost;
		break;
	case EEffectProjectionPolicy::DiscardSelected:
	case EEffectProjectionPolicy::ExhaustSelected:
		if (!IsNormalHandCardTarget(Context.State, Context.SelectedHandCardId))
		{
			OutEffect.bSkipped = true;
			OutEffect.SkipReason = EWacomBattleCardPreviewEffectSkipReason::UnsupportedTarget;
		}
		else if (ProjectionPolicy == EEffectProjectionPolicy::ExhaustSelected)
		{
			OutEffect.bWouldExhaustTargetHandCard = true;
		}
		else
		{
			OutEffect.bWouldDiscardTargetHandCard = true;
		}
		break;
	case EEffectProjectionPolicy::GainKeyword:
		if (Context.Effect.TargetZone.IsValid())
		{
			OutEffect.bWouldGainTargetHandCardKeyword = true;
			OutEffect.TargetHandCardKeyword = Context.Effect.TargetZone;
		}
		break;
	default:
		break;
	}
}
