// Copyright Wacom. All Rights Reserved.

#include "Effects/Semantics/BattleEffectSemanticsModule.h"

#include "Core/BattleOperationAdapter.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Effects/ConditionResolver.h"
#include "Effects/Semantics/EffectSemanticTypes.h"
#include "Effects/Semantics/EffectSemanticRegistry.h"
#include "Events/BattleEventBus.h"
#include "Resolution/BattleCardTargetPreview.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Tags/WacomGameplayTags.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Enemies/IntentEffect.h"

namespace
{
	enum class EEffectMagnitudeSourceKind : uint8
	{
		Literal,
		RuntimeCost,
		HandCount,
		TargetStatusStacks,
	};

	struct FEffectMagnitudePlan
	{
		EEffectMagnitudeSourceKind Source = EEffectMagnitudeSourceKind::Literal;
		int32 Literal = 0;
		FGameplayTag TargetStatus;
	};

	FEffectMagnitudePlan DecodeMagnitudePlan(const FCardEffect& Effect)
	{
		FEffectMagnitudePlan Plan;
		Plan.Literal = Effect.Magnitude;

		if (Effect.MagnitudeSource.IsValid())
		{
			if (Effect.MagnitudeSource == WacomTags::Magnitude_Source_RuntimeCost)
			{
				Plan.Source = EEffectMagnitudeSourceKind::RuntimeCost;
			}
			else if (Effect.MagnitudeSource == WacomTags::Magnitude_Source_HandCount)
			{
				Plan.Source = EEffectMagnitudeSourceKind::HandCount;
			}
			else if (Effect.MagnitudeSource == WacomTags::Magnitude_Source_TargetStatusStacks)
			{
				Plan.Source = EEffectMagnitudeSourceKind::TargetStatusStacks;
				Plan.TargetStatus = Effect.TargetZone;
			}
			return Plan;
		}

		if (Effect.bMagnitudeFromRuntimeCost)
		{
			Plan.Source = EEffectMagnitudeSourceKind::RuntimeCost;
		}
		return Plan;
	}

	const FEffectSemanticDescriptor* FindDescriptor(const FGameplayTag& EffectType)
	{
		return FEffectSemanticRegistry::Find(EffectType);
	}

	bool IsCriticalEligibleEffect(const FGameplayTag& EffectType)
	{
		return EffectType == WacomTags::Effect_Damage
			|| EffectType == WacomTags::Effect_Heal
			|| EffectType == WacomTags::Status_Shield
			|| EffectType == WacomTags::Effect_ApplyStatus_Poison
			|| EffectType == WacomTags::Effect_ApplyStatus_Burn;
	}

	void FillCardTarget(
		FEffectExecutionContext& Context,
		EEffectTargetPlanKind TargetPlan,
		const FCardEffectChainBindings& Bindings,
		const FGuid& LastShuffledCardId)
	{
		switch (TargetPlan)
		{
		case EEffectTargetPlanKind::Player:
			Context.TargetKind = EEffectTargetKind::Player;
			break;
		case EEffectTargetPlanKind::SourceCard:
			Context.TargetKind = EEffectTargetKind::HandCard;
			Context.TargetInstanceId = Bindings.SourceCardId;
			break;
		case EEffectTargetPlanKind::SelectedEnemyPart:
			Context.TargetKind = EEffectTargetKind::EnemyPart;
			Context.TargetInstanceId = Bindings.SelectedEnemyPartId;
			break;
		case EEffectTargetPlanKind::SelectedHandCard:
			Context.TargetKind = EEffectTargetKind::HandCard;
			Context.TargetInstanceId = Bindings.SelectedHandCardId;
			break;
		case EEffectTargetPlanKind::LastShuffledCard:
			Context.TargetKind = EEffectTargetKind::HandCard;
			Context.TargetInstanceId = LastShuffledCardId;
			break;
		case EEffectTargetPlanKind::RandomHandCard:
		case EEffectTargetPlanKind::ZoneHandCard:
			Context.TargetKind = EEffectTargetKind::HandCard;
			break;
		default:
			Context.TargetKind = EEffectTargetKind::None;
			break;
		}
	}

