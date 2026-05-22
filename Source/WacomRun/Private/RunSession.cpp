// Copyright Wacom. All Rights Reserved.

#include "RunSession.h"
#include "WacomSaveGame.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Deck/RunDeckRules.h"
#include "Enemies/EnemyDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Session/BattleSession.h"
#include "Tags/WacomGameplayTags.h"

#include "Kismet/GameplayStatics.h"
#include "UObject/SoftObjectPath.h"

// ================ 内部辅助 ================

namespace
{
	/**
	 * Definition 级旧 API 在 instance 模型下统一匹配第一个同定义实例。
	 * Card == nullptr 视为找不到，调用方自行决定返回 false 或记录日志。
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
		return FRunDeckRules::IsFluxContentCardDefinition(Card);
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

	const FWacomRunEventNodeDefinition* FindRunEventNode(const UWacomRunEventDefinition* EventDefinition, FName NodeId)
	{
		if (!EventDefinition || NodeId.IsNone())
		{
			return nullptr;
		}
		return EventDefinition->Nodes.FindByPredicate(
			[NodeId](const FWacomRunEventNodeDefinition& Node)
			{
				return Node.NodeId == NodeId;
			});
	}

	const FWacomRunEventChoiceDefinition* FindRunEventChoice(const FWacomRunEventNodeDefinition* Node, FName ChoiceId)
	{
		if (!Node || ChoiceId.IsNone())
		{
			return nullptr;
		}
		return Node->Choices.FindByPredicate(
			[ChoiceId](const FWacomRunEventChoiceDefinition& Choice)
			{
				return Choice.ChoiceId == ChoiceId;
			});
	}

	/**
	 * 把 SaveGame 里的 FCardInstanceSaveEntry 列表还原到 TempState，并校验：
	 * - InstanceId 必须有效；
	 * - InstanceId 在所有已还原 zone 中全局唯一；
	 * - DefinitionAssetPath 必须能加载出 UCardDefinition。
	 *
	 * 失败时 OutErr 写入诊断字符串，调用方负责 UE_LOG Error。
	 * 本函数保持 file-scope，避免 ApplySaveGameToRunState 继续膨胀。
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
					TEXT("zone=%s entry InstanceId 为 zero GUID"), ZoneName);
				return false;
			}
			bool bAlreadyInSet = false;
			SeenInstanceIds.Add(Entry.InstanceId, &bAlreadyInSet);
			if (bAlreadyInSet)
			{
				OutErr = FString::Printf(
					TEXT("zone=%s 中 InstanceId %s 与其他 zone 重复"),
					ZoneName, *Entry.InstanceId.ToString());
				return false;
			}
			UCardDefinition* Def = Cast<UCardDefinition>(Entry.DefinitionAssetPath.TryLoad());
			if (!Def)
			{
				OutErr = FString::Printf(
					TEXT("zone=%s InstanceId=%s DefinitionAssetPath 加载失败: %s"),
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
	// 当前粗粒度广播不区分变更字段，订阅方按需读 RunState 全量。
	OnRunStateChangedNative.Broadcast();
}

void URunSession::EnsureSpecialZoneEntryFor(const FCardInstance& Inst)
{
	FRunDeckRules::EnsureSpecialZoneEntryFor(RunState, Inst);
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

	// 起始卡组分流：
	//   - 非容器卡（普通卡）进 BattleDeck（默认参战）
	//   - 容器卡只进 Backpack（默认不参战，玩家可手动加入）
	//   - 一张卡同时只能在一个区，所以普通卡不再同时放入 Backpack
	//   - 每张非空 Definition 通过 `FGuid::NewGuid()` 生成新 InstanceId
	//   - StarterDeck 中 nullptr 条目跳过不创建 instance
	//   - 空 StarterDeck 保持 Backpack/BattleDeck 为空并记录 warning
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
			TEXT("[RunSession] Initialize: FGuid::NewGuid() 生成了 zero GUID"));
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
		// B 主卡 instance 进入 Backpack/BattleDeck 时，幂等追加 SpecialZones entry。
		EnsureSpecialZoneEntryFor(Inst);
	}

	// 时段 / 节点重置为清晨第一日。
	RunState.CurrentDayNumber  = 1;
	RunState.CurrentTimePhase  = ETimePhase::Morning;
	RunState.RemainingNodeCount = RunState.InitialNodeCount_Morning;

	// 初始化负重。这里走不广播的私有路径，本函数尾部统一 NotifyRunStateChanged 一次。
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

	// 阈值常量从 RunState 灌入战内，而不是战内硬编码。
	OutParams.HighHpThreshold = RunState.HighHpThreshold;
	OutParams.LowHpThreshold  = RunState.LowHpThreshold;

	// 战斗只读备战卡组。BattleDeckEntries 让来自 SpecialZone 的入战卡携带对应 B 主卡 CapacityEffect。
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

	// 撤离重入：灌入持久化破坏部位（如果该 Trigger 上次撤离时有记录）。
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
			// 撤离：敌人不进 DefeatedEnemies、节点不算完成。
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

	// 2) 战外结算压力。
	// 疲劳：每场战斗后 +1%（无论胜败）。
	AddPressure(EWacomPressureType::Fatigue, 1);

	// 伤口阈值跨越。
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

	// 3) 经验结算。
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

	// 4) 战斗中获得的卡牌结算。
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

	// 5) 击倒事件玩家选择记账。当前只打日志；后续可由 RunEvent 按 Choice 衔接分支。
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
			TEXT("[RunSession] KnockdownChoice: Part=%s, Choice=%s"),
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

	// ---- v2 instance 列表 ----
	//
	// 写入约束：
	//   1) 每条 entry 的 InstanceId 必须非 zero GUID（违反则 UE_LOG Error 并跳过）
	//   2) 全表合并（Backpack ∪ BattleDeck ∪ BurdenZone ∪ ⋃SpecialZones.Cards）后 InstanceId 全局唯一
	//      （违反则 UE_LOG Error 并跳过该条；首次出现的保留，后续重复的跳过）
	//   3) FSpecialZone.OwnerInstanceId 必须非 zero GUID 且能在 Backpack ∪ BattleDeck 中找到对应 owner instance
	//      （违反则 UE_LOG Error 并整条 SpecialZone 跳过；不写半截 entry）
	//
	// 注意：Definition == nullptr 的 instance 也允许写入（DefinitionAssetPath 为空 path）；
	//   读档时由 ApplySaveGameToRunState 的损坏档校验处理。
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
	//   - 非 zero 但在 Backpack ∪ BattleDeck 中找不到 owner instance → 跳过整条
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

	// 版本检查：新版本拒绝。
	if (SaveGame->SaveVersion > UWacomSaveGame::CurrentSaveVersion)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] 存档版本 %d 高于当前 %d，拒绝读档"),
			SaveGame->SaveVersion, UWacomSaveGame::CurrentSaveVersion);
		return false;
	}

	// 旧版本走迁移链。迁移失败拒绝读档。
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

	// 原子还原：所有字段先写入 TempState，校验全部通过后才替换 RunState。
	// 四个 instance 数组全空时按 Character.StarterDeck 重建；否则按 SaveEntry 还原。
	// 任何损坏档校验失败都会提前返回，保留调用前的 RunState。

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
		// ---- 按 StarterDeck 重建 instances ----
		// 复用 Initialize 的 A/B/普通卡分流：容器卡进 Backpack，非容器卡进 BattleDeck。
		// 每张新分配 InstanceId；B 主卡同步创建空 SpecialZone entry。
		// BurdenZone 保持空，等待后续容量重算。
		//
		// 此处 inline 实现而非调 EnsureSpecialZoneEntryFor，因为该 helper 操作的是
		// `this->RunState.SpecialZones`；这里写入的是 TempState。
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
				TEXT("[RunSession] ApplySaveGameToRunState (StarterDeck rebuild): FGuid::NewGuid() 生成 zero GUID"));
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
			// StarterDeck 同款 B 主卡可能多张（不同 InstanceId），各自一条 entry。
			if (IsTypeBContainerCard(Inst.Definition) && Inst.InstanceId.IsValid())
			{
				FSpecialZone NewEntry;
				NewEntry.OwnerInstanceId = Inst.InstanceId;
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
		// ---- 按 SaveEntry 还原 + 损坏档校验 ----
		// 校验聚合在一个 TSet<FGuid> SeenInstanceIds 中：每读一条 entry 就尝试 Add，
		// 若 bAlreadyInSet 命中即说明 InstanceId 重复。SeenInstanceIds 跨
		// Backpack / BattleDeck / BurdenZone / 各 SpecialZone.Cards 共享，覆盖全表唯一性。
		//
		// DefinitionAssetPath 为空 path 视为损坏：BuildSaveGameFromRunState 写入
		// nullptr Definition 时输出空 path，读档时 TryLoad 返回 nullptr 即按损坏处理。
		//
		// 还原逻辑提取为 file-scope helper RestoreCardInstanceList。
		// BurdenZone + SpecialZones 归属关系：
		//   - BurdenZone 直接走 RestoreCardInstanceList，与三区共享 SeenInstanceIds。
		//   - 每个 FSpecialZoneSaveEntry：
		//       a) OwnerInstanceId 非 zero GUID
		//       b) OwnerInstanceId 必须在 TempState.Backpack ∪ TempState.BattleDeck 中存在
		//          （B 主卡 instance 只能在这两区）
		//       c) OwnerInstanceId 跨 SpecialZoneSaveEntry 唯一（防御性；
		//          BuildSaveGameFromRunState 写入侧已保证 RunState.SpecialZones 内唯一）
		//       d) Cards 列表通过 RestoreCardInstanceList 还原，复用同一 SeenInstanceIds
		//   - 任一校验失败 → return false，TempState 已写入的部分被丢弃。

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
					TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: SpecialZone OwnerInstanceId 为 zero GUID"));
				return false;
			}

			// (c) OwnerInstanceId 跨 entry 唯一（防御性：写入侧保证不重复，但读取仍校验）
			bool bAlreadyOwner = false;
			SeenSpecialZoneOwners.Add(SZEntry.OwnerInstanceId, &bAlreadyOwner);
			if (bAlreadyOwner)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: SpecialZone OwnerInstanceId %s 在 SaveGame.SpecialZones 中重复"),
					*SZEntry.OwnerInstanceId.ToString());
				return false;
			}

			// (b) OwnerInstanceId 必须在 TempState.Backpack ∪ TempState.BattleDeck 中存在。
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
					TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: SpecialZone OwnerInstanceId %s 在 Backpack/BattleDeck 中找不到 owner instance"),
					*SZEntry.OwnerInstanceId.ToString());
				return false;
			}

			// (d) 还原 Cards 列表。共享 SeenInstanceIds 保证 SpecialZone 内 InstanceId
			//     与四区其它 InstanceId 全表唯一。
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

	// 所有还原 + 校验通过 → 原子赋值 + 广播一次。
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
		// 每缺 1 指 +5% 残疾。
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
	// 战外右手破坏行为 +1% 伤口。
	AddPressure(EWacomPressureType::Wound, 1);
	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] RightHand destructive action → Wound=%d"),
		GetPressureValue(EWacomPressureType::Wound));
}

void URunSession::OnCompanionCardPermanentlyDestroyed()
{
	// 每永久销毁一张伙伴卡 +1% 嗜血。
	AddPressure(EWacomPressureType::Bloodlust, 1);
	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] Companion card destroyed → Bloodlust=%d"),
		GetPressureValue(EWacomPressureType::Bloodlust));
}

void URunSession::OnTheftCommitted()
{
	// 劣迹增量：第 n 次完成时 +(n*(n+1)/2 + 1)%。
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
	// Public 入口：内部完成容量重算，末尾统一广播一次。
	// 其他 public 入口会调用 RecomputeBurdenInternal，避免重复广播。
	RecomputeBurdenInternal();
	NotifyRunStateChanged();
}

void URunSession::RecomputeBurdenInternal(bool bAllowBurdenRefill)
{
	FRunDeckRules::RecomputeBurden(RunState, bAllowBurdenRefill);
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

		// 技能内容未正式化前，用 SkillSlot.Placeholder 累加，不挂效果。
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
		// Sunrise 结束 = 进入次日清晨。
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
		// 每清晨到来 +5% 饥饿。
		AddPressure(EWacomPressureType::Hunger, 5);
		// 完成一天 +5% 腐朽。判定为"从 Sunrise 推进进入次日清晨"。
		// 露营特殊推进（Night -> Morning 跳过 Sunrise）应在对应路径自行加腐朽。
		if (PrevPhase == ETimePhase::Sunrise)
		{
			AddPressure(EWacomPressureType::Decay, 5);
		}
		break;
	case ETimePhase::Dusk:
		// 每黄昏到来 +5% 饥饿。
		AddPressure(EWacomPressureType::Hunger, 5);
		break;
	case ETimePhase::Sunrise:
		// 每日出 +10% 疲劳。
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
	return FRunDeckRules::IsContainerCard(Card);
}

bool URunSession::IsTypeAContainerCard(const UCardDefinition* Card)
{
	return FRunDeckRules::IsTypeAContainerCard(Card);
}

bool URunSession::IsTypeBContainerCard(const UCardDefinition* Card)
{
	return FRunDeckRules::IsTypeBContainerCard(Card);
}

int32 URunSession::GetSpecialZoneCapacity(const UCardDefinition* BCard)
{
	return FRunDeckRules::GetSpecialZoneCapacity(BCard);
}

void URunSession::CollectTypeBContainers(TArray<FGuid>& OutOwnerInstanceIds) const
{
	FRunDeckRules::CollectTypeBContainers(RunState, OutOwnerInstanceIds);
}

bool URunSession::GetSpecialZone(FGuid OwnerInstanceId, FSpecialZone& Out) const
{
	return FRunDeckRules::GetSpecialZone(RunState, OwnerInstanceId, Out);
}

int32 URunSession::GetSpecialZoneCapacityFor(FGuid OwnerInstanceId) const
{
	return FRunDeckRules::GetSpecialZoneCapacityFor(RunState, OwnerInstanceId);
}

bool URunSession::SetSpecialZoneCardBattleEnabled(FGuid InstanceId, bool bEnabled)
{
	// 仅切 SpecialZone 内卡牌的参战 flag，不移动卡牌。
	// 未命中时不修改 RunState、不广播；重复设置同值仍视为成功并广播一次。
	// 不同于 MoveInstance 复用 FindInstance：本函数需要通过引用直接修改 SpecialZone.Cards
	// 中的字段，而 FindInstance 输出的是按值拷贝。因此这里直接遍历 SpecialZones。

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
	// 未命中失败路径：不修改、不广播、return false。
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
	// 容量公式：
	//   通量内容容量 = Σ(玩家拥有的所有 A 类容器卡 Capacity)
	//
	// "玩家拥有" = 当前 RunState 全部物理持有区。
	// A 类容器卡自身也作为通量内容卡显示 / 占格，不再有通量主卡位。
	// B 类容器卡（CapacityEffect 非空）不进通量公式，自行开辟特殊存放区。
	return SumOwnedCardCapacity(/*bTypeAOnly=*/true);
}

