// Copyright Wacom. All Rights Reserved.

#include "RunSession.h"
#include "WacomSaveGame.h"

#include "Battle/RunBattleSettlementResolver.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Deck/RunDeckRules.h"
#include "Events/RunEventExecutor.h"
#include "Events/RunEventDefinition.h"
#include "Exploration/RunCampModule.h"
#include "Exploration/RunExplorationCommandResolver.h"
#include "Exploration/RunFloorMapSnapshotBuilder.h"
#include "Exploration/RunNodeActivityModule.h"
#include "Exploration/RunMapModule.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "Save/RunSaveGameSerializer.h"
#include "Session/BattleSession.h"
#include "Shops/RunShopTransaction.h"
#include "Tags/WacomGameplayTags.h"

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
	FWacomMapNodeHandle ResolveEncounterProgressNode(const FRunState& State)
	{
		return FWacomMapNodeHandle{
			State.ExplorationState.CurrentFloorId,
			State.ExplorationState.CurrentNodeId };
	}

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

	FRunMapNodeProgress* FindNodeProgress(FRunFloorProgress& FloorProgress, const FName NodeId)
	{
		return FloorProgress.Nodes.FindByPredicate([NodeId](const FRunMapNodeProgress& NodeProgress)
		{
			return NodeProgress.NodeId == NodeId;
		});
	}

	const FRunMapNodeProgress* FindNodeProgress(const FRunFloorProgress& FloorProgress, const FName NodeId)
	{
		return FloorProgress.Nodes.FindByPredicate([NodeId](const FRunMapNodeProgress& NodeProgress)
		{
			return NodeProgress.NodeId == NodeId;
		});
	}

	bool IsSafeArrivalNode(const EWacomMapNodeType NodeType)
	{
		return NodeType == EWacomMapNodeType::Navigation || NodeType == EWacomMapNodeType::Shop;
	}

	bool AcquireCardToWorkingState(FRunState& State, UCardDefinition* Card)
	{
		if (!Card)
		{
			return false;
		}
		FCardInstance Instance;
		Instance.Definition = Card;
		Instance.InstanceId = FGuid::NewGuid();
		if (!Instance.InstanceId.IsValid())
		{
			return false;
		}
		State.Backpack.Add(Instance);
		FRunDeckRules::EnsureSpecialZoneEntryFor(State, Instance);
		FRunDeckRules::RecomputeBurden(State, true);
		return true;
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

FRunInitializationResult URunSession::Initialize(const FRunInitializationParams& Params)
{
	FRunInitializationResult Result;
	Result.PostSnapshot = BuildExplorationSnapshot();

	if (!Params.Character)
	{
		Result.Status = FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("MissingCharacter"));
		return Result;
	}
	if (!Params.Journey || Params.Journey->JourneyId.IsNone() || Params.Journey->Floors.IsEmpty())
	{
		Result.Status = FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("MissingJourney"));
		return Result;
	}

	UWacomFloorMapDefinition* EntryFloor = Params.Journey->Floors[0];
	if (!EntryFloor || EntryFloor->FloorId.IsNone() || EntryFloor->EntryNodeId.IsNone())
	{
		Result.Status = FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("InvalidEntryFloor"));
		return Result;
	}
	const FWacomMapNodeDefinition* EntryNode = EntryFloor->FindNode(EntryFloor->EntryNodeId);
	if (!EntryNode)
	{
		Result.Status = FWacomStatus::Fail(EWacomError::NotFound, TEXT("MissingEntryNode"));
		return Result;
	}

	FRunState WorkingState{};
	WorkingState.Character = Params.Character;
	WorkingState.BattleSeed = 0;
	WorkingState.bRunActive = true;
	WorkingState.PlayerTransform = FTransform::Identity;
	WorkingState.bHasPlayerTransform = false;
	WorkingState.FingerCount = Params.Character->FingerCount;
	WorkingState.HpPerFinger = Params.Character->HpPerFinger;

	for (const TObjectPtr<UCardDefinition>& Card : Params.Character->StarterDeck)
	{
		if (!Card)
		{
			continue;
		}

		FCardInstance Instance;
		Instance.Definition = Card;
		Instance.InstanceId = FGuid::NewGuid();
		if (!Instance.InstanceId.IsValid())
		{
			Result.Status = FWacomStatus::Fail(
				EWacomError::InvalidState,
				TEXT("CardInstanceIdGenerationFailed"));
			return Result;
		}

		if (ShouldStarterCardStartInBattleDeck(Card) || !URunSession::IsContainerCard(Card))
		{
			WorkingState.BattleDeck.Add(Instance);
		}
		else
		{
			WorkingState.Backpack.Add(Instance);
		}
		FRunDeckRules::EnsureSpecialZoneEntryFor(WorkingState, Instance);
	}

	WorkingState.TimeState.CurrentDayNumber = 1;
	WorkingState.TimeState.CurrentTimePhase = ETimePhase::Morning;
	WorkingState.TimeState.PhaseBudgets = Params.Journey->PhaseBudgets;
	WorkingState.TimeState.RemainingActionPoints =
		FMath::Max(0, Params.Journey->PhaseBudgets.Morning - 1);
	WorkingState.TimeState.NightGate = ERunNightGate::Closed;

	WorkingState.ExplorationState.JourneyDefinition = Params.Journey;
	WorkingState.ExplorationState.CurrentFloorId = EntryFloor->FloorId;
	WorkingState.ExplorationState.CurrentNodeId = EntryNode->NodeId;
	WorkingState.ExplorationState.FloorEnteredDayNumber = 1;
	WorkingState.ExplorationState.LastDailyDecayAppliedDayNumber = 1;
	WorkingState.ExplorationState.ExplorationStateVersion = 1;

	for (const TObjectPtr<UWacomFloorMapDefinition>& Floor : Params.Journey->Floors)
	{
		if (!Floor || Floor->FloorId.IsNone())
		{
			Result.Status = FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("MissingFloorDefinition"));
			return Result;
		}

		FRunFloorProgress& FloorProgress =
			WorkingState.ExplorationState.FloorProgress.AddDefaulted_GetRef();
		FloorProgress.FloorId = Floor->FloorId;
		FloorProgress.EnteredDayNumber = Floor == EntryFloor ? 1 : 0;
		FloorProgress.Nodes.Reserve(Floor->Nodes.Num());
		for (const FWacomMapNodeDefinition& Node : Floor->Nodes)
		{
			FRunMapNodeProgress& NodeProgress = FloorProgress.Nodes.AddDefaulted_GetRef();
			NodeProgress.NodeId = Node.NodeId;
			NodeProgress.bLandmarkVisible =
				Node.LandmarkVisibility != EWacomMapLandmarkVisibility::None
				|| (Node.NodeType == EWacomMapNodeType::Encounter && Node.Content.Encounter.bBoss);
		}
	}

	FRunFloorProgress& EntryFloorProgress = WorkingState.ExplorationState.FloorProgress[0];
	FRunMapNodeProgress* EntryProgress = FindNodeProgress(EntryFloorProgress, EntryNode->NodeId);
	if (!EntryProgress)
	{
		Result.Status = FWacomStatus::Fail(EWacomError::InvalidState, TEXT("MissingEntryProgress"));
		return Result;
	}
	EntryProgress->Lifecycle = IsSafeArrivalNode(EntryNode->NodeType)
		? ERunMapNodeLifecycle::Resolved
		: ERunMapNodeLifecycle::Visited;
	const bool bEntryResolved = EntryProgress->Lifecycle == ERunMapNodeLifecycle::Resolved;

	TArray<const FWacomMapEdgeDefinition*> OutgoingEdges;
	EntryFloor->FindOutgoingEdges(EntryNode->NodeId, OutgoingEdges);
	for (const FWacomMapEdgeDefinition* Edge : OutgoingEdges)
	{
		if (Edge)
		{
			if (FRunMapNodeProgress* TargetProgress =
				FindNodeProgress(EntryFloorProgress, Edge->ToNodeId))
			{
				TargetProgress->Lifecycle = ERunMapNodeLifecycle::Revealed;
			}
		}
	}

	FRunDeckRules::RecomputeBurden(WorkingState, true);

	RunState = MoveTemp(WorkingState);
	ActiveTraversalTicket.Reset();
	ActiveNodeActivityTicket.Reset();
	ActiveCampTicket.Reset();
	ActiveFloorTransitionConfirmation.Reset();
	ActiveShopVisitToken.Invalidate();
	ActiveRunEventVisitToken.Invalidate();
	MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(
		ERunUiSnapshotDirtyFlags::BackpackStorage,
		ERunUiSnapshotDirtyFlags::Shop,
		ERunUiSnapshotDirtyFlags::Economy));

	auto AddEvent = [&Result](
		const ERunExplorationEventType Type,
		const FWacomMapNodeHandle& Node = {},
		const FWacomMapEdgeHandle& Edge = {},
		const FName Detail = NAME_None)
	{
		FRunExplorationEvent& Event = Result.Events.AddDefaulted_GetRef();
		Event.Type = Type;
		Event.Node = Node;
		Event.Edge = Edge;
		Event.Detail = Detail;
	};

	const FWacomMapNodeHandle EntryHandle{ EntryFloor->FloorId, EntryNode->NodeId };
	Result.Status = FWacomStatus::Ok();
	AddEvent(ERunExplorationEventType::Initialized, EntryHandle, {}, Params.Journey->JourneyId);
	AddEvent(ERunExplorationEventType::NodeRevealed, EntryHandle);
	AddEvent(ERunExplorationEventType::NodeVisited, EntryHandle);
	if (bEntryResolved)
	{
		AddEvent(ERunExplorationEventType::NodeResolved, EntryHandle);
	}
	for (const FWacomMapEdgeDefinition* Edge : OutgoingEdges)
	{
		if (Edge)
		{
			AddEvent(
				ERunExplorationEventType::NodeRevealed,
				{ EntryFloor->FloorId, Edge->ToNodeId },
				{ EntryFloor->FloorId, Edge->EdgeId });
		}
	}
	Result.PostSnapshot = BuildExplorationSnapshot();
	NotifyRunStateChanged();
	return Result;
}

