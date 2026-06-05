// Copyright Wacom. All Rights Reserved.

#include "Deck/DeckService.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Types/WacomEnums.h"

void FDeckService::SetCardLocation(FBattleState& State, const FGuid& CardInstanceId, ECardLocation NewLocation)
{
	FBattleRules::SetCardLocation(State, CardInstanceId, NewLocation);
}

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

		const FGuid TopId = State.Cards.DrawPile.Pop(EAllowShrinking::No);
		OutDrawnCardIds.Add(TopId);
		SetCardLocation(State, TopId, ECardLocation::Hand);
		++Drawn;
	}
	return Drawn;
}

void FDeckService::ShuffleDrawPile(FBattleState& State)
{
	// Fisher-Yates 从末尾向前。使用 BattleState.Rng 保证测试可复现。
	const int32 Num = State.Cards.DrawPile.Num();
	for (int32 i = Num - 1; i > 0; --i)
	{
		const int32 j = State.Rng.RandRange(0, i);
		if (j != i)
		{
			State.Cards.DrawPile.Swap(i, j);
		}
	}
}

void FDeckService::ReshuffleDiscardIntoDraw(FBattleState& State)
{
	if (State.Cards.DiscardPile.IsEmpty())
	{
		return;
	}

	// 把弃牌堆整体搬进 DrawPile，更新 Location，再对整个 DrawPile 洗一次。
	// PlayedPile 不参与本次洗牌；本回合自然打出的牌要等回合结束才进弃牌堆。
	State.Cards.DrawPile.Append(State.Cards.DiscardPile);
	State.Cards.DiscardPile.Reset();

	for (FGuid& Id : State.Cards.DrawPile)
	{
		SetCardLocation(State, Id, ECardLocation::Draw);
	}

	ShuffleDrawPile(State);
}

void FDeckService::MovePlayedPileToDiscard(FBattleState& State)
{
	if (State.Cards.PlayedPile.IsEmpty())
	{
		return;
	}

	State.Cards.DiscardPile.Append(State.Cards.PlayedPile);
	State.Cards.PlayedPile.Reset();

	for (const FGuid& Id : State.Cards.DiscardPile)
	{
		SetCardLocation(State, Id, ECardLocation::Discard);
	}
}

bool FDeckService::MoveFromHandToPlayedPile(FBattleState& State, const FGuid& CardInstanceId)
{
	const int32 Idx = State.Cards.Hand.IndexOfByKey(CardInstanceId);
	if (Idx == INDEX_NONE)
	{
		return false;
	}
	State.Cards.Hand.RemoveAt(Idx);
	State.Cards.PlayedPile.Add(CardInstanceId);
	SetCardLocation(State, CardInstanceId, ECardLocation::Played);
	return true;
}

bool FDeckService::DiscardFromHand(FBattleState& State, const FGuid& CardInstanceId)
{
	const int32 Idx = State.Cards.Hand.IndexOfByKey(CardInstanceId);
	if (Idx == INDEX_NONE)
	{
		return false;
	}
	State.Cards.Hand.RemoveAt(Idx);
	State.Cards.DiscardPile.Add(CardInstanceId);
	SetCardLocation(State, CardInstanceId, ECardLocation::Discard);
	return true;
}

bool FDeckService::ExhaustFromHand(FBattleState& State, const FGuid& CardInstanceId)
{
	const int32 Idx = State.Cards.Hand.IndexOfByKey(CardInstanceId);
	if (Idx == INDEX_NONE)
	{
		return false;
	}
	State.Cards.Hand.RemoveAt(Idx);
	State.Cards.ExhaustPile.Add(CardInstanceId);
	SetCardLocation(State, CardInstanceId, ECardLocation::Exhaust);
	return true;
}