int32 URunSession::GetBattleDeckCapacity() const
{
	// 容量公式：
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
	return FRunDeckRules::SumOwnedCardCapacity(RunState, bTypeAOnly);
}

int32 URunSession::CountFluxContentCards(const TArray<FCardInstance>& Pile) const
{
	return FRunDeckRules::CountFluxContentCards(Pile);
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
	// Card == nullptr 返回 false；否则按 Definition 在 Backpack 内查任意一张。
	return FindFirstIndexByDefinition(RunState.Backpack, Card) != INDEX_NONE;
}

bool URunSession::IsCardInBattleDeck(const UCardDefinition* Card) const
{
	// Card == nullptr 返回 false；否则按 Definition 在 BattleDeck 内查任意一张。
	return FindFirstIndexByDefinition(RunState.BattleDeck, Card) != INDEX_NONE;
}

bool URunSession::FindInstance(FGuid InstanceId, FCardInstance& OutInstance, EZoneKind& OutZone, FGuid& OutZoneOwnerInstanceId) const
{
	return FRunDeckRules::FindInstance(RunState, InstanceId, OutInstance, OutZone, OutZoneOwnerInstanceId);
}

bool URunSession::MoveInstance(FGuid InstanceId, EZoneKind ToZone, FGuid ToZoneOwnerInstanceId)
{
	// 设计要点：
	//   1) 任何校验失败都原子拒绝：不修改 RunState、不广播 OnRunStateChangedNative。
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

	// 1) 找源 zone。InstanceId 在所有 zone 中均不存在时拒绝。
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

	// 2) 校验全部通过 → 真正修改 RunState。
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
		const int32 SrcIdx = RunState.BurdenZone.IndexOfByPredicate(
			[&](const FCardInstance& I) { return I.InstanceId == InstanceId; });
		check(SrcIdx != INDEX_NONE);
		RunState.BurdenZone.RemoveAt(SrcIdx);
		break;
	}
	case EZoneKind::SpecialZone:
	{
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

	// B 主卡跨入 Backpack/BattleDeck 时幂等保底追加 SpecialZones entry。
	// 正常路径上 entry 已存在，本调用主要防御外部直接构造 RunState 的测试路径。
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

bool URunSession::DoesRunOwnCardDefinition(const UCardDefinition* Card) const
{
	if (!Card)
	{
		return false;
	}

	if (FindFirstIndexByDefinition(RunState.Backpack, Card) != INDEX_NONE
		|| FindFirstIndexByDefinition(RunState.BattleDeck, Card) != INDEX_NONE
		|| FindFirstIndexByDefinition(RunState.BurdenZone, Card) != INDEX_NONE)
	{
		return true;
	}

	for (const FSpecialZone& SpecialZone : RunState.SpecialZones)
	{
		if (FindFirstIndexByDefinition(SpecialZone.Cards, Card) != INDEX_NONE)
		{
			return true;
		}
	}

	return false;
}

bool URunSession::ValidateRunEventRemoveCard(const UCardDefinition* Card, FName& OutDisabledReason) const
{
	OutDisabledReason = NAME_None;
	if (!Card)
	{
		OutDisabledReason = TEXT("MissingCard");
		return false;
	}
	if (!DoesRunOwnCardDefinition(Card))
	{
		OutDisabledReason = TEXT("MissingRequiredCard");
		return false;
	}
	if (IsIntrinsicCard(Card))
	{
		OutDisabledReason = TEXT("ProtectedCard");
		return false;
	}
	if (IsContainerCard(Card) && !HasCapacityProviderAfterDestroyingFirstOwnedInstance(Card))
	{
		OutDisabledReason = TEXT("LastCapacityProvider");
		return false;
	}
	return true;
}

bool URunSession::RemoveOwnedCardForRunEventInternal(UCardDefinition* Card, FName& OutDisabledReason)
{
	if (!ValidateRunEventRemoveCard(Card, OutDisabledReason))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] RemoveOwnedCardForRunEvent: 拒绝 Card=%s Reason=%s"),
			*GetNameSafe(Card),
			*OutDisabledReason.ToString());
		return false;
	}

	const int32 BackpackIdx = FindFirstIndexByDefinition(RunState.Backpack, Card);
	const int32 BattleDeckIdx = FindFirstIndexByDefinition(RunState.BattleDeck, Card);
	const int32 BurdenIdx = FindFirstIndexByDefinition(RunState.BurdenZone, Card);

	const bool bRemovedIsBMaster = IsTypeBContainerCard(Card);
	FCardInstance RemovedInst;
	bool bRemoved = false;
	if (BackpackIdx != INDEX_NONE)
	{
		RemovedInst = RunState.Backpack[BackpackIdx];
		RunState.Backpack.RemoveAt(BackpackIdx);
		bRemoved = true;
	}
	else if (BattleDeckIdx != INDEX_NONE)
	{
		RemovedInst = RunState.BattleDeck[BattleDeckIdx];
		RunState.BattleDeck.RemoveAt(BattleDeckIdx);
		bRemoved = true;
	}
	else if (BurdenIdx != INDEX_NONE)
	{
		RemovedInst = RunState.BurdenZone[BurdenIdx];
		RunState.BurdenZone.RemoveAt(BurdenIdx);
		bRemoved = true;
	}
	else
	{
		for (FSpecialZone& SpecialZone : RunState.SpecialZones)
		{
			const int32 SpecialIdx = FindFirstIndexByDefinition(SpecialZone.Cards, Card);
			if (SpecialIdx != INDEX_NONE)
			{
				RemovedInst = SpecialZone.Cards[SpecialIdx];
				SpecialZone.Cards.RemoveAt(SpecialIdx);
				bRemoved = true;
				break;
			}
		}
	}

	if (!bRemoved)
	{
		OutDisabledReason = TEXT("MissingRequiredCard");
		return false;
	}

	if (Card->Keywords.HasTagExact(WacomTags::Card_Keyword_Companion))
	{
		RunState.Pressure.Add(EWacomPressureType::Bloodlust, 1);
	}

	if (bRemovedIsBMaster && RemovedInst.InstanceId.IsValid())
	{
		const int32 SZIdx = RunState.SpecialZones.IndexOfByPredicate(
			[&](const FSpecialZone& SZ)
			{
				return SZ.OwnerInstanceId == RemovedInst.InstanceId;
			});
		if (SZIdx != INDEX_NONE)
		{
			TArray<FCardInstance> CardsToReturn = MoveTemp(RunState.SpecialZones[SZIdx].Cards);
			RunState.SpecialZones.RemoveAt(SZIdx);

			const int32 FluxCap = GetFluxCapacity();
			for (FCardInstance& Inst : CardsToReturn)
			{
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
		}
	}

	RecomputeBurdenInternal(/*bAllowBurdenRefill=*/!IsContainerCard(Card));
	OutDisabledReason = NAME_None;
	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] RemoveOwnedCardForRunEvent: %s, Backpack=%d, BattleDeck=%d, Burden=%d"),
		*GetNameSafe(Card),
		RunState.Backpack.Num(),
		RunState.BattleDeck.Num(),
		RunState.BurdenZone.Num());
	return true;
}

