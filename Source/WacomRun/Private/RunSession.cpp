// Copyright Wacom. All Rights Reserved.

#include "RunSession.h"
#include "WacomSaveGame.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Session/BattleSession.h"
#include "Tags/WacomGameplayTags.h"

#include "Kismet/GameplayStatics.h"
#include "UObject/SoftObjectPath.h"

// ================ 内部辅助 ================

namespace
{
	/**
	 * Stage 4.5.0 任务 2.4 / R1.9 / R1.10：
	 *   按 Definition 操作的旧 public API（IsCardInBackpack / IsCardInBattleDeck /
	 *   AddCardToBattleDeck / RemoveCardFromBattleDeck / DestroyCardFromBackpack /
	 *   DeleteCardForGold）需要在 instance 模型下保持"按下标升序匹配第一个 Definition"
	 *   的语义，以维持现有 BackpackSpec 单测语义不变（R1.13）。
	 *
	 *   Card == nullptr 视为"找不到"统一返回 INDEX_NONE，由调用方自己决定如何反馈
	 *   （IsCardIn* 返回 false；Add/Remove/Destroy 返回 false 并 UE_LOG）。
	 */
	int32 FindFirstIndexByDefinition(const TArray<FCardInstance>& Pile, const UCardDefinition* Card)
	{
		if (!Card) { return INDEX_NONE; }
		for (int32 i = 0; i < Pile.Num(); ++i)
		{
			if (Pile[i].Definition == Card) { return i; }
		}
		return INDEX_NONE;
	}

	bool ShouldStarterCardStartInBattleDeck(const UCardDefinition* Card)
	{
		// 原型内容规则：暮色引虫灯默认进入备战区，但仍作为 A 类容器贡献通量容量。
		// 后续若类似规则增多，应抽成 Card/Character 数据字段，而不是继续扩硬编码列表。
		return Card && Card->CardId == FName(TEXT("MuseiYinchongdeng"));
	}

	bool IsFluxContentCardDefinition(const UCardDefinition* Card)
	{
		// 通量存放区现在没有 A 类主卡槽；A 类容器和普通卡都作为通量内容。
		// B 类容器仍是特殊存放区 owner，不进入通量内容。
		return Card && !URunSession::IsTypeBContainerCard(Card);
	}

	bool IsPreferredBurdenOverflowCandidate(const UCardDefinition* Card)
	{
		// 容量来源卡（A/B 类容器）尽量留在原区；容量缩小时优先把普通内容卡挪到负重区。
		return Card && !URunSession::IsContainerCard(Card);
	}

	FRunShopState BuildShopStateFromInputs(const TArray<FRunShopOfferInput>& Inputs)
	{
		FRunShopState State;
		for (const FRunShopOfferInput& Input : Inputs)
		{
			if (!Input.CardDefinition)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[RunSession] BeginShopVisit: 跳过空 CardDefinition 的商品"));
				continue;
			}
			if (Input.Price < 0)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[RunSession] BeginShopVisit: 跳过负价格商品 Card=%s Price=%d"),
					*GetNameSafe(Input.CardDefinition), Input.Price);
				continue;
			}

			FRunShopOffer Offer;
			Offer.OfferId = FGuid::NewGuid();
			ensureMsgf(Offer.OfferId.IsValid(),
				TEXT("[RunSession] BeginShopVisit: FGuid::NewGuid() 生成了 zero GUID"));
			Offer.CardDefinition = Input.CardDefinition;
			Offer.Price = Input.Price;
			Offer.bPurchased = false;
			State.Offers.Add(MoveTemp(Offer));
		}
		return State;
	}

	/**
	 * Stage 4.5.0 任务 4.6 / R7.5 / R7.6：
	 *   把 SaveGame 里的 FCardInstanceSaveEntry 列表还原到 TempState 的 FCardInstance 列表，
	 *   同时累计校验：
	 *     - R7.6.d：InstanceId 非 zero GUID
	 *     - R7.6.c：InstanceId 在 SeenInstanceIds 中全表唯一
	 *     - R7.6.a：DefinitionAssetPath.TryLoad() 必须返回有效 UCardDefinition
	 *
	 *   失败时 OutErr 写入诊断字符串，调用方负责 UE_LOG Error。
	 *
	 *   注：本函数提取为 file-scope free function 而非 ApplySaveGameToRunState 内的 lambda，
	 *   是为了规避 MSVC 14.38 在 ApplySaveGameToRunState 这种"长函数 + 多嵌套 lambda + 大量
	 *   控制流"组合上偶发的 internal compiler error C1001（详见 Docs/TODO.md §2 临时写法）。
	 */
	bool RestoreCardInstanceList(const TArray<FCardInstanceSaveEntry>& Source,
	                              TArray<FCardInstance>& Dest,
	                              TSet<FGuid>& SeenInstanceIds,
	                              const TCHAR* ZoneName,
	                              FString& OutErr)
	{
		Dest.Reset();
		Dest.Reserve(Source.Num());
		for (const FCardInstanceSaveEntry& Entry : Source)
		{
			if (!Entry.InstanceId.IsValid())
			{
				OutErr = FString::Printf(
					TEXT("zone=%s entry InstanceId 为 zero GUID（R7.6.d）"), ZoneName);
				return false;
			}
			bool bAlreadyInSet = false;
			SeenInstanceIds.Add(Entry.InstanceId, &bAlreadyInSet);
			if (bAlreadyInSet)
			{
				OutErr = FString::Printf(
					TEXT("zone=%s 中 InstanceId %s 与其他 zone 重复（R7.6.c）"),
					ZoneName, *Entry.InstanceId.ToString());
				return false;
			}
			UCardDefinition* Def = Cast<UCardDefinition>(Entry.DefinitionAssetPath.TryLoad());
			if (!Def)
			{
				OutErr = FString::Printf(
					TEXT("zone=%s InstanceId=%s DefinitionAssetPath 加载失败: %s（R7.6.a）"),
					ZoneName, *Entry.InstanceId.ToString(),
					*Entry.DefinitionAssetPath.ToString());
				return false;
			}
			FCardInstance Inst;
			Inst.InstanceId                  = Entry.InstanceId;
			Inst.Definition                  = Def;
			Inst.bBattleEnabledInSpecialZone = Entry.bBattleEnabledInSpecialZone;
			Dest.Add(MoveTemp(Inst));
		}
		return true;
	}
}

// ================ 通知辅助 ================

void URunSession::NotifyRunStateChanged()
{
	// OnRunStateChangedNative 是原生委托，订阅方用 AddUObject + RemoveAll(this) 管理生命周期。
	// 第一阶段粗粒度：不区分变更字段，订阅方按需读 RunState 全量。
	OnRunStateChangedNative.Broadcast();
}

void URunSession::EnsureSpecialZoneEntryFor(const FCardInstance& Inst)
{
	// Stage 4.5.1 任务 7.1 / R2.3：B 主卡 instance 进入 Backpack/BattleDeck 时
	//   幂等地为它在 RunState.SpecialZones 末尾追加一条空 entry。
	//
	// 防御性：
	//   - 非 B 主卡 → 不应建 entry（GDD §11.2：A 类容器卡 / 非容器卡都不持有特殊存放区）。
	//   - zero GUID InstanceId → 不应进入任何 zone（R1.14），更不该被建 entry；
	//     这里短路防止把无效 entry 写进 SpecialZones 污染后续 Find / Move 路径。
	if (!IsTypeBContainerCard(Inst.Definition))
	{
		return;
	}
	if (!Inst.InstanceId.IsValid())
	{
		return;
	}

	// 幂等：已有同 OwnerInstanceId 的 entry 就直接返回（R2.3）。
	for (const FSpecialZone& SZ : RunState.SpecialZones)
	{
		if (SZ.OwnerInstanceId == Inst.InstanceId)
		{
			return;
		}
	}

	FSpecialZone NewEntry;
	NewEntry.OwnerInstanceId = Inst.InstanceId;
	// NewEntry.Cards 默认空数组（R2.1）。
	RunState.SpecialZones.Add(MoveTemp(NewEntry));
}

// ================ 生命周期 ================

bool URunSession::Initialize(UCharacterDefinition* InCharacter)
{
	RunState = FRunState{};
	RunState.Character   = InCharacter;
	RunState.BattleSeed  = 0;
	RunState.bRunActive  = true;
	RunState.DefeatedEnemies.Reset();
	RunState.DestroyedTriggerIds.Reset();
	RunState.PlayerTransform   = FTransform::Identity;
	RunState.bHasPlayerTransform = false;

	if (!InCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] Initialize: Character 为空"));
		return false;
	}

	// 从角色读取手指字段。
	RunState.FingerCount = InCharacter->FingerCount;
	RunState.HpPerFinger = InCharacter->HpPerFinger;

	// GDD §11.6 / §11.9 / Stage 4.1（a2）：
	//   - 非容器卡（普通卡）进 BattleDeck（默认参战）
	//   - 容器卡只进 Backpack（默认不参战，玩家可手动加入）
	//   - 一张卡同时只能在一个区（GDD §11.4），所以普通卡 NOT 在 Backpack
	//
	// Stage 4.5.0 任务 2.2：
	//   - 每张非空 Definition 通过 `FGuid::NewGuid()` 生成新 InstanceId（R1.3）
	//   - StarterDeck 中 nullptr 条目跳过不创建 instance（R1.3）
	//   - 空 StarterDeck 走 fallback（清空两数组 + UE_LOG Warning，R1.4）
	//   - 兜底 `ensureMsgf(InstanceId.IsValid())`（R1.14，仅 Editor / Debug build 触发）
	RunState.Backpack.Reset();
	RunState.BattleDeck.Reset();

	if (InCharacter->StarterDeck.Num() == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] Initialize: Character=%s 的 StarterDeck 为空，Backpack/BattleDeck 保持空数组（fallback）"),
			*GetNameSafe(InCharacter));
	}

	for (const TObjectPtr<UCardDefinition>& Card : InCharacter->StarterDeck)
	{
		if (!Card)
		{
			continue;
		}
		FCardInstance Inst;
		Inst.Definition = Card;
		Inst.InstanceId = FGuid::NewGuid();
		ensureMsgf(Inst.InstanceId.IsValid(),
			TEXT("[RunSession] Initialize: FGuid::NewGuid() 生成了 zero GUID，违反 R1.14 不变量"));
		if (ShouldStarterCardStartInBattleDeck(Card))
		{
			RunState.BattleDeck.Add(Inst);
		}
		else if (URunSession::IsContainerCard(Card))
		{
			RunState.Backpack.Add(Inst);
		}
		else
		{
			RunState.BattleDeck.Add(Inst);
		}
		// Stage 4.5.1 任务 7.1 / R2.3：B 主卡 instance 进入 Backpack/BattleDeck 时
		//   幂等追加 SpecialZones entry。非 B 主卡 / zero GUID 由 helper 内部短路。
		EnsureSpecialZoneEntryFor(Inst);
	}

	// 时段 / 节点重置为清晨第一日。
	RunState.CurrentDayNumber  = 1;
	RunState.CurrentTimePhase  = ETimePhase::Morning;
	RunState.RemainingNodeCount = RunState.InitialNodeCount_Morning;

	// 初始化负重（首次计算，之后由 AddCard / DestroyCard 自动维护）。
	// 这里走"不广播"的私有路径：本函数尾部统一 NotifyRunStateChanged 一次（R2.16 / task 9.4）。
	RecomputeBurdenInternal();

	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] Initialized: Character=%s, FingerCount=%d, Backpack=%d, BattleDeck=%d, FluxCapacity=%d, Phase=Morning Day=1 Nodes=%d"),
		*GetNameSafe(InCharacter),
		RunState.FingerCount,
		RunState.Backpack.Num(),
		RunState.BattleDeck.Num(),
		GetFluxCapacity(),
		RunState.RemainingNodeCount);
	NotifyRunStateChanged();
	return true;
}

void URunSession::ResetRunState()
{
	UCharacterDefinition* KeepChar = RunState.Character;
	Initialize(KeepChar);
}

// ================ 战斗联动 ================

bool URunSession::BuildInitParamsForBattle(UEnemyDefinition* EnemyDef, FBattleInitParams& OutParams) const
{
	return BuildInitParamsForBattle(EnemyDef, NAME_None, OutParams);
}

bool URunSession::BuildInitParamsForBattle(UEnemyDefinition* EnemyDef, FName TriggerPersistentId, FBattleInitParams& OutParams) const
{
	if (!RunState.Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] BuildInitParamsForBattle: RunState.Character 为空"));
		return false;
	}
	if (!EnemyDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] BuildInitParamsForBattle: EnemyDef 为空"));
		return false;
	}

	OutParams.Character  = RunState.Character;
	OutParams.Enemy      = EnemyDef;
	OutParams.RandomSeed = RunState.BattleSeed;

	// GDD §10：阈值常量从 RunState 灌入战内（而非战内硬编码）。
	OutParams.HighHpThreshold = RunState.HighHpThreshold;
	OutParams.LowHpThreshold  = RunState.LowHpThreshold;

	// GDD §11.6 / §11.9：战斗只读备战卡组。Stage 4.5.2 起输出 entries，
	// 让来自 SpecialZone 的入战卡携带对应 B 主卡 CapacityEffect。
	OutParams.BattleDeckOverride.Reset();
	OutParams.BattleDeckEntries.Reset();
	OutParams.BattleDeckEntries.Reserve(RunState.BattleDeck.Num());
	for (const FCardInstance& Inst : RunState.BattleDeck)
	{
		if (Inst.Definition)
		{
			FBattleDeckEntry Entry;
			Entry.Definition = Inst.Definition;
			OutParams.BattleDeckEntries.Add(MoveTemp(Entry));
		}
	}

	for (const FSpecialZone& SZ : RunState.SpecialZones)
	{
		FCardInstance Owner;
		EZoneKind OwnerZone = EZoneKind::Backpack;
		FGuid OwnerZoneOwnerId;
		if (!FindInstance(SZ.OwnerInstanceId, Owner, OwnerZone, OwnerZoneOwnerId)
			|| OwnerZone != EZoneKind::BattleDeck
			|| !Owner.Definition
			|| !Owner.Definition->Physique.CapacityEffect.IsValid())
		{
			continue;
		}

		for (const FCardInstance& Inst : SZ.Cards)
		{
			if (!Inst.Definition || !Inst.bBattleEnabledInSpecialZone)
			{
				continue;
			}

			FBattleDeckEntry Entry;
			Entry.Definition = Inst.Definition;
			Entry.CapacityEffectTags.AddTag(Owner.Definition->Physique.CapacityEffect);
			OutParams.BattleDeckEntries.Add(MoveTemp(Entry));
		}
	}

	// GDD §10.5 撤离重入：灌入持久化破坏部位（如果该 Trigger 上次撤离时有记录）
	OutParams.PreDestroyedPartIds.Reset();
	if (!TriggerPersistentId.IsNone())
	{
		if (const FBattleProgressSnapshot* Progress = RunState.BattleProgress.Find(TriggerPersistentId))
		{
			OutParams.PreDestroyedPartIds = Progress->DestroyedPartIds;
		}
	}
	return true;
}

void URunSession::OnBattleFinished(const FBattleResultPacket& Packet, UEnemyDefinition* EnemyDef)
{
	OnBattleFinishedFromTrigger(Packet, EnemyDef, NAME_None);
}