FRunExplorationSnapshot URunSession::BuildExplorationSnapshot() const
{
	return FRunMapModule::BuildSnapshot(RunState);
}

FRunFloorMapSnapshot URunSession::BuildCurrentFloorMapSnapshot() const
{
	return FRunFloorMapSnapshotBuilder::Build(RunState);
}

FRunExplorationResolution URunSession::BeginCurrentNodeActivity(
	const ERunNodeActivityKind Kind)
{
	FRunExplorationResolution Result;
	Result.VersionBefore = RunState.ExplorationState.ExplorationStateVersion;
	const int32 ReservedActionPoints =
		Kind == ERunNodeActivityKind::Encounter || Kind == ERunNodeActivityKind::Treasure
			? 1
			: 0;
	FRunNodeActivityTicket Ticket;
	Result.Status = FRunNodeActivityModule::Begin(
		RunState,
		ActiveNodeActivityTicket,
		Kind,
		ReservedActionPoints,
		Ticket,
		Result.Events);
	if (!Result.IsOk())
	{
		Result.Events.Reset();
		Result.VersionAfter = Result.VersionBefore;
		Result.PostSnapshot = BuildExplorationSnapshot();
		return Result;
	}
	Result.VersionAfter = RunState.ExplorationState.ExplorationStateVersion;
	Result.NodeActivityTicket = Ticket;
	Result.PostSnapshot = BuildExplorationSnapshot();
	NotifyRunStateChanged();
	return Result;
}

FRunExplorationResolution URunSession::CancelNodeActivity(
	const FRunNodeActivityTicket& Ticket)
{
	FRunExplorationResolution Result;
	Result.VersionBefore = RunState.ExplorationState.ExplorationStateVersion;
	Result.Status = FRunNodeActivityModule::Cancel(
		RunState,
		ActiveNodeActivityTicket,
		Ticket,
		Result.Events);
	if (!Result.IsOk())
	{
		Result.Events.Reset();
		Result.VersionAfter = Result.VersionBefore;
		Result.PostSnapshot = BuildExplorationSnapshot();
		return Result;
	}
	Result.VersionAfter = RunState.ExplorationState.ExplorationStateVersion;
	Result.PostSnapshot = BuildExplorationSnapshot();
	NotifyRunStateChanged();
	return Result;
}