FRunDeckOperationValidation URunSession::ValidateMoveInstance(FGuid InstanceId, EZoneKind ToZone, FGuid ToZoneOwnerInstanceId) const
{
	return FRunDeckRules::ValidateMoveInstance(RunState, InstanceId, ToZone, ToZoneOwnerInstanceId);
}

void URunSession::AddCardToBackpack(UCardDefinition* Card)
{
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
		TEXT("[RunSession] AddCardToBackpack: FGuid::NewGuid() 生成了 zero GUID"));
	RunState.Backpack.Add(Inst);
	// B 主卡新加入背包时幂等追加 SpecialZones entry。
	EnsureSpecialZoneEntryFor(Inst);
	// 容器卡新加入时贡献新容量，可能让超容卡变得不再超容；非容器卡新加入则可能造成超容。
	// 任何情况都重算一次。走不广播的私有路径，本函数尾部统一 NotifyRunStateChanged 一次。
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
		TEXT("[RunSession] AcquireCardToRun: FGuid::NewGuid() 生成了 zero GUID"));
	RunState.Backpack.Add(Inst);
	EnsureSpecialZoneEntryFor(Inst);
	RecomputeBurdenInternal();
	return true;
}

bool URunSession::DestroyCardFromBackpack(UCardDefinition* Card)
{
	// Public 入口：委托 Internal 完成 zone 修改、压力和退回流，末尾广播一次。
	// DeleteCardForGold 调 Internal 后自行结算金币并广播。
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
	//    一张卡同时只能在一个区，所以两边只会在一处找到；按 Definition 选第一个匹配 instance。
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

	// 2) Intrinsic 拒绝。
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

	// 4) 移除一张：哪边有就从哪边移除。销毁的 instance 若是 B 主卡，需要保留其 InstanceId
	// 以便随后在 SpecialZones 中找到对应 entry，并按数组下标升序逐张退回内含卡。先把要销毁
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

	// 5) 若 Card 带 Companion 关键词 → 嗜血 +1%。
	//    直接写 RunState.Pressure 而非调用 OnCompanionCardPermanentlyDestroyed()，
	//    避免 public 入口尾部统一广播之外再多发一次。
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

	// 6) B 主卡销毁分支：按 FSpecialZone.Cards
	//    数组下标升序逐张退回；通量内容未满（CountFluxContentCards < GetFluxCapacity）
	//    → 追加 Backpack；已满 → 追加 BurdenZone；处理完后从 RunState.SpecialZones 移除该 entry。
	//
	//    退回前每张 instance 强制把 bBattleEnabledInSpecialZone 重置为 false，避免下次再进入残留。
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
			// 然后再移除空 entry。MoveTemp 后内含卡都已搬到 Backpack/BurdenZone，原 entry 是空壳。
			TArray<FCardInstance> CardsToReturn = MoveTemp(RunState.SpecialZones[SZIdx].Cards);
			RunState.SpecialZones.RemoveAt(SZIdx);

			const int32 FluxCap = GetFluxCapacity();
			for (FCardInstance& Inst : CardsToReturn)
			{
				// 从 SpecialZone 移出 → flag 重置为 false。
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

	// 7) 重算负重（容器卡销毁可能让其他卡溢出）。
	//    走不广播的私有路径，外层 public 入口统一广播一次。
	//    容量缩小导致的溢出要留在 BurdenZone，避免同一次重算又立刻回填到其他区。
	RecomputeBurdenInternal(/*bAllowBurdenRefill=*/false);

	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] DestroyCardFromBackpack: %s, Backpack=%d, BattleDeck=%d, FluxCapacity=%d"),
		*GetNameSafe(Card), RunState.Backpack.Num(), RunState.BattleDeck.Num(), GetFluxCapacity());

	// 本函数是私有路径，不在尾部 NotifyRunStateChanged。
	// public 入口 DestroyCardFromBackpack 和 DeleteCardForGold 分别负责成功后的尾部广播。
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

	// 委托 DestroyCardFromBackpackInternal 完成 instance 选择和销毁；本函数只追加金币结算。
	// Internal 不广播，这里直接写 Gold 后统一广播一次。

	// 计算金币：白=1 / 蓝=2。后续正式经济规则可替换这里。
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
		// Internal 已记 Warning 日志；失败路径不广播。
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

	// 1) 必须在背包中，按 Definition 匹配第一个 instance。
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

	// 3) 移动一张：Backpack -> BattleDeck。
	//    保留 InstanceId / bBattleEnabledInSpecialZone，维持 instance 整体迁移契约。
	const FCardInstance Moved = RunState.Backpack[BackpackIdx];
	RunState.Backpack.RemoveAt(BackpackIdx);
	RunState.BattleDeck.Add(Moved);

	// 容器卡转移会触发"通量区可见空格变化"，但 FluxCapacity 公式 Σ(全部) 不变；
	// 普通卡转移会让 Backpack 卡数下降，可能让 Burden 减少。
	// 走不广播的私有路径，本函数尾部统一 NotifyRunStateChanged 一次。
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

	// 按 Definition 匹配第一个 instance。
	const int32 Idx = FindFirstIndexByDefinition(RunState.BattleDeck, Card);
	if (Idx == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] RemoveCardFromBattleDeck: %s 不在备战区"),
			*GetNameSafe(Card));
		return false;
	}

	// 移动一张：BattleDeck -> Backpack。
	const FCardInstance Moved = RunState.BattleDeck[Idx];
	RunState.BattleDeck.RemoveAt(Idx);
	RunState.Backpack.Add(Moved);

	// 卡数从备战区移到背包，可能让背包超容。
	// 走不广播的私有路径，本函数尾部统一 NotifyRunStateChanged 一次。
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

