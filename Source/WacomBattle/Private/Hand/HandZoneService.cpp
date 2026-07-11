// Copyright Wacom. All Rights Reserved.

#include "Hand/HandZoneService.h"

#include "Cards/CardZoneAggregate.h"
#include "Cards/CardDefinition.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

namespace
{
	int32 IndexOfInHand(const FBattleState& State, const FGuid& CardId)
	{
		return State.Cards.Hand.IndexOfByKey(CardId);
	}

	/**
	 * 统一回合开始队列生成。
	 *
	 * 规则：
	 * 1. 预备普通卡池 = 上回合保留普通卡 + 新抽普通卡。
	 * 2. 预备普通卡池每回合重新随机排列。
	 * 3. 左右手锚点重新插入；两锚点都有效且普通卡池非空时，两者之间至少一张普通卡。
	 *
	 * 保留只保留"仍在手牌池"，不保留 index、相对顺序或区域。
	 */
	TArray<FGuid> BuildQueueFromNormalPool(FBattleState& State, const TArray<FGuid>& NormalPool)
	{
		TArray<FGuid> Pre;
		Pre.Reserve(NormalPool.Num());
		for (const FGuid& Id : NormalPool)
		{
			if (!Id.IsValid())
			{
				continue;
			}
			const int32 InsertAt = State.Rng.RandRange(0, Pre.Num());
			Pre.Insert(Id, InsertAt);
		}

		const bool bHasLeft  = State.Cards.LeftHandInstanceId.IsValid();
		const bool bHasRight = State.Cards.RightHandInstanceId.IsValid();

		if (!bHasLeft && !bHasRight)
		{
			return Pre;
		}
		if (bHasLeft && !bHasRight)
		{
			const int32 InsertAt = State.Rng.RandRange(0, Pre.Num());
			Pre.Insert(State.Cards.LeftHandInstanceId, InsertAt);
			return Pre;
		}
		if (!bHasLeft && bHasRight)
		{
			const int32 InsertAt = State.Rng.RandRange(0, Pre.Num());
			Pre.Insert(State.Cards.RightHandInstanceId, InsertAt);
			return Pre;
		}

		// bHasLeft && bHasRight
		const int32 N = Pre.Num();

		if (N == 0)
		{
			// 保底：无普通卡时无法满足"锚点间至少一张普通卡"。
			TArray<FGuid> Fallback;
			Fallback.Add(State.Cards.LeftHandInstanceId);
			Fallback.Add(State.Cards.RightHandInstanceId);
			return Fallback;
		}

		const int32 SlotLeft  = State.Rng.RandRange(0, N);
		int32 SlotRight       = State.Rng.RandRange(0, N - 1);  // N 个可选（去掉 SlotLeft）
		if (SlotRight >= SlotLeft)
		{
			SlotRight += 1;
		}
		// 现在 SlotLeft != SlotRight，均属于 [0, N]。

		// 保证左手牌在右手牌左边：较小的 slot 给 LeftHand，较大的给 RightHand。
		const int32 LeftSlotFinal  = FMath::Min(SlotLeft, SlotRight);
		const int32 RightSlotFinal = FMath::Max(SlotLeft, SlotRight);

		// 构建最终数组：从大到小插入（避免索引错位）。
		TArray<FGuid> Final;
		Final.Reserve(N + 2);
		Final = Pre;

		Final.Insert(State.Cards.RightHandInstanceId, RightSlotFinal);
		Final.Insert(State.Cards.LeftHandInstanceId, LeftSlotFinal);

		return Final;
	}
}

void FHandZoneService::GenerateHandQueueOnTurnStart(FBattleState& State, const TArray<FGuid>& NewlyDrawnCards)
{
	// FDeckService::DrawCards 已通过 CardZoneAggregate 把 NewlyDrawnCards 原子移入 Hand。
	// 本服务只负责将当前 Hand membership 编排成新回合顺序。
	TArray<FGuid> NormalPool;
	NormalPool.Reserve(State.Cards.Hand.Num());
	for (const FGuid& Id : State.Cards.Hand)
	{
		if (!IsHandAnchor(State, Id))
		{
			NormalPool.Add(Id);
		}
	}
	for (const FGuid& Id : NewlyDrawnCards)
	{
		ensureMsgf(State.Cards.Hand.Contains(Id),
			TEXT("[HandZoneService] Drawn card %s must already belong to Hand"),
			*Id.ToString());
	}

	const FGuid Anchors[] = {
		State.Cards.LeftHandInstanceId,
		State.Cards.RightHandInstanceId };
	for (const FGuid& AnchorId : Anchors)
	{
		if (AnchorId.IsValid() && !State.Cards.Hand.Contains(AnchorId))
		{
			const bool bMoved = FCardZoneAggregate::MoveCard(
				State,
				AnchorId,
				ECardLocation::Hand);
			ensureMsgf(bMoved,
				TEXT("[HandZoneService] Failed to return hand anchor %s"),
				*AnchorId.ToString());
		}
	}

	const TArray<FGuid> OrderedHand = BuildQueueFromNormalPool(State, NormalPool);
	ensureMsgf(FCardZoneAggregate::SetZoneOrder(State, ECardLocation::Hand, OrderedHand),
		TEXT("[HandZoneService] Failed to apply generated Hand order"));
}

