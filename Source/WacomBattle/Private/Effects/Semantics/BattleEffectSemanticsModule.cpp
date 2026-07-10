// Copyright Wacom. All Rights Reserved.

#include "Effects/Semantics/BattleEffectSemanticsModule.h"

#include "Cards/BattleCardRuntimeStateModule.h"
#include "Core/BattleOperationAdapter.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Effects/ConditionResolver.h"
#include "Effects/EffectHandlers.h"
#include "Effects/Semantics/EffectSemanticTypes.h"
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
	using FEffectHandler = FEffectApplyResult (*)(FEffectExecutionContext&);
	using EAuthoringContext = FWacomBattleRuleContentContract::ECardEffectContext;

	enum class EEffectParameterRole : uint8
	{
		None,
		HandZone,
		CardLocation,
		StackStatus,
		CardKeyword,
	};

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

	enum class EEffectTargetPlanKind : uint8
	{
		None,
		Player,
		SourceCard,
		SelectedEnemyPart,
		AllEnemyParts,
		SelectedHandCard,
		LastShuffledCard,
		RandomHandCard,
		ZoneHandCard,
	};

	struct FEffectProjectionScratch
	{
		int32 SelectedHandCardCostModifierDelta = 0;
	};

	struct FEffectProjectionContext
	{
		const FBattleState& State;
		const FCardEffect& Effect;
		const FRuntimeCardInstance* TargetHandCard = nullptr;
		FGuid SelectedHandCardId;
		int32 Magnitude = 0;
	};

	bool IsSingleEnemyTargetAllowed(EAuthoringContext Context, ECardTargetMode CardTargetMode)
	{
		return Context == EAuthoringContext::PerfectRelease
			|| ((Context == EAuthoringContext::MainEffect
					|| Context == EAuthoringContext::ZoneHookOnPlay)
				&& CardTargetMode == ECardTargetMode::SingleEnemyPart);
	}

	bool IsActorTarget(const FGameplayTag& Target)
	{
		return Target == WacomTags::Target_Player
			|| Target == WacomTags::Target_Self
			|| Target == WacomTags::Target_SingleEnemyPart
			|| Target == WacomTags::Target_AllEnemyParts;
	}

	bool IsLiteralMagnitudeSource(const FGameplayTag& Source)
	{
		return !Source.IsValid() || Source == WacomTags::Magnitude_Source_Literal;
	}

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

	bool IsHandAnchor(const FBattleState& State, const FGuid& CardId)
	{
		return CardId.IsValid()
			&& (CardId == State.Cards.LeftHandInstanceId
				|| CardId == State.Cards.RightHandInstanceId);
	}

	bool IsNormalHandCardTarget(const FBattleState& State, const FGuid& CardId)
	{
		const FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardId);
		return Card && Card->Location == ECardLocation::Hand && !IsHandAnchor(State, CardId);
	}

	int32 ComputeRuntimeCostWithDelta(const FRuntimeCardInstance& Card, int32 ModifierDelta)
	{
		return FBattleCardRuntimeStateModule::EvaluateCostWithRuntimeModifierDelta(
			Card,
			ModifierDelta).EffectiveCost;
	}

	EEffectTargetPlanKind BuildGenericCardTargetPlan(const FGameplayTag& Target)
	{
		if (Target == WacomTags::Target_Player || Target == WacomTags::Target_Self)
		{
			return EEffectTargetPlanKind::Player;
		}
		if (Target == WacomTags::Target_SingleEnemyPart)
		{
			return EEffectTargetPlanKind::SelectedEnemyPart;
		}
		if (Target == WacomTags::Target_AllEnemyParts)
		{
			return EEffectTargetPlanKind::AllEnemyParts;
		}
		if (Target == WacomTags::Target_SelectedHandCard)
		{
			return EEffectTargetPlanKind::SelectedHandCard;
		}
		if (Target == WacomTags::Target_LastShuffledCard)
		{
			return EEffectTargetPlanKind::LastShuffledCard;
		}
		if (Target == WacomTags::Target_RandomHandCard)
		{
			return EEffectTargetPlanKind::RandomHandCard;
		}
		if (Target == WacomTags::Target_ZoneHandCard)
		{
			return EEffectTargetPlanKind::ZoneHandCard;
		}
		return EEffectTargetPlanKind::None;
	}

	class IEffectSemantics
	{
	public:
		IEffectSemantics(
			FEffectHandler InHandler,
			EBattleOperationDeterminism InDeterminism)
			: Handler(InHandler)
			, Determinism(InDeterminism)
		{
		}

		virtual ~IEffectSemantics() = default;

		FEffectHandler GetHandler() const { return Handler; }
		EBattleOperationDeterminism GetDeterminism() const { return Determinism; }

		virtual bool SupportsCardEffect() const { return true; }
		virtual bool SupportsEnemyIntentEffect() const { return false; }
		virtual bool SupportsCardTarget(
			const FGameplayTag& Target,
			EAuthoringContext Context,
			ECardTargetMode CardTargetMode) const = 0;
		virtual bool SupportsEnemyIntentTarget(const FGameplayTag& /*Target*/) const { return false; }

		virtual bool SupportsCardMagnitudeSource(const FGameplayTag& Source) const
		{
			return IsLiteralMagnitudeSource(Source);
		}

		virtual EEffectParameterRole GetParameterRole() const
		{
			return EEffectParameterRole::None;
		}
		virtual bool RequiresParameter() const { return false; }
		virtual bool SupportsNegativeCardMagnitude() const { return false; }
		virtual bool SupportsNegativeIntentMagnitude() const { return false; }
		virtual bool UsesPositiveMagnitude() const { return false; }

		virtual EEffectTargetPlanKind BuildCardTargetPlan(const FGameplayTag& Target) const
		{
			return BuildGenericCardTargetPlan(Target);
		}

		virtual FEffectParameters DecodeCardParameters(const FCardEffect& Effect) const
		{
			switch (GetParameterRole())
			{
			case EEffectParameterRole::CardLocation:
			{
				ECardLocation SourceLocation = ECardLocation::Draw;
				if (Effect.TargetZone == WacomTags::CardLocation_Discard)
				{
					SourceLocation = ECardLocation::Discard;
				}
				else if (Effect.TargetZone == WacomTags::CardLocation_Exhaust)
				{
					SourceLocation = ECardLocation::Exhaust;
				}
				FEffectParameters Parameters;
				Parameters.Emplace<FDrawSourceEffectParameters>();
				Parameters.Get<FDrawSourceEffectParameters>().SourceLocation = SourceLocation;
				return Parameters;
			}
			case EEffectParameterRole::HandZone:
			{
				EHandZone Zone = EHandZone::None;
				if (Effect.TargetZone == WacomTags::HandZone_Left)
				{
					Zone = EHandZone::Left;
				}
				else if (Effect.TargetZone == WacomTags::HandZone_Both)
				{
					Zone = EHandZone::Both;
				}
				else if (Effect.TargetZone == WacomTags::HandZone_Right)
				{
					Zone = EHandZone::Right;
				}
				FEffectParameters Parameters;
				Parameters.Emplace<FHandZoneEffectParameters>();
				Parameters.Get<FHandZoneEffectParameters>().Zone = Zone;
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

		virtual void ProjectTargetPreview(
			const FEffectProjectionContext& /*Context*/,
			FEffectProjectionScratch& /*Scratch*/,
			FBattleCardTargetPreviewEffect& /*OutEffect*/) const
		{
		}

	private:
		FEffectHandler Handler = nullptr;
		EBattleOperationDeterminism Determinism = EBattleOperationDeterminism::Unknown;
	};

	enum class EIntentActorTargetPolicy : uint8
	{
		None,
		Player,
		Self,
		PlayerOrSelf,
	};

	class FActorEffectSemantics final : public IEffectSemantics
	{
	public:
		FActorEffectSemantics(
			FEffectHandler Handler,
			EIntentActorTargetPolicy InIntentPolicy,
			bool bInSupportsRuntimeCost,
			bool bInSupportsTargetStatusStacks,
			bool bInUsesPositiveMagnitude,
			EEffectParameterRole InParameterRole = EEffectParameterRole::None,
			bool bInRequiresParameter = false)
			: IEffectSemantics(Handler, EBattleOperationDeterminism::Deterministic)
			, IntentPolicy(InIntentPolicy)
			, bSupportsRuntimeCost(bInSupportsRuntimeCost)
			, bSupportsTargetStatusStacks(bInSupportsTargetStatusStacks)
			, bPositiveMagnitude(bInUsesPositiveMagnitude)
			, ParameterRole(InParameterRole)
			, bRequiresParameter(bInRequiresParameter)
		{
		}

		virtual bool SupportsEnemyIntentEffect() const override
		{
			return IntentPolicy != EIntentActorTargetPolicy::None;
		}

		virtual bool SupportsCardTarget(
			const FGameplayTag& Target,
			EAuthoringContext Context,
			ECardTargetMode CardTargetMode) const override
		{
			if (Target == WacomTags::Target_SingleEnemyPart)
			{
				return IsSingleEnemyTargetAllowed(Context, CardTargetMode);
			}
			return IsActorTarget(Target);
		}

		virtual bool SupportsEnemyIntentTarget(const FGameplayTag& Target) const override
		{
			switch (IntentPolicy)
			{
			case EIntentActorTargetPolicy::Player:
				return Target == WacomTags::Target_Player;
			case EIntentActorTargetPolicy::Self:
				return Target == WacomTags::Target_Self;
			case EIntentActorTargetPolicy::PlayerOrSelf:
				return Target == WacomTags::Target_Player || Target == WacomTags::Target_Self;
			default:
				return false;
			}
		}

		virtual bool SupportsCardMagnitudeSource(const FGameplayTag& Source) const override
		{
			return IsLiteralMagnitudeSource(Source)
				|| (bSupportsRuntimeCost && Source == WacomTags::Magnitude_Source_RuntimeCost)
				|| (bSupportsTargetStatusStacks
					&& Source == WacomTags::Magnitude_Source_TargetStatusStacks);
		}

		virtual EEffectParameterRole GetParameterRole() const override { return ParameterRole; }
		virtual bool RequiresParameter() const override { return bRequiresParameter; }
		virtual bool UsesPositiveMagnitude() const override { return bPositiveMagnitude; }

	private:
		EIntentActorTargetPolicy IntentPolicy = EIntentActorTargetPolicy::None;
		bool bSupportsRuntimeCost = false;
		bool bSupportsTargetStatusStacks = false;
		bool bPositiveMagnitude = false;
		EEffectParameterRole ParameterRole = EEffectParameterRole::None;
		bool bRequiresParameter = false;
	};

	class FHealEffectSemantics final : public IEffectSemantics
	{
	public:
		FHealEffectSemantics()
			: IEffectSemantics(&WacomEffects::HandleHeal, EBattleOperationDeterminism::Deterministic)
		{
		}

		virtual bool SupportsCardTarget(
			const FGameplayTag& Target,
			EAuthoringContext,
			ECardTargetMode) const override
		{
			return Target == WacomTags::Target_Player || Target == WacomTags::Target_Self;
		}

		virtual bool SupportsCardMagnitudeSource(const FGameplayTag& Source) const override
		{
			return IsLiteralMagnitudeSource(Source)
				|| Source == WacomTags::Magnitude_Source_TargetStatusStacks;
		}

		virtual bool UsesPositiveMagnitude() const override { return true; }
	};

	class FModifyInitiativeEffectSemantics final : public IEffectSemantics
	{
	public:
		FModifyInitiativeEffectSemantics()
			: IEffectSemantics(
				&WacomEffects::HandleModifyInitiative,
				EBattleOperationDeterminism::Deterministic)
		{
		}

		virtual bool SupportsCardTarget(
			const FGameplayTag& Target,
			EAuthoringContext Context,
			ECardTargetMode CardTargetMode) const override
		{
			if (Target == WacomTags::Target_SingleEnemyPart)
			{
				return IsSingleEnemyTargetAllowed(Context, CardTargetMode);
			}
			return Target == WacomTags::Target_AllEnemyParts;
		}

		virtual bool SupportsCardMagnitudeSource(const FGameplayTag& Source) const override
		{
			return IsLiteralMagnitudeSource(Source)
				|| Source == WacomTags::Magnitude_Source_TargetStatusStacks;
		}

		virtual bool SupportsNegativeCardMagnitude() const override { return true; }
	};

	enum class EShuffleEffectKind : uint8
	{
		Random,
		FromBothToOther,
		SelfToRandomZone,
	};

	class FShuffleEffectSemantics final : public IEffectSemantics
	{
	public:
		FShuffleEffectSemantics(FEffectHandler Handler, EShuffleEffectKind InKind)
			: IEffectSemantics(Handler, EBattleOperationDeterminism::Random)
			, Kind(InKind)
		{
		}

		virtual bool SupportsCardTarget(
			const FGameplayTag& Target,
			EAuthoringContext,
			ECardTargetMode) const override
		{
			switch (Kind)
			{
			case EShuffleEffectKind::Random:
				return Target == WacomTags::Target_RandomHandCard;
			case EShuffleEffectKind::FromBothToOther:
				return Target == WacomTags::Target_ZoneHandCard;
			case EShuffleEffectKind::SelfToRandomZone:
				return Target == WacomTags::Target_Self;
			default:
				return false;
			}
		}

		virtual EEffectParameterRole GetParameterRole() const override
		{
			return Kind == EShuffleEffectKind::FromBothToOther
				? EEffectParameterRole::HandZone
				: EEffectParameterRole::None;
		}

		virtual bool RequiresParameter() const override
		{
			return Kind == EShuffleEffectKind::FromBothToOther;
		}

		virtual EEffectTargetPlanKind BuildCardTargetPlan(const FGameplayTag& Target) const override
		{
			if (Kind == EShuffleEffectKind::SelfToRandomZone && Target == WacomTags::Target_Self)
			{
				return EEffectTargetPlanKind::SourceCard;
			}
			return IEffectSemantics::BuildCardTargetPlan(Target);
		}

	private:
		EShuffleEffectKind Kind = EShuffleEffectKind::Random;
	};

	class FCardCostEffectSemantics final : public IEffectSemantics
	{
	public:
		FCardCostEffectSemantics(FEffectHandler Handler, int32 InDirection)
			: IEffectSemantics(Handler, EBattleOperationDeterminism::Deterministic)
			, Direction(InDirection)
		{
		}

		virtual bool SupportsCardTarget(
			const FGameplayTag& Target,
			EAuthoringContext,
			ECardTargetMode CardTargetMode) const override
		{
			if (Target == WacomTags::Target_SelectedHandCard)
			{
				return CardTargetMode == ECardTargetMode::HandCard;
			}
			return Target == WacomTags::Target_Self
				|| Target == WacomTags::Target_LastShuffledCard;
		}

		virtual bool SupportsCardMagnitudeSource(const FGameplayTag& Source) const override
		{
			return IsLiteralMagnitudeSource(Source)
				|| Source == WacomTags::Magnitude_Source_TargetStatusStacks;
		}

		virtual bool UsesPositiveMagnitude() const override { return true; }

		virtual EEffectTargetPlanKind BuildCardTargetPlan(const FGameplayTag& Target) const override
		{
			return Target == WacomTags::Target_Self
				? EEffectTargetPlanKind::SourceCard
				: IEffectSemantics::BuildCardTargetPlan(Target);
		}

		virtual void ProjectTargetPreview(
			const FEffectProjectionContext& Context,
			FEffectProjectionScratch& Scratch,
			FBattleCardTargetPreviewEffect& OutEffect) const override
		{
			if (Context.Effect.Target != WacomTags::Target_SelectedHandCard
				|| !Context.TargetHandCard)
			{
				return;
			}

			OutEffect.bHasTargetHandCardCostPreview = true;
			OutEffect.TargetHandCardRuntimeCostBefore =
				ComputeRuntimeCostWithDelta(*Context.TargetHandCard, Scratch.SelectedHandCardCostModifierDelta);
			Scratch.SelectedHandCardCostModifierDelta += Direction * Context.Magnitude;
			OutEffect.TargetHandCardRuntimeCostAfter =
				ComputeRuntimeCostWithDelta(*Context.TargetHandCard, Scratch.SelectedHandCardCostModifierDelta);
		}

	private:
		int32 Direction = 1;
	};

	class FSelectedCardMoveEffectSemantics final : public IEffectSemantics
	{
	public:
		FSelectedCardMoveEffectSemantics(FEffectHandler Handler, bool bInExhaust)
			: IEffectSemantics(Handler, EBattleOperationDeterminism::Deterministic)
			, bExhaust(bInExhaust)
		{
		}

		virtual bool SupportsCardTarget(
			const FGameplayTag& Target,
			EAuthoringContext,
			ECardTargetMode CardTargetMode) const override
		{
			return Target == WacomTags::Target_SelectedHandCard
				&& CardTargetMode == ECardTargetMode::HandCard;
		}

		virtual bool SupportsCardMagnitudeSource(const FGameplayTag& Source) const override
		{
			return IsLiteralMagnitudeSource(Source)
				|| Source == WacomTags::Magnitude_Source_TargetStatusStacks;
		}

		virtual bool UsesPositiveMagnitude() const override { return true; }

		virtual void ProjectTargetPreview(
			const FEffectProjectionContext& Context,
			FEffectProjectionScratch&,
			FBattleCardTargetPreviewEffect& OutEffect) const override
		{
			if (Context.Effect.Target != WacomTags::Target_SelectedHandCard
				|| !Context.TargetHandCard)
			{
				return;
			}
			if (!IsNormalHandCardTarget(Context.State, Context.SelectedHandCardId))
			{
				OutEffect.bSkipped = true;
				OutEffect.SkipReason = EWacomBattleCardPreviewEffectSkipReason::UnsupportedTarget;
				return;
			}
			if (bExhaust)
			{
				OutEffect.bWouldExhaustTargetHandCard = true;
			}
			else
			{
				OutEffect.bWouldDiscardTargetHandCard = true;
			}
		}

	private:
		bool bExhaust = false;
	};

	enum class ESimpleCardEffectKind : uint8
	{
		Draw,
		Discard,
		ExhaustSelf,
	};

	class FSimpleCardEffectSemantics final : public IEffectSemantics
	{
	public:
		FSimpleCardEffectSemantics(
			FEffectHandler Handler,
			EBattleOperationDeterminism Determinism,
			ESimpleCardEffectKind InKind)
			: IEffectSemantics(Handler, Determinism)
			, Kind(InKind)
		{
		}

		virtual bool SupportsCardTarget(
			const FGameplayTag& Target,
			EAuthoringContext,
			ECardTargetMode) const override
		{
			return !Target.IsValid()
				|| Target == WacomTags::Target_Player
				|| Target == WacomTags::Target_Self;
		}

		virtual bool SupportsCardMagnitudeSource(const FGameplayTag& Source) const override
		{
			if (Kind == ESimpleCardEffectKind::Draw)
			{
				return IsLiteralMagnitudeSource(Source)
					|| Source == WacomTags::Magnitude_Source_RuntimeCost;
			}
			return IsLiteralMagnitudeSource(Source);
		}

		virtual EEffectParameterRole GetParameterRole() const override
		{
			return Kind == ESimpleCardEffectKind::Draw
				? EEffectParameterRole::CardLocation
				: EEffectParameterRole::None;
		}

		virtual bool UsesPositiveMagnitude() const override
		{
			return Kind != ESimpleCardEffectKind::ExhaustSelf;
		}

	private:
		ESimpleCardEffectKind Kind = ESimpleCardEffectKind::Draw;
	};

	class FGainKeywordEffectSemantics final : public IEffectSemantics
	{
	public:
		FGainKeywordEffectSemantics()
			: IEffectSemantics(
				&WacomEffects::HandleGainKeyword,
				EBattleOperationDeterminism::Deterministic)
		{
		}

		virtual bool SupportsCardTarget(
			const FGameplayTag& Target,
			EAuthoringContext,
			ECardTargetMode CardTargetMode) const override
		{
			if (Target == WacomTags::Target_SelectedHandCard)
			{
				return CardTargetMode == ECardTargetMode::HandCard;
			}
			return Target == WacomTags::Target_LastShuffledCard;
		}

		virtual EEffectParameterRole GetParameterRole() const override
		{
			return EEffectParameterRole::CardKeyword;
		}
		virtual bool RequiresParameter() const override { return true; }

		virtual void ProjectTargetPreview(
			const FEffectProjectionContext& Context,
			FEffectProjectionScratch&,
			FBattleCardTargetPreviewEffect& OutEffect) const override
		{
			if (Context.Effect.Target == WacomTags::Target_SelectedHandCard
				&& Context.TargetHandCard
				&& Context.Effect.TargetZone.IsValid())
			{
				OutEffect.bWouldGainTargetHandCardKeyword = true;
				OutEffect.TargetHandCardKeyword = Context.Effect.TargetZone;
			}
		}
	};

	const TMap<FGameplayTag, const IEffectSemantics*>& GetSemanticsRegistry()
	{
		static const FActorEffectSemantics Damage(
			&WacomEffects::HandleDamage,
			EIntentActorTargetPolicy::Player,
			/*RuntimeCost*/true,
			/*TargetStatus*/true,
			/*Positive*/true);
		static const FActorEffectSemantics Shield(
			&WacomEffects::HandleShield,
			EIntentActorTargetPolicy::Self,
			/*RuntimeCost*/false,
			/*TargetStatus*/true,
			/*Positive*/true);
		static const FActorEffectSemantics Poison(
			&WacomEffects::HandleApplyPoison,
			EIntentActorTargetPolicy::PlayerOrSelf,
			/*RuntimeCost*/true,
			/*TargetStatus*/true,
			/*Positive*/true);
		static const FActorEffectSemantics Slow(
			&WacomEffects::HandleApplySlow,
			EIntentActorTargetPolicy::PlayerOrSelf,
			/*RuntimeCost*/false,
			/*TargetStatus*/true,
			/*Positive*/true);
		static const FActorEffectSemantics Freeze(
			&WacomEffects::HandleApplyFreeze,
			EIntentActorTargetPolicy::PlayerOrSelf,
			/*RuntimeCost*/false,
			/*TargetStatus*/true,
			/*Positive*/true);
		static const FActorEffectSemantics Twilight(
			&WacomEffects::HandleApplyTwilight,
			EIntentActorTargetPolicy::PlayerOrSelf,
			/*RuntimeCost*/false,
			/*TargetStatus*/true,
			/*Positive*/true);
		static const FActorEffectSemantics RemoveStatus(
			&WacomEffects::HandleRemoveStatus,
			EIntentActorTargetPolicy::None,
			/*RuntimeCost*/false,
			/*TargetStatus*/true,
			/*Positive*/true,
			EEffectParameterRole::StackStatus,
			/*RequiresParameter*/true);
		static const FHealEffectSemantics Heal;
		static const FModifyInitiativeEffectSemantics ModifyInitiative;
		static const FShuffleEffectSemantics ShuffleRandom(
			&WacomEffects::HandleShuffleRandom,
			EShuffleEffectKind::Random);
		static const FShuffleEffectSemantics ShuffleFromBoth(
			&WacomEffects::HandleShuffleFromBothToOther,
			EShuffleEffectKind::FromBothToOther);
		static const FShuffleEffectSemantics ShuffleSelf(
			&WacomEffects::HandleShuffleToRandomZone,
			EShuffleEffectKind::SelfToRandomZone);
		static const FCardCostEffectSemantics AddCost(&WacomEffects::HandleCardAddCost, +1);
		static const FCardCostEffectSemantics ReduceCost(&WacomEffects::HandleCardReduceCost, -1);
		static const FSelectedCardMoveEffectSemantics DiscardSelected(
			&WacomEffects::HandleCardDiscardSelected,
			/*bExhaust*/false);
		static const FSelectedCardMoveEffectSemantics ExhaustSelected(
			&WacomEffects::HandleCardExhaustSelected,
			/*bExhaust*/true);
		static const FSimpleCardEffectSemantics Draw(
			&WacomEffects::HandleDraw,
			EBattleOperationDeterminism::Random,
			ESimpleCardEffectKind::Draw);
		static const FSimpleCardEffectSemantics Discard(
			&WacomEffects::HandleDiscard,
			EBattleOperationDeterminism::Random,
			ESimpleCardEffectKind::Discard);
		static const FSimpleCardEffectSemantics ExhaustSelf(
			&WacomEffects::HandleExhaustSelf,
			EBattleOperationDeterminism::Deterministic,
			ESimpleCardEffectKind::ExhaustSelf);
		static const FGainKeywordEffectSemantics GainKeyword;

		static const TMap<FGameplayTag, const IEffectSemantics*> Registry = {
			{ WacomTags::Effect_Damage, &Damage },
			{ WacomTags::Status_Shield, &Shield },
			{ WacomTags::Effect_ApplyStatus_Poison, &Poison },
			{ WacomTags::Effect_ApplyStatus_Slow, &Slow },
			{ WacomTags::Effect_ApplyStatus_Freeze, &Freeze },
			{ WacomTags::Effect_ApplyStatus_Twilight, &Twilight },
			{ WacomTags::Effect_Shuffle_Random, &ShuffleRandom },
			{ WacomTags::Effect_Shuffle_FromBothToOther, &ShuffleFromBoth },
			{ WacomTags::Effect_Shuffle_ToRandomZone, &ShuffleSelf },
			{ WacomTags::Effect_Card_AddCost, &AddCost },
			{ WacomTags::Effect_Card_ReduceCost, &ReduceCost },
			{ WacomTags::Effect_Card_DiscardSelected, &DiscardSelected },
			{ WacomTags::Effect_Card_ExhaustSelected, &ExhaustSelected },
			{ WacomTags::Effect_Draw, &Draw },
			{ WacomTags::Effect_Discard, &Discard },
			{ WacomTags::Effect_ExhaustSelf, &ExhaustSelf },
			{ WacomTags::Effect_Heal, &Heal },
			{ WacomTags::Effect_GainKeyword, &GainKeyword },
			{ WacomTags::Effect_RemoveStatus, &RemoveStatus },
			{ WacomTags::Effect_ModifyInitiative, &ModifyInitiative },
		};
		return Registry;
	}

	const IEffectSemantics* FindSemantics(const FGameplayTag& EffectType)
	{
		return GetSemanticsRegistry().FindRef(EffectType);
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
		const IEffectSemantics* Semantics,
		const FCardEffectChainBindings& Bindings,
		IBattleOperationAdapter* OperationAdapter)
	{
		Context.State = &State;
		Context.Events = &Events;
		Context.SourceKind = EEffectSourceKind::Card;
		Context.SourceInstanceId = Bindings.SourceCardId;
		Context.EffectTag = Effect.EffectType;
		Context.Magnitude = Magnitude;
		Context.Duration = Effect.Duration;
		Context.Parameters = Semantics
			? Semantics->DecodeCardParameters(Effect)
			: FEffectParameters{};
		Context.ExcludeHandCardId = Bindings.SourceCardId;
		Context.OperationAdapter = OperationAdapter;
	}

	bool ShouldExecuteInvocation(
		const IEffectSemantics* Semantics,
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
		int32 Magnitude,
		const IEffectSemantics* Semantics,
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
			Magnitude,
			Semantics,
			Bindings,
			OperationAdapter);

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
	for (const FCardEffect& Effect : Effects)
	{
		const IEffectSemantics* Semantics = FindSemantics(Effect.EffectType);
		const EEffectTargetPlanKind TargetPlan = Semantics
			? Semantics->BuildCardTargetPlan(Effect.Target)
			: BuildGenericCardTargetPlan(Effect.Target);
		const int32 FinalMagnitude = FBattleEffectSemanticsModule::EvaluateCardFinalMagnitude(
			*Chain.State,
			Effect,
			Chain.Bindings.RuntimeCost,
			Chain.Bindings.SelectedEnemyPartId,
			Chain.Bindings.SourceCardId);

		if (TargetPlan == EEffectTargetPlanKind::AllEnemyParts)
		{
			for (FRuntimeEnemyPart& Part : Chain.State->Enemy.Parts)
			{
				if (Part.bDestroyed)
				{
					continue;
				}
				if (!FConditionResolver::Evaluate(
					*Chain.State,
					Effect.Condition,
					Chain.Bindings.SourceCardId,
					Part.InstanceId))
				{
					continue;
				}
				ExecuteCardInvocation(
					*Chain.State,
					*Chain.Events,
					Effect,
					FinalMagnitude,
					Semantics,
					TargetPlan,
					Chain.Bindings,
					Chain.LastShuffledCardId,
					Chain.ShuffledCardIds,
					Chain.OperationAdapter,
					Part.InstanceId);
			}
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

		ExecuteCardInvocation(
			*Chain.State,
			*Chain.Events,
			Effect,
			FinalMagnitude,
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
		const IEffectSemantics* Semantics = FindSemantics(Effect.EffectType);
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
		if (!FConditionResolver::Evaluate(
			State,
			Effect.Condition,
			Bindings.SourceCardId,
			Bindings.SelectedEnemyPartId))
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
		EffectPreview.Magnitude = EvaluateCardFinalMagnitude(
			State,
			Effect,
			Bindings.RuntimeCost,
			Bindings.SelectedEnemyPartId,
			Bindings.SourceCardId);
		EffectPreview.bHasMagnitude = true;

		if (Effect.Target == WacomTags::Target_SelectedHandCard && TargetHandCard)
		{
			EffectPreview.bTargetsSelectedHandCard = true;
			if (const IEffectSemantics* Semantics = FindSemantics(Effect.EffectType))
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

bool FBattleEffectSemanticsModule::IsSupportedCardEffectType(const FGameplayTag& EffectType)
{
	const IEffectSemantics* Semantics = FindSemantics(EffectType);
	return Semantics && Semantics->SupportsCardEffect();
}

bool FBattleEffectSemanticsModule::IsSupportedEnemyIntentEffectType(const FGameplayTag& EffectType)
{
	const IEffectSemantics* Semantics = FindSemantics(EffectType);
	return Semantics && Semantics->SupportsEnemyIntentEffect();
}

bool FBattleEffectSemanticsModule::IsSupportedMagnitudeSource(const FGameplayTag& MagnitudeSource)
{
	return IsLiteralMagnitudeSource(MagnitudeSource)
		|| MagnitudeSource == WacomTags::Magnitude_Source_RuntimeCost
		|| MagnitudeSource == WacomTags::Magnitude_Source_HandCount
		|| MagnitudeSource == WacomTags::Magnitude_Source_TargetStatusStacks;
}

bool FBattleEffectSemanticsModule::IsSupportedCardEffectMagnitudeSource(
	const FGameplayTag& EffectType,
	const FGameplayTag& MagnitudeSource)
{
	const IEffectSemantics* Semantics = FindSemantics(EffectType);
	return Semantics && Semantics->SupportsCardMagnitudeSource(MagnitudeSource);
}

bool FBattleEffectSemanticsModule::IsSupportedCardEffectTarget(
	const FGameplayTag& EffectType,
	const FGameplayTag& Target,
	EAuthoringContext Context,
	ECardTargetMode CardTargetMode)
{
	const IEffectSemantics* Semantics = FindSemantics(EffectType);
	return Semantics && Semantics->SupportsCardTarget(Target, Context, CardTargetMode);
}

bool FBattleEffectSemanticsModule::IsSupportedEnemyIntentEffectTarget(
	const FGameplayTag& EffectType,
	const FGameplayTag& Target)
{
	const IEffectSemantics* Semantics = FindSemantics(EffectType);
	return Semantics && Semantics->SupportsEnemyIntentTarget(Target);
}

bool FBattleEffectSemanticsModule::CardEffectRequiresTargetZone(const FGameplayTag& EffectType)
{
	const IEffectSemantics* Semantics = FindSemantics(EffectType);
	return Semantics && Semantics->RequiresParameter();
}

bool FBattleEffectSemanticsModule::CardEffectAllowsTargetZone(const FGameplayTag& EffectType)
{
	const IEffectSemantics* Semantics = FindSemantics(EffectType);
	return Semantics && Semantics->GetParameterRole() != EEffectParameterRole::None;
}

bool FBattleEffectSemanticsModule::CardEffectTargetZoneMustBeHandZone(const FGameplayTag& EffectType)
{
	const IEffectSemantics* Semantics = FindSemantics(EffectType);
	return Semantics && Semantics->GetParameterRole() == EEffectParameterRole::HandZone;
}

bool FBattleEffectSemanticsModule::CardEffectTargetZoneMustBeCardLocation(const FGameplayTag& EffectType)
{
	const IEffectSemantics* Semantics = FindSemantics(EffectType);
	return Semantics && Semantics->GetParameterRole() == EEffectParameterRole::CardLocation;
}

bool FBattleEffectSemanticsModule::CardEffectTargetZoneMustBeStackStatus(const FGameplayTag& EffectType)
{
	const IEffectSemantics* Semantics = FindSemantics(EffectType);
	return Semantics && Semantics->GetParameterRole() == EEffectParameterRole::StackStatus;
}

bool FBattleEffectSemanticsModule::CardEffectTargetZoneMustBeCardKeyword(const FGameplayTag& EffectType)
{
	const IEffectSemantics* Semantics = FindSemantics(EffectType);
	return Semantics && Semantics->GetParameterRole() == EEffectParameterRole::CardKeyword;
}

bool FBattleEffectSemanticsModule::CardEffectSupportsNegativeMagnitude(const FGameplayTag& EffectType)
{
	const IEffectSemantics* Semantics = FindSemantics(EffectType);
	return Semantics && Semantics->SupportsNegativeCardMagnitude();
}

bool FBattleEffectSemanticsModule::EnemyIntentEffectSupportsNegativeMagnitude(const FGameplayTag& EffectType)
{
	const IEffectSemantics* Semantics = FindSemantics(EffectType);
	return Semantics && Semantics->SupportsNegativeIntentMagnitude();
}

bool FBattleEffectSemanticsModule::EffectUsesPositiveMagnitude(const FGameplayTag& EffectType)
{
	const IEffectSemantics* Semantics = FindSemantics(EffectType);
	return Semantics && Semantics->UsesPositiveMagnitude();
}