// ================ 探索事件 ================

bool URunSession::BeginRunEvent(FName PersistentId, UWacomRunEventDefinition* EventDefinition)
{
	if (PersistentId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] BeginRunEvent: PersistentId 为 None，拒绝"));
		return false;
	}
	if (!EventDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] BeginRunEvent: EventDefinition 为空，拒绝"));
		return false;
	}
	if (!FindRunEventNode(EventDefinition, EventDefinition->StartNodeId))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] BeginRunEvent: Event=%s StartNodeId=%s 无效"),
			*GetNameSafe(EventDefinition),
			*EventDefinition->StartNodeId.ToString());
		return false;
	}

	FRunEventState& EventState = RunState.RunEventStates.FindOrAdd(PersistentId);
	if (EventState.bCompleted)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[RunSession] BeginRunEvent: PersistentId=%s 已完成，拒绝重复打开"),
			*PersistentId.ToString());
		return false;
	}

	if (EventState.CurrentNodeId.IsNone() || !FindRunEventNode(EventDefinition, EventState.CurrentNodeId))
	{
		EventState.CurrentNodeId = EventDefinition->StartNodeId;
	}

	RunState.ActiveRunEventId = PersistentId;
	RunState.ActiveRunEventDefinition = EventDefinition;
	NotifyRunStateChanged();
	return true;
}