void FHandZoneService::InsertCardsIntoHandAtRandom(FBattleState& State, const TArray<FGuid>& CardInstanceIds)
{
	for (const FGuid& Id : CardInstanceIds)
	{
		if (!Id.IsValid())
		{
			continue;
		}

		const bool bAlreadyInHand = State.Cards.Hand.Contains(Id);
		const int32 TargetHandSize = State.Cards.Hand.Num() - (bAlreadyInHand ? 1 : 0);
		const int32 InsertAt = State.Rng.RandRange(0, TargetHandSize);
		ensureMsgf(FCardZoneAggregate::MoveCard(State, Id, ECardLocation::Hand, InsertAt),
			TEXT("[HandZoneService] Failed to insert card %s into Hand"),
			*Id.ToString());
	}
}

int32 FHandZoneService::GetAvailableNormalCardSlots(const FBattleState& State, const FGuid& ExcludeId)
{
	int32 NormalCount = 0;
	for (const FGuid& Id : State.Cards.Hand)
	{
		if (IsHandAnchor(State, Id))
		{
			continue;
		}
		if (ExcludeId.IsValid() && Id == ExcludeId)
		{
			continue;
		}
		++NormalCount;
	}

	return FMath::Max(0, NormalCardLimit - NormalCount);
}

EHandZone FHandZoneService::GetZoneOf(const FBattleState& State, const FGuid& CardInstanceId)
{
	if (!CardInstanceId.IsValid())
	{
		return EHandZone::None;
	}
	if (IsHandAnchor(State, CardInstanceId))
	{
		return EHandZone::None;
	}

	const int32 Idx = IndexOfInHand(State, CardInstanceId);
	if (Idx == INDEX_NONE)
	{
		return EHandZone::None;
	}

	const int32 LeftIdx  = IndexOfInHand(State, State.Cards.LeftHandInstanceId);
	const int32 RightIdx = IndexOfInHand(State, State.Cards.RightHandInstanceId);
	const bool bLeftIn   = LeftIdx  != INDEX_NONE;
	const bool bRightIn  = RightIdx != INDEX_NONE;

	if (!bLeftIn && !bRightIn)
	{
		return EHandZone::None;
	}

	if (bLeftIn && bRightIn)
	{
		const int32 Lo = FMath::Min(LeftIdx, RightIdx);
		const int32 Hi = FMath::Max(LeftIdx, RightIdx);
		// 注：LeftHandInstanceId 逻辑上代表"左手"，但在手牌队列里的位置不保证
		// 一定靠左（玩家打出等操作可能导致顺序颠倒）。当前按照"两锚点
		// 切三段"处理：位置较小的那个锚点左侧 = Left 区，中间 = Both 区，
		// 位置较大的那个锚点右侧 = Right 区。后续若正式区分"左手 vs 右手锚
		// 点的物理方向"再修订。
		if (Idx < Lo) { return EHandZone::Left;  }
		if (Idx > Hi) { return EHandZone::Right; }
		return EHandZone::Both;
	}

	// 只有一张锚点：双手区不存在。
	// 锚点左边的卡属于 Left 区，锚点右边的卡不属于任何区域（Zone = None）。
	// 只有一张锚点时，只有锚点左侧有区域归属。
	const int32 AnchorIdx = bLeftIn ? LeftIdx : RightIdx;
	return (Idx < AnchorIdx) ? EHandZone::Left : EHandZone::None;
}

bool FHandZoneService::IsHandAnchor(const FBattleState& State, const FGuid& CardInstanceId)
{
	return CardInstanceId.IsValid()
		&& (CardInstanceId == State.Cards.LeftHandInstanceId
		 || CardInstanceId == State.Cards.RightHandInstanceId);
}

