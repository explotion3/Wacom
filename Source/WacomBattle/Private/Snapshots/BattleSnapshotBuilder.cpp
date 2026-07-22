// Copyright Wacom. All Rights Reserved.

#include "Snapshots/BattleSnapshotBuilder.h"
#include "Cards/BattleCardRuntimeStateModule.h"
#include "Combatants/BattleCombatantMutationModule.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Hand/HandZoneService.h"
#include "Snapshots/BattleSnapshot.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Resolution/BattleResistanceEvaluator.h"
#include "Cards/CardDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Enemies/IntentDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "Statuses/BattleStatusSemanticsModule.h"

namespace
{
	FEnemyPartSnapshot BuildEnemyPartSnapshot(const FRuntimeEnemyPart& Part)
	{
		FEnemyPartSnapshot PartSnap;
		PartSnap.InstanceId        = Part.InstanceId;
		PartSnap.Definition        = Part.Definition;
		PartSnap.Identity          = Part.Identity;
		PartSnap.PartKey           = Part.Identity.ToEnemyPartKey();
		PartSnap.EncounterId       = Part.Identity.GetEffectiveEncounterId();
		PartSnap.EnemySlotId       = Part.Identity.GetEffectiveEnemySlotId();
		PartSnap.PartSlotId        = Part.Identity.GetEffectivePartSlotId();
		PartSnap.CurrentPhaseId    = Part.CurrentPhaseId;
		PartSnap.CurrentIntentSetId = Part.CurrentIntentSetId;
		PartSnap.CurrentIntentId   = Part.CurrentIntentId;
		PartSnap.CurrentHp         = Part.CurrentHp;
		PartSnap.MaxHp             = Part.Definition ? Part.Definition->MaxHp : 0;
		PartSnap.CurrentInitiative = Part.CurrentInitiative;
		PartSnap.Shield            = Part.Shield;
		PartSnap.bDestroyed        = Part.bDestroyed;
		PartSnap.Statuses          = FBattleCombatantStatusFacts::BuildTagProjection(Part.StatusStacks);
		PartSnap.StatusStacks      = Part.StatusStacks;

		if (!Part.CurrentIntentId.IsNone())
		{
			const FIntentDefinition& IntentDef = Part.CurrentIntent;
			PartSnap.CurrentIntent.IntentId    = IntentDef.IntentId;
			PartSnap.CurrentIntent.DisplayName = IntentDef.DisplayName;
			PartSnap.CurrentIntent.Initiative  = IntentDef.Initiative;
			PartSnap.CurrentIntent.PeakAttackDamage =
				FBattleResistanceEvaluator::EvaluateIntentPeakAttackDamage(IntentDef);
			PartSnap.CurrentIntent.bIsAttackIntent =
				PartSnap.CurrentIntent.PeakAttackDamage > 0;
		}

		return PartSnap;
	}

	void AddPartToEnemySnapshot(FEnemySnapshot& EnemySnap, const FEnemyPartSnapshot& PartSnap)
	{
		if (!PartSnap.bDestroyed)
		{
			EnemySnap.InitiativeSum += PartSnap.CurrentInitiative;
			EnemySnap.bAllPartsDestroyed = false;
		}
		EnemySnap.Parts.Add(PartSnap);
	}

	int32 ComputeRuntimeCost(const FRuntimeCardInstance& Card)
	{
		return FBattleRules::ComputeRuntimeCost(Card);
	}

	const FRuntimeCardInstance* FindCard(const FBattleState& State, const FGuid& InstanceId)
	{
		return FBattleRules::FindCard(State, InstanceId);
	}

	bool HasSwiftKeyword(const FRuntimeCardInstance& Card)
	{
		return (Card.Definition && Card.Definition->Keywords.HasTag(WacomTags::Card_Keyword_Swift))
			|| Card.TemporaryKeywords.HasTag(WacomTags::Card_Keyword_Swift);
	}
}