void URunSession::OnBattleFinishedFromTrigger(const FBattleResultPacket& Packet, UEnemyDefinition* EnemyDef, FName TriggerPersistentId)
{
	// 1) Outcome 主分支
	switch (Packet.Outcome)
	{
	case EBattleOutcome::Victory:
		if (Packet.bWithdrawn)
		{
			// 撤离（GDD §6 / §10.5）：敌人不进 DefeatedEnemies、节点不算完成。
			// 持久化破坏部位列表，下次进入同一战斗 Trigger 时维持破坏态。
			if (!TriggerPersistentId.IsNone())
			{
				FBattleProgressSnapshot Snapshot;
				Snapshot.DestroyedPartIds = Packet.DestroyedPartIds;
				RunState.BattleProgress.Add(TriggerPersistentId, MoveTemp(Snapshot));
			}
			UE_LOG(LogTemp, Display,
				TEXT("[RunSession] Battle withdrawn from %s (Trigger=%s, %d parts persisted destroyed)"),
				*GetNameSafe(EnemyDef),
				*TriggerPersistentId.ToString(),
				Packet.DestroyedPartIds.Num());
		}
		else
		{
			// 真胜利：进 DefeatedEnemies + 清理该 Trigger 的进度（一次性完成）
			if (EnemyDef)
			{
				RunState.DefeatedEnemies.AddUnique(EnemyDef);
			}
			if (!TriggerPersistentId.IsNone())
			{
				RunState.BattleProgress.Remove(TriggerPersistentId);
			}
			UE_LOG(LogTemp, Display,
				TEXT("[RunSession] Battle victory against %s (%d total defeated)"),
				*GetNameSafe(EnemyDef),
				RunState.DefeatedEnemies.Num());
		}
		break;

	case EBattleOutcome::Defeat:
		RunState.bRunActive = false;
		UE_LOG(LogTemp, Display, TEXT("[RunSession] Battle defeat, run ended"));
		break;

	case EBattleOutcome::Undetermined:
	default:
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] OnBattleFinished with Undetermined outcome, ignored"));
		// 未定结果不做战外结算（疲劳 / 伤口都不加），直接返回。
		return;
	}

	// 2) 战外结算压力（GDD §3.2 / §9.2）
	// 疲劳：每场战斗后 +1%（无论胜败）。
	AddPressure(EWacomPressureType::Fatigue, 1);

	// 伤口阈值跨越（Stage 1.2：flag 字段就位，触发逻辑等 Stage 6）。
	if (Packet.bCrossedHighHpThreshold)
	{
		AddPressure(EWacomPressureType::Wound, 1);
	}
	if (Packet.bCrossedLowHpThreshold)
	{
		AddPressure(EWacomPressureType::Wound, 5);
	}
	// 同归于尽：+10% 伤口；不影响 bRunActive（Outcome 已是 Victory）。
	if (Packet.bMutualDestruction)
	{
		AddPressure(EWacomPressureType::Wound, 10);
		UE_LOG(LogTemp, Display,
			TEXT("[RunSession] Mutual destruction: Wound +10%%, total Wound=%d"),
			GetPressureValue(EWacomPressureType::Wound));
	}

	// 3) 经验结算（GDD §3.3）
	// Defeat 不结算：Run 都结束了发了无意义。
	// Victory（含同归于尽）正常结算。
	if (Packet.Outcome == EBattleOutcome::Victory)
	{
		int32 TotalExp = 0;
		for (const FKnockdownExpGain& Gain : Packet.KnockdownExpGains)
		{
			TotalExp += Gain.ExpAmount;
		}
		if (TotalExp > 0)
		{
			AddExperience(TotalExp);
			UE_LOG(LogTemp, Display,
				TEXT("[RunSession] Exp granted: %d (from %d destroyed parts)"),
				TotalExp, Packet.KnockdownExpGains.Num());
		}
	}

	// 4) 战斗中获得的卡牌结算（万物成卡第一版）。
	// Victory 包括撤离；Defeat / Undetermined 不结算。
	if (Packet.Outcome == EBattleOutcome::Victory)
	{
		for (const FBattleGainedCard& GainedCard : Packet.GainedCards)
		{
			if (!GainedCard.Definition)
			{
				continue;
			}
			AcquireCardToRun(GainedCard.Definition.Get());
			UE_LOG(LogTemp, Display,
				TEXT("[RunSession] Gained card from battle: Card=%s, Part=%s, Choice=%d"),
				*GetNameSafe(GainedCard.Definition),
				*GainedCard.SourcePartId.ToString(),
				static_cast<int32>(GainedCard.SourceChoice));
		}
	}

	// 5) 击倒事件玩家选择记账（GDD §6）。
	// 第一阶段仅日志，节点事件 Stage 9 接入时按 Choice 触发分支：
	//   - Aid 援助：未来对应援助节点事件
	//   - Destroy 破坏：未来 +1% 伤口或劣迹（按节点事件配置）
	//   - Withdraw 撤离：上面已处理
	for (const FKnockdownChoice& Choice : Packet.KnockdownChoices)
	{
		const TCHAR* ChoiceName = TEXT("?");
		switch (Choice.Choice)
		{
		case EKnockdownChoice::Aid:      ChoiceName = TEXT("Aid"); break;
		case EKnockdownChoice::Destroy:  ChoiceName = TEXT("Destroy"); break;
		case EKnockdownChoice::Withdraw: ChoiceName = TEXT("Withdraw"); break;
		default: break;
		}
		UE_LOG(LogTemp, Display,
			TEXT("[RunSession] KnockdownChoice: Part=%s, Choice=%s（节点事件 Stage 9 接入时按 Choice 分支处理）"),
			*Choice.PartId.ToString(), ChoiceName);
	}

	// 6) 整体通知一次（即便上面没改字段也发，让 UI 在战斗结束统一刷新）
	NotifyRunStateChanged();
}

// ================ 场景状态 ================

void URunSession::MarkTriggerDestroyed(FName PersistentId)
{
	if (PersistentId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] MarkTriggerDestroyed 收到 NAME_None，忽略"));
		return;
	}
	RunState.DestroyedTriggerIds.Add(PersistentId);
	NotifyRunStateChanged();
}

bool URunSession::IsTriggerDestroyed(FName PersistentId) const
{
	if (PersistentId.IsNone()) { return false; }
	return RunState.DestroyedTriggerIds.Contains(PersistentId);
}

void URunSession::SetPlayerTransform(const FTransform& InTransform)
{
	RunState.PlayerTransform   = InTransform;
	RunState.bHasPlayerTransform = true;
}

// ================ 存档 / 读档 ================

UWacomSaveGame* URunSession::BuildSaveGameFromRunState() const
{
	UWacomSaveGame* Save = Cast<UWacomSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UWacomSaveGame::StaticClass()));
	if (!Save) { return nullptr; }

	Save->SaveVersion    = UWacomSaveGame::CurrentSaveVersion;
	Save->SavedAtUtc     = FDateTime::UtcNow();
	Save->ClientBuildId  = FString();  // 第一版留空，未来可接 FApp::GetBuildVersion()

	Save->CharacterAssetPath = RunState.Character
		? FSoftObjectPath(RunState.Character)
		: FSoftObjectPath();

	Save->BattleSeed = RunState.BattleSeed;
	Save->bRunActive = RunState.bRunActive;

	Save->DefeatedEnemyAssetPaths.Reset();
	Save->DefeatedEnemyAssetPaths.Reserve(RunState.DefeatedEnemies.Num());
	for (UEnemyDefinition* E : RunState.DefeatedEnemies)
	{
		if (E) { Save->DefeatedEnemyAssetPaths.Add(FSoftObjectPath(E)); }
	}

	Save->DestroyedTriggerIds = RunState.DestroyedTriggerIds.Array();

	Save->PlayerTransform     = RunState.PlayerTransform;
	Save->bHasPlayerTransform = RunState.bHasPlayerTransform;

	// ---- v2 instance 列表（Stage 4.5.0 任务 4.5 / 4.5.1 任务 11.1 / R7.2） ----
	//
	// 写入约束：
	//   1) 每条 entry 的 InstanceId 必须非 zero GUID（违反则 UE_LOG Error 并跳过）
	//   2) 全表合并（Backpack ∪ BattleDeck ∪ BurdenZone ∪ ⋃SpecialZones.Cards）后 InstanceId 全局唯一
	//      （违反则 UE_LOG Error 并跳过该条；首次出现的保留，后续重复的跳过）
	//   3) FSpecialZone.OwnerInstanceId 必须非 zero GUID 且能在 Backpack ∪ BattleDeck 中找到对应 owner instance
	//      （违反则 UE_LOG Error 并整条 SpecialZone 跳过；不写半截 entry）
	//
	// 4.5.1 阶段（task 11.1）：四个数组全部写入实际数据。
	//
	// 注意：Definition == nullptr 的 instance 也允许写入（DefinitionAssetPath 为空 path）；
	//   读档时由 ApplySaveGameToRunState 的损坏档校验（R7.6 a）处理。
	Save->Backpack.Reset();
	Save->BattleDeck.Reset();
	Save->BurdenZone.Reset();
	Save->SpecialZones.Reset();

	TSet<FGuid> SeenInstanceIds;
	{
		int32 SpecialZoneCardTotal = 0;
		for (const FSpecialZone& SZ : RunState.SpecialZones)
		{
			SpecialZoneCardTotal += SZ.Cards.Num();
		}
		SeenInstanceIds.Reserve(
			RunState.Backpack.Num() + RunState.BattleDeck.Num()
			+ RunState.BurdenZone.Num() + SpecialZoneCardTotal);
	}

	auto WriteInstanceList = [&SeenInstanceIds](const TArray<FCardInstance>& Source,
	                                             TArray<FCardInstanceSaveEntry>& Dest,
	                                             const TCHAR* ZoneName)
	{
		Dest.Reserve(Source.Num());
		for (const FCardInstance& Inst : Source)
		{
			if (!Inst.InstanceId.IsValid())
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunSession] BuildSaveGameFromRunState: %s 中存在 zero GUID InstanceId 的 instance（Definition=%s），跳过"),
					ZoneName, *GetNameSafe(Inst.Definition));
				continue;
			}
			bool bAlreadyInSet = false;
			SeenInstanceIds.Add(Inst.InstanceId, &bAlreadyInSet);
			if (bAlreadyInSet)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunSession] BuildSaveGameFromRunState: %s 中 InstanceId %s 与其他 zone 重复，跳过"),
					ZoneName, *Inst.InstanceId.ToString());
				continue;
			}
			FCardInstanceSaveEntry Entry;
			Entry.InstanceId = Inst.InstanceId;
			Entry.DefinitionAssetPath = Inst.Definition
				? FSoftObjectPath(Inst.Definition)
				: FSoftObjectPath();
			Entry.bBattleEnabledInSpecialZone = Inst.bBattleEnabledInSpecialZone;
			Dest.Add(MoveTemp(Entry));
		}
	};

	WriteInstanceList(RunState.Backpack,   Save->Backpack,   TEXT("Backpack"));
	WriteInstanceList(RunState.BattleDeck, Save->BattleDeck, TEXT("BattleDeck"));
	WriteInstanceList(RunState.BurdenZone, Save->BurdenZone, TEXT("BurdenZone"));

	// SpecialZones：每条 FSpecialZone → 一条 FSpecialZoneSaveEntry（OwnerInstanceId + Cards 列表）。
	// 同一 SeenInstanceIds 集合贯穿所有 zone，所以 SpecialZone 内的卡若 InstanceId 与 Backpack /
	// BattleDeck / BurdenZone / 其他 SpecialZone 中已写入的 InstanceId 重复也会被 lambda 跳过并报错。
	//
	// OwnerInstanceId 校验：
	//   - zero GUID → 整条 entry 跳过（写半截 entry 没有意义，读档侧也无法关联回 owner）
	//   - 非 zero 但在 Backpack ∪ BattleDeck 中找不到 owner instance → R7.6.b 损坏，跳过整条
	//     （RunState 不变量本来就保证此关联存在，此处是双保险防外部直接构造异常 RunState）
	Save->SpecialZones.Reserve(RunState.SpecialZones.Num());
	for (const FSpecialZone& SZ : RunState.SpecialZones)
	{
		if (!SZ.OwnerInstanceId.IsValid())
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunSession] BuildSaveGameFromRunState: SpecialZone OwnerInstanceId 为 zero GUID（Cards=%d），跳过整条"),
				SZ.Cards.Num());
			continue;
		}

		auto OwnerInBackpackOrBattleDeck = [this, &SZ]()
		{
			for (const FCardInstance& Inst : RunState.Backpack)
			{
				if (Inst.InstanceId == SZ.OwnerInstanceId) { return true; }
			}
			for (const FCardInstance& Inst : RunState.BattleDeck)
			{
				if (Inst.InstanceId == SZ.OwnerInstanceId) { return true; }
			}
			return false;
		};
		if (!OwnerInBackpackOrBattleDeck())
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunSession] BuildSaveGameFromRunState: SpecialZone OwnerInstanceId %s 在 Backpack/BattleDeck 中找不到 owner instance，跳过整条"),
				*SZ.OwnerInstanceId.ToString());
			continue;
		}

		FSpecialZoneSaveEntry Entry;
		Entry.OwnerInstanceId = SZ.OwnerInstanceId;
		const FString ZoneNameStr = FString::Printf(TEXT("SpecialZone[%s]"), *SZ.OwnerInstanceId.ToString());
		WriteInstanceList(SZ.Cards, Entry.Cards, *ZoneNameStr);
		Save->SpecialZones.Add(MoveTemp(Entry));
	}

	return Save;
}

