// Copyright Wacom. All Rights Reserved.

#include "RunSession.h"
#include "WacomSaveGame.h"

#include "Battle/RunBattleSettlementResolver.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Deck/RunDeckRules.h"
#include "Events/RunEventExecutor.h"
#include "Events/RunEventDefinition.h"
#include "Save/RunSaveGameSerializer.h"
#include "Session/BattleSession.h"
#include "Shops/RunShopTransaction.h"
#include "Tags/WacomGameplayTags.h"
#include "Time/RunTimeRules.h"

#include "Kismet/GameplayStatics.h"

// ================ 内部辅助 ================

enum class ERunUiSnapshotDirtyFlags : uint8
{
	None = 0,
	BackpackStorage = 1 << 0,
	Shop = 1 << 1,
	Economy = 1 << 2,
};

namespace
{
	ERunUiSnapshotDirtyFlags operator|(
		ERunUiSnapshotDirtyFlags A,
		ERunUiSnapshotDirtyFlags B)
	{
		return static_cast<ERunUiSnapshotDirtyFlags>(
			static_cast<uint8>(A) | static_cast<uint8>(B));
	}

	ERunUiSnapshotDirtyFlags& operator|=(
		ERunUiSnapshotDirtyFlags& A,
		ERunUiSnapshotDirtyFlags B)
	{
		A = A | B;
		return A;
	}

	ERunUiSnapshotDirtyFlags MakeRunUiSnapshotDirtyFlags()
	{
		return ERunUiSnapshotDirtyFlags::None;
	}