FBattleSnapshot FBattleSnapshotBuilder::Build(const FBattleState& State)
{
	FBattleSnapshot Out;
	Out.Version          = State.StateVersion;
	Out.Phase            = State.Phase;
	Out.TurnNumber       = State.TurnNumber;
	Out.CurrentWaitValue = State.CurrentWaitValue;
	Out.CompanionPlayedCount = State.Player.CompanionPlayedCount;
	Out.Outcome          = State.Outcome;
	Out.EncounterId      = State.Enemy.EncounterId;

	// ---- Player ----
	Out.Player.CurrentHp = State.Player.CurrentHp;
	Out.Player.MaxHp     = State.Player.MaxHp;
	Out.Player.Shield    = State.Player.Shield;
	Out.Player.StatusStacks = State.Player.StatusStacks;
	FBattleStatusSemanticsModule::ProjectPendingPlayerStatuses(
		State,
		Out.Player.StatusStacks);
	Out.Player.Statuses  = FBattleCombatantStatusFacts::BuildTagProjection(Out.Player.StatusStacks);

	// ---- Enemies ----
	Out.Enemies.Reserve(State.Enemy.EnemySlots.Num());
	for (const FEnemySlotState& EnemySlot : State.Enemy.EnemySlots)
	{
		FEnemySnapshot EnemySnap;
		EnemySnap.Definition = EnemySlot.Definition;
		EnemySnap.EncounterId = EnemySlot.EncounterId;
		EnemySnap.EnemySlotId = EnemySlot.EnemySlotId;
		EnemySnap.UnitKey = FBattleEnemyUnitKey::Make(EnemySlot.EncounterId, EnemySlot.EnemySlotId);
		EnemySnap.Parts.Reserve(EnemySlot.PartInstanceIds.Num());
		EnemySnap.bAllPartsDestroyed = !EnemySlot.PartInstanceIds.IsEmpty();

		for (const FGuid& PartInstanceId : EnemySlot.PartInstanceIds)
		{
			if (const FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(State, PartInstanceId))
			{
				AddPartToEnemySnapshot(EnemySnap, BuildEnemyPartSnapshot(*Part));
			}
		}

		Out.Enemies.Add(MoveTemp(EnemySnap));
	}

	// ---- Hand ----
	Out.Hand.Cards.Reserve(State.Cards.Hand.Num());
	Out.Hand.bLeftHandPresent  = false;
	Out.Hand.bRightHandPresent = false;
	Out.Hand.NormalCardCount   = 0;
	Out.Hand.NormalCardLimit   = 10;

	for (const FGuid& CardId : State.Cards.Hand)
	{
		const FRuntimeCardInstance* Card = FindCard(State, CardId);
		if (!Card)
		{
			continue;
		}

		FHandCardSnapshot HandCard;
		HandCard.InstanceId    = Card->InstanceId;
		HandCard.Definition    = Card->Definition;
		HandCard.RuntimeCost   = ComputeRuntimeCost(*Card);
		HandCard.bIsCostLegal  = FBattleCardRuntimeStateModule::IsCostLegal(State, *Card);
		HandCard.Statuses      = FBattleCardRuntimeStateModule::BuildStatusProjection(*Card);
		HandCard.StatusStacks  = Card->StatusStacks;
		HandCard.bIsFrozen     = FBattleCardRuntimeStateModule::IsFrozen(*Card);
		HandCard.Zone          = FHandZoneService::GetZoneOf(State, CardId);
		HandCard.bIsHandAnchor = FHandZoneService::IsHandAnchor(State, CardId);
		HandCard.bIsPlayable   = HandCard.bIsCostLegal && !HandCard.bIsFrozen;
		HandCard.bIsSwift      = HasSwiftKeyword(*Card);

		if (CardId == State.Cards.LeftHandInstanceId)  { Out.Hand.bLeftHandPresent = true; }
		if (CardId == State.Cards.RightHandInstanceId) { Out.Hand.bRightHandPresent = true; }
		if (!HandCard.bIsHandAnchor)             { ++Out.Hand.NormalCardCount; }

		Out.Hand.Cards.Add(MoveTemp(HandCard));
	}

	// ---- Pile counts ----
	Out.PileCounts.DrawCount    = State.Cards.DrawPile.Num();
	Out.PileCounts.PlayedCount  = State.Cards.PlayedPile.Num();
	Out.PileCounts.DiscardCount = State.Cards.DiscardPile.Num();
	Out.PileCounts.ExhaustCount = State.Cards.ExhaustPile.Num();

	return Out;
}