bool URunSession::ApplySaveGameToRunState(UWacomSaveGame* SaveGame)
{
	if (!SaveGame)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] ApplySaveGameToRunState: SaveGame 为空"));
		return false;
	}

	// 版本检查：新版本拒绝（R7.7）
	if (SaveGame->SaveVersion > UWacomSaveGame::CurrentSaveVersion)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] 存档版本 %d 高于当前 %d，拒绝读档"),
			SaveGame->SaveVersion, UWacomSaveGame::CurrentSaveVersion);
		return false;
	}

	// 旧版本走迁移链。迁移失败拒绝读档（R7.3）。
	if (!UWacomSaveGame::MigrateIfNeeded(SaveGame))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] 存档迁移失败（源版本 %d → 目标 %d），拒绝读档"),
			SaveGame->SaveVersion, UWacomSaveGame::CurrentSaveVersion);
		return false;
	}

	// Character 资产必须加载成功；失败说明 Character 被删了，整个档作废。
	UCharacterDefinition* LoadedChar = Cast<UCharacterDefinition>(
		SaveGame->CharacterAssetPath.TryLoad());
	if (!LoadedChar)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] CharacterAssetPath 加载失败: %s"),
			*SaveGame->CharacterAssetPath.ToString());
		return false;
	}

	// =================================================================================
	// Stage 4.5.0 任务 4.6 / R7.4 / R7.5 / R7.6：原子还原
	//
	// 关键不变量：本函数任何路径上的失败都不能让 RunState 被部分修改。做法是先在 TempState
	// 上做完所有还原 + 校验，全部通过才 `MoveTemp` 给 `this->RunState`。这样：
	//   - 损坏档 → 提前 return false，RunState 保留调用前状态（R7.6）
	//   - 成功路径 → 一次原子赋值 + 一次 NotifyRunStateChanged 广播
	//
	// 决策树（已迁移到 SaveVersion = 2 之后执行）：
	//   1) 四个 instance 数组（Backpack / BattleDeck / BurdenZone / SpecialZones）全空
	//      → R7.4 路径：按 Character.StarterDeck 重建 instances（每张新 InstanceId）。
	//      此路径覆盖 v0 / v1 → v2 迁移档（迁移链 case 1 把四数组清空）和真"新档"。
	//   2) 任一非空 → R7.5 路径：按 SaveEntry 还原 InstanceId / Definition / flag。
	//   3) R7.6 损坏档校验（任一命中即拒绝）：
	//        a) 任一 entry 的 DefinitionAssetPath 加载失败
	//        b) 任一 SpecialZone 的 OwnerInstanceId 在还原后的 Backpack ∪ BattleDeck 中
	//           找不到对应 owner instance（4.5.1 task 11.2 起接入）
	//        c) 全表合并（Backpack ∪ BattleDeck ∪ BurdenZone ∪ ⋃SpecialZones.Cards）
	//           中 InstanceId 重复
	//        d) entry 的 InstanceId 为 zero GUID（写入侧已校验，但读取仍显式拒绝以防外部改档）
	//
	// 4.5.1 task 11.2 起：完整 BurdenZone / SpecialZones 还原已接入。
	//   - rebuild 路径（四数组全空）会为每张 B 主卡 instance 在 SpecialZones 末尾追加空 entry，
	//     与 Initialize 路径上的 EnsureSpecialZoneEntryFor 行为一致。
	//   - 非空路径会通过 RestoreCardInstanceList 还原四区 + 校验 SpecialZones 主卡归属。
	// =================================================================================

	FRunState TempState;
	TempState.Character          = LoadedChar;
	TempState.BattleSeed         = SaveGame->BattleSeed;
	TempState.bRunActive         = SaveGame->bRunActive;
	TempState.PlayerTransform    = SaveGame->PlayerTransform;
	TempState.bHasPlayerTransform = SaveGame->bHasPlayerTransform;

	TempState.DefeatedEnemies.Reset();
	TempState.DefeatedEnemies.Reserve(SaveGame->DefeatedEnemyAssetPaths.Num());
	for (const FSoftObjectPath& Path : SaveGame->DefeatedEnemyAssetPaths)
	{
		if (UEnemyDefinition* E = Cast<UEnemyDefinition>(Path.TryLoad()))
		{
			TempState.DefeatedEnemies.Add(E);
		}
		else
		{
			// 单条敌人加载失败仅警告 + 跳过：DefeatedEnemies 是日志性数据，
			// 不参与 instance 模型校验，丢一两条不致整档作废。
			UE_LOG(LogTemp, Warning,
				TEXT("[RunSession] DefeatedEnemy 加载失败，跳过: %s"),
				*Path.ToString());
		}
	}

	TempState.DestroyedTriggerIds.Reset();
	for (const FName& Id : SaveGame->DestroyedTriggerIds)
	{
		if (!Id.IsNone()) { TempState.DestroyedTriggerIds.Add(Id); }
	}

	const bool bAllInstanceArraysEmpty =
		   SaveGame->Backpack.Num()     == 0
		&& SaveGame->BattleDeck.Num()   == 0
		&& SaveGame->BurdenZone.Num()   == 0
		&& SaveGame->SpecialZones.Num() == 0;

	if (bAllInstanceArraysEmpty)
	{
		// ---- R7.4：按 StarterDeck 重建 instances ----
		// 复用 Initialize 的 A/B/普通卡分流：容器卡进 Backpack，非容器卡进 BattleDeck。
		// 每张新分配 InstanceId（保证 R7.2 全表唯一 + 非 zero GUID）。
		//
		// Stage 4.5.1 任务 11.2：rebuild 路径同步为每张 B 主卡 instance 在 TempState.SpecialZones
		// 末尾追加一条空 entry，与 Initialize 路径上 EnsureSpecialZoneEntryFor 的行为保持一致
		// （R2.3：B 主卡 instance 进入 Backpack/BattleDeck 时幂等建立 SpecialZone entry）。
		// BurdenZone 保持空（R7.4：迁移档 / 新档没有溢出卡）。
		//
		// 此处 inline 实现而非调 EnsureSpecialZoneEntryFor，因为该 helper 操作的是
		// `this->RunState.SpecialZones`；我们写入的是 TempState（原子还原契约 R7.6 要求）。
		for (const TObjectPtr<UCardDefinition>& Card : LoadedChar->StarterDeck)
		{
			if (!Card)
			{
				continue;
			}
			FCardInstance Inst;
			Inst.Definition = Card;
			Inst.InstanceId = FGuid::NewGuid();
			ensureMsgf(Inst.InstanceId.IsValid(),
				TEXT("[RunSession] ApplySaveGameToRunState (StarterDeck rebuild): FGuid::NewGuid() 生成 zero GUID，违反 R1.14"));
			if (ShouldStarterCardStartInBattleDeck(Card))
			{
				TempState.BattleDeck.Add(Inst);
			}
			else if (URunSession::IsContainerCard(Card))
			{
				TempState.Backpack.Add(Inst);
			}
			else
			{
				TempState.BattleDeck.Add(Inst);
			}
			// R2.3：B 主卡 instance 进入 Backpack/BattleDeck 时幂等追加 SpecialZones entry。
			// StarterDeck 同款 B 主卡可能多张（不同 InstanceId），各自一条 entry。
			if (IsTypeBContainerCard(Inst.Definition) && Inst.InstanceId.IsValid())
			{
				FSpecialZone NewEntry;
				NewEntry.OwnerInstanceId = Inst.InstanceId;
				// Cards 默认空数组（R2.1）
				TempState.SpecialZones.Add(MoveTemp(NewEntry));
			}
		}

		UE_LOG(LogTemp, Display,
			TEXT("[RunSession] ApplySaveGameToRunState: SaveVersion=%d 四数组全空，按 Character=%s StarterDeck 重建（Backpack=%d, BattleDeck=%d, SpecialZones=%d）"),
			SaveGame->SaveVersion, *GetNameSafe(LoadedChar),
			TempState.Backpack.Num(), TempState.BattleDeck.Num(), TempState.SpecialZones.Num());
	}
	else
	{
		// ---- R7.5 + R7.6：按 SaveEntry 还原 + 损坏档校验 ----
		//
		// 校验聚合在一个 TSet<FGuid> SeenInstanceIds 中：每读一条 entry 就尝试 Add，
		// 若 bAlreadyInSet 命中即触发 R7.6.c 重复 InstanceId → 拒绝。SeenInstanceIds 跨
		// Backpack / BattleDeck / BurdenZone / 各 SpecialZone.Cards 共享，覆盖全表唯一性。
		//
		// DefinitionAssetPath 为空 path 视为损坏（R7.6.a）：BuildSaveGameFromRunState 写入
		// nullptr Definition 时输出空 path，读档时 TryLoad 返回 nullptr 即按损坏处理。
		//
		// 还原逻辑提取为 file-scope helper RestoreCardInstanceList（规避 MSVC ICE，
		// 详见 namespace 内函数注释）。
		//
		// Stage 4.5.1 任务 11.2 升级：还原 BurdenZone + SpecialZones 归属关系
		//   - BurdenZone 直接走 RestoreCardInstanceList，与三区共享 SeenInstanceIds。
		//   - 每个 FSpecialZoneSaveEntry：
		//       a) OwnerInstanceId 非 zero GUID（R7.6.d）
		//       b) OwnerInstanceId 必须在 TempState.Backpack ∪ TempState.BattleDeck 中存在
		//          （R7.6.b：B 主卡 instance 只能在这两区，对应 R5.6 不变量）
		//       c) OwnerInstanceId 跨 SpecialZoneSaveEntry 唯一（防御性，R7.6 隐含；
		//          BuildSaveGameFromRunState 写入侧已保证 RunState.SpecialZones 内唯一）
		//       d) Cards 列表通过 RestoreCardInstanceList 还原，复用同一 SeenInstanceIds
		//   - 任一校验失败 → return false，TempState 已写入的部分被丢弃（this->RunState 不变，R7.6）。

		TSet<FGuid> SeenInstanceIds;
		{
			int32 SpecialZoneCardTotal = 0;
			for (const FSpecialZoneSaveEntry& SZ : SaveGame->SpecialZones)
			{
				SpecialZoneCardTotal += SZ.Cards.Num();
			}
			SeenInstanceIds.Reserve(
				SaveGame->Backpack.Num() + SaveGame->BattleDeck.Num()
				+ SaveGame->BurdenZone.Num() + SpecialZoneCardTotal);
		}

		FString ErrMsg;
		if (!RestoreCardInstanceList(SaveGame->Backpack, TempState.Backpack, SeenInstanceIds, TEXT("Backpack"), ErrMsg))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: %s"), *ErrMsg);
			return false;
		}
		if (!RestoreCardInstanceList(SaveGame->BattleDeck, TempState.BattleDeck, SeenInstanceIds, TEXT("BattleDeck"), ErrMsg))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: %s"), *ErrMsg);
			return false;
		}
		if (!RestoreCardInstanceList(SaveGame->BurdenZone, TempState.BurdenZone, SeenInstanceIds, TEXT("BurdenZone"), ErrMsg))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: %s"), *ErrMsg);
			return false;
		}

		// SpecialZones 还原：每条 SaveEntry 走 (a) → (c) → (b) → (d) → (e) 校验/还原。
		// 顺序约束：(b) 依赖 TempState.Backpack/BattleDeck 已还原（前面三个 RestoreCardInstanceList
		// 调用之后），所以本块必然在三区还原之后执行。
		TSet<FGuid> SeenSpecialZoneOwners;
		SeenSpecialZoneOwners.Reserve(SaveGame->SpecialZones.Num());
		TempState.SpecialZones.Reserve(SaveGame->SpecialZones.Num());

		for (const FSpecialZoneSaveEntry& SZEntry : SaveGame->SpecialZones)
		{
			// (a) OwnerInstanceId 非 zero GUID
			if (!SZEntry.OwnerInstanceId.IsValid())
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: SpecialZone OwnerInstanceId 为 zero GUID（R7.6.d）"));
				return false;
			}

			// (c) OwnerInstanceId 跨 entry 唯一（防御性 — 写入侧 invariant 保证不重复，但读取仍校验）
			bool bAlreadyOwner = false;
			SeenSpecialZoneOwners.Add(SZEntry.OwnerInstanceId, &bAlreadyOwner);
			if (bAlreadyOwner)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: SpecialZone OwnerInstanceId %s 在 SaveGame.SpecialZones 中重复（R7.6）"),
					*SZEntry.OwnerInstanceId.ToString());
				return false;
			}

			// (b) OwnerInstanceId 必须在 TempState.Backpack ∪ TempState.BattleDeck 中存在
			//     （R7.6.b：SpecialZone 主卡只能位于 Backpack 或 BattleDeck，对应 R5.6 不变量）
			auto OwnerInBackpackOrBattleDeck = [&]()
			{
				for (const FCardInstance& Inst : TempState.Backpack)
				{
					if (Inst.InstanceId == SZEntry.OwnerInstanceId) { return true; }
				}
				for (const FCardInstance& Inst : TempState.BattleDeck)
				{
					if (Inst.InstanceId == SZEntry.OwnerInstanceId) { return true; }
				}
				return false;
			};
			if (!OwnerInBackpackOrBattleDeck())
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: SpecialZone OwnerInstanceId %s 在 Backpack/BattleDeck 中找不到 owner instance（R7.6.b）"),
					*SZEntry.OwnerInstanceId.ToString());
				return false;
			}

			// (d) 还原 Cards 列表 — 共享 SeenInstanceIds 保证 SpecialZone 内 InstanceId
			//     与四区其它 InstanceId 全表唯一（R7.6.c）。
			FSpecialZone Restored;
			Restored.OwnerInstanceId = SZEntry.OwnerInstanceId;
			const FString ZoneNameStr = FString::Printf(TEXT("SpecialZone[%s]"), *SZEntry.OwnerInstanceId.ToString());
			if (!RestoreCardInstanceList(SZEntry.Cards, Restored.Cards, SeenInstanceIds, *ZoneNameStr, ErrMsg))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: %s"), *ErrMsg);
				return false;
			}

			// (e) 追加到 TempState.SpecialZones
			TempState.SpecialZones.Add(MoveTemp(Restored));
		}

		UE_LOG(LogTemp, Display,
			TEXT("[RunSession] ApplySaveGameToRunState: SaveVersion=%d 按 SaveEntry 还原（Backpack=%d, BattleDeck=%d, BurdenZone=%d, SpecialZones=%d）"),
			SaveGame->SaveVersion,
			TempState.Backpack.Num(), TempState.BattleDeck.Num(),
			TempState.BurdenZone.Num(), TempState.SpecialZones.Num());
	}

	// 所有还原 + 校验通过 → 原子赋值 + 广播一次（R7.6 不变量保证）。
	RunState = MoveTemp(TempState);
	NotifyRunStateChanged();
	return true;
}

bool URunSession::SaveToSlot(const FString& SlotName) const
{
	UWacomSaveGame* Save = BuildSaveGameFromRunState();
	if (!Save)
	{
		UE_LOG(LogTemp, Error, TEXT("[RunSession] SaveToSlot(%s) 构造 SaveGame 失败"), *SlotName);
		return false;
	}

	const bool bOk = UGameplayStatics::SaveGameToSlot(Save, SlotName, /*UserIndex*/0);
	if (!bOk)
	{
		UE_LOG(LogTemp, Error, TEXT("[RunSession] SaveToSlot(%s) 写入磁盘失败"), *SlotName);
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("[RunSession] SaveToSlot(%s) OK，版本 %d"),
		*SlotName, Save->SaveVersion);
	return true;
}

bool URunSession::LoadFromSlot(const FString& SlotName)
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, /*UserIndex*/0))
	{
		UE_LOG(LogTemp, Display, TEXT("[RunSession] LoadFromSlot(%s): 存档不存在"), *SlotName);
		return false;
	}

	USaveGame* Base = UGameplayStatics::LoadGameFromSlot(SlotName, /*UserIndex*/0);
	UWacomSaveGame* Save = Cast<UWacomSaveGame>(Base);
	if (!Save)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] LoadFromSlot(%s): SaveGame 类型不匹配"), *SlotName);
		return false;
	}

	if (!ApplySaveGameToRunState(Save))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] LoadFromSlot(%s): 应用到 RunState 失败"), *SlotName);
		return false;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] LoadFromSlot(%s) OK: Character=%s, Defeated=%d, Triggers=%d, HasPlayerTransform=%d"),
		*SlotName,
		*GetNameSafe(RunState.Character),
		RunState.DefeatedEnemies.Num(),
		RunState.DestroyedTriggerIds.Num(),
		RunState.bHasPlayerTransform ? 1 : 0);
	return true;
}

bool URunSession::HasSaveInSlot(const FString& SlotName) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, /*UserIndex*/0);
}


// ================ §3.1 / §3.4：手指 ================

void URunSession::RemoveFinger(int32 Count)
{
	if (Count <= 0)
	{
		return;
	}

	const int32 OldCount = RunState.FingerCount;
	RunState.FingerCount = FMath::Max(0, OldCount - Count);
	const int32 ActuallyLost = OldCount - RunState.FingerCount;

	if (ActuallyLost > 0)
	{
		// GDD §3.2：每缺 1 指 +5% 残疾。
		RunState.Pressure.Add(EWacomPressureType::Disability, ActuallyLost * 5);

		UE_LOG(LogTemp, Display,
			TEXT("[RunSession] RemoveFinger: lost=%d, FingerCount=%d, Disability=%d"),
			ActuallyLost, RunState.FingerCount, RunState.Pressure.Disability);

		NotifyRunStateChanged();
	}
}

// ================ §3.2：压力 ================

int32 URunSession::GetPressureValue(EWacomPressureType Type) const
{
	return RunState.Pressure.Get(Type);
}

int32 URunSession::GetTotalPressure() const
{
	return RunState.Pressure.GetTotal();
}

void URunSession::AddPressure(EWacomPressureType Type, int32 Delta)
{
	if (Delta == 0)
	{
		return;
	}
	RunState.Pressure.Add(Type, Delta);
	NotifyRunStateChanged();
}