FRunExplorationResolution URunSession::CompleteCampActivity(
	const FRunCampTicket& Ticket,
	const IRunCampActivityHandler& Handler)
{
	FRunExplorationResolution Result;
	Result.VersionBefore = RunState.ExplorationState.ExplorationStateVersion;
	FRunState WorkingState = RunState;
	TOptional<FRunCampTicket> WorkingCamp = ActiveCampTicket;
	Result.Status = FRunCampModule::Complete(
		WorkingState,
		WorkingCamp,
		Ticket,
		Handler,
		Result.Events);
	if (!Result.IsOk())
	{
		Result.Events.Reset();
		Result.VersionAfter = Result.VersionBefore;
		Result.PostSnapshot = BuildExplorationSnapshot();
		return Result;
	}

	WorkingState.ExplorationState.ExplorationStateVersion = Result.VersionBefore + 1;
	Result.VersionAfter = Result.VersionBefore + 1;
	Result.PostSnapshot = FRunMapModule::BuildSnapshot(WorkingState);
	RunState = MoveTemp(WorkingState);
	ActiveCampTicket = MoveTemp(WorkingCamp);
	NotifyRunStateChanged();
	return Result;
}

FRunExplorationResolution URunSession::SettleEncounterNodeActivity(
	const FRunNodeActivityTicket& Ticket,
	const FBattleResultPacket& Packet)
{
	FRunExplorationResolution Result;
	Result.VersionBefore = RunState.ExplorationState.ExplorationStateVersion;
	if (Ticket.Kind != ERunNodeActivityKind::Encounter
		|| !FRunNodeActivityModule::Matches(RunState, ActiveNodeActivityTicket, Ticket))
	{
		Result.Status = FWacomStatus::Fail(
			EWacomError::InvalidState,
			TEXT("EncounterActivityTicketMismatch"));
		Result.VersionAfter = Result.VersionBefore;
		Result.PostSnapshot = BuildExplorationSnapshot();
		return Result;
	}

	FRunState WorkingState = RunState;
	TOptional<FRunNodeActivityTicket> WorkingActivity = ActiveNodeActivityTicket;
	if (!FRunBattleSettlementResolver::Resolve(
		WorkingState,
		Packet,
		Ticket.Node))
	{
		Result.Status = FWacomStatus::Fail(
			EWacomError::InvalidArgument,
			TEXT("InvalidBattleResult"));
		Result.VersionAfter = Result.VersionBefore;
		Result.PostSnapshot = BuildExplorationSnapshot();
		return Result;
	}

	const bool bVictory = Packet.Outcome == EBattleOutcome::Victory && !Packet.bWithdrawn;
	Result.Status = FRunNodeActivityModule::Complete(
		WorkingState,
		WorkingActivity,
		Ticket,
		/*ActionPointCost=*/bVictory ? Ticket.ReservedActionPoints : 0,
		/*bResolveNode=*/bVictory,
		Result.Events);
	if (!Result.IsOk())
	{
		Result.Events.Reset();
		Result.VersionAfter = Result.VersionBefore;
		Result.PostSnapshot = BuildExplorationSnapshot();
		return Result;
	}

	RunState = MoveTemp(WorkingState);
	ActiveNodeActivityTicket = MoveTemp(WorkingActivity);
	if (!Packet.GainedCards.IsEmpty())
	{
		MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(
			ERunUiSnapshotDirtyFlags::BackpackStorage));
	}
	Result.Status = FWacomStatus::Ok();
	Result.VersionAfter = RunState.ExplorationState.ExplorationStateVersion;
	Result.PostSnapshot = BuildExplorationSnapshot();
	NotifyRunStateChanged();
	return Result;
}

FRunExplorationResolution URunSession::ResolveExplorationCommand(
	const FRunExplorationCommand& Command)
{
	FRunState WorkingState = RunState;
	TOptional<FRunTraversalTicket> WorkingTraversal = ActiveTraversalTicket;
	TOptional<FRunCampTicket> WorkingCamp = ActiveCampTicket;
	TOptional<FRunFloorTransitionConfirmation> WorkingFloorTransition =
		ActiveFloorTransitionConfirmation;
	FRunExplorationResolution Result = FRunExplorationCommandResolver::Resolve(
		WorkingState,
		WorkingTraversal,
		WorkingCamp,
		WorkingFloorTransition,
		Command);
	if (!Result.IsOk())
	{
		Result.Events.Reset();
		Result.VersionBefore = RunState.ExplorationState.ExplorationStateVersion;
		Result.VersionAfter = Result.VersionBefore;
		Result.PostSnapshot = BuildExplorationSnapshot();
		return Result;
	}

	RunState = MoveTemp(WorkingState);
	ActiveTraversalTicket = MoveTemp(WorkingTraversal);
	ActiveCampTicket = MoveTemp(WorkingCamp);
	ActiveFloorTransitionConfirmation = MoveTemp(WorkingFloorTransition);
	NotifyRunStateChanged();
	return Result;
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
		const FWacomMapNodeHandle ProgressNode = ResolveEncounterProgressNode(RunState);
		if (const FBattleProgressSnapshot* Progress = RunState.BattleProgress.Find(ProgressNode))
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
	ActiveShopVisitToken.Invalidate();
	ActiveRunEventVisitToken.Invalidate();
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
	FName DisabledReason = NAME_None;
	if (!FRunDeckRules::SetSpecialZoneCardBattleEnabled(RunState, InstanceId, bEnabled, &DisabledReason))
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[RunSession] SetSpecialZoneCardBattleEnabled: 拒绝 InstanceId=%s Reason=%s"),
			*InstanceId.ToString(), *DisabledReason.ToString());
		return false;
	}

	MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::BackpackStorage));
	NotifyRunStateChanged();
	return true;
}

FRunDeckOperationValidation URunSession::ValidateSetSpecialZoneCardBattleEnabled(FGuid InstanceId, bool bEnabled) const
{
	return FRunDeckRules::ValidateSetSpecialZoneCardBattleEnabled(RunState, InstanceId, bEnabled);
}

bool URunSession::ToggleSpecialZoneCardBattleEnabled(FGuid InstanceId)
{
	FName DisabledReason = NAME_None;
	if (!FRunDeckRules::ToggleSpecialZoneCardBattleEnabled(RunState, InstanceId, &DisabledReason))
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[RunSession] ToggleSpecialZoneCardBattleEnabled: 拒绝 InstanceId=%s Reason=%s"),
			*InstanceId.ToString(), *DisabledReason.ToString());
		return false;
	}

	MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::BackpackStorage));
	NotifyRunStateChanged();
	return true;
}