void URunSession::EndRunEvent()
{
	if (RunState.ActiveRunEventId.IsNone() && !RunState.ActiveRunEventDefinition)
	{
		return;
	}

	RunState.ActiveRunEventId = NAME_None;
	RunState.ActiveRunEventDefinition = nullptr;
	NotifyRunStateChanged();
}

bool URunSession::IsRunEventCompleted(FName PersistentId) const
{
	if (const FRunEventState* EventState = RunState.RunEventStates.Find(PersistentId))
	{
		return EventState->bCompleted;
	}
	return false;
}

FRunEventSnapshot URunSession::BuildCurrentRunEventSnapshot() const
{
	FRunEventSnapshot Snapshot;
	Snapshot.PersistentId = RunState.ActiveRunEventId;
	Snapshot.bIsActive = !RunState.ActiveRunEventId.IsNone() && RunState.ActiveRunEventDefinition;

	const UWacomRunEventDefinition* EventDefinition = RunState.ActiveRunEventDefinition;
	if (!Snapshot.bIsActive || !EventDefinition)
	{
		return Snapshot;
	}

	Snapshot.EventId = EventDefinition->EventId;
	const FRunEventState* EventState = RunState.RunEventStates.Find(RunState.ActiveRunEventId);
	Snapshot.bCompleted = EventState ? EventState->bCompleted : false;
	Snapshot.CurrentNodeId = EventState ? EventState->CurrentNodeId : EventDefinition->StartNodeId;

	const FWacomRunEventNodeDefinition* Node = FindRunEventNode(EventDefinition, Snapshot.CurrentNodeId);
	if (!Node)
	{
		return Snapshot;
	}

	Snapshot.TitleText = Node->TitleText.IsEmpty() ? EventDefinition->DisplayName : Node->TitleText;
	Snapshot.BodyText = Node->BodyText;
	Snapshot.Choices.Reserve(Node->Choices.Num());
	for (const FWacomRunEventChoiceDefinition& Choice : Node->Choices)
	{
		FRunEventChoiceSnapshot ChoiceSnapshot;
		ChoiceSnapshot.ChoiceId = Choice.ChoiceId;
		ChoiceSnapshot.LabelText = Choice.LabelText;
		ChoiceSnapshot.bAvailable = IsRunEventChoiceAvailable(Choice, ChoiceSnapshot.DisabledReason);
		Snapshot.Choices.Add(MoveTemp(ChoiceSnapshot));
	}

	return Snapshot;
}