void URunSession::SetPressure(EWacomPressureType Type, int32 Value)
{
	RunState.Pressure.Set(Type, Value);
	NotifyRunStateChanged();
}

void URunSession::RemovePressure(EWacomPressureType Type, int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	RunState.Pressure.Add(Type, -Amount);
	NotifyRunStateChanged();
}

void URunSession::ClearPressure(EWacomPressureType Type)
{
	RunState.Pressure.Set(Type, 0);
	NotifyRunStateChanged();
}

bool URunSession::IsPressureCapReached() const
{
	return RunState.Pressure.GetTotal() >= 100;
}

// ================ §3.2：战外行为触发器 ================

void URunSession::OnRightHandDestructiveAction()
{
	// GDD §3.2 伤口：战外右手破坏行为 +1%。
	AddPressure(EWacomPressureType::Wound, 1);
	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] RightHand destructive action → Wound=%d"),
		GetPressureValue(EWacomPressureType::Wound));
}

void URunSession::OnCompanionCardPermanentlyDestroyed()
{
	// GDD §3.2 嗜血：每永久销毁一张伙伴卡 +1%。
	AddPressure(EWacomPressureType::Bloodlust, 1);
	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] Companion card destroyed → Bloodlust=%d"),
		GetPressureValue(EWacomPressureType::Bloodlust));
}

void URunSession::OnTheftCommitted()
{
	// GDD §3.2 劣迹（b 增量语义）：第 n 次完成时 +(n*(n+1)/2 + 1)%
	++RunState.TheftCount;
	const int32 N = RunState.TheftCount;
	const int32 Delta = N * (N + 1) / 2 + 1;
	AddPressure(EWacomPressureType::Misdeed, Delta);
	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] Theft #%d committed → Misdeed +%d, total=%d"),
		N, Delta, GetPressureValue(EWacomPressureType::Misdeed));
}

void URunSession::RecomputeBurden()
{
	// public 入口（Stage 4.5.1 任务 9.4 / R2.16）：
	//   委托给 RecomputeBurdenInternal 完成步骤 ① / ② / ③，再在末尾广播一次。
	//   外部直接调用本函数（蓝图 / 测试 / 其他模块）时，单次广播由这里发出，符合
	//   "所有改动 zone 内容的 public 入口成功路径尾部广播一次"（R2.16）。
	//
	//   其他 public 入口（Initialize / AddCardToBackpack / DestroyCardFromBackpack /
	//   AddCardToBattleDeck / RemoveCardFromBattleDeck）内部链式调用 RecomputeBurden
	//   时改走 RecomputeBurdenInternal，避免在尾部 NotifyRunStateChanged 之外多发一次。
	RecomputeBurdenInternal();
	NotifyRunStateChanged();
}

void URunSession::RecomputeBurdenInternal(bool bAllowBurdenRefill)
{
	// Stage 4.5.1 重写（design.md §Components and Interfaces #6 / requirements R2.12-R2.14 / R9.1）：
	// 本函数分三步执行；任务 9.1 实现步骤 ①，9.2 / 9.3 依次填入 ② / ③。
	// 任务 9.4 / R2.16：本函数为私有路径，不广播 OnRunStateChangedNative；外层 public
	// 入口（含本文件 RecomputeBurden）统一在尾部 NotifyRunStateChanged 一次。
	//
	//   ① 超容溢出（R2.12，task 9.1）：
	//        while CountFluxContentCards(Backpack) > GetFluxCapacity() → 弹末尾通量内容卡追加 BurdenZone
	//        while BattleDeck.Num() > GetBattleDeckCapacity() → 优先迁入 Backpack，Backpack 接不住再追加 BurdenZone
	//   ② 回填（R2.14，task 9.2）：BurdenZone 头部按 通量 → 备战 → SpecialZones（数组下标升序）
	//        优先序回填到第一个有空位的目标区，回填到 SpecialZone 时强制
	//        bBattleEnabledInSpecialZone = false。
	//   ③ 压力写入（R9.1，task 9.3）：n = BurdenZone.Num()，
	//        Burden 通道值 = FMath::Clamp(n*(n+1)/2, 0, 100)。
	//        直接写 RunState.Pressure.Set(...) 而不是 SetPressure(...)：避开 SetPressure
	//        内部的 NotifyRunStateChanged，确保私有路径不广播（R2.16）。

	// ---- 步骤 ① 超容溢出（R2.12 / R2.2a / R5.6 — "skip B-master"）----
	// 末尾摘卡。通量区溢出追加到 BurdenZone 末尾；备战区溢出优先迁入通量区，通量区接不住再追加
	// BurdenZone。处理完成后保证
	//   CountFluxContentCards(Backpack) <= GetFluxCapacity() AND BattleDeck.Num() <= GetBattleDeckCapacity()，
	//   除非整个数组全是 B 主卡 instance（极端退化情形）。
	//
	// 通量区溢出只移动内容卡；普通卡和 A 类容器卡占用通量内容格，B 主卡只开启特殊存放区。
	// "skip B-master" 规则（R2.2a / R2.12 / R5.6）：
	//   B 主卡 instance 在生命周期内只可能位于 Backpack ∪ BattleDeck，永远不能进入
	//   BurdenZone（与 Property 5 双射 reverse 方向构成的不变量）。否则 SpecialZones
	//   entry 的 OwnerInstanceId 就会找不到对应主卡 instance（悬空），违反 R5.6。
	//   因此通量区溢出只摘通量内容卡；备战区溢出会先尝试把卡放回 Backpack，只有非 B 主卡且
	//   Backpack 接不住时才进入 BurdenZone。
	//   若找不到任何非 B 主卡（整个数组都是 B 主卡）→ 立即终止溢出循环；Backpack /
	//   BattleDeck 临时保持 Num() > Capacity，但 B 主卡绝不会被错放入 BurdenZone。
	//
	// 退化情形说明：玩家拥有 > FluxCapacity 张 B 主卡 instance 才能触发"整数组都是 B 主卡"。
	//   该状态本身违反 GDD §11 的容量约束（一张 A 主卡贡献 1 容量），属于异常 RunState。
	//   Stage 4.5.3b UI 不主动暴露此入口；测试 fixture 不应制造此状态。
	{
		auto PopFluxContent = [this](TArray<FCardInstance>& Pile) -> bool
		{
			for (int32 i = Pile.Num() - 1; i >= 0; --i)
			{
				if (IsPreferredBurdenOverflowCandidate(Pile[i].Definition))
				{
					RunState.BurdenZone.Add(Pile[i]);
					Pile.RemoveAt(i);
					return true;
				}
			}
			for (int32 i = Pile.Num() - 1; i >= 0; --i)
			{
				if (IsFluxContentCardDefinition(Pile[i].Definition))
				{
					RunState.BurdenZone.Add(Pile[i]);
					Pile.RemoveAt(i);
					return true;
				}
			}
			return false;
		};

		auto CanMoveOverflowCardToFlux = [this](const FCardInstance& Instance) -> bool
		{
			if (!Instance.Definition)
			{
				return false;
			}
			if (IsTypeBContainerCard(Instance.Definition))
			{
				return true;
			}
			return IsFluxContentCardDefinition(Instance.Definition)
				&& CountFluxContentCards(RunState.Backpack) < GetFluxCapacity();
		};

		auto MoveBattleDeckOverflowAt = [this, &CanMoveOverflowCardToFlux](TArray<FCardInstance>& Pile, int32 Index) -> bool
		{
			if (!Pile.IsValidIndex(Index))
			{
				return false;
			}

			FCardInstance Instance = Pile[Index];
			Pile.RemoveAt(Index);

			if (CanMoveOverflowCardToFlux(Instance))
			{
				RunState.Backpack.Add(Instance);
				EnsureSpecialZoneEntryFor(Instance);
				return true;
			}

			if (IsTypeBContainerCard(Instance.Definition))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[RunSession] RecomputeBurden: BattleDeck 溢出卡 %s 是 B 主卡且无法进入通量区，临时保持在 BattleDeck 外不可行"),
					*GetNameSafe(Instance.Definition));
				Pile.Insert(Instance, Index);
				return false;
			}

			RunState.BurdenZone.Add(Instance);
			return true;
		};

		auto PopBattleDeckOverflow = [this, &MoveBattleDeckOverflowAt](TArray<FCardInstance>& Pile) -> bool
		{
			for (int32 i = Pile.Num() - 1; i >= 0; --i)
			{
				if (IsPreferredBurdenOverflowCandidate(Pile[i].Definition))
				{
					return MoveBattleDeckOverflowAt(Pile, i);
				}
			}
			for (int32 i = Pile.Num() - 1; i >= 0; --i)
			{
				if (!IsTypeBContainerCard(Pile[i].Definition))
				{
					return MoveBattleDeckOverflowAt(Pile, i);
				}
			}
			return false;  // 所有 instance 都是 B 主卡 → 无法溢出
		};

		while (CountFluxContentCards(RunState.Backpack) > GetFluxCapacity())
		{
			if (!PopFluxContent(RunState.Backpack))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[RunSession] RecomputeBurden: Backpack 没有可溢出的通量内容卡（Content=%d > FluxCapacity=%d），临时超容"),
					CountFluxContentCards(RunState.Backpack), GetFluxCapacity());
				break;
			}
		}
		while (RunState.BattleDeck.Num() > GetBattleDeckCapacity())
		{
			if (!PopBattleDeckOverflow(RunState.BattleDeck))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[RunSession] RecomputeBurden: BattleDeck 全是 B 主卡 instance（Num=%d > BattleDeckCapacity=%d），无法溢出，临时超容"),
					RunState.BattleDeck.Num(), GetBattleDeckCapacity());
				break;
			}
		}
	}

	// ---- 步骤 ② 回填（R2.14）----
	// 把 BurdenZone 头部 instance 按"通量 → 备战 → SpecialZones（数组下标升序，
	// 第一个有空位的）"优先序回填到第一个有空位的目标区。回填到 SpecialZone 时
	// 强制 bBattleEnabledInSpecialZone = false（R8.6 / R2.9：通过负重区再进入
	// SpecialZone 的卡牌不应残留旧的参战标记）。
	//
	// 若所有目标都满（含 SpecialZone capacity == 0 的退化情形），头部 instance
	// "卡住"，break 退出循环 — 剩余 BurdenZone 保持原序不变。
	//
	// 幂等性：稳态下要么 BurdenZone 已清空、要么所有目标都满；下次再调时
	// 第一次循环或立即退出，或 break 退出，均不再迁移任何 instance。
	while (bAllowBurdenRefill && RunState.BurdenZone.Num() > 0)
	{
		// 头部按值拷贝出来；后面 RemoveAt(0) 会让原引用失效。
		FCardInstance Instance = RunState.BurdenZone[0];

		// 优先级 1：通量区。B 主卡仍是特殊存放区 owner，必须回到 Backpack/BattleDeck 之一，
		// 不占通量内容格；A 类容器和普通卡按通量内容容量占格。
		if (IsTypeBContainerCard(Instance.Definition))
		{
			RunState.Backpack.Add(Instance);
			EnsureSpecialZoneEntryFor(Instance);
			RunState.BurdenZone.RemoveAt(0);
			continue;
		}
		if (IsFluxContentCardDefinition(Instance.Definition)
			&& CountFluxContentCards(RunState.Backpack) < GetFluxCapacity())
		{
			RunState.Backpack.Add(Instance);
			EnsureSpecialZoneEntryFor(Instance);
			RunState.BurdenZone.RemoveAt(0);
			continue;
		}

		// 优先级 2：备战区。
		if (RunState.BattleDeck.Num() < GetBattleDeckCapacity())
		{
			RunState.BattleDeck.Add(Instance);
			RunState.BurdenZone.RemoveAt(0);
			continue;
		}

		// 优先级 3：SpecialZones — 按数组下标升序找第一个有空位的。
		// 防御性：若 OwnerInstanceId 悬空或主卡 Physique.Capacity ≤ 1 导致
		// GetSpecialZoneCapacityFor 返回 0，视作无法接收，跳过该 zone。
		int32 PickedIdx = INDEX_NONE;
		for (int32 i = 0; i < RunState.SpecialZones.Num(); ++i)
		{
			const FSpecialZone& SZ = RunState.SpecialZones[i];
			const int32 Capacity = GetSpecialZoneCapacityFor(SZ.OwnerInstanceId);
			if (Capacity <= 0)
			{
				continue;
			}
			if (SZ.Cards.Num() < Capacity)
			{
				PickedIdx = i;
				break;
			}
		}

		if (PickedIdx != INDEX_NONE)
		{
			// R8.6 / R2.9：进入 SpecialZone 时强制重置参战标记。
			Instance.bBattleEnabledInSpecialZone = false;
			RunState.SpecialZones[PickedIdx].Cards.Add(Instance);
			RunState.BurdenZone.RemoveAt(0);
			continue;
		}

		// 所有目标都满 → 头部卡住，停止回填。
		break;
	}

	// ---- 步骤 ③ 压力写入（R9.1）----
	// 公式：n = RunState.BurdenZone.Num()，写入 Burden 通道值 = Clamp(n*(n+1)/2, 0, 100)。
	// n ≥ 14 时 n*(n+1)/2 ≥ 105 → 被 clamp 到 100（GDD §3.2 / requirements R9.1）。
	// 直接写 RunState.Pressure.Set(...) 而非 SetPressure(...)：本函数是"不广播"的私有路径
	// （R2.16 / task 9.4），SetPressure 内部会调 NotifyRunStateChanged 违反不变量。
	// FPressureValues::Set 自身内部已 clamp 到 [0, 100]，这里的 FMath::Clamp 是显式表达
	// 公式约束（保留给阅读者一眼看到 R9.1 的钳制语义）。
	const int32 N = RunState.BurdenZone.Num();
	const int32 Pressure = FMath::Clamp(N * (N + 1) / 2, 0, 100);
	RunState.Pressure.Set(EWacomPressureType::Burden, Pressure);
}

// ================ 失败综合判定 ================

bool URunSession::IsRunFailed() const
{
	if (!RunState.bRunActive)
	{
		return true;
	}
	if (IsPressureCapReached())
	{
		return true;
	}
	if (IsFingerDepleted())
	{
		return true;
	}
	return false;
}

// ================ §3.3：经验值与技能 ================

void URunSession::AddExperience(int32 Amount)
{
	if (Amount == 0)
	{
		return;
	}

	RunState.ExperienceCurrent = FMath::Max(0, RunState.ExperienceCurrent + Amount);
	TryConsumeExperienceForSkills();
	NotifyRunStateChanged();
}

void URunSession::TryConsumeExperienceForSkills()
{
	const int32 Capacity = FMath::Max(1, RunState.ExperienceCapacity);

	while (RunState.ExperienceCurrent >= Capacity)
	{
		RunState.ExperienceCurrent -= Capacity;

		// 第一阶段：用 SkillSlot.Placeholder 累加，不挂效果。
		RunState.AcquiredSkills.Add(WacomTags::SkillSlot_Placeholder);

		UE_LOG(LogTemp, Display,
			TEXT("[RunSession] Experience full → skill granted (count=%d), remaining exp=%d"),
			RunState.AcquiredSkills.Num(), RunState.ExperienceCurrent);
	}
}

// ================ §8：时段 / 节点 ================

bool URunSession::ConsumeNode(int32 Count)
{
	if (Count <= 0)
	{
		return true;
	}

	const bool bEnough = RunState.RemainingNodeCount >= Count;
	RunState.RemainingNodeCount = FMath::Max(0, RunState.RemainingNodeCount - Count);

	if (RunState.RemainingNodeCount == 0)
	{
		AdvanceToNextPhase();
		// AdvanceToNextPhase 内部已 Notify
	}
	else
	{
		NotifyRunStateChanged();
	}

	return bEnough;
}