int32 FHandZoneService::CountNormalCardsInHand(const FBattleState& State)
{
	int32 Count = 0;
	for (const FGuid& Id : State.Cards.Hand)
	{
		if (!IsHandAnchor(State, Id))
		{
			++Count;
		}
	}
	return Count;
}

// ================ 回合结束：保留判定 / 非保留卡入弃牌区 ================
bool FHandZoneService::ShouldRetainCardAtTurnEnd(const FBattleState& State, const FGuid& CardInstanceId)
{
	if (!CardInstanceId.IsValid())
	{
		return false;
	}

	// 锚点：自带保留。
	if (IsHandAnchor(State, CardInstanceId))
	{
		return true;
	}

	// 找到卡实例，读 Definition / TemporaryKeywords。
	const FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardInstanceId);
	if (!Card)
	{
		return false;
	}

	// Retain 关键字：永久或临时。
	const bool bHasRetainDef  = Card->Definition
		&& Card->Definition->Keywords.HasTag(WacomTags::Card_Keyword_Retain);
	const bool bHasRetainTemp = Card->TemporaryKeywords.HasTag(WacomTags::Card_Keyword_Retain);
	if (bHasRetainDef || bHasRetainTemp)
	{
		return true;
	}

	// 虫妹专属：左右手都在手牌 + 本卡在双手区 -> 保留。
	const bool bLeftIn  = State.Cards.Hand.Contains(State.Cards.LeftHandInstanceId)  && State.Cards.LeftHandInstanceId.IsValid();
	const bool bRightIn = State.Cards.Hand.Contains(State.Cards.RightHandInstanceId) && State.Cards.RightHandInstanceId.IsValid();
	if (bLeftIn && bRightIn && GetZoneOf(State, CardInstanceId) == EHandZone::Both)
	{
		return true;
	}

	return false;
}

// ================ Shuffle 腾挪 ================
void FHandZoneService::GetAvailableZones(const FBattleState& State, TArray<EHandZone>& OutZones)
{
	OutZones.Reset();

	const bool bLeft  = State.Cards.Hand.Contains(State.Cards.LeftHandInstanceId)  && State.Cards.LeftHandInstanceId.IsValid();
	const bool bRight = State.Cards.Hand.Contains(State.Cards.RightHandInstanceId) && State.Cards.RightHandInstanceId.IsValid();

	// 区域存在性：
	// - 左右手都在：三个区都存在
	// - 只有一张锚点：双手区不存在，存在 Left / Right
	// - 都不在：区域不做判定，不提供腾挪目标
	if (bLeft && bRight)
	{
		OutZones.Add(EHandZone::Left);
		OutZones.Add(EHandZone::Both);
		OutZones.Add(EHandZone::Right);
		return;
	}
	if (bLeft || bRight)
	{
		// 只有一张锚点：只有锚点左侧是 Left 区，右侧无区域。
		// 腾挪可用区域 = 只有 Left。
		OutZones.Add(EHandZone::Left);
		return;
	}
	// 都不在：空集合
}

void FHandZoneService::InsertIntoZoneAtRandom(FBattleState& State, const FGuid& CardId, EHandZone Zone)
{
	if (!CardId.IsValid() || Zone == EHandZone::None)
	{
		return;
	}

	TArray<FGuid> OrderedHand = State.Cards.Hand;
	if (OrderedHand.RemoveSingle(CardId) != 1)
	{
		return;
	}

	const int32 LeftIdx  = OrderedHand.IndexOfByKey(State.Cards.LeftHandInstanceId);
	const int32 RightIdx = OrderedHand.IndexOfByKey(State.Cards.RightHandInstanceId);
	const bool bLeftIn  = LeftIdx != INDEX_NONE;
	const bool bRightIn = RightIdx != INDEX_NONE;

	// 计算目标区间 [Begin, End]（闭区间，都是"插入位置"坐标）。
	int32 Begin = 0;
	int32 End   = OrderedHand.Num();

	if (bLeftIn && bRightIn)
	{
		const int32 Lo = FMath::Min(LeftIdx, RightIdx);
		const int32 Hi = FMath::Max(LeftIdx, RightIdx);
		switch (Zone)
		{
		case EHandZone::Left:  Begin = 0;     End = Lo;          break;
		case EHandZone::Both:  Begin = Lo + 1;End = Hi;          break;
		case EHandZone::Right: Begin = Hi + 1;End = OrderedHand.Num(); break;
		default: return;
		}
	}
	else if (bLeftIn || bRightIn)
	{
		// 只有一张锚点：只有 Left 区存在（锚点左侧）。
		// Right 不是合法区域，如果调用方传了 Right 直接 return。
		const int32 AnchorIdx = bLeftIn ? LeftIdx : RightIdx;
		switch (Zone)
		{
		case EHandZone::Left:  Begin = 0;  End = AnchorIdx;  break;
		default: return;  // Right / Both 不存在
		}
	}
	else
	{
		// 锚点都不在：§6 不提供区域判定，插末尾兜底。
		Begin = 0;
		End   = OrderedHand.Num();
	}

	const int32 InsertAt = State.Rng.RandRange(Begin, End);
	OrderedHand.Insert(CardId, InsertAt);
	ensureMsgf(FCardZoneAggregate::SetZoneOrder(State, ECardLocation::Hand, OrderedHand),
		TEXT("[HandZoneService] Failed to apply shuffled Hand order"));
}