	ERunUiSnapshotDirtyFlags MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags Flag)
	{
		return Flag;
	}

	ERunUiSnapshotDirtyFlags MakeRunUiSnapshotDirtyFlags(
		ERunUiSnapshotDirtyFlags A,
		ERunUiSnapshotDirtyFlags B)
	{
		return A | B;
	}

	ERunUiSnapshotDirtyFlags MakeRunUiSnapshotDirtyFlags(
		ERunUiSnapshotDirtyFlags A,
		ERunUiSnapshotDirtyFlags B,
		ERunUiSnapshotDirtyFlags C)
	{
		return A | B | C;
	}

	bool HasRunUiSnapshotDirtyFlag(
		ERunUiSnapshotDirtyFlags DirtyFlags,
		ERunUiSnapshotDirtyFlags Flag)
	{
		return (static_cast<uint8>(DirtyFlags) & static_cast<uint8>(Flag)) != 0;
	}

	bool ShouldStarterCardStartInBattleDeck(const UCardDefinition* Card)
	{
		// 原型内容规则：暮色引虫灯默认进入备战区，但仍作为 A 类容器贡献通量容量。
		// 后续若类似规则增多，应抽成 Card/Character 数据字段，而不是继续扩硬编码列表。
		return Card && Card->CardId == FName(TEXT("MuseiYinchongdeng"));
	}

	FName GetRunBattleEncounterId(FName TriggerPersistentId)
	{
		return TriggerPersistentId.IsNone() ? FName(TEXT("Encounter")) : TriggerPersistentId;
	}

	bool IsFluxContentCardDefinition(const UCardDefinition* Card)
	{
		return FRunDeckRules::IsFluxContentCardDefinition(Card);
	}

	bool ContainsCardDefinition(
		const TArray<TObjectPtr<UCardDefinition>>& Definitions,
		const UCardDefinition* Definition)
	{
		return Definition
			&& Definitions.ContainsByPredicate(
				[Definition](const TObjectPtr<UCardDefinition>& Candidate)
				{
					return Candidate.Get() == Definition;
				});
	}

	bool ContainsGuid(const TArray<FGuid>& Guids, FGuid Guid)
	{
		return Guids.ContainsByPredicate(
			[Guid](const FGuid& Candidate)
			{
				return Candidate == Guid;
			});
	}

	const TCHAR* ToRunCardWorkspaceKindDebugString(ERunCardWorkspaceKind Kind)
	{
		switch (Kind)
		{
		case ERunCardWorkspaceKind::DefaultExploration:
			return TEXT("DefaultExploration");
		case ERunCardWorkspaceKind::OwnedCardsFilter:
			return TEXT("OwnedCardsFilter");
		default:
			return TEXT("Unknown");
		}
	}

	bool HasRunCardWorkspaceFilter(const FRunCardWorkspaceRequest& Request)
	{
		return Request.AllowedCardDefinitions.Num() > 0
			|| Request.AllowedCardIds.Num() > 0
			|| Request.ExplicitCardInstanceIds.Num() > 0
			|| !Request.RequiredKeywords.IsEmpty()
			|| !Request.BlockedKeywords.IsEmpty();
	}

	bool DoesInstanceMatchRunCardWorkspaceRequest(
		const FCardInstance& Instance,
		const FRunCardWorkspaceRequest& Request)
	{
		const UCardDefinition* Definition = Instance.Definition;
		if (!Instance.InstanceId.IsValid() || !Definition)
		{
			return false;
		}

		if (Request.ExplicitCardInstanceIds.Num() > 0
			&& !ContainsGuid(Request.ExplicitCardInstanceIds, Instance.InstanceId))
		{
			return false;
		}

		const bool bHasIdentityFilter =
			Request.AllowedCardDefinitions.Num() > 0
			|| Request.AllowedCardIds.Num() > 0;
		if (bHasIdentityFilter)
		{
			const bool bMatchesDefinition =
				ContainsCardDefinition(Request.AllowedCardDefinitions, Definition);
			const bool bMatchesCardId =
				Request.AllowedCardIds.Contains(Definition->CardId);
			if (!bMatchesDefinition && !bMatchesCardId)
			{
				return false;
			}
		}

		if (!Request.RequiredKeywords.IsEmpty()
			&& !Definition->Keywords.HasAllExact(Request.RequiredKeywords))
		{
			return false;
		}

		if (!Request.BlockedKeywords.IsEmpty()
			&& Definition->Keywords.HasAnyExact(Request.BlockedKeywords))
		{
			return false;
		}

		return true;
	}

	FRunCardWorkspaceEntry MakeRunCardWorkspaceEntry(
		const FCardInstance& Instance,
		EZoneKind PhysicalZone,
		FGuid ZoneOwnerInstanceId,
		bool bIsProjectedBattleDeckCard)
	{
		FRunCardWorkspaceEntry Entry;
		Entry.Instance = Instance;
		Entry.PhysicalZone = PhysicalZone;
		Entry.ZoneOwnerInstanceId = ZoneOwnerInstanceId;
		Entry.bIsProjectedBattleDeckCard = bIsProjectedBattleDeckCard;
		return Entry;
	}

	void AppendRunCardWorkspaceCandidatesFromZone(
		const TArray<FCardInstance>& ZoneCards,
		EZoneKind PhysicalZone,
		FGuid ZoneOwnerInstanceId,
		const FRunCardWorkspaceRequest& Request,
		FRunCardWorkspaceSnapshot& InOutSnapshot)
	{
		for (const FCardInstance& Instance : ZoneCards)
		{
			if (!Instance.InstanceId.IsValid() || !Instance.Definition)
			{
				continue;
			}

			++InOutSnapshot.ConsideredCount;
			if (DoesInstanceMatchRunCardWorkspaceRequest(Instance, Request))
			{
				InOutSnapshot.Entries.Add(
					MakeRunCardWorkspaceEntry(
						Instance,
						PhysicalZone,
						ZoneOwnerInstanceId,
						/*bIsProjectedBattleDeckCard*/ false));
			}
		}
	}

	FString BuildRunCardWorkspaceDebugSummary(
		const FRunCardWorkspaceRequest& Request,
		const FRunCardWorkspaceSnapshot& Snapshot)
	{
		return FString::Printf(
			TEXT("RunCardWorkspace{Id=%s Kind=%s Succeeded=%s Reject=%s Entries=%d Candidates=%d Considered=%d PhysicalBattleDeck=%d ProjectedBattleDeck=%d Filters{Definitions=%d CardIds=%d Instances=%d Required=%s Blocked=%s AllowAllWhenEmpty=%s Include{Backpack=%s BattleDeck=%s Burden=%s Special=%s Projected=%s}}}"),
			*Snapshot.WorkspaceId.ToString(),
			ToRunCardWorkspaceKindDebugString(Snapshot.Kind),
			Snapshot.bSucceeded ? TEXT("true") : TEXT("false"),
			*Snapshot.RejectReason.ToString(),
			Snapshot.Entries.Num(),
			Snapshot.Entries.Num(),
			Snapshot.ConsideredCount,
			Snapshot.PhysicalBattleDeckCount,
			Snapshot.ProjectedBattleDeckCount,
			Request.AllowedCardDefinitions.Num(),
			Request.AllowedCardIds.Num(),
			Request.ExplicitCardInstanceIds.Num(),
			*Request.RequiredKeywords.ToStringSimple(),
			*Request.BlockedKeywords.ToStringSimple(),
			Request.bAllowAllOwnedCardsWhenNoFilter ? TEXT("true") : TEXT("false"),
			Request.bIncludeBackpack ? TEXT("true") : TEXT("false"),
			Request.bIncludeBattleDeck ? TEXT("true") : TEXT("false"),
			Request.bIncludeBurdenZone ? TEXT("true") : TEXT("false"),
			Request.bIncludeSpecialZones ? TEXT("true") : TEXT("false"),
			Request.bIncludeProjectedBattleDeckCards ? TEXT("true") : TEXT("false"));
	}

	bool HasRunWorldCardInteractionFilter(const FRunWorldCardInteractionRequest& Request)
	{
		return Request.AllowedCardDefinitions.Num() > 0
			|| Request.AllowedCardIds.Num() > 0
			|| !Request.RequiredKeywords.IsEmpty()
			|| !Request.BlockedKeywords.IsEmpty();
	}

	int32 GetRunWorldCardInteractionGoldTotal(
		const TArray<FWacomRunWorldCardInteractionReward>& Rewards)
	{
		int32 Total = 0;
		for (const FWacomRunWorldCardInteractionReward& Reward : Rewards)
		{
			if (Reward.Type == EWacomRunWorldCardInteractionRewardType::Gold)
			{
				Total += Reward.GoldAmount;
			}
		}
		return Total;
	}

	int32 GetRunWorldCardInteractionCardRewardCount(
		const TArray<FWacomRunWorldCardInteractionReward>& Rewards)
	{
		int32 Count = 0;
		for (const FWacomRunWorldCardInteractionReward& Reward : Rewards)
		{
			if (Reward.Type == EWacomRunWorldCardInteractionRewardType::Card)
			{
				++Count;
			}
		}
		return Count;
	}

	void FinalizeRunWorldCardInteractionValidationDebug(
		FRunWorldCardInteractionValidation& Result,
		const FRunWorldCardInteractionRequest& Request)
	{
		Result.DebugSummary = FString::Printf(
			TEXT("RunWorldCardInteraction{PersistentId=%s SourceCard=%s SourceCardId=%s Consume=%s RewardCount=%d GoldTotal=%d CardRewardCount=%d CanSubmit=%s Reason=%s AllowedDefs=%d AllowedIds=%d RequiredKeywords=%d BlockedKeywords=%d}"),
			*Request.PersistentId.ToString(),
			*Request.SourceCardInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
			*Result.SourceCardId.ToString(),
			Request.bConsumeCardOnSuccess ? TEXT("true") : TEXT("false"),
			Request.Rewards.Num(),
			GetRunWorldCardInteractionGoldTotal(Request.Rewards),
			GetRunWorldCardInteractionCardRewardCount(Request.Rewards),
			Result.bCanSubmit ? TEXT("true") : TEXT("false"),
			*Result.DisabledReason.ToString(),
			Request.AllowedCardDefinitions.Num(),
			Request.AllowedCardIds.Num(),
			Request.RequiredKeywords.Num(),
			Request.BlockedKeywords.Num());
	}

	ERunUiSnapshotDirtyFlags GetRunWorldCardInteractionDirtyFlags(
		const FRunWorldCardInteractionRequest& Request)
	{
		ERunUiSnapshotDirtyFlags DirtyFlags = MakeRunUiSnapshotDirtyFlags();
		if (Request.bConsumeCardOnSuccess || GetRunWorldCardInteractionCardRewardCount(Request.Rewards) > 0)
		{
			DirtyFlags |= MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::BackpackStorage);
		}
		if (GetRunWorldCardInteractionGoldTotal(Request.Rewards) > 0)
		{
			DirtyFlags |= MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::Economy);
		}
		return DirtyFlags;
	}

	ERunUiSnapshotDirtyFlags GetRunEventChoiceResultDirtyFlags(const FRunEventChoiceResult& Result)
	{
		ERunUiSnapshotDirtyFlags DirtyFlags = MakeRunUiSnapshotDirtyFlags();
		if (Result.PaidCardInstanceId.IsValid())
		{
			DirtyFlags |= MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::BackpackStorage);
		}
		for (const FRunEventChoiceEffectResult& EffectResult : Result.EffectResults)
		{
			if (!EffectResult.bApplied)
			{
				continue;
			}
			if (EffectResult.EffectType == EWacomRunEventEffectType::GainCard
				|| EffectResult.EffectType == EWacomRunEventEffectType::RemoveCard)
			{
				DirtyFlags |= MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::BackpackStorage);
			}
			else if (EffectResult.EffectType == EWacomRunEventEffectType::AddGold)
			{
				DirtyFlags |= MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::Economy);
			}
		}
		return DirtyFlags;
	}

}

