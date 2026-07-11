// Copyright Wacom. All Rights Reserved.

#include "Deck/DeckService.h"
#include "Cards/CardZoneAggregate.h"
#include "Core/BattleState.h"
#include "Types/WacomEnums.h"

int32 FDeckService::DrawCards(FBattleState& State, int32 Count, TArray<FGuid>& OutDrawnCardIds)
{
	if (Count <= 0)
	{
		return 0;
	}

	int32 Drawn = 0;
	while (Drawn < Count)
	{
		if (State.Cards.DrawPile.IsEmpty())
		{
			if (State.Cards.DiscardPile.IsEmpty())
			{
				break;  // 无卡可抽
			}
			ReshuffleDiscardIntoDraw(State);
		}

		const FGuid TopId = State.Cards.DrawPile.Last();
		if (!FCardZoneAggregate::MoveCardFrom(
			State,
			TopId,
			ECardLocation::Draw,
			ECardLocation::Hand))
		{
			break;
		}
		OutDrawnCardIds.Add(TopId);
		++Drawn;
	}
	return Drawn;
}

void FDeckService::ShuffleDrawPile(FBattleState& State)
{
	FCardZoneAggregate::ShuffleZone(State, ECardLocation::Draw, State.Rng);
}

void FDeckService::ReshuffleDiscardIntoDraw(FBattleState& State)
{
	if (State.Cards.DiscardPile.IsEmpty())
	{
		return;
	}

	// 把弃牌堆整体搬进 DrawPile，再对整个 DrawPile 洗一次。
	// PlayedPile 不参与本次洗牌；本回合自然打出的牌要等回合结束才进弃牌堆。
	if (!FCardZoneAggregate::MoveAllCards(
		State,
		ECardLocation::Discard,
		ECardLocation::Draw))
	{
		return;
	}
	ShuffleDrawPile(State);
}

void FDeckService::MovePlayedPileToDiscard(FBattleState& State)
{
	if (State.Cards.PlayedPile.IsEmpty())
	{
		return;
	}

	FCardZoneAggregate::MoveAllCards(
		State,
		ECardLocation::Played,
		ECardLocation::Discard);
}

bool FDeckService::MoveFromHandToPlayedPile(FBattleState& State, const FGuid& CardInstanceId)
{
	return FCardZoneAggregate::MoveCardFrom(
		State,
		CardInstanceId,
		ECardLocation::Hand,
		ECardLocation::Played);
}
