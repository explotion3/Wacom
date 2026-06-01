// Copyright Wacom. All Rights Reserved.

#include "Snapshots/BattleSnapshotBuilder.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Hand/HandZoneService.h"
#include "Snapshots/BattleSnapshot.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Cards/CardDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Enemies/IntentDefinition.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
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

	// ---- Player ----
	Out.Player.CurrentHp = State.Player.CurrentHp;
	Out.Player.MaxHp     = State.Player.MaxHp;
	Out.Player.Shield    = State.Player.Shield;
	Out.Player.Statuses  = State.Player.Statuses;
	Out.Player.StatusStacks = State.Player.StatusStacks;

	// ---- Enemy ----
	Out.Enemy.Definition = State.Enemy.Definition;
	Out.Enemy.Parts.Reserve(State.Enemy.Parts.Num());

	int32 InitiativeSum = 0;
	bool bAllDestroyed  = !State.Enemy.Parts.IsEmpty();

	for (const FRuntimeEnemyPart& Part : State.Enemy.Parts)
	{
		FEnemyPartSnapshot PartSnap;
		PartSnap.InstanceId        = Part.InstanceId;
		PartSnap.Definition        = Part.Definition;
		PartSnap.CurrentHp         = Part.CurrentHp;
		PartSnap.MaxHp             = Part.Definition ? Part.Definition->MaxHp : 0;
		PartSnap.CurrentInitiative = Part.CurrentInitiative;
		PartSnap.Shield            = Part.Shield;
		PartSnap.bDestroyed        = Part.bDestroyed;
		PartSnap.Statuses          = Part.Statuses;
		PartSnap.StatusStacks      = Part.StatusStacks;

		// CurrentIntent：从 IntentSequence[CurrentIntentIndex] 读取。
		if (Part.Definition && Part.Definition->IntentSequence.IsValidIndex(Part.CurrentIntentIndex))
		{
			const FIntentDefinition& IntentDef = Part.Definition->IntentSequence[Part.CurrentIntentIndex];
			PartSnap.CurrentIntent.IntentId        = IntentDef.IntentId;
			PartSnap.CurrentIntent.DisplayName     = IntentDef.DisplayName;
			PartSnap.CurrentIntent.Initiative      = IntentDef.Initiative;
			PartSnap.CurrentIntent.ResistanceValue = IntentDef.ResistanceValue;
		}

		if (!Part.bDestroyed)
		{
			InitiativeSum += Part.CurrentInitiative;
			bAllDestroyed = false;
		}

		Out.Enemy.Parts.Add(MoveTemp(PartSnap));
	}
	Out.Enemy.InitiativeSum      = InitiativeSum;
	Out.Enemy.bAllPartsDestroyed = bAllDestroyed;

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
		HandCard.Zone          = FHandZoneService::GetZoneOf(State, CardId);
		HandCard.bIsHandAnchor = FHandZoneService::IsHandAnchor(State, CardId);
		HandCard.bIsPlayable   = FBattleRules::IsCardCostLegal(State, *Card);
		HandCard.bIsSwift      = HasSwiftKeyword(*Card);

		if (CardId == State.Cards.LeftHandInstanceId)  { Out.Hand.bLeftHandPresent = true; }
		if (CardId == State.Cards.RightHandInstanceId) { Out.Hand.bRightHandPresent = true; }
		if (!HandCard.bIsHandAnchor)             { ++Out.Hand.NormalCardCount; }

		Out.Hand.Cards.Add(MoveTemp(HandCard));
	}

	// ---- Pile counts ----
	Out.PileCounts.DrawCount    = State.Cards.DrawPile.Num();
	Out.PileCounts.DiscardCount = State.Cards.DiscardPile.Num();
	Out.PileCounts.ExhaustCount = State.Cards.ExhaustPile.Num();

	return Out;
}