// ================ 通知辅助 ================

struct URunSession::FScopedRunStateNotificationBatch
{
	explicit FScopedRunStateNotificationBatch(URunSession& InRun)
		: Run(InRun)
	{
		Run.BeginRunStateNotificationBatch();
	}

	~FScopedRunStateNotificationBatch()
	{
		Run.EndRunStateNotificationBatch();
	}

	URunSession& Run;
};

void URunSession::NotifyRunStateChanged()
{
	if (RunStateNotificationDeferralDepth > 0)
	{
		bRunStateNotificationPending = true;
		return;
	}

	BroadcastRunStateChangedImmediately();
}

void URunSession::BroadcastRunStateChangedImmediately()
{
	// OnRunStateChangedNative 是原生委托，订阅方用 AddUObject + RemoveAll(this) 管理生命周期。
	// 当前粗粒度广播不区分变更字段，订阅方按需读 RunState 全量。
	OnRunStateChangedNative.Broadcast();
}

void URunSession::BeginRunStateNotificationBatch()
{
	++RunStateNotificationDeferralDepth;
}

void URunSession::EndRunStateNotificationBatch()
{
	if (RunStateNotificationDeferralDepth <= 0)
	{
		ensureMsgf(false, TEXT("URunSession notification batch ended without matching begin."));
		RunStateNotificationDeferralDepth = 0;
		return;
	}

	--RunStateNotificationDeferralDepth;
	if (RunStateNotificationDeferralDepth > 0 || !bRunStateNotificationPending)
	{
		return;
	}

	bRunStateNotificationPending = false;
	BroadcastRunStateChangedImmediately();
}

void URunSession::MarkRunUiSnapshotsDirty(ERunUiSnapshotDirtyFlags DirtyFlags)
{
	if (HasRunUiSnapshotDirtyFlag(DirtyFlags, ERunUiSnapshotDirtyFlags::BackpackStorage))
	{
		++BackpackStorageSnapshotRevision;
	}
	if (HasRunUiSnapshotDirtyFlag(DirtyFlags, ERunUiSnapshotDirtyFlags::Shop))
	{
		++ShopSnapshotRevision;
	}
	if (HasRunUiSnapshotDirtyFlag(DirtyFlags, ERunUiSnapshotDirtyFlags::Economy))
	{
		++EconomySnapshotRevision;
	}
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
	MarkRunUiSnapshotsDirty(
		MakeRunUiSnapshotDirtyFlags(
			ERunUiSnapshotDirtyFlags::BackpackStorage,
			ERunUiSnapshotDirtyFlags::Shop,
			ERunUiSnapshotDirtyFlags::Economy));
	NotifyRunStateChanged();
	return true;
}