FRunDeckOperationValidation URunSession::ValidateToggleSpecialZoneCardBattleEnabled(FGuid InstanceId) const
{
	return FRunDeckRules::ValidateToggleSpecialZoneCardBattleEnabled(RunState, InstanceId);
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
		View.bCanToggleBattleEnabledInSpecialZone =
			PhysicalZone == EZoneKind::SpecialZone && Inst.InstanceId.IsValid();
		View.bShowBattleEnabledInSpecialZoneBadge =
			View.bCanToggleBattleEnabledInSpecialZone && Inst.bBattleEnabledInSpecialZone;
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

	FRunOwnedCardLocation FromLocation;
	FName DisabledReason = NAME_None;
	if (!FRunDeckRules::MoveInstance(
		RunState,
		InstanceId,
		ToZone,
		ToZoneOwnerInstanceId,
		&FromLocation,
		&DisabledReason))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] MoveInstance: 拒绝 InstanceId %s ToZone=%d Reason=%s"),
			*InstanceId.ToString(), (int32)ToZone, *DisabledReason.ToString());
		return false;
	}

	UE_LOG(LogTemp, Verbose,
		TEXT("[RunSession] MoveInstance: InstanceId=%s, %d → %d, Backpack=%d, BattleDeck=%d"),
		*InstanceId.ToString(), (int32)FromLocation.Zone, (int32)ToZone,
		RunState.Backpack.Num(), RunState.BattleDeck.Num());

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
	if (!AcquireCardToWorkingState(RunState, Card))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] AcquireCardToRun: Card 为空，拒绝"));
		return false;
	}
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

FRunTreasureSettlementResult URunSession::CollectGoldPickup(FName PersistentId, int32 GoldAmount)
{
	FRunTreasureSettlementResult Result;
	if (PersistentId.IsNone() || GoldAmount <= 0)
	{
		Result.DisabledReason = PersistentId.IsNone()
			? FName(TEXT("MissingPersistentId"))
			: FName(TEXT("InvalidGoldReward"));
		return Result;
	}
	if (RunState.CollectedPickupIds.Contains(PersistentId))
	{
		Result.DisabledReason = TEXT("AlreadyCollected");
		return Result;
	}

	FRunState WorkingState = RunState;
	TOptional<FRunNodeActivityTicket> WorkingActivity = ActiveNodeActivityTicket;
	const bool bUsesFormalExploration =
		WorkingState.ExplorationState.JourneyDefinition
		&& WorkingState.ExplorationState.ExplorationStateVersion > 0;
	if (bUsesFormalExploration)
	{
		Result.ExplorationResolution.VersionBefore =
			RunState.ExplorationState.ExplorationStateVersion;
		Result.ExplorationResolution.Status = FRunNodeActivityModule::ResolveImmediate(
			WorkingState,
			WorkingActivity,
			ERunNodeActivityKind::Treasure,
			1,
			true,
			Result.ExplorationResolution.Events);
		if (!Result.ExplorationResolution.IsOk())
		{
			Result.DisabledReason = Result.ExplorationResolution.Status.Detail;
			Result.ExplorationResolution.Events.Reset();
			Result.ExplorationResolution.VersionAfter = Result.ExplorationResolution.VersionBefore;
			Result.ExplorationResolution.PostSnapshot = BuildExplorationSnapshot();
			return Result;
		}
		Result.ActionPointCost = 1;
		Result.ExplorationResolution.VersionAfter =
			WorkingState.ExplorationState.ExplorationStateVersion;
		Result.ExplorationResolution.PostSnapshot = FRunMapModule::BuildSnapshot(WorkingState);
	}
	else
	{
		Result.ExplorationResolution.Status = FWacomStatus::Fail(
			EWacomError::InvalidState,
			TEXT("LegacyRunSessionHasNoExplorationResult"));
	}

	WorkingState.Gold += GoldAmount;
	WorkingState.CollectedPickupIds.Add(PersistentId);
	RunState = MoveTemp(WorkingState);
	ActiveNodeActivityTicket = MoveTemp(WorkingActivity);
	MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::Economy));
	NotifyRunStateChanged();
	Result.bSucceeded = true;
	return Result;
}

FRunTreasureSettlementResult URunSession::CollectCardPickup(
	FName PersistentId,
	UCardDefinition* CardDefinition)
{
	FRunTreasureSettlementResult Result;
	if (PersistentId.IsNone() || !CardDefinition)
	{
		Result.DisabledReason = PersistentId.IsNone()
			? FName(TEXT("MissingPersistentId"))
			: FName(TEXT("MissingCardDefinition"));
		return Result;
	}
	if (RunState.CollectedPickupIds.Contains(PersistentId))
	{
		Result.DisabledReason = TEXT("AlreadyCollected");
		return Result;
	}

	FRunState WorkingState = RunState;
	TOptional<FRunNodeActivityTicket> WorkingActivity = ActiveNodeActivityTicket;
	const bool bUsesFormalExploration =
		WorkingState.ExplorationState.JourneyDefinition
		&& WorkingState.ExplorationState.ExplorationStateVersion > 0;
	if (bUsesFormalExploration)
	{
		Result.ExplorationResolution.VersionBefore =
			RunState.ExplorationState.ExplorationStateVersion;
		Result.ExplorationResolution.Status = FRunNodeActivityModule::ResolveImmediate(
			WorkingState,
			WorkingActivity,
			ERunNodeActivityKind::Treasure,
			1,
			true,
			Result.ExplorationResolution.Events);
		if (!Result.ExplorationResolution.IsOk())
		{
			Result.DisabledReason = Result.ExplorationResolution.Status.Detail;
			Result.ExplorationResolution.Events.Reset();
			Result.ExplorationResolution.VersionAfter = Result.ExplorationResolution.VersionBefore;
			Result.ExplorationResolution.PostSnapshot = BuildExplorationSnapshot();
			return Result;
		}
		Result.ActionPointCost = 1;
		Result.ExplorationResolution.VersionAfter =
			WorkingState.ExplorationState.ExplorationStateVersion;
		Result.ExplorationResolution.PostSnapshot = FRunMapModule::BuildSnapshot(WorkingState);
	}
	else
	{
		Result.ExplorationResolution.Status = FWacomStatus::Fail(
			EWacomError::InvalidState,
			TEXT("LegacyRunSessionHasNoExplorationResult"));
	}
	if (!AcquireCardToWorkingState(WorkingState, CardDefinition))
	{
		Result.DisabledReason = TEXT("CardRewardFailed");
		return Result;
	}

	WorkingState.CollectedPickupIds.Add(PersistentId);
	RunState = MoveTemp(WorkingState);
	ActiveNodeActivityTicket = MoveTemp(WorkingActivity);
	MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::BackpackStorage));
	NotifyRunStateChanged();
	Result.bSucceeded = true;
	return Result;
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