bool URunSession::ChooseRunEventOption(FName ChoiceId)
{
	return ChooseRunEventOptionWithResult(ChoiceId).bSucceeded;
}

FRunEventChoiceResult URunSession::ChooseRunEventOptionWithResult(FName ChoiceId)
{
	FRunEventChoiceResult Result;
	Result.ChoiceId = ChoiceId;

	if (RunState.ActiveRunEventId.IsNone() || !RunState.ActiveRunEventDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] ChooseRunEventOption: 当前没有 active event，拒绝"));
		Result.DisabledReason = TEXT("NoActiveEvent");
		return Result;
	}

	FRunEventState* EventState = RunState.RunEventStates.Find(RunState.ActiveRunEventId);
	if (!EventState || EventState->bCompleted)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] ChooseRunEventOption: 事件状态无效或已完成，拒绝"));
		Result.DisabledReason = TEXT("InvalidEventState");
		return Result;
	}

	const FWacomRunEventNodeDefinition* Node = FindRunEventNode(RunState.ActiveRunEventDefinition, EventState->CurrentNodeId);
	const FWacomRunEventChoiceDefinition* Choice = FindRunEventChoice(Node, ChoiceId);
	if (!Choice)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] ChooseRunEventOption: 找不到 ChoiceId=%s"),
			*ChoiceId.ToString());
		Result.DisabledReason = TEXT("ChoiceNotFound");
		return Result;
	}

	FName DisabledReason = NAME_None;
	if (!IsRunEventChoiceAvailable(*Choice, DisabledReason))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] ChooseRunEventOption: ChoiceId=%s 不可用 Reason=%s"),
			*ChoiceId.ToString(),
			*DisabledReason.ToString());
		Result.DisabledReason = DisabledReason;
		return Result;
	}

	if (!ApplyRunEventChoiceEffects(*Choice, &Result.EffectResults, &Result.DisabledReason))
	{
		if (Result.DisabledReason.IsNone())
		{
			Result.DisabledReason = TEXT("EffectFailed");
		}
		return Result;
	}

	if (Choice->bMarkEventCompleted)
	{
		EventState->bCompleted = true;
	}

	if (!Choice->NextNodeId.IsNone())
	{
		if (!FindRunEventNode(RunState.ActiveRunEventDefinition, Choice->NextNodeId))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RunSession] ChooseRunEventOption: NextNodeId=%s 无效，保持当前节点"),
				*Choice->NextNodeId.ToString());
		}
		else
		{
			EventState->CurrentNodeId = Choice->NextNodeId;
		}
	}

	if (Choice->bCloseEventAfterResolve || EventState->bCompleted)
	{
		RunState.ActiveRunEventId = NAME_None;
		RunState.ActiveRunEventDefinition = nullptr;
	}

	NotifyRunStateChanged();
	Result.bSucceeded = true;
	return Result;
}