void URunSession::ResetRunState()
{
	UCharacterDefinition* KeepChar = RunState.Character;
	Initialize(KeepChar);
}

// ================ 战斗联动 ================

bool URunSession::BuildInitParamsForBattle(FName TriggerPersistentId, FBattleInitParams& OutParams) const
{
	if (!RunState.Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] BuildInitParamsForBattle: RunState.Character 为空"));
		return false;
	}

	OutParams.Character  = RunState.Character;
	OutParams.EncounterId = GetRunBattleEncounterId(TriggerPersistentId);
	OutParams.RandomSeed = RunState.BattleSeed;

	// 阈值常量从 RunState 灌入战内，而不是战内硬编码。
	OutParams.HighHpThreshold = RunState.HighHpThreshold;
	OutParams.LowHpThreshold  = RunState.LowHpThreshold;

	// 战斗只读备战卡组。BattleDeckEntries 让来自 SpecialZone 的入战卡携带对应 B 主卡 CapacityEffect。
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
	OutParams.PreDestroyedParts.Reset();
	if (!TriggerPersistentId.IsNone())
	{
		if (const FBattleProgressSnapshot* Progress = RunState.BattleProgress.Find(TriggerPersistentId))
		{
			if (Progress->DestroyedPartKeys.Num() > 0)
			{
				OutParams.PreDestroyedParts.Reserve(Progress->DestroyedPartKeys.Num());
				for (const FBattleEnemyPartKey& DestroyedPartKey : Progress->DestroyedPartKeys)
				{
					if (DestroyedPartKey.IsValidKey())
					{
						OutParams.PreDestroyedParts.AddUnique(
							FBattlePartSlotIdentity::FromEnemyPartKey(DestroyedPartKey));
					}
				}
			}
			else if (Progress->DestroyedParts.Num() > 0)
			{
				OutParams.PreDestroyedParts = Progress->DestroyedParts;
			}
		}
	}
	return true;
}

void URunSession::OnBattleFinished(const FBattleResultPacket& Packet)
{
	OnBattleFinishedFromTrigger(Packet, NAME_None);
}

void URunSession::OnBattleFinishedFromTrigger(const FBattleResultPacket& Packet, FName TriggerPersistentId)
{
	FScopedRunStateNotificationBatch NotificationBatch(*this);

	const bool bResolved = FRunBattleSettlementResolver::Resolve(
		RunState,
		Packet,
		TriggerPersistentId,
		FRunBattleSettlementResolver::FCallbacks{
			[this](EWacomPressureType Type, int32 Delta)
			{
				AddPressure(Type, Delta);
			},
			[this](int32 Amount)
			{
				AddExperience(Amount);
			},
			[this](UCardDefinition* Card)
			{
				AcquireCardToRun(Card);
			},
			[this](EWacomPressureType Type)
			{
				return GetPressureValue(Type);
			},
		});
	if (!bResolved)
	{
		return;
	}

	// 整体通知一次（即便上面没改字段也发，让 UI 在战斗结束统一刷新）
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
	return FRunSaveGameSerializer::BuildSaveGameFromRunState(RunState);
}

bool URunSession::ApplySaveGameToRunState(UWacomSaveGame* SaveGame)
{
	if (!FRunSaveGameSerializer::TryApplySaveGameToRunState(SaveGame, RunState))
	{
		return false;
	}
	MarkRunUiSnapshotsDirty(
		MakeRunUiSnapshotDirtyFlags(
			ERunUiSnapshotDirtyFlags::BackpackStorage,
			ERunUiSnapshotDirtyFlags::Shop,
			ERunUiSnapshotDirtyFlags::Economy));
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
		TEXT("[RunSession] LoadFromSlot(%s) OK: Character=%s, Triggers=%d, HasPlayerTransform=%d"),
		*SlotName,
		*GetNameSafe(RunState.Character),
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
	MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::BackpackStorage));
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
	const bool bEnough = FRunTimeRules::ConsumeNode(RunState, Count);
	if (Count > 0)
	{
		NotifyRunStateChanged();
	}
	return bEnough;
}

void URunSession::AdvanceToNextPhase()
{
	FRunTimeRules::AdvanceToNextPhase(RunState);
	NotifyRunStateChanged();
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
				MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::BackpackStorage));
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