	void FillCardContext(
		FEffectExecutionContext& Context,
		FBattleState& State,
		FBattleEventBus& Events,
		const FCardEffect& Effect,
		int32 Magnitude,
		const FEffectSemanticDescriptor* Semantics,
		const FCardEffectChainBindings& Bindings,
		IBattleOperationAdapter* OperationAdapter)
	{
		Context.State = &State;
		Context.Events = &Events;
		Context.SourceKind = EEffectSourceKind::Card;
		Context.SourceInstanceId = Bindings.SourceCardId;
		Context.EffectTag = Effect.EffectType;
		Context.SourceEffect = &Effect;
		Context.Magnitude = Magnitude;
		Context.Duration = Effect.Duration;
		Context.Parameters = Semantics
			? Semantics->DecodeCardParameters(Effect)
			: FEffectParameters{};
		Context.ExcludeHandCardId = Bindings.SourceCardId;
		Context.OperationAdapter = OperationAdapter;
	}

	bool ShouldExecuteInvocation(
		const FEffectSemanticDescriptor* Semantics,
		const FGameplayTag& EffectType,
		IBattleOperationAdapter* OperationAdapter)
	{
		if (!OperationAdapter)
		{
			return true;
		}

		const FBattleOperationDescriptor Operation{
			EBattleOperationKind::Effect,
			Semantics
				? Semantics->GetDeterminism()
				: EBattleOperationDeterminism::Unknown,
			EffectType,
			/*bReportUnresolvedWhenSkipped*/true };
		return OperationAdapter->ShouldExecute(Operation);
	}

	void ExecuteCardInvocation(
		FBattleState& State,
		FBattleEventBus& Events,
		const FCardEffect& Effect,
		int32 PreCriticalMagnitude,
		int32 ResolvedMagnitude,
		bool bCritical,
		const FEffectSemanticDescriptor* Semantics,
		EEffectTargetPlanKind TargetPlan,
		const FCardEffectChainBindings& Bindings,
		FGuid& InOutLastShuffledCardId,
		TSet<FGuid>& InOutShuffledCardIds,
		IBattleOperationAdapter* OperationAdapter,
		const FGuid& ExpandedEnemyPartId = FGuid())
	{
		FEffectExecutionContext Context;
		FillCardContext(
			Context,
			State,
			Events,
			Effect,
			ResolvedMagnitude,
			Semantics,
			Bindings,
			OperationAdapter);
		Context.PreCriticalMagnitude = PreCriticalMagnitude;
		Context.bCritical = bCritical;

		if (ExpandedEnemyPartId.IsValid())
		{
			Context.TargetKind = EEffectTargetKind::EnemyPart;
			Context.TargetInstanceId = ExpandedEnemyPartId;
		}
		else
		{
			FillCardTarget(Context, TargetPlan, Bindings, InOutLastShuffledCardId);
		}

		if (!ShouldExecuteInvocation(Semantics, Effect.EffectType, OperationAdapter))
		{
			return;
		}

		if (Semantics && Semantics->GetHandler())
		{
			const FEffectApplyResult Result = Semantics->GetHandler()(Context);
			if (Result.ShuffledCardId.IsValid())
			{
				InOutLastShuffledCardId = Result.ShuffledCardId;
				InOutShuffledCardIds.Add(Result.ShuffledCardId);
			}
			if (Result.bApplied)
			{
				FBattleEvent Event;
				Event.Type = EBattleEventType::EffectResolved;
				Event.CardInstanceId = Bindings.SourceCardId;
				Event.ActorInstanceId = Context.TargetInstanceId;
				Event.Tag = Effect.EffectType;
				Event.EffectResolution.EffectType = Effect.EffectType;
				Event.EffectResolution.PreCriticalMagnitude = PreCriticalMagnitude;
				Event.EffectResolution.ResolvedMagnitude = ResolvedMagnitude;
				Event.EffectResolution.bCritical = bCritical;
				if (Context.TargetKind == EEffectTargetKind::EnemyPart)
				{
					Event.ActorEnemyPartKey =
						FBattleRules::FindEnemyPartKey(State, Context.TargetInstanceId);
				}
				Events.Emit(MoveTemp(Event));
			}
		}
	}