FRunTreasureSettlementResult URunSession::SubmitRunWorldCardInteraction(
	const FRunWorldCardInteractionRequest& Request)
{
	FRunTreasureSettlementResult Result;
	const FRunWorldCardInteractionValidation Validation =
		ValidateRunWorldCardInteraction(Request);
	if (!Validation.bCanSubmit)
	{
		Result.DisabledReason = Validation.DisabledReason;
		return Result;
	}

	FRunState WorkingState = RunState;
	TOptional<FRunNodeActivityTicket> WorkingActivity = ActiveNodeActivityTicket;
	const bool bUsesFormalExploration =
		WorkingState.ExplorationState.JourneyDefinition
		&& WorkingState.ExplorationState.ExplorationStateVersion > 0;
	if (bUsesFormalExploration)
	{
		Result.ExplorationResolution.VersionBefore =
			RunState.ExplorationState.ExplorationStateVersion;
		Result.ExplorationResolution.Status = FRunNodeActivityModule::ResolveImmediate(
			WorkingState,
			WorkingActivity,
			ERunNodeActivityKind::Treasure,
			1,
			true,
			Result.ExplorationResolution.Events);
		if (!Result.ExplorationResolution.IsOk())
		{
			Result.DisabledReason = Result.ExplorationResolution.Status.Detail;
			Result.ExplorationResolution.Events.Reset();
			Result.ExplorationResolution.VersionAfter = Result.ExplorationResolution.VersionBefore;
			Result.ExplorationResolution.PostSnapshot = BuildExplorationSnapshot();
			return Result;
		}
		Result.ActionPointCost = 1;
		Result.ExplorationResolution.VersionAfter =
			WorkingState.ExplorationState.ExplorationStateVersion;
		Result.ExplorationResolution.PostSnapshot = FRunMapModule::BuildSnapshot(WorkingState);
	}
	else
	{
		Result.ExplorationResolution.Status = FWacomStatus::Fail(
			EWacomError::InvalidState,
			TEXT("LegacyRunSessionHasNoExplorationResult"));
	}

	if (Request.bConsumeCardOnSuccess
		&& !FRunDeckRules::PermanentRemoveOwnedInstance(
			WorkingState,
			Request.SourceCardInstanceId))
	{
		Result.DisabledReason = TEXT("SourceCardRemovalFailed");
		return Result;
	}

	for (const FWacomRunWorldCardInteractionReward& Reward : Request.Rewards)
	{
		switch (Reward.Type)
		{
		case EWacomRunWorldCardInteractionRewardType::Gold:
			WorkingState.Gold += Reward.GoldAmount;
			break;
		case EWacomRunWorldCardInteractionRewardType::Card:
			if (!AcquireCardToWorkingState(WorkingState, Reward.CardDefinition))
			{
				Result.DisabledReason = TEXT("CardRewardFailed");
				return Result;
			}
			break;
		case EWacomRunWorldCardInteractionRewardType::None:
		default:
			Result.DisabledReason = TEXT("MissingReward");
			return Result;
		}
	}
	WorkingState.CompletedRunWorldInteractionIds.Add(Request.PersistentId);
	RunState = MoveTemp(WorkingState);
	ActiveNodeActivityTicket = MoveTemp(WorkingActivity);
	MarkRunUiSnapshotsDirty(GetRunWorldCardInteractionDirtyFlags(Request));
	NotifyRunStateChanged();
	Result.bSucceeded = true;
	return Result;
}

// ================ 商店购买 ================

bool URunSession::BeginShopVisit(FName ShopId, const TArray<FRunShopOfferInput>& Offers)
{
	return BeginShopVisitWithResult(ShopId, Offers).bSucceeded;
}

FRunShopVisitResult URunSession::BeginShopVisitWithResult(
	const FName ShopId,
	const TArray<FRunShopOfferInput>& Offers)
{
	FRunShopVisitResult Result;
	Result.ExplorationResolution.VersionBefore =
		RunState.ExplorationState.ExplorationStateVersion;
	Result.ExplorationResolution.VersionAfter =
		Result.ExplorationResolution.VersionBefore;
	Result.ExplorationResolution.PostSnapshot = BuildExplorationSnapshot();
	if (IsShopVisitActive())
	{
		Result.DisabledReason = TEXT("ShopVisitAlreadyActive");
		Result.ExplorationResolution.Status = FWacomStatus::Fail(
			EWacomError::InvalidState,
			Result.DisabledReason);
		return Result;
	}

	const FGuid NewVisitToken = FGuid::NewGuid();
	if (!NewVisitToken.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[RunSession] BeginShopVisit: 生成访问 token 失败"));
		Result.DisabledReason = TEXT("ShopVisitTokenGenerationFailed");
		Result.ExplorationResolution.Status = FWacomStatus::Fail(
			EWacomError::InvalidState,
			Result.DisabledReason);
		return Result;
	}

	FRunState WorkingState = RunState;
	TOptional<FRunNodeActivityTicket> WorkingActivity = ActiveNodeActivityTicket;
	const bool bUsesFormalExploration =
		WorkingState.ExplorationState.JourneyDefinition
		&& WorkingState.ExplorationState.ExplorationStateVersion > 0;
	if (bUsesFormalExploration)
	{
		FRunNodeActivityTicket ActivityTicket;
		Result.ExplorationResolution.Status = FRunNodeActivityModule::Begin(
			WorkingState,
			WorkingActivity,
			ERunNodeActivityKind::Shop,
			/*ReservedActionPoints=*/0,
			ActivityTicket,
			Result.ExplorationResolution.Events);
		if (!Result.ExplorationResolution.IsOk())
		{
			Result.DisabledReason = Result.ExplorationResolution.Status.Detail;
			Result.ExplorationResolution.Events.Reset();
			return Result;
		}
	}
	else
	{
		Result.ExplorationResolution.Status = FWacomStatus::Ok();
	}

	if (!FRunShopTransaction::BeginVisit(WorkingState, ShopId, Offers))
	{
		Result.DisabledReason = TEXT("ShopVisitBeginFailed");
		Result.ExplorationResolution.Status = FWacomStatus::Fail(
			EWacomError::InvalidArgument,
			Result.DisabledReason);
		Result.ExplorationResolution.Events.Reset();
		return Result;
	}

	Result.ExplorationResolution.VersionAfter =
		WorkingState.ExplorationState.ExplorationStateVersion;
	Result.ExplorationResolution.PostSnapshot =
		FRunMapModule::BuildSnapshot(WorkingState);
	RunState = MoveTemp(WorkingState);
	ActiveNodeActivityTicket = MoveTemp(WorkingActivity);
	ActiveShopVisitToken = NewVisitToken;
	Result.bSucceeded = true;
	Result.VisitToken = NewVisitToken;
	MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::Shop));
	NotifyRunStateChanged();
	return Result;
}