FRunCardWorkspaceSnapshot URunSession::BuildRunCardWorkspaceSnapshot(
	const FRunCardWorkspaceRequest& Request) const
{
	FRunCardWorkspaceSnapshot Snapshot;
	Snapshot.WorkspaceId = Request.WorkspaceId;
	Snapshot.Kind = Request.Kind;

	auto Finish = [&Request, &Snapshot](bool bSucceeded, FName RejectReason)
	{
		Snapshot.bSucceeded = bSucceeded;
		Snapshot.RejectReason = RejectReason;
		Snapshot.DebugSummary = BuildRunCardWorkspaceDebugSummary(Request, Snapshot);
		return Snapshot;
	};

	if (Request.Kind == ERunCardWorkspaceKind::DefaultExploration)
	{
		const FRunBackpackStorageSnapshot StorageSnapshot =
			BuildBackpackStorageSnapshot();
		Snapshot.PhysicalBattleDeckCount =
			StorageSnapshot.BattleDeckPhysicalCards.Num();
		Snapshot.ProjectedBattleDeckCount =
			Request.bIncludeProjectedBattleDeckCards
				? StorageSnapshot.BattleDeckProjectedCards.Num()
				: 0;
		Snapshot.Entries.Reserve(
			Snapshot.PhysicalBattleDeckCount
			+ Snapshot.ProjectedBattleDeckCount);

		for (const FRunStorageCardView& View :
			StorageSnapshot.BattleDeckPhysicalCards)
		{
			if (!View.Instance.InstanceId.IsValid() || !View.Instance.Definition)
			{
				continue;
			}
			Snapshot.Entries.Add(
				MakeRunCardWorkspaceEntry(
					View.Instance,
					View.PhysicalZone,
					View.ZoneOwnerInstanceId,
					/*bIsProjectedBattleDeckCard*/ false));
		}

		if (Request.bIncludeProjectedBattleDeckCards)
		{
			for (const FRunStorageCardView& View :
				StorageSnapshot.BattleDeckProjectedCards)
			{
				if (!View.Instance.InstanceId.IsValid()
					|| !View.Instance.Definition)
				{
					continue;
				}
				Snapshot.Entries.Add(
					MakeRunCardWorkspaceEntry(
						View.Instance,
						View.PhysicalZone,
						View.ZoneOwnerInstanceId,
						/*bIsProjectedBattleDeckCard*/ true));
			}
		}

		Snapshot.ConsideredCount = Snapshot.Entries.Num();
		return Finish(/*bSucceeded*/ true, NAME_None);
	}

	if (Request.Kind != ERunCardWorkspaceKind::OwnedCardsFilter)
	{
		return Finish(/*bSucceeded*/ false, TEXT("UnsupportedWorkspaceKind"));
	}

	if (!HasRunCardWorkspaceFilter(Request)
		&& !Request.bAllowAllOwnedCardsWhenNoFilter)
	{
		return Finish(/*bSucceeded*/ false, TEXT("EmptyFilter"));
	}

	if (Request.bIncludeBackpack)
	{
		AppendRunCardWorkspaceCandidatesFromZone(
			RunState.Backpack,
			EZoneKind::Backpack,
			FGuid(),
			Request,
			Snapshot);
	}
	if (Request.bIncludeBattleDeck)
	{
		AppendRunCardWorkspaceCandidatesFromZone(
			RunState.BattleDeck,
			EZoneKind::BattleDeck,
			FGuid(),
			Request,
			Snapshot);
	}
	if (Request.bIncludeBurdenZone)
	{
		AppendRunCardWorkspaceCandidatesFromZone(
			RunState.BurdenZone,
			EZoneKind::BurdenZone,
			FGuid(),
			Request,
			Snapshot);
	}
	if (Request.bIncludeSpecialZones)
	{
		for (const FSpecialZone& SpecialZone : RunState.SpecialZones)
		{
			AppendRunCardWorkspaceCandidatesFromZone(
				SpecialZone.Cards,
				EZoneKind::SpecialZone,
				SpecialZone.OwnerInstanceId,
				Request,
				Snapshot);
		}
	}

	return Finish(
		/*bSucceeded*/ !Snapshot.Entries.IsEmpty(),
		Snapshot.Entries.IsEmpty() ? FName(TEXT("NoMatchingCandidates")) : NAME_None);
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
	MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::BackpackStorage));
	NotifyRunStateChanged();
	return true;
}

FRunDeckOperationValidation URunSession::ValidateMoveInstance(FGuid InstanceId, EZoneKind ToZone, FGuid ToZoneOwnerInstanceId) const
{
	return FRunDeckRules::ValidateMoveInstance(RunState, InstanceId, ToZone, ToZoneOwnerInstanceId);
}

void URunSession::AcquireCardToRun(UCardDefinition* Card)
{
	if (AcquireCardToRunInternal(Card))
	{
		MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::BackpackStorage));
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

bool URunSession::DestroyCardByInstance(FGuid InstanceId)
{
	const bool bOk = DestroyCardByInstanceInternal(InstanceId);
	if (bOk)
	{
		MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::BackpackStorage));
		NotifyRunStateChanged();
	}
	return bOk;
}

bool URunSession::DestroyCardByInstanceInternal(FGuid InstanceId)
{
	FName DisabledReason = NAME_None;
	if (!FRunDeckRules::PermanentRemoveOwnedInstance(RunState, InstanceId, &DisabledReason))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] DestroyCardByInstance: 拒绝 InstanceId=%s Reason=%s"),
			*InstanceId.ToString(), *DisabledReason.ToString());
		return false;
	}

	return true;
}