	FBattleCardTargetPreviewEffect MakeSkippedPreview(
		int32 EffectIndex,
		const FCardEffect& Effect,
		EWacomBattleCardPreviewEffectSkipReason Reason)
	{
		FBattleCardTargetPreviewEffect Preview;
		Preview.EffectIndex = EffectIndex;
		Preview.EffectType = Effect.EffectType;
		Preview.Target = Effect.Target;
		Preview.bSkipped = true;
		Preview.SkipReason = Reason;
		return Preview;
	}

	void ApplyProjectionToAggregate(
		FBattleCardTargetPreview& Preview,
		const FBattleCardTargetPreviewEffect& EffectPreview)
	{
		if (EffectPreview.bHasTargetHandCardCostPreview)
		{
			Preview.bHasTargetHandCardCostPreview = true;
			Preview.TargetHandCardRuntimeCostAfter =
				EffectPreview.TargetHandCardRuntimeCostAfter;
		}
		Preview.bWouldDiscardTargetHandCard |= EffectPreview.bWouldDiscardTargetHandCard;
		Preview.bWouldExhaustTargetHandCard |= EffectPreview.bWouldExhaustTargetHandCard;
		if (EffectPreview.bWouldGainTargetHandCardKeyword)
		{
			Preview.bWouldGainTargetHandCardKeyword = true;
			Preview.TargetHandCardKeyword = EffectPreview.TargetHandCardKeyword;
		}
	}
}

bool FCardCriticalResolutionLedger::Resolve(
	FBattleState& State,
	const FGuid& SourceCardId,
	const FGameplayTag& EffectType,
	const FCardCriticalInvocationKey& Key)
{
	if (!IsCriticalEligibleEffect(EffectType))
	{
		return false;
	}
	if (const bool* Existing = Rolls.Find(Key))
	{
		return *Existing;
	}

	bool bCritical = false;
	if (bAllowRolls)
	{
		const FRuntimeCardInstance* SourceCard = FBattleRules::FindCard(State, SourceCardId);
		if (SourceCard && SourceCard->Definition)
		{
			const int32 Chance = FMath::Clamp(
				SourceCard->Definition->ResolveBaseCriticalChancePercent(
					SourceCard->UpgradeTier)
					+ SourceCard->CriticalChanceBonusPercent,
				0,
				100);
			bCritical = Chance >= 100
				|| (Chance > 0 && State.Rng.RandRange(1, 100) <= Chance);
		}
	}
	Rolls.Add(Key, bCritical);
	return bCritical;
}

FCardEffectChain::FCardEffectChain(
	FBattleState& InState,
	FBattleEventBus& InEvents,
	const FCardEffectChainBindings& InBindings,
	IBattleOperationAdapter* InOperationAdapter)
	: State(&InState)
	, Events(&InEvents)
	, Bindings(InBindings)
	, OperationAdapter(InOperationAdapter)
{
	if (!Bindings.CriticalLedger && OperationAdapter)
	{
		const FBattleOperationDescriptor CriticalOperation{
			EBattleOperationKind::DirectRule,
			EBattleOperationDeterminism::Random,
			FGameplayTag(),
			/*bReportUnresolvedWhenSkipped*/false };
		OwnedCriticalLedger.bAllowRolls =
			OperationAdapter->ShouldExecute(CriticalOperation);
	}
	CriticalLedger = Bindings.CriticalLedger
		? Bindings.CriticalLedger
		: &OwnedCriticalLedger;
}

void FCardEffectChain::Execute(TConstArrayView<FCardEffect> Effects)
{
	FBattleEffectSemanticsModule::ExecuteCardEffects(*this, Effects);
}