void URunSession::AdvanceToNextPhase()
{
	const ETimePhase PrevPhase = RunState.CurrentTimePhase;

	switch (RunState.CurrentTimePhase)
	{
	case ETimePhase::Morning: RunState.CurrentTimePhase = ETimePhase::Day;     break;
	case ETimePhase::Day:     RunState.CurrentTimePhase = ETimePhase::Dusk;    break;
	case ETimePhase::Dusk:    RunState.CurrentTimePhase = ETimePhase::Night;   break;
	case ETimePhase::Night:   RunState.CurrentTimePhase = ETimePhase::Sunrise; break;
	case ETimePhase::Sunrise:
		// Sunrise 结束 = 进入次日清晨。GDD §8.1 / §8.4。
		RunState.CurrentTimePhase = ETimePhase::Morning;
		++RunState.CurrentDayNumber;
		break;
	default:
		ensureMsgf(false, TEXT("[RunSession] AdvanceToNextPhase 收到未知时段 %d"),
			(int32)RunState.CurrentTimePhase);
		RunState.CurrentTimePhase = ETimePhase::Morning;
		break;
	}

	ResetRemainingNodeForPhase();

	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] Phase advanced: Day=%d Phase=%d RemainingNodes=%d"),
		RunState.CurrentDayNumber, (int32)RunState.CurrentTimePhase, RunState.RemainingNodeCount);

	// 时段进入副作用（饥饿 / 疲劳 / 腐朽）。OnPhaseEntered 通过 AddPressure 间接广播；
	// 即使没有副作用（例如进入 Day），也在末尾发一次确保 UI 收到时段切换。
	OnPhaseEntered(RunState.CurrentTimePhase, PrevPhase);
	NotifyRunStateChanged();
}

void URunSession::OnPhaseEntered(ETimePhase NewPhase, ETimePhase PrevPhase)
{
	switch (NewPhase)
	{
	case ETimePhase::Morning:
		// GDD §3.2 饥饿：每清晨到来 +5%
		AddPressure(EWacomPressureType::Hunger, 5);
		// GDD §3.2 腐朽：完成一天 +5%。判定为"从 Sunrise 推进进入次日清晨"。
		// 露营特殊推进（Night → Morning 跳过 Sunrise）等 Stage 8 接入时
		// 在那个路径自行加腐朽，不走本分支。
		if (PrevPhase == ETimePhase::Sunrise)
		{
			AddPressure(EWacomPressureType::Decay, 5);
		}
		break;
	case ETimePhase::Dusk:
		// GDD §3.2 饥饿：每黄昏到来 +5%
		AddPressure(EWacomPressureType::Hunger, 5);
		break;
	case ETimePhase::Sunrise:
		// GDD §3.2 疲劳：每日出 +10%
		AddPressure(EWacomPressureType::Fatigue, 10);
		break;
	case ETimePhase::Day:
	case ETimePhase::Night:
	default:
		break;
	}
}

void URunSession::ResetRemainingNodeForPhase()
{
	switch (RunState.CurrentTimePhase)
	{
	case ETimePhase::Morning: RunState.RemainingNodeCount = RunState.InitialNodeCount_Morning; break;
	case ETimePhase::Day:     RunState.RemainingNodeCount = RunState.InitialNodeCount_Day;     break;
	case ETimePhase::Dusk:    RunState.RemainingNodeCount = RunState.InitialNodeCount_Dusk;    break;
	case ETimePhase::Night:   RunState.RemainingNodeCount = RunState.InitialNodeCount_Night;   break;
	case ETimePhase::Sunrise: RunState.RemainingNodeCount = RunState.InitialNodeCount_Sunrise; break;
	default:
		RunState.RemainingNodeCount = 0;
		break;
	}
}


// ================ §11：背包与备战卡组 ================

bool URunSession::IsContainerCard(const UCardDefinition* Card)
{
	return Card != nullptr && Card->Physique.Capacity > 0;
}

bool URunSession::IsTypeAContainerCard(const UCardDefinition* Card)
{
	// A 类：容器卡 且 CapacityEffect 为空（无容量效果）。
	return IsContainerCard(Card) && !Card->Physique.CapacityEffect.IsValid();
}

bool URunSession::IsTypeBContainerCard(const UCardDefinition* Card)
{
	// B 类：容器卡 且 CapacityEffect 是有效 tag。
	return IsContainerCard(Card) && Card->Physique.CapacityEffect.IsValid();
}

int32 URunSession::GetSpecialZoneCapacity(const UCardDefinition* BCard)
{
	// GDD §11.4：特殊存放区容量 = b.Capacity - 1
	if (!BCard) { return 0; }
	return FMath::Max(0, BCard->Physique.Capacity - 1);
}

void URunSession::CollectTypeBContainers(TArray<FGuid>& OutOwnerInstanceIds) const
{
	// Stage 4.5.1 任务 10.2 / R3.5 / R3.6：
	//   - 输出按 `RunState.SpecialZones` 数组下标升序、去重、不含悬空 InstanceId
	//   - 玩家无 B 主卡（SpecialZones 为空）→ 输出空数组
	//
	// 调用方语义：每条 OwnerInstanceId 都能在 Backpack ∪ BattleDeck 中找到对应 instance（R3.5 末段）。
	//
	// 注：R2.2 的双射不变量保证 SpecialZones 内 OwnerInstanceId 已唯一、且 owner instance 必定
	// 存在于 Backpack ∪ BattleDeck（B 主卡只在这两区中存在）。下面的 dedupe + dangling 检查
	// 是防御性代码，正常状态下都不会触发。
	OutOwnerInstanceIds.Reset();

	TSet<FGuid> Seen;
	for (const FSpecialZone& SZ : RunState.SpecialZones)
	{
		const FGuid& OwnerId = SZ.OwnerInstanceId;
		if (!OwnerId.IsValid())
		{
			// zero GUID owner（理论上不应存在）防御性跳过
			continue;
		}
		if (Seen.Contains(OwnerId))
		{
			// 同一 OwnerInstanceId 在 SpecialZones 中重复（理论上 R2.2 不允许）防御性跳过
			continue;
		}

		// 悬空检查：owner instance 必须在 Backpack ∪ BattleDeck 中存在。
		// 这里直接用 FindInstance 复用已有遍历逻辑（4.5.0 阶段它仅遍历 Backpack / BattleDeck，
		// 与 R3.5 末段约束一致；4.5.1 task 8.2 会扩展 FindInstance 到 BurdenZone / SpecialZones，
		// 届时仍需保证 owner instance 必须在 Backpack ∪ BattleDeck 中——R5.6 不变量
		// "B 主卡只能在 Backpack ∪ BattleDeck"——所以本判断到时改为校验 OutZone 是否
		// 属于 {Backpack, BattleDeck}。当前阶段简单的"FindInstance 命中即可"足够）。
		FCardInstance OwnerInst;
		EZoneKind     OwnerZone = EZoneKind::Backpack;
		FGuid         OwnerSelfOwner;
		if (!FindInstance(OwnerId, OwnerInst, OwnerZone, OwnerSelfOwner))
		{
			continue;
		}

		OutOwnerInstanceIds.Add(OwnerId);
		Seen.Add(OwnerId);
	}
}

bool URunSession::GetSpecialZone(FGuid OwnerInstanceId, FSpecialZone& Out) const
{
	// R2.6 严格契约：未命中（含传入 FGuid()）时不修改 Out。
	// 这里不提前清空 Out，让调用方传入的初值在未命中路径上保留。
	for (const FSpecialZone& SZ : RunState.SpecialZones)
	{
		if (SZ.OwnerInstanceId == OwnerInstanceId)
		{
			Out = SZ;
			return true;
		}
	}
	return false;
}

int32 URunSession::GetSpecialZoneCapacityFor(FGuid OwnerInstanceId) const
{
	// Stage 4.5.1 任务 7.3 / R2.5：按 OwnerInstanceId 查询某 B 主卡的特殊存放区当前容量。
	//
	// 与已有的 static GetSpecialZoneCapacity(BCard) 数值一致（公式都是 Capacity - 1 钳到 0），
	// 区别在于本函数按"当前 RunState 中实际存在的 SpecialZone"查询：
	//   ① 先在 RunState.SpecialZones 中找 OwnerInstanceId == 入参的 entry；找不到 → 返回 0
	//      （未命中即"该 InstanceId 不是任何 B 主卡 instance"或"已被销毁退回流移除 entry"）。
	//   ② 找到 entry 后，再用 FindInstance 定位 owner instance 拿到 Definition。
	//      理论上 R2.2 不变量保证 entry 存在 ⇒ owner instance 在 Backpack ∪ BattleDeck 中存在；
	//      若违反（防御性路径，例如外部直接改 RunState 的中间态）→ 返回 0，避免空指针解引用。
	//   ③ 拿到 OwnerDefinition 后走 FMath::Max(0, Definition->Physique.Capacity - 1)。
	//      Definition 为 nullptr 防御性按 0 处理；Capacity <= 1 时 Max 钳到 0（R2.5 末段约束）。

	if (!OwnerInstanceId.IsValid())
	{
		return 0;
	}

	// 步骤 ① — entry 是否存在
	bool bFoundEntry = false;
	for (const FSpecialZone& SZ : RunState.SpecialZones)
	{
		if (SZ.OwnerInstanceId == OwnerInstanceId)
		{
			bFoundEntry = true;
			break;
		}
	}
	if (!bFoundEntry)
	{
		return 0;
	}

	// 步骤 ② — 定位 owner instance 取 Definition
	FCardInstance OwnerInst;
	EZoneKind     OwnerZone = EZoneKind::Backpack;
	FGuid         OwnerSelfOwner;
	if (!FindInstance(OwnerInstanceId, OwnerInst, OwnerZone, OwnerSelfOwner))
	{
		return 0;
	}

	// 步骤 ③ — 公式与 static GetSpecialZoneCapacity 一致
	const UCardDefinition* OwnerDef = OwnerInst.Definition;
	if (!OwnerDef)
	{
		return 0;
	}
	return FMath::Max(0, OwnerDef->Physique.Capacity - 1);
}

bool URunSession::SetSpecialZoneCardBattleEnabled(FGuid InstanceId, bool bEnabled)
{
	// Stage 4.5.1 任务 8.3 / R2.10 / R8.1 / R8.5：仅切 flag 不移卡。
	//
	// 决策 Q-D 的核心契约（design §Components and Interfaces #4）：
	//   - 命中某 SpecialZone.Cards → 只改 bBattleEnabledInSpecialZone 字段，
	//     不修改 instance 在 SpecialZone 中的物理归属（R8.1：仍由原数组在原下标位置持有）；
	//   - 未命中（任何原因：InstanceId 不存在 / 在 Backpack / BattleDeck / BurdenZone）
	//     → return false 且不修改 RunState、不广播（R2.10 严格契约）。
	//
	// 注：bEnabled 与当前 flag 值相等时仍走成功路径并广播一次。这与 design §4 广播规则表
	// 一致——粗粒度多播不 dedupe，调用方/订阅方刷新逻辑应保证幂等。
	//
	// 不同于 MoveInstance 复用 FindInstance：本函数需要通过引用直接修改 SpecialZone.Cards
	// 中的字段，而 FindInstance 输出的是按值拷贝（R1.8 严格契约）。因此这里直接遍历
	// SpecialZones 拿到可写引用。

	if (!InstanceId.IsValid())
	{
		// zero GUID 不在任何 SpecialZone 中，直接拒绝（不警告：UI 端可能在状态过期时调用）。
		return false;
	}

	for (FSpecialZone& SZ : RunState.SpecialZones)
	{
		for (FCardInstance& Inst : SZ.Cards)
		{
			if (Inst.InstanceId == InstanceId)
			{
				Inst.bBattleEnabledInSpecialZone = bEnabled;
				NotifyRunStateChanged();
				return true;
			}
		}
	}

	// InstanceId 不在任何 SpecialZone 中（可能在 Backpack / BattleDeck / BurdenZone 或不存在）
	// → R2.10 失败路径：不修改、不广播、return false。
	return false;
}

bool URunSession::IsBagProviderCard(const UCardDefinition* Card)
{
	return Card != nullptr && Card->Keywords.HasTagExact(WacomTags::Card_Keyword_BagProvider);
}

bool URunSession::IsDeleteProviderCard(const UCardDefinition* Card)
{
	return Card != nullptr && Card->Keywords.HasTagExact(WacomTags::Card_Keyword_DeleteProvider);
}

bool URunSession::IsIntrinsicCard(const UCardDefinition* Card)
{
	return Card != nullptr && Card->Rarity.MatchesTagExact(WacomTags::Card_Rarity_Intrinsic);
}

int32 URunSession::GetFluxCapacity() const
{
	// GDD §11.4 容量公式：
	//   通量内容容量 = Σ(玩家拥有的所有 A 类容器卡 Capacity)
	//
	// "玩家拥有" = 当前 RunState 全部物理持有区。
	// A 类容器卡自身也作为通量内容卡显示 / 占格，不再有通量主卡位。
	// B 类容器卡（CapacityEffect 非空）不进通量公式，自行开辟特殊存放区。
	return SumOwnedCardCapacity(/*bTypeAOnly=*/true);
}

int32 URunSession::GetBattleDeckCapacity() const
{
	// GDD §11.4 容量公式：
	//   备战区容量 = Σ(玩家拥有的所有容器卡 Capacity)
	//
	// A 类和 B 类容器卡都计入；UI 中的主卡投影不改变物理 instance 归属。
	return SumOwnedCardCapacity(/*bTypeAOnly=*/false);
}

