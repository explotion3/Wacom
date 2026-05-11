// Copyright Wacom. All Rights Reserved.

#include "Hand/HandZoneService.h"

#include "Cards/CardDefinition.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

namespace
{
	void SetCardLocation(FBattleState& State, const FGuid& CardId, ECardLocation NewLocation)
	{
		FBattleRules::SetCardLocation(State, CardId, NewLocation);
	}

	int32 IndexOfInHand(const FBattleState& State, const FGuid& CardId)
	{
		return State.Cards.Hand.IndexOfByKey(CardId);
	}

	bool IsInHand(const FBattleState& State, const FGuid& CardId)
	{
		return CardId.IsValid() && State.Cards.Hand.Contains(CardId);
	}

	/**
	 * "左右手都不在" 分支。
	 *
	 * 规则（Hand_Zone_Rules §3 第一条）：
	 * 1. 预备队列 = 已有手牌（普通卡）+ 新抽普通卡混合
	 * 2. 左右手插入预备队列，两锚点之间至少一张普通卡
	 *
	 * 实现策略（对应 Data_Schema_Draft / Hand_Zone_Rules §10 TBD 占位算法）：
	 * - 步骤 1：把 NewlyDrawnCards 逐张随机插入 Existing（Existing 是上回合遗留的
	 *   普通卡；首回合为空）。这样得到一个混合过的预备队列 Pre。
	 * - 步骤 2：取 Left / Right 两个锚点。若两锚点都有效：
	 *     * 在 [0, Pre.Num()] 中选出一个位置 L 插入 LeftHand。
	 *     * 剩下可选位置中，再选一个位置 R 插入 RightHand，
	 *       约束：L 和 R 之间至少一张普通卡。即 R != L, R != L+1
	 *       （以 Pre 的原位置编号，插入 Left 后整体右移 1，但这里我们直接按
	 *       最终数组坐标计算）。
	 * - 如果任一锚点无效，退化为只插入另一锚点，放末尾即可。
	 *
	 * 返回值：生成的最终 Hand（不含 Discard 溢出，由调用方在外层处理）。
	 */
	TArray<FGuid> BuildQueue_BothAnchorsAbsent(FBattleState& State, const TArray<FGuid>& NewlyDrawnCards)
	{
		// Step 1. 把新抽卡随机插入已有手牌（当前 State.Cards.Hand，此时普通卡全在）。
		// 首回合 State.Cards.Hand 为空，所以最后是纯随机排列 NewlyDrawnCards。
		TArray<FGuid> Pre = State.Cards.Hand;   // 先复制已有普通卡
		for (const FGuid& NewId : NewlyDrawnCards)
		{
			const int32 InsertAt = State.Rng.RandRange(0, Pre.Num());
			Pre.Insert(NewId, InsertAt);
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
		// 目标：最终队列中 LeftIndex 和 RightIndex 之间至少一张普通卡
		// （顺序不限，左手可能在右手右侧，但名义上仍叫 LeftHandInstanceId）。
		//
		// 简化算法：直接在最终数组的索引空间里找一对 (A, B)，|A - B| >= 2。
		// N = Pre.Num()。最终数组长度 N + 2，锚点占其中两个位置。
		// 先在 [0, N+1] 区间随机取 A 放第一个锚点；
		// 然后在剩余合法位置集合 {x : x in [0, N+1], x != A, |x - A| >= 2} 中取 B。
		// 之所以不是 |x - A| >= 2 而是 >= 2 的正确性推导：
		//   假设先插入 A 得到数组长度 N+1，再在此数组上选 InsertAt 插入 B。
		//   插入后 A 的最终索引 = A（若 InsertAt > A）或 A+1（若 InsertAt <= A）。
		//   B 的最终索引 = InsertAt。
		//   两者之间的普通卡数 = |FinalA - FinalB| - 1。
		//   要求 >= 1 即 |FinalA - FinalB| >= 2。
		//
		// 为了避免讨论，我们用等价的两步直接建终局：
		//   把 Pre 的 N 张普通卡看成 N 个"格子"，锚点插入 = 选两个不同的
		//   插槽 0..N。两个插槽之间有 |s1 - s2| 张普通卡。
		//   要求 |s1 - s2| >= 1。（s1 == s2 的情况在 TArray::Insert 里代表
		//   两锚点相邻，中间零张普通卡，违反规则；所以要求 s1 != s2。）
		//
		// 因此：从 [0, N] 中选两个不同的 slot。
		// 注意：如果 N == 0（预备队列为空），两锚点之间不可能有普通卡，
		// 第一阶段按文档 §3 该情形不应发生（起始阶段至少抽了 5 张）。
		// 保底兜回：若 N == 0，直接返回 [Left, Right]，警告留给上层处理。
		const int32 N = Pre.Num();

		if (N == 0)
		{
			// 保底：两锚点并排，违反规则。正常情况下被调用方拦住。
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

		// 构建最终数组：把 Pre 拷贝出来，按 slot 顺序从大到小插入（避免索引错位）。
		TArray<FGuid> Final;
		Final.Reserve(N + 2);
		Final = Pre;

		const int32 FirstSlot  = FMath::Max(SlotLeft, SlotRight);
		const int32 SecondSlot = FMath::Min(SlotLeft, SlotRight);
		const FGuid FirstId    = (FirstSlot  == SlotLeft) ? State.Cards.LeftHandInstanceId : State.Cards.RightHandInstanceId;
		const FGuid SecondId   = (SecondSlot == SlotLeft) ? State.Cards.LeftHandInstanceId : State.Cards.RightHandInstanceId;

		Final.Insert(FirstId, FirstSlot);
		Final.Insert(SecondId, SecondSlot);

		return Final;
	}

	/**
	 * "左右手都在" 分支。
	 * 不改变已有卡位置，NewlyDrawnCards 随机插入当前 Hand。
	 */
	TArray<FGuid> BuildQueue_BothAnchorsPresent(FBattleState& State, const TArray<FGuid>& NewlyDrawnCards)
	{
		// Note: 进入本分支时 State.Cards.Hand 已经包含了左右手锚点。
		// NewlyDrawnCards 是本回合新抽出的、FDeckService 已放到 State.Cards.Hand
		// 末尾（S3 DrawCards 逻辑）—— 实际上 S3 流程里 FDeckService 只把
		// 抽到的 ID 返回给 OutDrawnCardIds，并不直接 append 到 Hand；
		// 本服务作为唯一进入 Hand 的入口，在这里重写 State.Cards.Hand。
		//
		// 因此本分支的输入假设：State.Cards.Hand 已包含锚点 + 上回合保留卡；
		// NewlyDrawnCards 尚未进入 Hand。我们不改变 State.Cards.Hand 中已有卡的相对位置，
		// 逐张随机插入新卡。
		TArray<FGuid> Result = State.Cards.Hand;
		for (const FGuid& NewId : NewlyDrawnCards)
		{
			const int32 InsertAt = State.Rng.RandRange(0, Result.Num());
			Result.Insert(NewId, InsertAt);
		}
		return Result;
	}
}

void FHandZoneService::GenerateHandQueueOnTurnStart(FBattleState& State, const TArray<FGuid>& NewlyDrawnCards)
{
	// 调用方约定：FDeckService::DrawCards 已把 NewlyDrawnCards 的 Location 置为 Hand，
	// 但本服务才是 Hand 的"唯一编排者"。我们把 NewlyDrawnCards 从 State.Cards.Hand 中移除
	// （如果调用方提前放进去了），然后重新编排。
	for (const FGuid& Id : NewlyDrawnCards)
	{
		const int32 Idx = State.Cards.Hand.IndexOfByKey(Id);
		if (Idx != INDEX_NONE)
		{
			State.Cards.Hand.RemoveAt(Idx);
		}
	}

	const bool bLeftInHand  = IsInHand(State, State.Cards.LeftHandInstanceId);
	const bool bRightInHand = IsInHand(State, State.Cards.RightHandInstanceId);

	TArray<FGuid> NewHand;
	if (bLeftInHand && bRightInHand)
	{
		NewHand = BuildQueue_BothAnchorsPresent(State, NewlyDrawnCards);
	}
	else
	{
		// Hand_Zone_Rules §3 "只有一张锚点"按"都不在"重新生成。
		// 需要先把 State.Cards.Hand 中的锚点抽离，让 BuildQueue_BothAnchorsAbsent
		// 拿到的 State.Cards.Hand 只剩普通卡，再由它负责重新插入两个锚点。
		auto ExtractAnchor = [&State](const FGuid& AnchorId)
		{
			if (!AnchorId.IsValid()) { return; }
			const int32 Idx = State.Cards.Hand.IndexOfByKey(AnchorId);
			if (Idx != INDEX_NONE)
			{
				State.Cards.Hand.RemoveAt(Idx);
			}
		};
		ExtractAnchor(State.Cards.LeftHandInstanceId);
		ExtractAnchor(State.Cards.RightHandInstanceId);

		NewHand = BuildQueue_BothAnchorsAbsent(State, NewlyDrawnCards);
	}

	State.Cards.Hand = MoveTemp(NewHand);

	// 更新 Location。
	for (const FGuid& Id : State.Cards.Hand)
	{
		SetCardLocation(State, Id, ECardLocation::Hand);
	}
}

void FHandZoneService::EnforceNormalCardLimit(FBattleState& State, TArray<FGuid>& OutDiscarded)
{
	OutDiscarded.Reset();

	int32 NormalCount = CountNormalCardsInHand(State);
	if (NormalCount <= NormalCardLimit)
	{
		return;
	}

	// 从末尾向前扫描，跳过锚点，把超限的普通卡移入弃牌区。
	for (int32 i = State.Cards.Hand.Num() - 1; i >= 0 && NormalCount > NormalCardLimit; --i)
	{
		const FGuid Id = State.Cards.Hand[i];
		if (IsHandAnchor(State, Id))
		{
			continue;
		}
		State.Cards.Hand.RemoveAt(i);
		State.Cards.DiscardPile.Add(Id);
		SetCardLocation(State, Id, ECardLocation::Discard);
		OutDiscarded.Add(Id);
		--NormalCount;
	}
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
		return EHandZone::None;  // Hand_Zone_Rules §6
	}

	if (bLeftIn && bRightIn)
	{
		const int32 Lo = FMath::Min(LeftIdx, RightIdx);
		const int32 Hi = FMath::Max(LeftIdx, RightIdx);
		// 注：LeftHandInstanceId 逻辑上代表"左手"，但在手牌队列里的位置不保证
		// 一定靠左（玩家打出等操作可能导致顺序颠倒）。第一阶段按照"两锚点
		// 切三段"处理：位置较小的那个锚点左侧 = Left 区，中间 = Both 区，
		// 位置较大的那个锚点右侧 = Right 区。后续若正式区分"左手 vs 右手锚
		// 点的物理方向"再修订。
		if (Idx < Lo) { return EHandZone::Left;  }
		if (Idx > Hi) { return EHandZone::Right; }
		return EHandZone::Both;
	}

	// 只有一张锚点：双手区不存在。
	const int32 AnchorIdx = bLeftIn ? LeftIdx : RightIdx;
	return (Idx < AnchorIdx) ? EHandZone::Left : EHandZone::Right;
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
// 对齐 Battle_Rules §12 + Hand_Zone_Rules §7。

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

	// 虫妹专属：左右手都在手牌 + 本卡在双手区 → 保留（Hand_Zone_Rules §7 第三段）。
	const bool bLeftIn  = State.Cards.Hand.Contains(State.Cards.LeftHandInstanceId)  && State.Cards.LeftHandInstanceId.IsValid();
	const bool bRightIn = State.Cards.Hand.Contains(State.Cards.RightHandInstanceId) && State.Cards.RightHandInstanceId.IsValid();
	if (bLeftIn && bRightIn && GetZoneOf(State, CardInstanceId) == EHandZone::Both)
	{
		return true;
	}

	return false;
}

void FHandZoneService::DiscardNonRetainedNormalCardsAtTurnEnd(FBattleState& State, TArray<FGuid>& OutDiscarded)
{
	OutDiscarded.Reset();

	// 从末尾向前扫，索引稳定；用快照的 State.Cards.Hand 做决策（所有"双手区"计算基于
	// 回合结束那一刻的手牌布局）。
	// 注意：ShouldRetainCardAtTurnEnd 对"双手区"的判断依赖锚点位置，我们不在
	// 扫描中途移除锚点，所以"双手区"判断在整个过程中保持一致。
	for (int32 i = State.Cards.Hand.Num() - 1; i >= 0; --i)
	{
		const FGuid Id = State.Cards.Hand[i];

		// 锚点永不进弃牌，ShouldRetainCardAtTurnEnd 会返回 true，这里直接跳过。
		if (IsHandAnchor(State, Id))
		{
			continue;
		}

		if (ShouldRetainCardAtTurnEnd(State, Id))
		{
			continue;
		}

		State.Cards.Hand.RemoveAt(i);
		State.Cards.DiscardPile.Add(Id);
		SetCardLocation(State, Id, ECardLocation::Discard);
		OutDiscarded.Add(Id);
	}
}

// ================ Shuffle 腾挪 ================
// 对齐 Hand_Zone_Rules §8。

void FHandZoneService::GetAvailableZones(const FBattleState& State, TArray<EHandZone>& OutZones)
{
	OutZones.Reset();

	const bool bLeft  = State.Cards.Hand.Contains(State.Cards.LeftHandInstanceId)  && State.Cards.LeftHandInstanceId.IsValid();
	const bool bRight = State.Cards.Hand.Contains(State.Cards.RightHandInstanceId) && State.Cards.RightHandInstanceId.IsValid();

	// 规则 §6：
	// - 左右手都在：三个区都存在
	// - 只有一张锚点：双手区不存在，存在 Left / Right
	// - 都不在：区域不做判定，第一阶段不提供腾挪目标
	if (bLeft && bRight)
	{
		OutZones.Add(EHandZone::Left);
		OutZones.Add(EHandZone::Both);
		OutZones.Add(EHandZone::Right);
		return;
	}
	if (bLeft || bRight)
	{
		OutZones.Add(EHandZone::Left);
		OutZones.Add(EHandZone::Right);
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

	const int32 LeftIdx  = State.Cards.Hand.IndexOfByKey(State.Cards.LeftHandInstanceId);
	const int32 RightIdx = State.Cards.Hand.IndexOfByKey(State.Cards.RightHandInstanceId);
	const bool bLeftIn  = LeftIdx != INDEX_NONE;
	const bool bRightIn = RightIdx != INDEX_NONE;

	// 计算目标区间 [Begin, End]（闭区间，都是"插入位置"坐标）。
	int32 Begin = 0;
	int32 End   = State.Cards.Hand.Num();

	if (bLeftIn && bRightIn)
	{
		const int32 Lo = FMath::Min(LeftIdx, RightIdx);
		const int32 Hi = FMath::Max(LeftIdx, RightIdx);
		switch (Zone)
		{
		case EHandZone::Left:  Begin = 0;     End = Lo;          break;
		case EHandZone::Both:  Begin = Lo + 1;End = Hi;          break;
		case EHandZone::Right: Begin = Hi + 1;End = State.Cards.Hand.Num(); break;
		default: return;
		}
	}
	else if (bLeftIn || bRightIn)
	{
		const int32 AnchorIdx = bLeftIn ? LeftIdx : RightIdx;
		switch (Zone)
		{
		case EHandZone::Left:  Begin = 0;            End = AnchorIdx;        break;
		case EHandZone::Right: Begin = AnchorIdx + 1;End = State.Cards.Hand.Num(); break;
		default: return;
		}
	}
	else
	{
		// 锚点都不在：§6 不提供区域判定，插末尾兜底。
		Begin = 0;
		End   = State.Cards.Hand.Num();
	}

	const int32 InsertAt = State.Rng.RandRange(Begin, End);
	State.Cards.Hand.Insert(CardId, InsertAt);

	FBattleRules::SetCardLocation(State, CardId, ECardLocation::Hand);
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

	// 先从 Hand 取出，再选目标区域插入。
	State.Cards.Hand.RemoveAt(Idx);

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

	State.Cards.Hand.RemoveAt(Pick);

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