bool URunSession::TryResolveRunEventPressureType(FName PressureTypeId, EWacomPressureType& OutType) const
{
	if (PressureTypeId == TEXT("Hunger"))     { OutType = EWacomPressureType::Hunger; return true; }
	if (PressureTypeId == TEXT("Wound"))      { OutType = EWacomPressureType::Wound; return true; }
	if (PressureTypeId == TEXT("Fatigue"))    { OutType = EWacomPressureType::Fatigue; return true; }
	if (PressureTypeId == TEXT("Burden"))     { OutType = EWacomPressureType::Burden; return true; }
	if (PressureTypeId == TEXT("Decay"))      { OutType = EWacomPressureType::Decay; return true; }
	if (PressureTypeId == TEXT("Misdeed"))    { OutType = EWacomPressureType::Misdeed; return true; }
	if (PressureTypeId == TEXT("Bloodlust"))  { OutType = EWacomPressureType::Bloodlust; return true; }
	if (PressureTypeId == TEXT("Disability")) { OutType = EWacomPressureType::Disability; return true; }
	return false;
}

bool URunSession::IsRunEventChoiceAvailable(const FWacomRunEventChoiceDefinition& Choice, FName& OutDisabledReason) const
{
	OutDisabledReason = NAME_None;
	for (const FWacomRunEventConditionDefinition& Condition : Choice.Conditions)
	{
		switch (Condition.Type)
		{
		case EWacomRunEventConditionType::None:
			break;
		case EWacomRunEventConditionType::MinGold:
			if (RunState.Gold < Condition.Value)
			{
				OutDisabledReason = TEXT("InsufficientGold");
				return false;
			}
			break;
		case EWacomRunEventConditionType::MinNodeCount:
			if (RunState.RemainingNodeCount < Condition.Value)
			{
				OutDisabledReason = TEXT("InsufficientNode");
				return false;
			}
			break;
		case EWacomRunEventConditionType::MaxPressure:
		{
			EWacomPressureType PressureType = EWacomPressureType::Count;
			if (!TryResolveRunEventPressureType(Condition.PressureType, PressureType))
			{
				OutDisabledReason = TEXT("InvalidPressureType");
				return false;
			}
			if (GetPressureValue(PressureType) > Condition.Value)
			{
				OutDisabledReason = TEXT("PressureTooHigh");
				return false;
			}
			break;
		}
		case EWacomRunEventConditionType::HasCard:
			if (!Condition.CardDefinition)
			{
				OutDisabledReason = TEXT("MissingCard");
				return false;
			}
			if (!DoesRunOwnCardDefinition(Condition.CardDefinition.Get()))
			{
				OutDisabledReason = TEXT("MissingRequiredCard");
				return false;
			}
			break;
		case EWacomRunEventConditionType::MissingCard:
			if (!Condition.CardDefinition)
			{
				OutDisabledReason = TEXT("MissingCard");
				return false;
			}
			if (DoesRunOwnCardDefinition(Condition.CardDefinition.Get()))
			{
				OutDisabledReason = TEXT("AlreadyHasCard");
				return false;
			}
			break;
		case EWacomRunEventConditionType::EventCompleted:
			if (Condition.TargetPersistentId.IsNone())
			{
				OutDisabledReason = TEXT("MissingTargetPersistentId");
				return false;
			}
			if (!IsRunEventCompleted(Condition.TargetPersistentId))
			{
				OutDisabledReason = TEXT("RequiredEventNotCompleted");
				return false;
			}
			break;
		case EWacomRunEventConditionType::EventNotCompleted:
			if (Condition.TargetPersistentId.IsNone())
			{
				OutDisabledReason = TEXT("MissingTargetPersistentId");
				return false;
			}
			if (IsRunEventCompleted(Condition.TargetPersistentId))
			{
				OutDisabledReason = TEXT("RequiredEventAlreadyCompleted");
				return false;
			}
			break;
		default:
			OutDisabledReason = TEXT("UnknownCondition");
			return false;
		}
	}
	return true;
}

