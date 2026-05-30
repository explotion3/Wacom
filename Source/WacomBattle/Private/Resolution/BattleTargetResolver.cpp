// Copyright Wacom. All Rights Reserved.

#include "BattleTargetResolver.h"

#include "Core/BattleState.h"
#include "Cards/CardDefinition.h"
#include "Types/WacomInteractionTargetTypes.h"

bool FBattleTargetResolver::CanTargetWithCard(const FBattleState& State, const FGuid& CardInstanceId,
	const FWacomInteractionTargetHandle& Target)
{
	if (!Target.IsValid())
	{
		return false;
	}

	const int32* CardIndex = State.Cards.CardIndexById.Find(CardInstanceId);
	if (!CardIndex || !State.Cards.AllCards.IsValidIndex(*CardIndex))
	{
		return false;
	}

	const FRuntimeCardInstance& Card = State.Cards.AllCards[*CardIndex];
	const UCardDefinition* Def = Card.Definition;
	if (!Def || Card.Location != ECardLocation::Hand)
	{
		return false;
	}

	switch (Target.TargetKind)
	{
	case EWacomInteractionTargetKind::World:
	{
		if (!Target.WorldTargetId.IsValid())
		{
			return false;
		}

		switch (Def->TargetMode)
		{
		case ECardTargetMode::SingleEnemyPart:
		{
			const int32* PartIndex = State.Enemy.PartIndexById.Find(Target.WorldTargetId);
			if (!PartIndex || !State.Enemy.Parts.IsValidIndex(*PartIndex))
			{
				return false;
			}
			return !State.Enemy.Parts[*PartIndex].bDestroyed;
		}
		case ECardTargetMode::AllEnemyParts:
		case ECardTargetMode::Self:
			// 这些模式不要求指定具体目标；handle 里的 WorldTargetId 被忽略。
			return true;
		case ECardTargetMode::None:
		case ECardTargetMode::HandCard:
		default:
			return false;
		}
	}

	case EWacomInteractionTargetKind::Card:
	{
		if (Def->TargetMode != ECardTargetMode::HandCard
			|| !Target.CardInstanceId.IsValid()
			|| Target.CardInstanceId == CardInstanceId)
		{
			return false;
		}

		const int32* TargetCardIndex = State.Cards.CardIndexById.Find(Target.CardInstanceId);
		if (!TargetCardIndex || !State.Cards.AllCards.IsValidIndex(*TargetCardIndex))
		{
			return false;
		}

		return State.Cards.AllCards[*TargetCardIndex].Location == ECardLocation::Hand;
	}

	case EWacomInteractionTargetKind::Zone:
		// 当前战斗规则不支持区域目标。
		return false;

	case EWacomInteractionTargetKind::None:
	default:
		return false;
	}
}
