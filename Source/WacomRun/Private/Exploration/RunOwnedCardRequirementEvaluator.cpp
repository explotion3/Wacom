// Copyright Wacom. All Rights Reserved.

#include "Exploration/RunOwnedCardRequirementEvaluator.h"

#include "Cards/CardDefinition.h"
#include "Map/WacomMapTypes.h"
#include "RunState.h"

namespace
{
	bool MatchesRequirement(
		const FCardInstance& Card,
		const FWacomOwnedCardRequirement& Requirement)
	{
		const UCardDefinition* Definition = Card.Definition;
		if (!Definition)
		{
			return false;
		}

		const bool bHasDefinitionWhitelist =
			!Requirement.AllowedCardDefinitions.IsEmpty()
			|| !Requirement.AllowedCardIds.IsEmpty();
		if (bHasDefinitionWhitelist)
		{
			const bool bDefinitionAllowed = Requirement.AllowedCardDefinitions.Contains(Definition);
			const bool bCardIdAllowed = Requirement.AllowedCardIds.ContainsByPredicate(
				[Definition](const FName AllowedCardId)
				{
					return Definition->MatchesCardIdOrUpgradeFamily(AllowedCardId);
				});
			if (!bDefinitionAllowed && !bCardIdAllowed)
			{
				return false;
			}
		}

		if (!Definition->Keywords.HasAll(Requirement.RequiredKeywords))
		{
			return false;
		}
		return !Definition->Keywords.HasAny(Requirement.BlockedKeywords);
	}

	template <typename TVisitor>
	bool VisitOwnedCards(const FRunState& State, TVisitor&& Visitor)
	{
		auto VisitPile = [&Visitor](const TArray<FCardInstance>& Pile)
		{
			for (const FCardInstance& Card : Pile)
			{
				if (Visitor(Card))
				{
					return true;
				}
			}
			return false;
		};

		if (VisitPile(State.Backpack)
			|| VisitPile(State.BattleDeck)
			|| VisitPile(State.BurdenZone))
		{
			return true;
		}
		for (const FSpecialZone& Zone : State.SpecialZones)
		{
			if (VisitPile(Zone.Cards))
			{
				return true;
			}
		}
		return false;
	}
}

bool FRunOwnedCardRequirementEvaluator::AreAllSatisfied(
	const FRunState& State,
	const TArray<FWacomOwnedCardRequirement>& Requirements)
{
	for (const FWacomOwnedCardRequirement& Requirement : Requirements)
	{
		if (!Requirement.HasPositiveFilter())
		{
			return false;
		}
		const bool bSatisfied = VisitOwnedCards(
			State,
			[&Requirement](const FCardInstance& Card)
			{
				return MatchesRequirement(Card, Requirement);
			});
		if (!bSatisfied)
		{
			return false;
		}
	}
	return true;
}