FRunDeckBatchDeletePreview URunSession::ValidateDeleteCardsForGoldAtomic(
	const FRunDeckBatchDeleteRequest& Request) const
{
	return FRunDeckRules::ValidateDeleteCardsForGoldAtomic(
		RunState,
		BackpackStorageSnapshotRevision,
		Request);
}

FRunDeckBatchOperationResult URunSession::DeleteCardsForGoldAtomic(
	const FRunDeckBatchDeleteRequest& Request)
{
	FRunDeckBatchOperationResult Result;
	Result.StorageRevision = BackpackStorageSnapshotRevision;
	const FRunDeckBatchDeletePreview Preview = ValidateDeleteCardsForGoldAtomic(Request);
	if (!Preview.Validation.bCanExecute)
	{
		Result.DisabledReason = Preview.Validation.DisabledReason;
		return Result;
	}
	FRunState WorkingState = RunState;
	Result = FRunDeckRules::ApplyDeleteCardsForGoldAtomic(
		WorkingState,
		BackpackStorageSnapshotRevision,
		Request);
	if (!Result.bSucceeded)
	{
		Result.AffectedCount = 0;
		Result.GoldReward = 0;
		Result.StorageRevision = BackpackStorageSnapshotRevision;
		return Result;
	}
	WorkingState.Gold += Result.GoldReward;
	RunState = MoveTemp(WorkingState);
	MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(
		ERunUiSnapshotDirtyFlags::BackpackStorage,
		ERunUiSnapshotDirtyFlags::Economy));
	NotifyRunStateChanged();
	Result.StorageRevision = BackpackStorageSnapshotRevision;
	return Result;
}

FRunDeckBatchOperationValidation URunSession::ValidateMoveInstancesAtomic(
	const FRunDeckBatchMoveRequest& Request) const
{
	return FRunDeckRules::ValidateMoveInstancesAtomic(
		RunState,
		BackpackStorageSnapshotRevision,
		Request);
}

FRunDeckBatchOperationResult URunSession::MoveInstancesAtomic(
	const FRunDeckBatchMoveRequest& Request)
{
	FRunDeckBatchOperationResult Result;
	Result.StorageRevision = BackpackStorageSnapshotRevision;
	const FRunDeckBatchOperationValidation Validation = ValidateMoveInstancesAtomic(Request);
	if (!Validation.bCanExecute)
	{
		Result.DisabledReason = Validation.DisabledReason;
		return Result;
	}
	FRunState WorkingState = RunState;
	Result = FRunDeckRules::ApplyMoveInstancesAtomic(
		WorkingState,
		BackpackStorageSnapshotRevision,
		Request);
	if (!Result.bSucceeded)
	{
		Result.AffectedCount = 0;
		Result.StorageRevision = BackpackStorageSnapshotRevision;
		return Result;
	}
	RunState = MoveTemp(WorkingState);
	MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::BackpackStorage));
	NotifyRunStateChanged();
	Result.StorageRevision = BackpackStorageSnapshotRevision;
	return Result;
}

bool URunSession::EndShopVisitIfOwned(FGuid VisitToken)
{
	return EndShopVisitIfOwnedWithResult(VisitToken).bSucceeded;
}

FRunShopVisitResult URunSession::EndShopVisitIfOwnedWithResult(
	const FGuid VisitToken)
{
	if (!VisitToken.IsValid() || VisitToken != ActiveShopVisitToken)
	{
		FRunShopVisitResult Result;
		Result.DisabledReason = TEXT("ShopVisitTokenMismatch");
		Result.ExplorationResolution.Status = FWacomStatus::Fail(
			EWacomError::InvalidState,
			Result.DisabledReason);
		Result.ExplorationResolution.VersionBefore =
			RunState.ExplorationState.ExplorationStateVersion;
		Result.ExplorationResolution.VersionAfter =
			Result.ExplorationResolution.VersionBefore;
		Result.ExplorationResolution.PostSnapshot = BuildExplorationSnapshot();
		return Result;
	}

	return EndShopVisitWithResult();
}

void URunSession::EndShopVisit()
{
	EndShopVisitWithResult();
}