bool FHandZoneService::MoveCardToRandomZone(FBattleState& State, const FGuid& CardInstanceId)
{
	if (!CardInstanceId.IsValid())
	{
		return false;
	}
	if (IsHandAnchor(State, CardInstanceId))
	{
		return false;
	}

	const int32 Idx = State.Cards.Hand.IndexOfByKey(CardInstanceId);
	if (Idx == INDEX_NONE)
	{
		return false;
	}

	TArray<EHandZone> AvailableZones;
	GetAvailableZones(State, AvailableZones);
	if (AvailableZones.IsEmpty())
	{
		return false;
	}

	const int32 ZonePick = State.Rng.RandRange(0, AvailableZones.Num() - 1);
	InsertIntoZoneAtRandom(State, CardInstanceId, AvailableZones[ZonePick]);
	return true;
}

FGuid FHandZoneService::MoveRandomFromBothToOther(FBattleState& State, const FGuid& ExcludeId)
{
	// 双手区必须存在。
	const int32 LeftIdx  = State.Cards.Hand.IndexOfByKey(State.Cards.LeftHandInstanceId);
	const int32 RightIdx = State.Cards.Hand.IndexOfByKey(State.Cards.RightHandInstanceId);
	if (LeftIdx == INDEX_NONE || RightIdx == INDEX_NONE)
	{
		return FGuid();
	}

	const int32 Lo = FMath::Min(LeftIdx, RightIdx);
	const int32 Hi = FMath::Max(LeftIdx, RightIdx);
	if (Hi - Lo < 2)
	{
		// 双手区为空
		return FGuid();
	}

	// 收集双手区候选（普通卡，排除 ExcludeId）。
	TArray<int32> BothIndices;
	BothIndices.Reserve(Hi - Lo - 1);
	for (int32 i = Lo + 1; i < Hi; ++i)
	{
		if (IsHandAnchor(State, State.Cards.Hand[i]))
		{
			continue;
		}
		if (ExcludeId.IsValid() && State.Cards.Hand[i] == ExcludeId)
		{
			continue;
		}
		BothIndices.Add(i);
	}
	if (BothIndices.IsEmpty())
	{
		return FGuid();
	}

	const int32 Pick = BothIndices[State.Rng.RandRange(0, BothIndices.Num() - 1)];
	const FGuid CardId = State.Cards.Hand[Pick];

	// 目标区域排除双手区：只从 Left / Right 中选。
	TArray<EHandZone> TargetZones;
	TargetZones.Add(EHandZone::Left);
	TargetZones.Add(EHandZone::Right);
	const int32 ZonePick = State.Rng.RandRange(0, TargetZones.Num() - 1);

	InsertIntoZoneAtRandom(State, CardId, TargetZones[ZonePick]);
	return CardId;
}

FGuid FHandZoneService::RandomShuffleOneInHand(FBattleState& State, const FGuid& ExcludeId)
{
	// 收集非锚点卡，排除 ExcludeId。
	TArray<int32> CandIdx;
	CandIdx.Reserve(State.Cards.Hand.Num());
	for (int32 i = 0; i < State.Cards.Hand.Num(); ++i)
	{
		if (IsHandAnchor(State, State.Cards.Hand[i]))
		{
			continue;
		}
		if (ExcludeId.IsValid() && State.Cards.Hand[i] == ExcludeId)
		{
			continue;
		}
		CandIdx.Add(i);
	}
	if (CandIdx.IsEmpty())
	{
		return FGuid();
	}

	const int32 Pick = CandIdx[State.Rng.RandRange(0, CandIdx.Num() - 1)];
	const FGuid CardId = State.Cards.Hand[Pick];

	// MoveCardToRandomZone 会处理取出与重新插入。
	MoveCardToRandomZone(State, CardId);
	return CardId;
}