bool URunSession::DeleteCardForGoldByInstance(FGuid InstanceId)
{
	const FRunDeckOperationValidation Validation = ValidateDeleteCardForGoldByInstance(InstanceId);
	if (!Validation.bCanExecute)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] DeleteCardForGoldByInstance: 拒绝 InstanceId=%s Reason=%s"),
			*InstanceId.ToString(), *Validation.DisabledReason.ToString());
		return false;
	}

	const int32 GoldReward = GetDeleteGoldRewardForInstance(InstanceId);

	FName DisabledReason = NAME_None;
	if (!FRunDeckRules::PermanentRemoveOwnedInstance(RunState, InstanceId, &DisabledReason))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] DeleteCardForGoldByInstance: 提交失败 InstanceId=%s Reason=%s"),
			*InstanceId.ToString(), *DisabledReason.ToString());
		return false;
	}

	if (GoldReward > 0)
	{
		RunState.Gold += GoldReward;
	}
	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] DeleteCardForGoldByInstance: %s → +%d gold (total=%d)"),
		*InstanceId.ToString(), GoldReward, RunState.Gold);
	MarkRunUiSnapshotsDirty(
		MakeRunUiSnapshotDirtyFlags(
			ERunUiSnapshotDirtyFlags::BackpackStorage,
			ERunUiSnapshotDirtyFlags::Economy));
	NotifyRunStateChanged();
	return true;
}

int32 URunSession::GetDeleteGoldRewardForCard(const UCardDefinition* Card)
{
	return FRunDeckRules::GetDeleteGoldRewardForCard(Card);
}

int32 URunSession::GetDeleteGoldRewardForInstance(FGuid InstanceId) const
{
	FRunOwnedCardLocation Location;
	if (!FRunDeckRules::FindOwnedCardInstance(RunState, InstanceId, Location))
	{
		return 0;
	}
	return FRunDeckRules::GetDeleteGoldRewardForCard(Location.Instance.Definition);
}

FRunDeckOperationValidation URunSession::ValidateDestroyCardByInstance(FGuid InstanceId) const
{
	return FRunDeckRules::ValidatePermanentRemoveInstance(RunState, InstanceId);
}

FRunDeckOperationValidation URunSession::ValidateDeleteCardForGoldByInstance(FGuid InstanceId) const
{
	return FRunDeckRules::ValidatePermanentRemoveInstance(RunState, InstanceId);
}

// ================ §11.7 / 经济：金币 ================

void URunSession::AddGold(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	RunState.Gold += Amount;
	MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::Economy));
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
	MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::Economy));
	NotifyRunStateChanged();
	return true;
}

bool URunSession::IsPickupCollected(FName PersistentId) const
{
	if (PersistentId.IsNone())
	{
		return false;
	}
	return RunState.CollectedPickupIds.Contains(PersistentId);
}

bool URunSession::CollectGoldPickup(FName PersistentId, int32 GoldAmount)
{
	if (PersistentId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] CollectGoldPickup: PersistentId 为空，拒绝"));
		return false;
	}
	if (GoldAmount <= 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] CollectGoldPickup: PersistentId=%s GoldAmount=%d 非正数，拒绝"),
			*PersistentId.ToString(),
			GoldAmount);
		return false;
	}
	if (RunState.CollectedPickupIds.Contains(PersistentId))
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[RunSession] CollectGoldPickup: PersistentId=%s 已拾取，忽略重复提交"),
			*PersistentId.ToString());
		return false;
	}

	RunState.Gold += GoldAmount;
	RunState.CollectedPickupIds.Add(PersistentId);
	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] CollectGoldPickup: PersistentId=%s +%d gold (total=%d)"),
		*PersistentId.ToString(),
		GoldAmount,
		RunState.Gold);
	MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::Economy));
	NotifyRunStateChanged();
	return true;
}

bool URunSession::CollectCardPickup(FName PersistentId, UCardDefinition* CardDefinition)
{
	if (PersistentId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] CollectCardPickup: PersistentId 为空，拒绝"));
		return false;
	}
	if (!CardDefinition)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] CollectCardPickup: PersistentId=%s CardDefinition 为空，拒绝"),
			*PersistentId.ToString());
		return false;
	}
	if (RunState.CollectedPickupIds.Contains(PersistentId))
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[RunSession] CollectCardPickup: PersistentId=%s 已拾取，忽略重复提交"),
			*PersistentId.ToString());
		return false;
	}
	if (!AcquireCardToRunInternal(CardDefinition))
	{
		return false;
	}

	RunState.CollectedPickupIds.Add(PersistentId);
	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] CollectCardPickup: PersistentId=%s Card=%s"),
		*PersistentId.ToString(),
		*GetNameSafe(CardDefinition));
	MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::BackpackStorage));
	NotifyRunStateChanged();
	return true;
}

bool URunSession::IsRunWorldInteractionCompleted(FName PersistentId) const
{
	if (PersistentId.IsNone())
	{
		return false;
	}
	return RunState.CompletedRunWorldInteractionIds.Contains(PersistentId);
}