FRunShopVisitResult URunSession::EndShopVisitWithResult()
{
	FRunShopVisitResult Result;
	Result.ExplorationResolution.VersionBefore =
		RunState.ExplorationState.ExplorationStateVersion;
	Result.ExplorationResolution.VersionAfter =
		Result.ExplorationResolution.VersionBefore;
	Result.ExplorationResolution.PostSnapshot = BuildExplorationSnapshot();
	if (!IsShopVisitActive())
	{
		ActiveShopVisitToken.Invalidate();
		Result.DisabledReason = TEXT("ShopVisitNotActive");
		Result.ExplorationResolution.Status = FWacomStatus::Fail(
			EWacomError::InvalidState,
			Result.DisabledReason);
		return Result;
	}

	FRunState WorkingState = RunState;
	TOptional<FRunNodeActivityTicket> WorkingActivity = ActiveNodeActivityTicket;
	if (!FRunShopTransaction::EndVisit(WorkingState))
	{
		Result.DisabledReason = TEXT("ShopVisitEndFailed");
		Result.ExplorationResolution.Status = FWacomStatus::Fail(
			EWacomError::InvalidState,
			Result.DisabledReason);
		return Result;
	}
	Result.ExplorationResolution.Status = FWacomStatus::Ok();
	if (WorkingActivity.IsSet()
		&& WorkingActivity->Kind == ERunNodeActivityKind::Shop)
	{
		Result.ExplorationResolution.Status = FRunNodeActivityModule::Cancel(
			WorkingState,
			WorkingActivity,
			WorkingActivity.GetValue(),
			Result.ExplorationResolution.Events);
		if (!Result.ExplorationResolution.IsOk())
		{
			Result.DisabledReason = Result.ExplorationResolution.Status.Detail;
			Result.ExplorationResolution.Events.Reset();
			return Result;
		}
	}
	Result.ExplorationResolution.VersionAfter =
		WorkingState.ExplorationState.ExplorationStateVersion;
	Result.ExplorationResolution.PostSnapshot =
		FRunMapModule::BuildSnapshot(WorkingState);
	RunState = MoveTemp(WorkingState);
	ActiveNodeActivityTicket = MoveTemp(WorkingActivity);
	ActiveShopVisitToken.Invalidate();
	Result.bSucceeded = true;
	MarkRunUiSnapshotsDirty(MakeRunUiSnapshotDirtyFlags(ERunUiSnapshotDirtyFlags::Shop));
	NotifyRunStateChanged();
	return Result;
}

FRunShopSnapshot URunSession::BuildCurrentShopSnapshot() const
{
	return FRunShopTransaction::BuildSnapshot(RunState);
}

FRunShopPurchaseResult URunSession::PurchaseShopOffer(FGuid OfferId)
{
	FRunShopPurchaseResult Result;
	FRunState WorkingState = RunState;
	TOptional<FRunNodeActivityTicket> WorkingActivity = ActiveNodeActivityTicket;
	const bool bFirstPurchaseThisVisit = !WorkingState.bShopVisitHasPurchase;
	const bool bPurchased = FRunShopTransaction::PurchaseOffer(
		WorkingState,
		OfferId,
		[&WorkingState](UCardDefinition* Card)
		{
			return AcquireCardToWorkingState(WorkingState, Card);
		});
	if (!bPurchased)
	{
		Result.DisabledReason = TEXT("PurchaseRejected");
		return Result;
	}

	const bool bUsesFormalExploration =
		RunState.ExplorationState.JourneyDefinition
		&& RunState.ExplorationState.ExplorationStateVersion > 0;
	if (bUsesFormalExploration)
	{
		Result.ExplorationResolution.VersionBefore =
			RunState.ExplorationState.ExplorationStateVersion;
		if (bFirstPurchaseThisVisit)
		{
			if (!WorkingActivity.IsSet()
				|| WorkingActivity->Kind != ERunNodeActivityKind::Shop
				|| !FRunNodeActivityModule::Matches(
					WorkingState,
					WorkingActivity,
					WorkingActivity.GetValue()))
			{
				Result.DisabledReason = TEXT("ShopActivityTicketMismatch");
				return Result;
			}

			bool bActivityContinues = false;
			FRunNodeActivityTicket UpdatedTicket;
			Result.ExplorationResolution.Status = FRunNodeActivityModule::SpendAndContinue(
				WorkingState,
				WorkingActivity,
				WorkingActivity.GetValue(),
				/*ActionPointCost=*/1,
				/*bResolveNode=*/true,
				bActivityContinues,
				UpdatedTicket,
				Result.ExplorationResolution.Events);
			if (!Result.ExplorationResolution.IsOk())
			{
				Result.DisabledReason = Result.ExplorationResolution.Status.Detail;
				Result.ExplorationResolution.Events.Reset();
				Result.ExplorationResolution.VersionAfter =
					Result.ExplorationResolution.VersionBefore;
				Result.ExplorationResolution.PostSnapshot = BuildExplorationSnapshot();
				return Result;
			}
			Result.ActionPointCost = 1;
			if (!bActivityContinues)
			{
				FRunShopTransaction::EndVisit(WorkingState);
				Result.bVisitClosedAfterPurchase = true;
			}
		}
		else
		{
			Result.ExplorationResolution.Status = FWacomStatus::Ok();
		}
		Result.ExplorationResolution.VersionAfter =
			WorkingState.ExplorationState.ExplorationStateVersion;
		Result.ExplorationResolution.PostSnapshot = FRunMapModule::BuildSnapshot(WorkingState);
	}
	else
	{
		Result.ExplorationResolution.Status = FWacomStatus::Fail(
			EWacomError::InvalidState,
			TEXT("LegacyRunSessionHasNoExplorationResult"));
	}

	RunState = MoveTemp(WorkingState);
	ActiveNodeActivityTicket = MoveTemp(WorkingActivity);
	Result.bSucceeded = true;
	Result.bFirstPurchaseThisVisit = bFirstPurchaseThisVisit;
	if (Result.bVisitClosedAfterPurchase)
	{
		ActiveShopVisitToken.Invalidate();
	}

	MarkRunUiSnapshotsDirty(
		MakeRunUiSnapshotDirtyFlags(
			ERunUiSnapshotDirtyFlags::BackpackStorage,
			ERunUiSnapshotDirtyFlags::Shop,
			ERunUiSnapshotDirtyFlags::Economy));
	NotifyRunStateChanged();
	return Result;
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
	if (IsRunEventActive())
	{
		return false;
	}

	const FGuid NewVisitToken = FGuid::NewGuid();
	if (!NewVisitToken.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[RunSession] BeginRunEvent: 生成访问 token 失败"));
		return false;
	}

	FRunState WorkingState = RunState;
	TOptional<FRunNodeActivityTicket> WorkingActivity = ActiveNodeActivityTicket;
	FRunNodeActivityTicket ActivityTicket;
	TArray<FRunExplorationEvent> ActivityEvents;
	const bool bUsesFormalExploration =
		WorkingState.ExplorationState.JourneyDefinition
		&& WorkingState.ExplorationState.ExplorationStateVersion > 0;
	if (bUsesFormalExploration)
	{
		const FWacomStatus BeginStatus = FRunNodeActivityModule::Begin(
			WorkingState,
			WorkingActivity,
			ERunNodeActivityKind::RunEvent,
			/*ReservedActionPoints=*/0,
			ActivityTicket,
			ActivityEvents);
		if (!BeginStatus.IsOk())
		{
			return false;
		}
	}

	if (FRunEventExecutor::BeginEvent(WorkingState, PersistentId, EventDefinition))
	{
		RunState = MoveTemp(WorkingState);
		ActiveNodeActivityTicket = MoveTemp(WorkingActivity);
		ActiveRunEventVisitToken = NewVisitToken;
		NotifyRunStateChanged();
		return true;
	}
	return false;
}