FRunBackpackStorageSnapshot URunSession::BuildBackpackStorageSnapshot() const
{
	FRunBackpackStorageSnapshot Snapshot;
	Snapshot.FluxCapacity = GetFluxCapacity();
	Snapshot.BattleDeckCapacity = GetBattleDeckCapacity();
	Snapshot.BackpackPhysicalCount = RunState.Backpack.Num();
	Snapshot.BattleDeckPhysicalCount = RunState.BattleDeck.Num();
	Snapshot.BurdenCount = RunState.BurdenZone.Num();
	Snapshot.Flux.FluxCapacity = Snapshot.FluxCapacity;

	auto MakeCardView = [](const FCardInstance& Inst, EZoneKind PhysicalZone, FGuid ZoneOwnerInstanceId)
	{
		FRunStorageCardView View;
		View.Instance = Inst;
		View.PhysicalZone = PhysicalZone;
		View.ZoneOwnerInstanceId = ZoneOwnerInstanceId;
		View.bIsContainer = URunSession::IsContainerCard(Inst.Definition);
		View.bIsTypeAContainer = URunSession::IsTypeAContainerCard(Inst.Definition);
		View.bIsTypeBContainer = URunSession::IsTypeBContainerCard(Inst.Definition);
		View.bIsPhysicalInBattleDeck = PhysicalZone == EZoneKind::BattleDeck;
		return View;
	};

	for (const FCardInstance& Inst : RunState.Backpack)
	{
		if (!Inst.Definition)
		{
			continue;
		}

		if (IsFluxContentCardDefinition(Inst.Definition))
		{
			Snapshot.Flux.ContentCards.Add(MakeCardView(Inst, EZoneKind::Backpack, FGuid()));
		}
	}
	Snapshot.FluxContentCount = Snapshot.Flux.ContentCards.Num();

	for (const FCardInstance& Inst : RunState.BattleDeck)
	{
		if (!Inst.Definition)
		{
			continue;
		}

		const FRunStorageCardView View = MakeCardView(Inst, EZoneKind::BattleDeck, FGuid());
		Snapshot.BattleDeckPhysicalCards.Add(View);

		// BattleDeck 中的 A 类容器仍贡献通量容量，但不投影到通量内容区；
		// 它只作为物理备战卡显示在 BattleDeckPhysicalCards。
	}

	for (const FSpecialZone& SZ : RunState.SpecialZones)
	{
		FCardInstance OwnerInst;
		EZoneKind OwnerZone = EZoneKind::Backpack;
		FGuid OwnerSelfOwnerId;
		if (!FindInstance(SZ.OwnerInstanceId, OwnerInst, OwnerZone, OwnerSelfOwnerId)
			|| !OwnerInst.Definition
			|| !IsTypeBContainerCard(OwnerInst.Definition)
			|| (OwnerZone != EZoneKind::Backpack && OwnerZone != EZoneKind::BattleDeck))
		{
			continue;
		}

		FRunSpecialStorageView SpecialView;
		SpecialView.OwnerCard = MakeCardView(OwnerInst, OwnerZone, FGuid());
		SpecialView.Capacity = GetSpecialZoneCapacityFor(SZ.OwnerInstanceId);
		SpecialView.bOwnerInBattleDeck = OwnerZone == EZoneKind::BattleDeck;

		for (const FCardInstance& Inst : SZ.Cards)
		{
			if (!Inst.Definition)
			{
				continue;
			}

			FRunStorageCardView ContentView = MakeCardView(Inst, EZoneKind::SpecialZone, SZ.OwnerInstanceId);
			SpecialView.ContentCards.Add(ContentView);
			if (SpecialView.bOwnerInBattleDeck && Inst.bBattleEnabledInSpecialZone)
			{
				Snapshot.BattleDeckProjectedCards.Add(ContentView);
			}
		}

		Snapshot.SpecialZones.Add(MoveTemp(SpecialView));
	}

	for (const FCardInstance& Inst : RunState.BurdenZone)
	{
		if (!Inst.Definition)
		{
			continue;
		}
		Snapshot.BurdenCards.Add(MakeCardView(Inst, EZoneKind::BurdenZone, FGuid()));
	}

	return Snapshot;
}

int32 URunSession::SumOwnedCardCapacity(bool bTypeAOnly) const
{
	auto ShouldCount = [bTypeAOnly](const UCardDefinition* Card)
	{
		return bTypeAOnly
			? URunSession::IsTypeAContainerCard(Card)
			: URunSession::IsContainerCard(Card);
	};

	int32 Sum = 0;
	auto AccumulatePile = [&Sum, &ShouldCount](const TArray<FCardInstance>& Pile)
	{
		for (const FCardInstance& Inst : Pile)
		{
			if (ShouldCount(Inst.Definition))
			{
				Sum += Inst.Definition->Physique.Capacity;
			}
		}
	};

	AccumulatePile(RunState.Backpack);
	AccumulatePile(RunState.BattleDeck);
	AccumulatePile(RunState.BurdenZone);
	for (const FSpecialZone& SpecialZone : RunState.SpecialZones)
	{
		AccumulatePile(SpecialZone.Cards);
	}

	return Sum;
}

int32 URunSession::CountFluxContentCards(const TArray<FCardInstance>& Pile) const
{
	int32 Count = 0;
	for (const FCardInstance& Inst : Pile)
	{
		if (IsFluxContentCardDefinition(Inst.Definition))
		{
			++Count;
		}
	}
	return Count;
}

bool URunSession::IsBackpackUiAvailable() const
{
	auto HasCapacityProvider = [](const TArray<FCardInstance>& Pile)
	{
		for (const FCardInstance& Inst : Pile)
		{
			if (IsContainerCard(Inst.Definition))
			{
				return true;
			}
		}
		return false;
	};
	if (HasCapacityProvider(RunState.Backpack) || HasCapacityProvider(RunState.BattleDeck) || HasCapacityProvider(RunState.BurdenZone))
	{
		return true;
	}
	for (const FSpecialZone& SpecialZone : RunState.SpecialZones)
	{
		if (HasCapacityProvider(SpecialZone.Cards))
		{
			return true;
		}
	}
	return false;
}

bool URunSession::IsDeleteFunctionAvailable() const
{
	auto HasProvider = [](const TArray<FCardInstance>& Pile)
	{
		for (const FCardInstance& Inst : Pile)
		{
			if (IsDeleteProviderCard(Inst.Definition))
			{
				return true;
			}
		}
		return false;
	};
	if (HasProvider(RunState.Backpack) || HasProvider(RunState.BattleDeck) || HasProvider(RunState.BurdenZone))
	{
		return true;
	}
	for (const FSpecialZone& SpecialZone : RunState.SpecialZones)
	{
		if (HasProvider(SpecialZone.Cards))
		{
			return true;
		}
	}
	return false;
}

bool URunSession::IsCardInBackpack(const UCardDefinition* Card) const
{
	// R1.9：Card == nullptr → false；否则按 Definition 在 Backpack 内查到任意一张返回 true。
	// 实现细节：复用 FindFirstIndexByDefinition（Card==nullptr 时该 helper 返回 INDEX_NONE）。
	return FindFirstIndexByDefinition(RunState.Backpack, Card) != INDEX_NONE;
}

bool URunSession::IsCardInBattleDeck(const UCardDefinition* Card) const
{
	// R1.9：Card == nullptr → false；否则按 Definition 在 BattleDeck 内查到任意一张返回 true。
	return FindFirstIndexByDefinition(RunState.BattleDeck, Card) != INDEX_NONE;
}

bool URunSession::FindInstance(FGuid InstanceId, FCardInstance& OutInstance, EZoneKind& OutZone, FGuid& OutZoneOwnerInstanceId) const
{
	// Stage 4.5.0 任务 3.1 / R1.8：
	//   - InstanceId 为 `FGuid()`（无效）直接返回 false，避免误命中"InstanceId 默认值的占位 instance"
	//     （所有合法 instance 在 Initialize / AddCardToBackpack 路径上都有非零 InstanceId，
	//      所以 zero GUID 不应在 zone 中出现；这里防御性短路）。
	//   - 命中：写入三个 out 参数后 return true。
	//   - 未命中：保持 out 不变并 return false（R1.8 严格契约——不能覆写调用方传入的初值）。
	//
	// Stage 4.5.1 任务 8.1：遍历范围扩展到全部四区
	//   Backpack / BattleDeck / BurdenZone / ⋃SpecialZones.Cards
	//   - Backpack / BattleDeck / BurdenZone 命中 → OutZoneOwnerInstanceId = FGuid()
	//   - SpecialZone 命中 → OutZoneOwnerInstanceId = 该 SpecialZone 的 OwnerInstanceId（B 主卡 InstanceId）
	if (!InstanceId.IsValid())
	{
		return false;
	}

	for (const FCardInstance& Inst : RunState.Backpack)
	{
		if (Inst.InstanceId == InstanceId)
		{
			OutInstance              = Inst;
			OutZone                  = EZoneKind::Backpack;
			OutZoneOwnerInstanceId   = FGuid();
			return true;
		}
	}

	for (const FCardInstance& Inst : RunState.BattleDeck)
	{
		if (Inst.InstanceId == InstanceId)
		{
			OutInstance              = Inst;
			OutZone                  = EZoneKind::BattleDeck;
			OutZoneOwnerInstanceId   = FGuid();
			return true;
		}
	}

	for (const FCardInstance& Inst : RunState.BurdenZone)
	{
		if (Inst.InstanceId == InstanceId)
		{
			OutInstance              = Inst;
			OutZone                  = EZoneKind::BurdenZone;
			OutZoneOwnerInstanceId   = FGuid();
			return true;
		}
	}

	for (const FSpecialZone& SZ : RunState.SpecialZones)
	{
		for (const FCardInstance& Inst : SZ.Cards)
		{
			if (Inst.InstanceId == InstanceId)
			{
				OutInstance              = Inst;
				OutZone                  = EZoneKind::SpecialZone;
				OutZoneOwnerInstanceId   = SZ.OwnerInstanceId;
				return true;
			}
		}
	}

	return false;
}

bool URunSession::MoveInstance(FGuid InstanceId, EZoneKind ToZone, FGuid ToZoneOwnerInstanceId)
{
	// Stage 4.5.0 任务 3.2 / R1.6 / R1.7：通用迁移入口。
	// Stage 4.5.1 任务 8.1：扩展支持 SpecialZone / BurdenZone（含 R2.7 a-d 校验表）。
	//
	// 设计要点：
	//   1) 任何校验失败都"原子拒绝"：不修改 RunState、不广播 OnRunStateChangedNative（R5.5）。
	//   2) "in-place 移动"语义：当 FromZone == ToZone（且 SpecialZone 同 owner）时仍走"先移除再追加"
	//      路径，副作用是末尾位置变化；ToZone 容量校验排除 in-place 情况以避免误判（见各分支
	//      EffectiveCount 计算）。
	//   3) 从 SpecialZone 移出、或从其他 zone 进入 SpecialZone 时，清理
	//      bBattleEnabledInSpecialZone；同一 SpecialZone 内重排保留原 flag。

	const FRunDeckOperationValidation Validation = ValidateMoveInstance(InstanceId, ToZone, ToZoneOwnerInstanceId);
	if (!Validation.bCanExecute)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] MoveInstance: 拒绝 InstanceId %s ToZone=%d Reason=%s"),
			*InstanceId.ToString(), (int32)ToZone, *Validation.DisabledReason.ToString());
		return false;
	}

	// 1) 找源 zone（R1.7：InstanceId 在所有 zone 中均不存在 → 拒绝）。
	//    复用 FindInstance（task 8.1 起遍历全部四区）。
	FCardInstance Found;
	EZoneKind     FromZone               = EZoneKind::Backpack;
	FGuid         FromZoneOwnerInstanceId; // 仅当 FromZone == SpecialZone 时由 FindInstance 写入主卡 InstanceId
	if (!FindInstance(InstanceId, Found, FromZone, FromZoneOwnerInstanceId))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] MoveInstance: InstanceId %s 在所有 zone 中不存在，拒绝"),
			*InstanceId.ToString());
		return false;
	}

	// 2) 校验目标 zone（按 design §5 校验表）。
	switch (ToZone)
	{
	case EZoneKind::Backpack:
	{
		if (IsFluxContentCardDefinition(Found.Definition))
		{
			const int32 CurrentCount = CountFluxContentCards(RunState.Backpack);
			const bool bInPlaceBackpack = FromZone == EZoneKind::Backpack;
			const int32 EffectiveCount = bInPlaceBackpack ? (CurrentCount - 1) : CurrentCount;
			const int32 Capacity = GetFluxCapacity();
			if (EffectiveCount >= Capacity)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[RunSession] MoveInstance: Backpack 通量内容已达容量上限 %d，拒绝 InstanceId %s"),
					Capacity, *InstanceId.ToString());
				return false;
			}
		}
		break;
	}

	case EZoneKind::BattleDeck:
	{
		// in-place 移动（FromZone 已是 BattleDeck）不计入容量检查——instance 总数不变。
		const int32 EffectiveCount = (FromZone == EZoneKind::BattleDeck)
			? (RunState.BattleDeck.Num() - 1)
			: RunState.BattleDeck.Num();
		const int32 Capacity = GetBattleDeckCapacity();
		if (EffectiveCount >= Capacity)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RunSession] MoveInstance: BattleDeck 已达容量上限 %d，拒绝 InstanceId %s"),
				Capacity, *InstanceId.ToString());
			return false;
		}
		break;
	}

	case EZoneKind::SpecialZone:
	{
		// Stage 4.5.1 任务 8.1 / R2.7：SpecialZone 校验表 a-e
		//   a) ToZoneOwnerInstanceId 在 RunState.SpecialZones 中存在
		//   b) InstanceId != ToZoneOwnerInstanceId（B 主卡不能放进自己的 SpecialZone）
		//   c) 目标 SpecialZone.Cards.Num() < GetSpecialZoneCapacityFor(ToZoneOwnerInstanceId)
		//   d) InstanceId 在所有 zone 中存在 — 已由步骤 1 的 FindInstance 调用校验，
		//      此处无需重复检查。
		//   e) Found 不能是 B 主卡 instance（R2.7.e / R5.6 不变量：B 主卡不能进入任何
		//      SpecialZone.Cards；与 R2.7.b 自指拒绝独立 — b 只覆盖
		//      InstanceId == ToOwner 的自指情形，e 还要拦"B 主卡试图进入另一张 B 主卡的 SZ"）
		//
		// 任一校验失败 → return false + 不修改 RunState + 不广播（R2.7 / R5.5 / R5.6）。

		// (a) 找到目标 SpecialZone entry。
		const int32 ToSZIdx = RunState.SpecialZones.IndexOfByPredicate(
			[&](const FSpecialZone& SZ) { return SZ.OwnerInstanceId == ToZoneOwnerInstanceId; });
		if (ToSZIdx == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RunSession] MoveInstance: ToZoneOwnerInstanceId %s 在 SpecialZones 中不存在，拒绝（R2.7.a）"),
				*ToZoneOwnerInstanceId.ToString());
			return false;
		}

		// (b) 自指拒绝。
		if (InstanceId == ToZoneOwnerInstanceId)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RunSession] MoveInstance: InstanceId %s 不能放入自己的 SpecialZone（R2.7.b）"),
				*InstanceId.ToString());
			return false;
		}

		// (e) B 主卡不能放进任何 SpecialZone.Cards（R2.7.e / R5.6）。
		//     必须在 (c) 容量校验之前判定，避免"刚好容量满 + B 主卡"时被 (c) 抢先拒绝
		//     而日志误报"容量满"，掩盖真实根因。
		if (IsTypeBContainerCard(Found.Definition))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RunSession] MoveInstance: InstanceId %s 是 B 主卡 instance，拒绝放入 SpecialZone(%s)（R2.7.e / R5.6 不变量）"),
				*InstanceId.ToString(), *ToZoneOwnerInstanceId.ToString());
			return false;
		}

		// (c) 容量校验。FromZone == SpecialZone 且源 == 目标 SpecialZone（in-place）时
		//     不计入 capacity 检查，因为本张 instance 已计在 Cards.Num() 内，移除后再追加
		//     总数不变。
		const int32 CurrentCount = RunState.SpecialZones[ToSZIdx].Cards.Num();
		const bool bInPlaceSameSZ =
			(FromZone == EZoneKind::SpecialZone)
			&& (FromZoneOwnerInstanceId == ToZoneOwnerInstanceId);
		const int32 EffectiveCount = bInPlaceSameSZ ? (CurrentCount - 1) : CurrentCount;
		const int32 Capacity = GetSpecialZoneCapacityFor(ToZoneOwnerInstanceId);
		if (EffectiveCount >= Capacity)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RunSession] MoveInstance: SpecialZone(%s) 已达容量上限 %d，拒绝 InstanceId %s（R2.7.c）"),
				*ToZoneOwnerInstanceId.ToString(), Capacity, *InstanceId.ToString());
			return false;
		}
		break;
	}

	case EZoneKind::BurdenZone:
		// Stage 4.5.1 任务 8.1 / design §5 校验表：BurdenZone 无额外容量校验，
		// 但 R2.7a / R5.6 不变量要求 B 主卡 instance 不能进入 BurdenZone。
		// API 允许写入；UI 层不主动暴露此入口（design §5 注）。
		if (IsTypeBContainerCard(Found.Definition))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RunSession] MoveInstance: InstanceId %s 是 B 主卡 instance，拒绝放入 BurdenZone（R2.7a / R5.6 不变量）"),
				*InstanceId.ToString());
			return false;
		}
		break;

	default:
		ensureMsgf(false,
			TEXT("[RunSession] MoveInstance: 未知 ToZone 枚举 %d"), (int32)ToZone);
		return false;
	}

	// 3) 校验全部通过 → 真正修改 RunState。
	//    "先从源删除、再追加到目标"：避免在中间态广播。
	//    保留 InstanceId / Definition / bBattleEnabledInSpecialZone 三字段值不变（迁移整张 instance）。
	switch (FromZone)
	{
	case EZoneKind::Backpack:
	{
		const int32 SrcIdx = RunState.Backpack.IndexOfByPredicate(
			[&](const FCardInstance& I) { return I.InstanceId == InstanceId; });
		check(SrcIdx != INDEX_NONE);  // FindInstance 已确认存在
		RunState.Backpack.RemoveAt(SrcIdx);
		break;
	}
	case EZoneKind::BattleDeck:
	{
		const int32 SrcIdx = RunState.BattleDeck.IndexOfByPredicate(
			[&](const FCardInstance& I) { return I.InstanceId == InstanceId; });
		check(SrcIdx != INDEX_NONE);
		RunState.BattleDeck.RemoveAt(SrcIdx);
		break;
	}
	case EZoneKind::BurdenZone:
	{
		// Stage 4.5.1 任务 8.1：从 BurdenZone 移除。
		const int32 SrcIdx = RunState.BurdenZone.IndexOfByPredicate(
			[&](const FCardInstance& I) { return I.InstanceId == InstanceId; });
		check(SrcIdx != INDEX_NONE);
		RunState.BurdenZone.RemoveAt(SrcIdx);
		break;
	}
	case EZoneKind::SpecialZone:
	{
		// Stage 4.5.1 任务 8.1：从 SpecialZone 移除。
		// FindInstance 已写入 FromZoneOwnerInstanceId，按 OwnerInstanceId 找到对应 entry，
		// 再在 Cards 中找 InstanceId 移除。
		const int32 SrcSZIdx = RunState.SpecialZones.IndexOfByPredicate(
			[&](const FSpecialZone& SZ) { return SZ.OwnerInstanceId == FromZoneOwnerInstanceId; });
		check(SrcSZIdx != INDEX_NONE);
		const int32 SrcIdx = RunState.SpecialZones[SrcSZIdx].Cards.IndexOfByPredicate(
			[&](const FCardInstance& I) { return I.InstanceId == InstanceId; });
		check(SrcIdx != INDEX_NONE);
		RunState.SpecialZones[SrcSZIdx].Cards.RemoveAt(SrcIdx);
		break;
	}
	default:
		ensureMsgf(false,
			TEXT("[RunSession] MoveInstance: 未知 FromZone 枚举 %d"), (int32)FromZone);
		return false;
	}

	const bool bSameSpecialZoneMove =
		FromZone == EZoneKind::SpecialZone
		&& ToZone == EZoneKind::SpecialZone
		&& FromZoneOwnerInstanceId == ToZoneOwnerInstanceId;
	if (!bSameSpecialZoneMove && (FromZone == EZoneKind::SpecialZone || ToZone == EZoneKind::SpecialZone))
	{
		Found.bBattleEnabledInSpecialZone = false;
	}

	switch (ToZone)
	{
	case EZoneKind::Backpack:
		RunState.Backpack.Add(Found);
		break;
	case EZoneKind::BattleDeck:
		RunState.BattleDeck.Add(Found);
		break;
	case EZoneKind::BurdenZone:
		RunState.BurdenZone.Add(Found);
		break;
	case EZoneKind::SpecialZone:
	{
		// 校验阶段已确保 ToZoneOwnerInstanceId 在 SpecialZones 中存在。
		const int32 ToSZIdx = RunState.SpecialZones.IndexOfByPredicate(
			[&](const FSpecialZone& SZ) { return SZ.OwnerInstanceId == ToZoneOwnerInstanceId; });
		check(ToSZIdx != INDEX_NONE);
		RunState.SpecialZones[ToSZIdx].Cards.Add(Found);
		break;
	}
	default:
		ensureMsgf(false,
			TEXT("[RunSession] MoveInstance: 未知 ToZone 枚举 %d 的提交路径"), (int32)ToZone);
		return false;
	}

	// Stage 4.5.1 任务 7.1 / R2.3 + R5.1 跟随依赖：
	//   B 主卡 instance 跨入 Backpack/BattleDeck 时幂等保底追加 SpecialZones entry。
	//   正常路径上 entry 在 Initialize / AddCardToBackpack 阶段已创建，本调用主要起防御作用：
	//     - 外部直接构造 RunState 后调 MoveInstance 的测试路径；
	//     - R5.1 跟随依赖：B 主卡跨 Backpack ↔ BattleDeck 移动时 entry 必须存在。
	//   非 B 主卡 / 目标 zone 不是 Backpack/BattleDeck（task 8.1 起 SpecialZone / BurdenZone 也会
	//   走到这里，但 helper 内部按 IsTypeBContainerCard 短路）由 helper 内部短路。
	if (ToZone == EZoneKind::Backpack || ToZone == EZoneKind::BattleDeck)
	{
		EnsureSpecialZoneEntryFor(Found);
	}

	UE_LOG(LogTemp, Verbose,
		TEXT("[RunSession] MoveInstance: InstanceId=%s, %d → %d, Backpack=%d, BattleDeck=%d"),
		*InstanceId.ToString(), (int32)FromZone, (int32)ToZone,
		RunState.Backpack.Num(), RunState.BattleDeck.Num());

	if (ToZone != EZoneKind::BurdenZone)
	{
		RecomputeBurdenInternal();
	}
	NotifyRunStateChanged();
	return true;
}