void FBattleEffectSemanticsModule::ExecuteCardEffects(
	FCardEffectChain& Chain,
	TConstArrayView<FCardEffect> Effects)
{
	check(Chain.State && Chain.Events);
	const int32 SegmentIndex = Chain.NextSegmentIndex++;
	for (int32 EffectIndex = 0; EffectIndex < Effects.Num(); ++EffectIndex)
	{
		const FCardEffect& Effect = Effects[EffectIndex];
		const FEffectSemanticDescriptor* Semantics = FindDescriptor(Effect.EffectType);
		const EEffectTargetPlanKind TargetPlan = Semantics
			? Semantics->BuildCardTargetPlan(Effect.Target)
			: EEffectTargetPlanKind::None;

		if (TargetPlan == EEffectTargetPlanKind::AllEnemyParts)
		{
			TArray<FCardEnemyPartEffectInvocation> Invocations;
			BuildCardEnemyPartInvocations(
				*Chain.State,
				Effect,
				Chain.Bindings.RuntimeCost,
				Chain.Bindings.SourceCardId,
				Chain.Bindings.SelectedEnemyPartId,
				Invocations);
			for (int32 InvocationOrdinal = 0;
				InvocationOrdinal < Invocations.Num();
				++InvocationOrdinal)
			{
				const FCardEnemyPartEffectInvocation& Invocation =
					Invocations[InvocationOrdinal];
				const bool bCritical = Chain.CriticalLedger->Resolve(
					*Chain.State,
					Chain.Bindings.SourceCardId,
					Effect.EffectType,
					FCardCriticalInvocationKey{
						SegmentIndex,
						EffectIndex,
						Invocation.TargetEnemyPartInstanceId,
						InvocationOrdinal });
				ExecuteCardInvocation(
					*Chain.State,
					*Chain.Events,
					Effect,
					Invocation.FinalMagnitude,
					bCritical
						? Invocation.FinalMagnitude * 2
						: Invocation.FinalMagnitude,
					bCritical,
					Semantics,
					TargetPlan,
					Chain.Bindings,
					Chain.LastShuffledCardId,
					Chain.ShuffledCardIds,
					Chain.OperationAdapter,
					Invocation.TargetEnemyPartInstanceId);
			}
			continue;
		}

		if (TargetPlan == EEffectTargetPlanKind::SelectedEnemyPart)
		{
			TArray<FCardEnemyPartEffectInvocation> Invocations;
			BuildCardEnemyPartInvocations(
				*Chain.State,
				Effect,
				Chain.Bindings.RuntimeCost,
				Chain.Bindings.SourceCardId,
				Chain.Bindings.SelectedEnemyPartId,
				Invocations);
			if (Invocations.IsEmpty())
			{
				continue;
			}
			const FCardEnemyPartEffectInvocation& Invocation = Invocations[0];
			const bool bCritical = Chain.CriticalLedger->Resolve(
				*Chain.State,
				Chain.Bindings.SourceCardId,
				Effect.EffectType,
				FCardCriticalInvocationKey{
					SegmentIndex,
					EffectIndex,
					Invocation.TargetEnemyPartInstanceId,
					0 });

			ExecuteCardInvocation(
				*Chain.State,
				*Chain.Events,
				Effect,
				Invocation.FinalMagnitude,
				bCritical
					? Invocation.FinalMagnitude * 2
					: Invocation.FinalMagnitude,
				bCritical,
				Semantics,
				TargetPlan,
				Chain.Bindings,
				Chain.LastShuffledCardId,
				Chain.ShuffledCardIds,
				Chain.OperationAdapter,
				Invocation.TargetEnemyPartInstanceId);
			continue;
		}

		if (!FConditionResolver::Evaluate(
			*Chain.State,
			Effect.Condition,
			Chain.Bindings.SourceCardId,
			Chain.Bindings.SelectedEnemyPartId))
		{
			continue;
		}

		const int32 FinalMagnitude = FBattleEffectSemanticsModule::EvaluateCardFinalMagnitude(
			*Chain.State,
			Effect,
			Chain.Bindings.RuntimeCost,
			Chain.Bindings.SelectedEnemyPartId,
			Chain.Bindings.SourceCardId);
		FGuid CriticalTargetId;
		if (TargetPlan == EEffectTargetPlanKind::SourceCard)
		{
			CriticalTargetId = Chain.Bindings.SourceCardId;
		}
		else if (TargetPlan == EEffectTargetPlanKind::SelectedHandCard)
		{
			CriticalTargetId = Chain.Bindings.SelectedHandCardId;
		}
		const bool bCritical = Chain.CriticalLedger->Resolve(
			*Chain.State,
			Chain.Bindings.SourceCardId,
			Effect.EffectType,
			FCardCriticalInvocationKey{
				SegmentIndex,
				EffectIndex,
				CriticalTargetId,
				0 });

		ExecuteCardInvocation(
			*Chain.State,
			*Chain.Events,
			Effect,
			FinalMagnitude,
			bCritical ? FinalMagnitude * 2 : FinalMagnitude,
			bCritical,
			Semantics,
			TargetPlan,
			Chain.Bindings,
			Chain.LastShuffledCardId,
			Chain.ShuffledCardIds,
			Chain.OperationAdapter);
	}
}