FRunWorldCardInteractionValidation URunSession::ValidateRunWorldCardInteraction(
	const FRunWorldCardInteractionRequest& Request) const
{
	FRunWorldCardInteractionValidation Result;
	auto RejectWith = [&Result, &Request](FName Reason)
	{
		Result.bCanSubmit = false;
		Result.DisabledReason = Reason;
		FinalizeRunWorldCardInteractionValidationDebug(Result, Request);
		return Result;
	};

	if (Request.PersistentId.IsNone())
	{
		return RejectWith(TEXT("MissingPersistentId"));
	}
	if (RunState.CompletedRunWorldInteractionIds.Contains(Request.PersistentId))
	{
		return RejectWith(TEXT("AlreadyCompleted"));
	}
	if (!Request.SourceCardInstanceId.IsValid())
	{
		return RejectWith(TEXT("MissingSourceCard"));
	}
	if (Request.Rewards.Num() <= 0)
	{
		return RejectWith(TEXT("MissingReward"));
	}
	for (const FWacomRunWorldCardInteractionReward& Reward : Request.Rewards)
	{
		switch (Reward.Type)
		{
		case EWacomRunWorldCardInteractionRewardType::Gold:
			if (Reward.GoldAmount <= 0)
			{
				return RejectWith(TEXT("InvalidGoldReward"));
			}
			break;
		case EWacomRunWorldCardInteractionRewardType::Card:
			if (!Reward.CardDefinition)
			{
				return RejectWith(TEXT("MissingCardDefinition"));
			}
			break;
		case EWacomRunWorldCardInteractionRewardType::None:
		default:
			return RejectWith(TEXT("MissingReward"));
		}
	}
	if (!HasRunWorldCardInteractionFilter(Request))
	{
		return RejectWith(TEXT("MissingCardFilter"));
	}

	FRunOwnedCardLocation Location;
	if (!FRunDeckRules::FindOwnedCardInstance(
		RunState,
		Request.SourceCardInstanceId,
		Location))
	{
		return RejectWith(TEXT("MissingSourceCard"));
	}

	UCardDefinition* SourceDefinition = Location.Instance.Definition.Get();
	Result.SourceCardDefinition = SourceDefinition;
	Result.SourceCardId = SourceDefinition ? SourceDefinition->CardId : NAME_None;
	if (!SourceDefinition)
	{
		return RejectWith(TEXT("MissingCardDefinition"));
	}

	const bool bHasIdentityFilter =
		Request.AllowedCardDefinitions.Num() > 0
		|| Request.AllowedCardIds.Num() > 0;
	if (bHasIdentityFilter)
	{
		const bool bMatchesDefinition =
			ContainsCardDefinition(Request.AllowedCardDefinitions, SourceDefinition);
		const bool bMatchesCardId =
			Request.AllowedCardIds.Contains(SourceDefinition->CardId);
		if (!bMatchesDefinition && !bMatchesCardId)
		{
			return RejectWith(TEXT("CardNotAccepted"));
		}
	}

	if (!Request.RequiredKeywords.IsEmpty()
		&& !SourceDefinition->Keywords.HasAllExact(Request.RequiredKeywords))
	{
		return RejectWith(TEXT("MissingRequiredKeyword"));
	}
	if (!Request.BlockedKeywords.IsEmpty()
		&& SourceDefinition->Keywords.HasAnyExact(Request.BlockedKeywords))
	{
		return RejectWith(TEXT("BlockedKeyword"));
	}

	if (Request.bConsumeCardOnSuccess)
	{
		const FRunDeckOperationValidation DestroyValidation =
			FRunDeckRules::ValidatePermanentRemoveInstance(
				RunState,
				Request.SourceCardInstanceId);
		if (!DestroyValidation.bCanExecute)
		{
			return RejectWith(DestroyValidation.DisabledReason.IsNone()
				? FName(TEXT("RunValidationFailed"))
				: DestroyValidation.DisabledReason);
		}
	}

	Result.bCanSubmit = true;
	Result.DisabledReason = NAME_None;
	FinalizeRunWorldCardInteractionValidationDebug(Result, Request);
	return Result;
}

bool URunSession::SubmitRunWorldCardInteraction(
	const FRunWorldCardInteractionRequest& Request)
{
	FRunWorldCardInteractionValidation Validation =
		ValidateRunWorldCardInteraction(Request);
	if (!Validation.bCanSubmit)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] SubmitRunWorldCardInteraction: 拒绝 PersistentId=%s Card=%s Reason=%s"),
			*Request.PersistentId.ToString(),
			*Request.SourceCardInstanceId.ToString(),
			*Validation.DisabledReason.ToString());
		return false;
	}

	if (Request.bConsumeCardOnSuccess
		&& !DestroyCardByInstanceInternal(Request.SourceCardInstanceId))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] SubmitRunWorldCardInteraction: 精确销毁卡牌失败 PersistentId=%s Card=%s"),
			*Request.PersistentId.ToString(),
			*Request.SourceCardInstanceId.ToString());
		return false;
	}

	for (const FWacomRunWorldCardInteractionReward& Reward : Request.Rewards)
	{
		switch (Reward.Type)
		{
		case EWacomRunWorldCardInteractionRewardType::Gold:
			RunState.Gold += Reward.GoldAmount;
			break;
		case EWacomRunWorldCardInteractionRewardType::Card:
			if (!AcquireCardToRunInternal(Reward.CardDefinition))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[RunSession] SubmitRunWorldCardInteraction: 卡牌奖励发放失败 PersistentId=%s RewardCard=%s"),
					*Request.PersistentId.ToString(),
					*GetNameSafe(Reward.CardDefinition.Get()));
				return false;
			}
			break;
		case EWacomRunWorldCardInteractionRewardType::None:
		default:
			UE_LOG(LogTemp, Warning,
				TEXT("[RunSession] SubmitRunWorldCardInteraction: 无效奖励类型 PersistentId=%s"),
				*Request.PersistentId.ToString());
			return false;
		}
	}
	RunState.CompletedRunWorldInteractionIds.Add(Request.PersistentId);
	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] SubmitRunWorldCardInteraction: PersistentId=%s Card=%s Rewards=%d GoldTotal=%d CardRewards=%d (gold=%d)"),
		*Request.PersistentId.ToString(),
		*Request.SourceCardInstanceId.ToString(),
		Request.Rewards.Num(),
		GetRunWorldCardInteractionGoldTotal(Request.Rewards),
		GetRunWorldCardInteractionCardRewardCount(Request.Rewards),
		RunState.Gold);
	MarkRunUiSnapshotsDirty(GetRunWorldCardInteractionDirtyFlags(Request));
	NotifyRunStateChanged();
	return true;
}