bool URunSession::HasCapacityProviderAfterDestroyingFirstOwnedInstance(const UCardDefinition* Card) const
{
	bool bSkippedTargetInstance = false;
	auto HasProviderAfterSkippingTarget = [&bSkippedTargetInstance, Card](const TArray<FCardInstance>& Pile) -> bool
	{
		for (const FCardInstance& Inst : Pile)
		{
			if (!bSkippedTargetInstance && Inst.Definition == Card)
			{
				bSkippedTargetInstance = true;
				continue;
			}
			if (IsContainerCard(Inst.Definition))
			{
				return true;
			}
		}
		return false;
	};

	if (HasProviderAfterSkippingTarget(RunState.Backpack)
		|| HasProviderAfterSkippingTarget(RunState.BattleDeck)
		|| HasProviderAfterSkippingTarget(RunState.BurdenZone))
	{
		return true;
	}

	for (const FSpecialZone& SpecialZone : RunState.SpecialZones)
	{
		if (HasProviderAfterSkippingTarget(SpecialZone.Cards))
		{
			return true;
		}
	}

	return false;
}

FRunDeckOperationValidation URunSession::ValidateMoveInstance(FGuid InstanceId, EZoneKind ToZone, FGuid ToZoneOwnerInstanceId) const
{
	FRunDeckOperationValidation Result;
	Result.DisabledReason = TEXT("Unknown");

	FCardInstance Found;
	EZoneKind FromZone = EZoneKind::Backpack;
	FGuid FromZoneOwnerInstanceId;
	if (!FindInstance(InstanceId, Found, FromZone, FromZoneOwnerInstanceId))
	{
		Result.DisabledReason = TEXT("CardNotFound");
		return Result;
	}

	switch (ToZone)
	{
	case EZoneKind::Backpack:
	{
		if (IsFluxContentCardDefinition(Found.Definition))
		{
			const int32 CurrentCount = CountFluxContentCards(RunState.Backpack);
			const bool bInPlaceBackpack = FromZone == EZoneKind::Backpack;
			const int32 EffectiveCount = bInPlaceBackpack ? (CurrentCount - 1) : CurrentCount;
			if (EffectiveCount >= GetFluxCapacity())
			{
				Result.DisabledReason = TEXT("FluxFull");
				return Result;
			}
		}
		break;
	}

	case EZoneKind::BattleDeck:
	{
		const int32 EffectiveCount = (FromZone == EZoneKind::BattleDeck)
			? (RunState.BattleDeck.Num() - 1)
			: RunState.BattleDeck.Num();
		if (EffectiveCount >= GetBattleDeckCapacity())
		{
			Result.DisabledReason = TEXT("BattleDeckFull");
			return Result;
		}
		break;
	}

	case EZoneKind::SpecialZone:
	{
		const int32 ToSZIdx = RunState.SpecialZones.IndexOfByPredicate(
			[&](const FSpecialZone& SZ) { return SZ.OwnerInstanceId == ToZoneOwnerInstanceId; });
		if (ToSZIdx == INDEX_NONE)
		{
			Result.DisabledReason = TEXT("SpecialZoneMissing");
			return Result;
		}
		if (InstanceId == ToZoneOwnerInstanceId)
		{
			Result.DisabledReason = TEXT("SelfSpecialZone");
			return Result;
		}
		if (IsTypeBContainerCard(Found.Definition))
		{
			Result.DisabledReason = TEXT("TypeBInSpecialZone");
			return Result;
		}

		const int32 CurrentCount = RunState.SpecialZones[ToSZIdx].Cards.Num();
		const bool bInPlaceSameSZ =
			(FromZone == EZoneKind::SpecialZone)
			&& (FromZoneOwnerInstanceId == ToZoneOwnerInstanceId);
		const int32 EffectiveCount = bInPlaceSameSZ ? (CurrentCount - 1) : CurrentCount;
		if (EffectiveCount >= GetSpecialZoneCapacityFor(ToZoneOwnerInstanceId))
		{
			Result.DisabledReason = TEXT("SpecialZoneFull");
			return Result;
		}
		break;
	}

	case EZoneKind::BurdenZone:
		if (IsTypeBContainerCard(Found.Definition))
		{
			Result.DisabledReason = TEXT("TypeBInBurdenZone");
			return Result;
		}
		break;

	default:
		Result.DisabledReason = TEXT("InvalidTargetZone");
		return Result;
	}

	Result.bCanExecute = true;
	Result.DisabledReason = NAME_None;
	return Result;
}

void URunSession::AddCardToBackpack(UCardDefinition* Card)
{
	// Stage 4.5.0 任务 2.3 / R1.5：
	//   - Card == nullptr：拒绝、不修改 Backpack、UE_LOG Warning（R1.5）
	//   - Card != nullptr：生成新 InstanceId 包成 FCardInstance 追加（R1.5）
	//   - 兜底 ensureMsgf(InstanceId.IsValid())（R1.14，仅 Editor / Debug build 触发）
	if (!Card)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] AddCardToBackpack: Card 为空，拒绝"));
		return;
	}
	FCardInstance Inst;
	Inst.Definition = Card;
	Inst.InstanceId = FGuid::NewGuid();
	ensureMsgf(Inst.InstanceId.IsValid(),
		TEXT("[RunSession] AddCardToBackpack: FGuid::NewGuid() 生成了 zero GUID，违反 R1.14 不变量"));
	RunState.Backpack.Add(Inst);
	// Stage 4.5.1 任务 7.1 / R2.3：B 主卡新加入背包时幂等追加 SpecialZones entry。
	//   非 B 主卡 / zero GUID 由 helper 内部短路。
	EnsureSpecialZoneEntryFor(Inst);
	// 容器卡新加入时贡献新容量，可能让超容卡变得不再超容；非容器卡新加入则可能造成超容。
	// 任何情况都重算一次。走"不广播"的私有路径：本函数尾部统一 NotifyRunStateChanged
	// 一次（R2.16 / task 9.4：避免一次操作多次广播尾部串）。
	RecomputeBurdenInternal();
	NotifyRunStateChanged();
}

void URunSession::AcquireCardToRun(UCardDefinition* Card)
{
	if (AcquireCardToRunInternal(Card))
	{
		NotifyRunStateChanged();
	}
}

bool URunSession::AcquireCardToRunInternal(UCardDefinition* Card)
{
	if (!Card)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] AcquireCardToRun: Card 为空，拒绝"));
		return false;
	}

	FCardInstance Inst;
	Inst.Definition = Card;
	Inst.InstanceId = FGuid::NewGuid();
	ensureMsgf(Inst.InstanceId.IsValid(),
		TEXT("[RunSession] AcquireCardToRun: FGuid::NewGuid() 生成了 zero GUID，违反 R1.14 不变量"));
	RunState.Backpack.Add(Inst);
	EnsureSpecialZoneEntryFor(Inst);
	RecomputeBurdenInternal();
	return true;
}

bool URunSession::DestroyCardFromBackpack(UCardDefinition* Card)
{
	// public 入口（Stage 4.5.1 任务 9.4 / R2.16）：
	//   委托 Internal 完成 zone 修改 + 压力 + 退回流，再在末尾广播一次。
	//   DeleteCardForGold 不走此 public 入口，改调 Internal 后自行发尾部广播一次。
	const bool bOk = DestroyCardFromBackpackInternal(Card);
	if (bOk)
	{
		NotifyRunStateChanged();
	}
	return bOk;
}