bool URunSession::ApplyRunEventChoiceEffects(const FWacomRunEventChoiceDefinition& Choice, TArray<FRunEventChoiceEffectResult>* OutEffectResults, FName* OutDisabledReason)
{
	for (const FWacomRunEventEffectDefinition& Effect : Choice.Effects)
	{
		FRunEventChoiceEffectResult EffectResult;
		EffectResult.EffectType = Effect.Type;
		EffectResult.CardDefinition = Effect.CardDefinition;
		EffectResult.Amount = Effect.Value;

		switch (Effect.Type)
		{
		case EWacomRunEventEffectType::None:
			EffectResult.bApplied = true;
			break;
		case EWacomRunEventEffectType::GainCard:
			if (!Effect.CardDefinition)
			{
				UE_LOG(LogTemp, Warning, TEXT("[RunSession] ApplyRunEventChoiceEffects: GainCard 缺少 CardDefinition"));
				if (OutDisabledReason)
				{
					*OutDisabledReason = TEXT("MissingCard");
				}
				return false;
			}
			if (!AcquireCardToRunInternal(Effect.CardDefinition.Get()))
			{
				if (OutDisabledReason)
				{
					*OutDisabledReason = TEXT("EffectFailed");
				}
				return false;
			}
			EffectResult.bApplied = true;
			break;
		case EWacomRunEventEffectType::AddGold:
		{
			const int32 GoldBefore = RunState.Gold;
			RunState.Gold = FMath::Max(0, RunState.Gold + Effect.Value);
			EffectResult.ActualDelta = RunState.Gold - GoldBefore;
			EffectResult.bApplied = true;
			break;
		}
		case EWacomRunEventEffectType::AddPressure:
		{
			EWacomPressureType PressureType = EWacomPressureType::Count;
			if (!TryResolveRunEventPressureType(Effect.PressureType, PressureType))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[RunSession] ApplyRunEventChoiceEffects: 无效 PressureType=%s"),
					*Effect.PressureType.ToString());
				if (OutDisabledReason)
				{
					*OutDisabledReason = TEXT("InvalidPressureType");
				}
				return false;
			}
			const int32 PressureBefore = RunState.Pressure.Get(PressureType);
			RunState.Pressure.Add(PressureType, Effect.Value);
			EffectResult.PressureType = PressureType;
			EffectResult.ActualDelta = RunState.Pressure.Get(PressureType) - PressureBefore;
			EffectResult.bApplied = true;
			break;
		}
		case EWacomRunEventEffectType::ConsumeNode:
		{
			const int32 Count = FMath::Max(0, Effect.Value);
			const int32 NodesBefore = RunState.RemainingNodeCount;
			EffectResult.ActualDelta = -FMath::Min(Count, NodesBefore);
			if (Count > 0)
			{
				const bool bHadEnoughNode = RunState.RemainingNodeCount >= Count;
				RunState.RemainingNodeCount = FMath::Max(0, RunState.RemainingNodeCount - Count);
				if (RunState.RemainingNodeCount <= 0)
				{
					AdvanceToNextPhase();
				}
				if (!bHadEnoughNode)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[RunSession] ApplyRunEventChoiceEffects: 节点不足但已推进时段 Count=%d"),
						Count);
				}
			}
			EffectResult.bApplied = true;
			break;
		}
		case EWacomRunEventEffectType::RemoveCard:
		{
			FName RemoveDisabledReason = NAME_None;
			if (!RemoveOwnedCardForRunEventInternal(Effect.CardDefinition.Get(), RemoveDisabledReason))
			{
				if (OutDisabledReason)
				{
					*OutDisabledReason = RemoveDisabledReason.IsNone() ? FName(TEXT("EffectFailed")) : RemoveDisabledReason;
				}
				return false;
			}
			EffectResult.ActualDelta = -1;
			EffectResult.bApplied = true;
			break;
		}
		case EWacomRunEventEffectType::MarkEventCompleted:
			if (Effect.TargetPersistentId.IsNone())
			{
				UE_LOG(LogTemp, Warning, TEXT("[RunSession] ApplyRunEventChoiceEffects: MarkEventCompleted 缺少 TargetPersistentId"));
				if (OutDisabledReason)
				{
					*OutDisabledReason = TEXT("MissingTargetPersistentId");
				}
				return false;
			}
			RunState.RunEventStates.FindOrAdd(Effect.TargetPersistentId).bCompleted = true;
			EffectResult.ActualDelta = 1;
			EffectResult.bApplied = true;
			break;
		default:
			UE_LOG(LogTemp, Warning, TEXT("[RunSession] ApplyRunEventChoiceEffects: 未知效果类型"));
			if (OutDisabledReason)
			{
				*OutDisabledReason = TEXT("EffectFailed");
			}
			return false;
		}

		if (OutEffectResults)
		{
			OutEffectResults->Add(MoveTemp(EffectResult));
		}
	}
	return true;
}