bool URunSession::EndRunEventIfOwned(FGuid VisitToken)
{
	if (!VisitToken.IsValid() || VisitToken != ActiveRunEventVisitToken)
	{
		return false;
	}

	EndRunEvent();
	return true;
}

void URunSession::EndRunEvent()
{
	if (RunState.ActiveRunEventId.IsNone() && !RunState.ActiveRunEventDefinition)
	{
		ActiveRunEventVisitToken.Invalidate();
		return;
	}

	FRunState WorkingState = RunState;
	TOptional<FRunNodeActivityTicket> WorkingActivity = ActiveNodeActivityTicket;
	WorkingState.ActiveRunEventId = NAME_None;
	WorkingState.ActiveRunEventDefinition = nullptr;
	if (WorkingActivity.IsSet()
		&& WorkingActivity->Kind == ERunNodeActivityKind::RunEvent)
	{
		TArray<FRunExplorationEvent> Events;
		const FWacomStatus CancelStatus = FRunNodeActivityModule::Cancel(
			WorkingState,
			WorkingActivity,
			WorkingActivity.GetValue(),
			Events);
		if (!CancelStatus.IsOk())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RunSession] EndRunEvent: formal activity cancel failed Reason=%s"),
				*CancelStatus.Detail.ToString());
			return;
		}
	}
	RunState = MoveTemp(WorkingState);
	ActiveNodeActivityTicket = MoveTemp(WorkingActivity);
	ActiveRunEventVisitToken.Invalidate();
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
	return ResolveRunEventChoiceInternal(ChoiceId, TOptional<FGuid>());
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
	return ResolveRunEventChoiceInternal(ChoiceId, PaidCardInstanceId);
}

FRunEventChoiceResult URunSession::ResolveRunEventChoiceInternal(
	const FName ChoiceId,
	const TOptional<FGuid> PaidCardInstanceId)
{
	FScopedRunStateNotificationBatch NotificationBatch(*this);
	FRunState WorkingState = RunState;
	TOptional<FRunNodeActivityTicket> WorkingActivity = ActiveNodeActivityTicket;
	FRunEventChoiceResult Result = PaidCardInstanceId.IsSet()
		? FRunEventExecutor::ChooseOptionWithPaidCard(
			WorkingState,
			ChoiceId,
			PaidCardInstanceId.GetValue())
		: FRunEventExecutor::ChooseOption(WorkingState, ChoiceId);
	if (!Result.bSucceeded)
	{
		return Result;
	}

	const bool bUsesFormalExploration =
		RunState.ExplorationState.JourneyDefinition
		&& RunState.ExplorationState.ExplorationStateVersion > 0;
	if (bUsesFormalExploration)
	{
		Result.ExplorationResolution.VersionBefore =
			RunState.ExplorationState.ExplorationStateVersion;
		if (!WorkingActivity.IsSet()
			|| WorkingActivity->Kind != ERunNodeActivityKind::RunEvent
			|| !FRunNodeActivityModule::Matches(
				WorkingState,
				WorkingActivity,
				WorkingActivity.GetValue()))
		{
			Result.bSucceeded = false;
			Result.ActionPointCost = 0;
			Result.EffectResults.Reset();
			Result.DisabledReason = TEXT("RunEventActivityTicketMismatch");
			Result.ExplorationResolution.Status = FWacomStatus::Fail(
				EWacomError::InvalidState,
				Result.DisabledReason);
			Result.ExplorationResolution.VersionAfter =
				Result.ExplorationResolution.VersionBefore;
			Result.ExplorationResolution.PostSnapshot = BuildExplorationSnapshot();
			return Result;
		}

		if (Result.bEventClosedAfterResolve)
		{
			Result.ExplorationResolution.Status = FRunNodeActivityModule::Complete(
				WorkingState,
				WorkingActivity,
				WorkingActivity.GetValue(),
				Result.ActionPointCost,
				/*bResolveNode=*/true,
				Result.ExplorationResolution.Events);
		}
		else if (Result.ActionPointCost > 0)
		{
			Result.ExplorationResolution.Status = FWacomStatus::Fail(
				EWacomError::InvalidState,
				TEXT("PositiveActionPointCostRequiresTerminalChoice"));
		}
		else
		{
			Result.ExplorationResolution.Status = FWacomStatus::Ok();
		}

		if (!Result.ExplorationResolution.IsOk())
		{
			Result.bSucceeded = false;
			Result.ActionPointCost = 0;
			Result.EffectResults.Reset();
			Result.DisabledReason = Result.ExplorationResolution.Status.Detail.IsNone()
				? FName(TEXT("RunEventSettlementFailed"))
				: Result.ExplorationResolution.Status.Detail;
			Result.ExplorationResolution.Events.Reset();
			Result.ExplorationResolution.VersionAfter =
				Result.ExplorationResolution.VersionBefore;
			Result.ExplorationResolution.PostSnapshot = BuildExplorationSnapshot();
			return Result;
		}
		Result.ExplorationResolution.VersionAfter =
			WorkingState.ExplorationState.ExplorationStateVersion;
		Result.ExplorationResolution.PostSnapshot = FRunMapModule::BuildSnapshot(WorkingState);
	}
	else
	{
		Result.ExplorationResolution.Status = FWacomStatus::Fail(
			EWacomError::InvalidState,
			TEXT("LegacyRunSessionHasNoExplorationResult"));
	}

	RunState = MoveTemp(WorkingState);
	ActiveNodeActivityTicket = MoveTemp(WorkingActivity);
	if (Result.bSucceeded)
	{
		if (!IsRunEventActive())
		{
			ActiveRunEventVisitToken.Invalidate();
		}
		MarkRunUiSnapshotsDirty(GetRunEventChoiceResultDirtyFlags(Result));
		NotifyRunStateChanged();
	}
	return Result;
}