FCardEffectChain FBattleEffectSemanticsModule::BeginCardChain(
	FBattleState& State,
	FBattleEventBus& Events,
	const FCardEffectChainBindings& Bindings,
	IBattleOperationAdapter* OperationAdapter)
{
	return FCardEffectChain(State, Events, Bindings, OperationAdapter);
}

void FBattleEffectSemanticsModule::ExecuteEnemyIntentChain(
	FBattleState& State,
	FBattleEventBus& Events,
	TConstArrayView<FIntentEffect> Effects,
	const FGuid& ActingPartId,
	IBattleOperationAdapter* OperationAdapter)
{
	for (const FIntentEffect& Effect : Effects)
	{
		const FEffectSemanticDescriptor* Semantics = FindDescriptor(Effect.EffectType);
		FEffectExecutionContext Context;
		Context.State = &State;
		Context.Events = &Events;
		Context.SourceKind = EEffectSourceKind::EnemyPartIntent;
		Context.SourceInstanceId = ActingPartId;
		Context.EffectTag = Effect.EffectType;
		Context.Magnitude = Effect.Magnitude;
		Context.Duration = Effect.Duration;
		Context.HandAffliction = Effect.HandAffliction;
		Context.OperationAdapter = OperationAdapter;

		if (Effect.Target == WacomTags::Target_Player)
		{
			Context.TargetKind = EEffectTargetKind::Player;
		}
		else if (Effect.Target == WacomTags::Target_Self)
		{
			Context.TargetKind = EEffectTargetKind::EnemyPart;
			Context.TargetInstanceId = ActingPartId;
		}

		if (ShouldExecuteInvocation(Semantics, Effect.EffectType, OperationAdapter)
			&& Semantics
			&& Semantics->GetHandler())
		{
			Semantics->GetHandler()(Context);
		}

		if (State.Player.CurrentHp <= 0)
		{
			break;
		}
	}
}

