// Copyright Wacom. All Rights Reserved.

#include "Deck/DeckService.h"
#include "Core/BattleState.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Types/WacomEnums.h"

void FDeckService::SetCardLocation(FBattleState& State, const FGuid& CardInstanceId, ECardLocation NewLocation)
{
	for (FRuntimeCardInstance& Card : State.AllCards)
	{
		if (Card.InstanceId == CardInstanceId)
		{
			Card.Location = NewLocation;
			return;
		}
	}
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
		if (State.DrawPile.IsEmpty())
		{
			if (State.DiscardPile.IsEmpty())
			{
				break;  // 无卡可抽
			}
			ReshuffleDiscardIntoDraw(State);
		}

		const FGuid TopId = State.DrawPile.Pop(EAllowShrinking::No);
		OutDrawnCardIds.Add(TopId);
		SetCardLocation(State, TopId, ECardLocation::Hand);
		++Drawn;
	}
	return Drawn;
}

void FDeckService::ReshuffleDiscardIntoDraw(FBattleState& State)
{
	if (State.DiscardPile.IsEmpty())
	{
		return;
	}

	// 把弃牌堆整体搬进 DrawPile 再用 Fisher-Yates 洗一次。
	// 使用 BattleState.Rng 保证测试可复现。
	const int32 OldDrawCount = State.DrawPile.Num();
	State.DrawPile.Append(State.DiscardPile);
	State.DiscardPile.Reset();

	for (FGuid& Id : State.DrawPile)
	{
		SetCardLocation(State, Id, ECardLocation::Draw);
	}

	// Fisher-Yates 从末尾向前，保持"新加入卡"在抽牌堆顶部时也能被洗乱。
	const int32 Num = State.DrawPile.Num();
	for (int32 i = Num - 1; i > 0; --i)
	{
		const int32 j = State.Rng.RandRange(0, i);
		if (j != i)
		{
			State.DrawPile.Swap(i, j);
		}
	}

	// OldDrawCount 保留变量便于未来调试日志使用。
	(void)OldDrawCount;
}

bool FDeckService::DiscardFromHand(FBattleState& State, const FGuid& CardInstanceId)
{
	const int32 Idx = State.Hand.IndexOfByKey(CardInstanceId);
	if (Idx == INDEX_NONE)
	{
		return false;
	}
	State.Hand.RemoveAt(Idx);
	State.DiscardPile.Add(CardInstanceId);
	SetCardLocation(State, CardInstanceId, ECardLocation::Discard);
	return true;
}

bool FDeckService::ExhaustFromHand(FBattleState& State, const FGuid& CardInstanceId)
{
	const int32 Idx = State.Hand.IndexOfByKey(CardInstanceId);
	if (Idx == INDEX_NONE)
	{
		return false;
	}
	State.Hand.RemoveAt(Idx);
	State.ExhaustPile.Add(CardInstanceId);
	SetCardLocation(State, CardInstanceId, ECardLocation::Exhaust);
	return true;
}