// ================ 商店购买 ================

bool URunSession::BeginShopVisit(FName ShopId, const TArray<FRunShopOfferInput>& Offers)
{
	if (FRunShopTransaction::BeginVisit(RunState, ShopId, Offers))
	{
		MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::Shop));
		NotifyRunStateChanged();
		return true;
	}
	return false;
}

void URunSession::EndShopVisit()
{
	if (!IsShopVisitActive())
	{
		return;
	}

	FScopedRunStateNotificationBatch NotificationBatch(*this);

	if (FRunShopTransaction::EndVisit(RunState))
	{
		MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::Shop));
		ConsumeNode(1);
	}
	else
	{
		MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::Shop));
		NotifyRunStateChanged();
	}
}

FRunShopSnapshot URunSession::BuildCurrentShopSnapshot() const
{
	return FRunShopTransaction::BuildSnapshot(RunState);
}

bool URunSession::PurchaseShopOffer(FGuid OfferId)
{
	const bool bPurchased = FRunShopTransaction::PurchaseOffer(
		RunState,
		OfferId,
		[this](UCardDefinition* Card)
		{
			return AcquireCardToRunInternal(Card);
		});
	if (!bPurchased)
	{
		return false;
	}

	MarkRunUiSnapshotsDirty(
		MakeRunUiSnapshotDirtyFlags(
			ERunUiSnapshotDirtyFlags::BackpackStorage,
			ERunUiSnapshotDirtyFlags::Shop,
			ERunUiSnapshotDirtyFlags::Economy));
	NotifyRunStateChanged();
	return true;
}

bool URunSession::PurchaseCardFromShop(UCardDefinition* Card, int32 Price)
{
	const bool bPurchased = FRunShopTransaction::PurchaseCard(
		RunState,
		Card,
		Price,
		[this](UCardDefinition* CardToAcquire)
		{
			return AcquireCardToRunInternal(CardToAcquire);
		});
	if (!bPurchased)
	{
		return false;
	}

	MarkRunUiSnapshotsDirty(
		MakeRunUiSnapshotDirtyFlags(
			ERunUiSnapshotDirtyFlags::BackpackStorage,
			ERunUiSnapshotDirtyFlags::Economy));
	NotifyRunStateChanged();
	return true;
}

// ================ 探索事件 ================

bool URunSession::BeginRunEvent(FName PersistentId, UWacomRunEventDefinition* EventDefinition)
{
	if (FRunEventExecutor::BeginEvent(RunState, PersistentId, EventDefinition))
	{
		NotifyRunStateChanged();
		return true;
	}
	return false;
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
	return FRunEventExecutor::IsEventCompleted(RunState, PersistentId);
}

bool URunSession::IsRunFlagSet(FName FlagId) const
{
	return !FlagId.IsNone() && RunState.RunFlags.Contains(FlagId);
}

FRunEventSnapshot URunSession::BuildCurrentRunEventSnapshot() const
{
	return FRunEventExecutor::BuildSnapshot(RunState);
}

bool URunSession::ChooseRunEventOption(FName ChoiceId)
{
	return ChooseRunEventOptionWithResult(ChoiceId).bSucceeded;
}

FRunEventChoiceResult URunSession::ChooseRunEventOptionWithResult(FName ChoiceId)
{
	FScopedRunStateNotificationBatch NotificationBatch(*this);
	FRunEventChoiceResult Result = FRunEventExecutor::ChooseOption(RunState, ChoiceId);
	if (Result.bSucceeded)
	{
		MarkRunUiSnapshotsDirty(GetRunEventChoiceResultDirtyFlags(Result));
		NotifyRunStateChanged();
	}
	return Result;
}

FRunDeckOperationValidation URunSession::ValidateRunEventOptionCardPayment(
	FName ChoiceId,
	FGuid PaidCardInstanceId) const
{
	return FRunEventExecutor::ValidateChoiceCardPayment(RunState, ChoiceId, PaidCardInstanceId);
}

FRunEventChoiceResult URunSession::ChooseRunEventOptionWithPaidCardResult(
	FName ChoiceId,
	FGuid PaidCardInstanceId)
{
	FScopedRunStateNotificationBatch NotificationBatch(*this);
	FRunEventChoiceResult Result =
		FRunEventExecutor::ChooseOptionWithPaidCard(RunState, ChoiceId, PaidCardInstanceId);
	if (Result.bSucceeded)
	{
		MarkRunUiSnapshotsDirty(GetRunEventChoiceResultDirtyFlags(Result));
		NotifyRunStateChanged();
	}
	return Result;
}