void FBattleEffectSemanticsModule::ProjectCardChain(
	const FBattleState& State,
	TConstArrayView<FCardEffect> Effects,
	const FCardEffectChainBindings& Bindings,
	FBattleCardTargetPreview& OutPreview)
{
	const FRuntimeCardInstance* TargetHandCard = Bindings.SelectedHandCardId.IsValid()
		? FBattleRules::FindCard(State, Bindings.SelectedHandCardId)
		: nullptr;
	FEffectProjectionScratch Scratch;
	OutPreview.Effects.Reserve(OutPreview.Effects.Num() + Effects.Num());

	for (int32 EffectIndex = 0; EffectIndex < Effects.Num(); ++EffectIndex)
	{
		const FCardEffect& Effect = Effects[EffectIndex];
		const FEffectSemanticDescriptor* Semantics = FindDescriptor(Effect.EffectType);
		const EEffectTargetPlanKind TargetPlan = Semantics
			? Semantics->BuildCardTargetPlan(Effect.Target)
			: EEffectTargetPlanKind::None;
		int32 FinalMagnitude = 0;
		const bool bUseEnemyPartEvaluation = Bindings.SelectedEnemyPartId.IsValid()
			&& (TargetPlan == EEffectTargetPlanKind::SelectedEnemyPart
				|| TargetPlan == EEffectTargetPlanKind::AllEnemyParts);
		const bool bCanProject = bUseEnemyPartEvaluation
			? TryEvaluateCardEffectForEnemyPart(
				State,
				Effect,
				Bindings.RuntimeCost,
				Bindings.SourceCardId,
				Bindings.SelectedEnemyPartId,
				FinalMagnitude)
			: FConditionResolver::Evaluate(
				State,
				Effect.Condition,
				Bindings.SourceCardId,
				Bindings.SelectedEnemyPartId);
		if (!bCanProject)
		{
			OutPreview.Effects.Add(MakeSkippedPreview(
				EffectIndex,
				Effect,
				EWacomBattleCardPreviewEffectSkipReason::ConditionFailed));
			continue;
		}

		if (Effect.Target == WacomTags::Target_SelectedHandCard
			&& !Bindings.SelectedHandCardId.IsValid())
		{
			OutPreview.Effects.Add(MakeSkippedPreview(
				EffectIndex,
				Effect,
				EWacomBattleCardPreviewEffectSkipReason::InvalidTarget));
			continue;
		}

		FBattleCardTargetPreviewEffect EffectPreview;
		EffectPreview.EffectIndex = EffectIndex;
		EffectPreview.EffectType = Effect.EffectType;
		EffectPreview.Target = Effect.Target;
		EffectPreview.Magnitude = bUseEnemyPartEvaluation
			? FinalMagnitude
			: EvaluateCardFinalMagnitude(
				State,
				Effect,
				Bindings.RuntimeCost,
				Bindings.SelectedEnemyPartId,
				Bindings.SourceCardId);
		EffectPreview.bHasMagnitude = true;

		if (Effect.Target == WacomTags::Target_SelectedHandCard && TargetHandCard)
		{
			EffectPreview.bTargetsSelectedHandCard = true;
			if (Semantics)
			{
				Semantics->ProjectTargetPreview(
					FEffectProjectionContext{
						State,
						Effect,
						TargetHandCard,
						Bindings.SelectedHandCardId,
						EffectPreview.Magnitude },
					Scratch,
					EffectPreview);
			}
		}

		ApplyProjectionToAggregate(OutPreview, EffectPreview);
		OutPreview.Effects.Add(MoveTemp(EffectPreview));
	}
}

int32 FBattleEffectSemanticsModule::EvaluateCardBaseMagnitude(
	const FBattleState& State,
	const FCardEffect& Effect,
	int32 RuntimeCost,
	const FGuid& TargetEnemyPartId)
{
	const FEffectMagnitudePlan Plan = DecodeMagnitudePlan(Effect);
	switch (Plan.Source)
	{
	case EEffectMagnitudeSourceKind::RuntimeCost:
		return RuntimeCost;
	case EEffectMagnitudeSourceKind::HandCount:
		return State.Cards.Hand.Num();
	case EEffectMagnitudeSourceKind::TargetStatusStacks:
		if (!TargetEnemyPartId.IsValid() || !Plan.TargetStatus.IsValid())
		{
			return Plan.Literal;
		}
		for (const FRuntimeEnemyPart& Part : State.Enemy.Parts)
		{
			if (Part.InstanceId == TargetEnemyPartId && !Part.bDestroyed)
			{
				const int32* Stacks = Part.StatusStacks.Find(Plan.TargetStatus);
				return Stacks ? *Stacks : 0;
			}
		}
		return Plan.Literal;
	case EEffectMagnitudeSourceKind::Literal:
	default:
		return Plan.Literal;
	}
}