bool URunSession::DestroyCardFromBackpackInternal(UCardDefinition* Card)
{
	if (!Card)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] DestroyCardFromBackpack: Card 为空"));
		return false;
	}

	// 1) 必须在 Backpack 或 BattleDeck 里（玩家拥有），其中任何一个分支都允许销毁。
	//    GDD §11.4：一张卡同时只能在一个区，所以两边只会在一处找到。
	//
	// Stage 4.5.0 任务 2.4 / R1.10：zone 元素是 FCardInstance；按 Definition 在数组内
	// 下标升序选第一个匹配的 instance 操作（FindFirstIndexByDefinition 实现）。
	const int32 BackpackIdx   = FindFirstIndexByDefinition(RunState.Backpack,   Card);
	const int32 BattleDeckIdx = FindFirstIndexByDefinition(RunState.BattleDeck, Card);
	const bool bInBackpack    = (BackpackIdx   != INDEX_NONE);
	const bool bInBattleDeck  = (BattleDeckIdx != INDEX_NONE);
	if (!bInBackpack && !bInBattleDeck)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] DestroyCardFromBackpack: %s 不在玩家手中"), *GetNameSafe(Card));
		return false;
	}

	// 2) Intrinsic 拒绝（GDD §3.5 / §11.8）
	if (IsIntrinsicCard(Card))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] DestroyCardFromBackpack: %s 是 Intrinsic，拒绝销毁"),
			*GetNameSafe(Card));
		return false;
	}

	// 3) 最后一张容量来源卡拒绝（销毁后玩家无容器卡会让背包容量归零）
	if (IsContainerCard(Card) && !HasCapacityProviderAfterDestroyingFirstOwnedInstance(Card))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] DestroyCardFromBackpack: %s 是最后一张容量来源卡，拒绝销毁"),
			*GetNameSafe(Card));
		return false;
	}

	// 4) 移除一张：哪边有就从哪边移除（按 R1.10 的"下标升序匹配第一个"语义）
	//
	// Stage 4.5.1 任务 7.2 / R2.4：销毁的 instance 若是 B 主卡，需要保留其 InstanceId
	// 以便随后在 SpecialZones 中找到对应 entry 并按数组下标升序逐张退回内含卡。先把要销毁
	// 的 FCardInstance 拷贝出来再 RemoveAt，确保拿到 InstanceId 后引用不悬空。
	const bool bDestroyedIsBMaster = IsTypeBContainerCard(Card);
	FCardInstance DestroyedInst;
	if (bInBackpack)
	{
		DestroyedInst = RunState.Backpack[BackpackIdx];
		RunState.Backpack.RemoveAt(BackpackIdx);
	}
	else
	{
		DestroyedInst = RunState.BattleDeck[BattleDeckIdx];
		RunState.BattleDeck.RemoveAt(BattleDeckIdx);
	}

	// 5) 若 Card 带 Companion 关键词 → 嗜血 +1%
	//    Stage 4.5.1 任务 9.4 / R2.16：直接写 RunState.Pressure 而非调用
	//    OnCompanionCardPermanentlyDestroyed()。后者会触发 NotifyRunStateChanged，
	//    破坏"public 入口尾部统一发一次"的不变量（一次玩家销毁 Companion 卡会被发
	//    多次广播，订阅方虽幂等但成本浪费且违反 R2.16 字面规则）。
	//    OnCompanionCardPermanentlyDestroyed 仍保留作为外部独立调用入口（如未来节点
	//    事件分支），它有自己的 tail 广播契约。
	if (Card->Keywords.HasTagExact(WacomTags::Card_Keyword_Companion))
	{
		RunState.Pressure.Add(EWacomPressureType::Bloodlust, 1);
		UE_LOG(LogTemp, Display,
			TEXT("[RunSession] DestroyCardFromBackpack (Companion): %s → Bloodlust=%d"),
			*GetNameSafe(Card),
			GetPressureValue(EWacomPressureType::Bloodlust));
	}

	// 6) Stage 4.5.1 任务 7.2 / R2.4 / R8.6：B 主卡销毁分支 — 按 FSpecialZone.Cards
	//    数组下标升序逐张退回；通量内容未满（CountFluxContentCards < GetFluxCapacity）
	//    → 追加 Backpack；已满 → 追加 BurdenZone；处理完后从 RunState.SpecialZones 移除该 entry。
	//
	//    退回前每张 instance 强制把 bBattleEnabledInSpecialZone 重置为 false（R8.6 /
	//    Property 10：从 SpecialZone 移出时必须清掉旧参战标记，避免下次再进入残留）。
	//
	//    注：B 主卡（B 类容器）本身 CapacityEffect 非空，不计入 GetFluxCapacity 公式
	//    （A 类总和），因此本次销毁不会改变 GetFluxCapacity 返回值，循环内的 Backpack
	//    满判定可以稳定使用 GetFluxCapacity()。
	if (bDestroyedIsBMaster && DestroyedInst.InstanceId.IsValid())
	{
		const int32 SZIdx = RunState.SpecialZones.IndexOfByPredicate(
			[&](const FSpecialZone& SZ)
			{
				return SZ.OwnerInstanceId == DestroyedInst.InstanceId;
			});
		if (SZIdx != INDEX_NONE)
		{
			// 把内含卡列表移出（避免在循环中读 RunState.SpecialZones[SZIdx] 引用悬空），
			// 然后再移除空 entry。R2.4 的语义"处理完所有 instance 后移除 entry"在
			// MoveTemp 后等价：内含卡都已搬到 Backpack/BurdenZone，原 entry 是空壳。
			TArray<FCardInstance> CardsToReturn = MoveTemp(RunState.SpecialZones[SZIdx].Cards);
			RunState.SpecialZones.RemoveAt(SZIdx);

			const int32 FluxCap = GetFluxCapacity();
			for (FCardInstance& Inst : CardsToReturn)
			{
				// R8.6 / Property 10：从 SpecialZone 移出 → flag 重置为 false。
				Inst.bBattleEnabledInSpecialZone = false;
				if (CountFluxContentCards(RunState.Backpack) < FluxCap)
				{
					RunState.Backpack.Add(MoveTemp(Inst));
				}
				else
				{
					RunState.BurdenZone.Add(MoveTemp(Inst));
				}
			}

			UE_LOG(LogTemp, Display,
				TEXT("[RunSession] DestroyCardFromBackpack (B-master): %s 退回 %d 张内含卡，Backpack=%d, BurdenZone=%d, FluxCapacity=%d"),
				*GetNameSafe(Card), CardsToReturn.Num(),
				RunState.Backpack.Num(), RunState.BurdenZone.Num(), FluxCap);
		}
	}

	// 7) 重算负重（容器卡销毁可能让其他卡溢出）
	//    走"不广播"的私有路径：本函数尾部统一 NotifyRunStateChanged 一次（R2.16 / task 9.4）。
	//    容量缩小导致的溢出要留在 BurdenZone，避免同一次重算又立刻回填到其他区。
	RecomputeBurdenInternal(/*bAllowBurdenRefill=*/false);

	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] DestroyCardFromBackpack: %s, Backpack=%d, BattleDeck=%d, FluxCapacity=%d"),
		*GetNameSafe(Card), RunState.Backpack.Num(), RunState.BattleDeck.Num(), GetFluxCapacity());

	// Stage 4.5.1 任务 9.4 / R2.16：本函数是私有路径（"不广播"版本），不在尾部
	// NotifyRunStateChanged。public 入口 DestroyCardFromBackpack 在 Internal 返回
	// 成功后统一广播一次；DeleteCardForGold 在调 Internal + AddGold(直接写 Gold) 后
	// 自行发尾部广播一次。
	return true;
}

bool URunSession::DeleteCardForGold(UCardDefinition* Card)
{
	const FRunDeckOperationValidation Validation = ValidateDeleteCardForGold(Card);
	if (!Validation.bCanExecute)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] DeleteCardForGold: 拒绝 Card=%s Reason=%s"),
			*GetNameSafe(Card), *Validation.DisabledReason.ToString());
		return false;
	}

	// Stage 4.5.0 任务 2.4 / R1.10：DeleteCardForGold 委托给 DestroyCardFromBackpackInternal
	// 完成"按下标升序选第一个 Definition 匹配 instance 销毁"的语义；本函数仅在销毁
	// 成功后追加金币结算，因此 instance 选择规则与 DestroyCardFromBackpack 保持一致。
	//
	// Stage 4.5.1 任务 9.4 / R2.16：调 Internal（不广播）+ 直接写 RunState.Gold（绕过
	// AddGold 内部广播）+ 尾部 NotifyRunStateChanged 一次。这样一次成功路径只发一次
	// 广播，符合"public 入口尾部统一发一次"的不变量。

	// 计算金币（GDD §11.7：白=1 / 蓝=2，第一阶段占位）。
	// 在销毁前算，因为销毁后 Card 可能不再被引用。
	int32 GoldReward = 0;
	if (Card->Rarity.MatchesTagExact(WacomTags::Card_Rarity_White))
	{
		GoldReward = 1;
	}
	else if (Card->Rarity.MatchesTagExact(WacomTags::Card_Rarity_Blue))
	{
		GoldReward = 2;
	}

	if (!DestroyCardFromBackpackInternal(Card))
	{
		// Internal 已记 Warning 日志；失败路径不广播（R2.16）。
		return false;
	}

	if (GoldReward > 0)
	{
		// 直接写 Gold 字段而非调 AddGold(...)：避开 AddGold 内部 NotifyRunStateChanged。
		RunState.Gold += GoldReward;
	}
	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] DeleteCardForGold: %s → +%d gold (total=%d)"),
		*GetNameSafe(Card), GoldReward, RunState.Gold);
	NotifyRunStateChanged();
	return true;
}

FRunDeckOperationValidation URunSession::ValidateDeleteCardForGold(UCardDefinition* Card) const
{
	FRunDeckOperationValidation Result;
	Result.DisabledReason = TEXT("Unknown");

	if (!Card)
	{
		Result.DisabledReason = TEXT("MissingCard");
		return Result;
	}

	const int32 BackpackIdx = FindFirstIndexByDefinition(RunState.Backpack, Card);
	const int32 BattleDeckIdx = FindFirstIndexByDefinition(RunState.BattleDeck, Card);
	if (BackpackIdx == INDEX_NONE && BattleDeckIdx == INDEX_NONE)
	{
		Result.DisabledReason = TEXT("CardNotOwned");
		return Result;
	}

	if (IsIntrinsicCard(Card))
	{
		Result.DisabledReason = TEXT("Intrinsic");
		return Result;
	}

	if (IsContainerCard(Card) && !HasCapacityProviderAfterDestroyingFirstOwnedInstance(Card))
	{
		Result.DisabledReason = TEXT("LastCapacityProvider");
		return Result;
	}

	Result.bCanExecute = true;
	Result.DisabledReason = NAME_None;
	return Result;
}

bool URunSession::AddCardToBattleDeck(UCardDefinition* Card)
{
	if (!Card)
	{
		return false;
	}

	// 1) 必须在背包中（按 Definition 匹配第一个 instance；R1.10）
	const int32 BackpackIdx = FindFirstIndexByDefinition(RunState.Backpack, Card);
	if (BackpackIdx == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] AddCardToBattleDeck: %s 不在背包中，拒绝"),
			*GetNameSafe(Card));
		return false;
	}

	// 2) 容量上限（备战区）
	const int32 Capacity = GetBattleDeckCapacity();
	if (RunState.BattleDeck.Num() >= Capacity)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] AddCardToBattleDeck: 备战区已达容量上限 %d"), Capacity);
		return false;
	}

	// 3) 移动一张：Backpack -> BattleDeck（GDD §11.4：一张卡同时只能在一个区）
	//    保留 InstanceId / bBattleEnabledInSpecialZone（4.5.0 阶段 Backpack 内 instance 的 flag
	//    无意义，但维持 instance 整体迁移的契约，方便后续 task 接入 SpecialZone 时不破坏迁移路径）。
	const FCardInstance Moved = RunState.Backpack[BackpackIdx];
	RunState.Backpack.RemoveAt(BackpackIdx);
	RunState.BattleDeck.Add(Moved);

	// 容器卡转移会触发"通量区可见空格变化"，但 FluxCapacity 公式 Σ(全部) 不变；
	// 普通卡转移会让 Backpack 卡数下降，可能让 Burden 减少。
	// 走"不广播"的私有路径：本函数尾部统一 NotifyRunStateChanged 一次（R2.16 / task 9.4）。
	RecomputeBurdenInternal();
	NotifyRunStateChanged();

	return true;
}

bool URunSession::RemoveCardFromBattleDeck(UCardDefinition* Card)
{
	if (!Card)
	{
		return false;
	}

	// Intrinsic 拒绝
	if (IsIntrinsicCard(Card))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] RemoveCardFromBattleDeck: %s 是 Intrinsic，拒绝从备战区移除"),
			*GetNameSafe(Card));
		return false;
	}

	// 按 Definition 匹配第一个 instance（R1.10）
	const int32 Idx = FindFirstIndexByDefinition(RunState.BattleDeck, Card);
	if (Idx == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] RemoveCardFromBattleDeck: %s 不在备战区"),
			*GetNameSafe(Card));
		return false;
	}

	// 移动一张：BattleDeck -> Backpack（GDD §11.4：一张卡同时只能在一个区）
	const FCardInstance Moved = RunState.BattleDeck[Idx];
	RunState.BattleDeck.RemoveAt(Idx);
	RunState.Backpack.Add(Moved);

	// 卡数从备战区移到背包，可能让背包超容。
	// 走"不广播"的私有路径：本函数尾部统一 NotifyRunStateChanged 一次（R2.16 / task 9.4）。
	RecomputeBurdenInternal();
	NotifyRunStateChanged();

	return true;
}

// ================ §11.7 / 经济：金币 ================

void URunSession::AddGold(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	RunState.Gold += Amount;
	NotifyRunStateChanged();
}

bool URunSession::RemoveGold(int32 Amount)
{
	if (Amount <= 0)
	{
		return true;
	}
	if (RunState.Gold < Amount)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] RemoveGold: 余额不足（%d < %d）"),
			RunState.Gold, Amount);
		return false;
	}
	RunState.Gold -= Amount;
	NotifyRunStateChanged();
	return true;
}

// ================ 商店购买 ================

bool URunSession::BeginShopVisit(FName ShopId, const TArray<FRunShopOfferInput>& Offers)
{
	if (ShopId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] BeginShopVisit: ShopId 为 None，拒绝"));
		return false;
	}

	if (!RunState.ShopStates.Contains(ShopId))
	{
		RunState.ShopStates.Add(ShopId, BuildShopStateFromInputs(Offers));
	}

	RunState.ActiveShopId = ShopId;
	RunState.bShopVisitHasPurchase = false;
	NotifyRunStateChanged();
	return true;
}

void URunSession::EndShopVisit()
{
	if (RunState.ActiveShopId.IsNone())
	{
		return;
	}

	const bool bShouldConsumeNode = RunState.bShopVisitHasPurchase;
	RunState.ActiveShopId = NAME_None;
	RunState.bShopVisitHasPurchase = false;

	if (bShouldConsumeNode)
	{
		ConsumeNode(1);
	}
	else
	{
		NotifyRunStateChanged();
	}
}

FRunShopSnapshot URunSession::BuildCurrentShopSnapshot() const
{
	FRunShopSnapshot Snapshot;
	Snapshot.ShopId = RunState.ActiveShopId;
	Snapshot.bIsActive = !RunState.ActiveShopId.IsNone();
	Snapshot.bHasPurchaseThisVisit = RunState.bShopVisitHasPurchase;

	if (const FRunShopState* ShopState = RunState.ShopStates.Find(RunState.ActiveShopId))
	{
		Snapshot.Offers = ShopState->Offers;
	}

	return Snapshot;
}

bool URunSession::PurchaseShopOffer(FGuid OfferId)
{
	if (RunState.ActiveShopId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] PurchaseShopOffer: 当前没有 active shop，拒绝"));
		return false;
	}
	if (!OfferId.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] PurchaseShopOffer: OfferId 无效，拒绝"));
		return false;
	}

	FRunShopState* ShopState = RunState.ShopStates.Find(RunState.ActiveShopId);
	if (!ShopState)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] PurchaseShopOffer: active shop %s 没有库存状态"),
			*RunState.ActiveShopId.ToString());
		return false;
	}

	FRunShopOffer* FoundOffer = ShopState->Offers.FindByPredicate(
		[OfferId](const FRunShopOffer& Offer)
		{
			return Offer.OfferId == OfferId;
		});
	if (!FoundOffer)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] PurchaseShopOffer: 找不到 OfferId=%s"),
			*OfferId.ToString());
		return false;
	}
	if (FoundOffer->bPurchased)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] PurchaseShopOffer: OfferId=%s 已购买，拒绝"),
			*OfferId.ToString());
		return false;
	}
	if (!FoundOffer->CardDefinition)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] PurchaseShopOffer: OfferId=%s CardDefinition 为空，拒绝"),
			*OfferId.ToString());
		return false;
	}
	if (FoundOffer->Price < 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] PurchaseShopOffer: OfferId=%s Price=%d 非法，拒绝"),
			*OfferId.ToString(), FoundOffer->Price);
		return false;
	}
	if (RunState.Gold < FoundOffer->Price)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] PurchaseShopOffer: 余额不足（%d < %d）"),
			RunState.Gold, FoundOffer->Price);
		return false;
	}

	RunState.Gold -= FoundOffer->Price;
	if (!AcquireCardToRunInternal(FoundOffer->CardDefinition.Get()))
	{
		RunState.Gold += FoundOffer->Price;
		return false;
	}

	FoundOffer->bPurchased = true;
	RunState.bShopVisitHasPurchase = true;
	NotifyRunStateChanged();
	return true;
}

bool URunSession::PurchaseCardFromShop(UCardDefinition* Card, int32 Price)
{
	if (!Card)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] PurchaseCardFromShop: Card 为空，拒绝"));
		return false;
	}
	if (Price < 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] PurchaseCardFromShop: Price=%d 非法，拒绝"), Price);
		return false;
	}
	if (RunState.Gold < Price)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] PurchaseCardFromShop: 余额不足（%d < %d）"),
			RunState.Gold, Price);
		return false;
	}

	RunState.Gold -= Price;

	if (!AcquireCardToRunInternal(Card))
	{
		RunState.Gold += Price;
		return false;
	}

	NotifyRunStateChanged();
	return true;
}
