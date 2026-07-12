// Copyright Wacom. All Rights Reserved.

#include "Deck/DeckService.h"
#include "Cards/CardZoneAggregate.h"
#include "Core/BattleState.h"
#include "Types/WacomEnums.h"

FDeckDrawResult FDeckService::DrawCards(FBattleState& State, int32 Count)
{
	FDeckDrawResult Result;
	if (Count <= 0)
	{
		return Result;
	}

	while (Result.DrawnCardIds.Num() < Count)
	{
		if (State.Cards.DrawPile.IsEmpty())
		{
			if (State.Cards.DiscardPile.IsEmpty())
			{
				break;  // 无卡可抽
			}

			FDeckDrawStepFact& ReshuffleStep = Result.Steps.AddDefaulted_GetRef();
			ReshuffleStep.Kind = EDeckDrawStepKind::DiscardPileReshuffledIntoDraw;
			ReshuffleStep.CardInstanceIds = State.Cards.DiscardPile;
			ReshuffleDiscardIntoDraw(State);
			ReshuffleStep.DrawPileCountAfter = State.Cards.DrawPile.Num();
			ReshuffleStep.DiscardPileCountAfter = State.Cards.DiscardPile.Num();
		}

		FDeckDrawStepFact DrawStep;
		DrawStep.Kind = EDeckDrawStepKind::DrawBatch;
		while (Result.DrawnCardIds.Num() < Count && !State.Cards.DrawPile.IsEmpty())
		{
			const FGuid TopId = State.Cards.DrawPile.Last();
			if (!FCardZoneAggregate::MoveCardFrom(
				State,
				TopId,
				ECardLocation::Draw,
				ECardLocation::Hand))
			{
				break;
			}
			Result.DrawnCardIds.Add(TopId);
			DrawStep.CardInstanceIds.Add(TopId);
		}
		if (DrawStep.CardInstanceIds.IsEmpty())
		{
			break;
		}
		DrawStep.DrawPileCountAfter = State.Cards.DrawPile.Num();
		DrawStep.DiscardPileCountAfter = State.Cards.DiscardPile.Num();
		Result.Steps.Add(MoveTemp(DrawStep));
	}
	return Result;
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