int32 FBattleEffectSemanticsModule::EvaluateCardFinalMagnitude(
	const FBattleState& State,
	const FCardEffect& Effect,
	int32 RuntimeCost,
	const FGuid& TargetEnemyPartId,
	const FGuid& SourceCardId)
{
	int32 FinalMagnitude = EvaluateCardBaseMagnitude(
		State,
		Effect,
		RuntimeCost,
		TargetEnemyPartId);

	for (const FMagnitudeModifier& Modifier : Effect.MagnitudeModifiers)
	{
		if (!FConditionResolver::Evaluate(
			State,
			Modifier.Condition,
			SourceCardId,
			TargetEnemyPartId))
		{
			continue;
		}

		switch (Modifier.Op)
		{
		case EMagnitudeModOp::Add:
			FinalMagnitude += Modifier.Value;
			break;
		case EMagnitudeModOp::Multiply:
			FinalMagnitude *= Modifier.Value;
			break;
		default:
			break;
		}
	}

	if (const FRuntimeCardInstance* SourceCard =
		FBattleRules::FindCard(State, SourceCardId))
	{
		if (const int32* Bonus = SourceCard->EffectMagnitudeBonuses.Find(Effect.EffectType))
		{
			FinalMagnitude += *Bonus;
		}
		if (const float* Multiplier =
			SourceCard->EffectMagnitudeMultipliers.Find(Effect.EffectType))
		{
			FinalMagnitude = FMath::RoundToInt(
				static_cast<float>(FinalMagnitude) * FMath::Max(0.0f, *Multiplier));
		}
	}

	if (Effect.EffectType == WacomTags::Effect_Damage)
	{
		if (const int32* SourceIndex = State.Cards.CardIndexById.Find(SourceCardId))
		{
			const FRuntimeCardInstance& SourceCard = State.Cards.AllCards[*SourceIndex];
			if (SourceCard.Definition
				&& SourceCard.Definition->Keywords.HasTagExact(WacomTags::Card_Keyword_Weapon)
				&& SourceCard.CapacityEffectTags.HasTagExact(
					WacomTags::Card_CapacityEffect_WeaponDamagePlus3))
			{
				FinalMagnitude += 3;
			}
		}
		FinalMagnitude = FMath::Max(0, FinalMagnitude);
	}

	return FinalMagnitude;
}

bool FBattleEffectSemanticsModule::TryEvaluateCardEffectForEnemyPart(
	const FBattleState& State,
	const FCardEffect& Effect,
	const int32 RuntimeCost,
	const FGuid& SourceCardId,
	const FGuid& TargetEnemyPartId,
	int32& OutFinalMagnitude)
{
	OutFinalMagnitude = 0;
	const FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(State, TargetEnemyPartId);
	if (!Part || Part->bDestroyed)
	{
		return false;
	}
	if (!FConditionResolver::Evaluate(
		State,
		Effect.Condition,
		SourceCardId,
		TargetEnemyPartId))
	{
		return false;
	}

	OutFinalMagnitude = EvaluateCardFinalMagnitude(
		State,
		Effect,
		RuntimeCost,
		TargetEnemyPartId,
		SourceCardId);
	return true;
}

void FBattleEffectSemanticsModule::BuildCardEnemyPartInvocations(
	const FBattleState& State,
	const FCardEffect& Effect,
	const int32 RuntimeCost,
	const FGuid& SourceCardId,
	const FGuid& SelectedEnemyPartId,
	TArray<FCardEnemyPartEffectInvocation>& OutInvocations)
{
	OutInvocations.Reset();
	const FEffectSemanticDescriptor* Semantics = FindDescriptor(Effect.EffectType);
	const EEffectTargetPlanKind TargetPlan = Semantics
		? Semantics->BuildCardTargetPlan(Effect.Target)
		: EEffectTargetPlanKind::None;

	auto AddInvocation = [&](const FGuid& TargetPartId)
	{
		int32 FinalMagnitude = 0;
		if (TryEvaluateCardEffectForEnemyPart(
			State,
			Effect,
			RuntimeCost,
			SourceCardId,
			TargetPartId,
			FinalMagnitude))
		{
			OutInvocations.Add({ TargetPartId, FinalMagnitude });
		}
	};

	if (TargetPlan == EEffectTargetPlanKind::SelectedEnemyPart)
	{
		AddInvocation(SelectedEnemyPartId);
	}
	else if (TargetPlan == EEffectTargetPlanKind::AllEnemyParts)
	{
		for (const FRuntimeEnemyPart& Part : State.Enemy.Parts)
		{
			AddInvocation(Part.InstanceId);
		}
	}
}
