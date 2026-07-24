// Copyright Wacom. All Rights Reserved.

#include "Components/WacomRunFirstPersonCardSourceComponent.h"

#include "Cards/CardDefinition.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "RunStateTypes.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomFirstPersonCardLayerSourceIds.h"
#include "UI/Card/WacomFirstPersonCardPresentationAssetCollector.h"
#include "UI/Card/WacomFirstPersonCardPresentationPrewarmController.h"

namespace
{
	const FName RunFirstPersonCardLayerSuppressedSourceId =
		WacomFirstPersonCardLayerSourceIds::RunMenuSuppressed();

	uint32 BuildPresentationAssetSetHash(
		const FWacomFirstPersonCardPresentationAssetCollection& Collection)
	{
		uint32 Hash = 0;
		for (const FSoftObjectPath& Path : Collection.RequiredVisualAssets)
		{
			Hash = HashCombineFast(Hash, GetTypeHash(Path));
		}
		Hash = HashCombineFast(Hash, 0x9E3779B9u);
		for (const FSoftObjectPath& Path : Collection.OptionalAudioAssets)
		{
			Hash = HashCombineFast(Hash, GetTypeHash(Path));
		}
		return Hash == 0 ? 1u : Hash;
	}

	EWacomFirstPersonCardLayerFrameCommitMode ResolveRunCardLayerFrameCommitMode(
		const FWacomFirstPersonCardLayerPresentationFrame& Frame)
	{
		return Frame.HasPresentationHints()
			? EWacomFirstPersonCardLayerFrameCommitMode::PresentationFrame
			: EWacomFirstPersonCardLayerFrameCommitMode::StateRefresh;
	}

	EWacomFirstPersonCardInteractionIntent ResolveRunFirstPersonCardInteractionIntent(
		ERunCardWorkspaceKind WorkspaceKind)
	{
		switch (WorkspaceKind)
		{
		case ERunCardWorkspaceKind::DefaultExploration:
		case ERunCardWorkspaceKind::OwnedCardsFilter:
		default:
			return EWacomFirstPersonCardInteractionIntent::DragToDropTarget;
		}
	}

	FWacomFirstPersonCardLayerEntry BuildRunFirstPersonCardLayerEntryFromInstance(
		const FCardInstance& Instance,
		EWacomFirstPersonCardInteractionIntent InteractionIntent,
		const bool bAllowLockedFaceInspection)
	{
		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardInstanceId = Instance.InstanceId;
		if (Instance.Definition && Instance.Definition->HasEnabledRunFace())
		{
			FWacomCardPresentationRuntimeContext RunRuntimeContext;
			RunRuntimeContext.bHasPlayableState = true;
			RunRuntimeContext.bIsPlayable = true;
			Entry.CardViewData = UWacomCardPresentationBuilder::BuildCardViewData(
				Instance.Definition,
				EWacomCardFaceContext::Run,
				RunRuntimeContext);
			Entry.DefaultFaceContext = EWacomCardFaceContext::Run;
			Entry.AlternateFaceCardViewData =
				UWacomCardPresentationBuilder::BuildCardViewDataForFace(
					Instance.Definition,
					EWacomCardFaceContext::Battle);
			Entry.bHasAlternateFace = true;
			Entry.bAllowLockedFaceInspection = bAllowLockedFaceInspection;
		}
		else
		{
			Entry.CardViewData =
				UWacomCardPresentationBuilder::BuildCardViewData(Instance.Definition);
			Entry.CardViewData.bShowCost = true;
			Entry.CardViewData.bDisabled = false;
			Entry.DefaultFaceContext = EWacomCardFaceContext::Battle;
		}
		Entry.Zone = EHandZone::None;
		Entry.bIsHandAnchor = false;
		Entry.bIsPlayable = true;
		Entry.bIsPendingTargeting = false;
		Entry.InteractionIntent = InteractionIntent;
		return Entry;
	}

	FWacomFirstPersonCardLayerEntry BuildRunFirstPersonCardLayerEntryFromWorkspaceEntry(
		const FRunCardWorkspaceEntry& WorkspaceEntry,
		EWacomFirstPersonCardInteractionIntent InteractionIntent,
		const bool bAllowLockedFaceInspection)
	{
		return BuildRunFirstPersonCardLayerEntryFromInstance(
			WorkspaceEntry.Instance,
			InteractionIntent,
			bAllowLockedFaceInspection);
	}

	FRunCardWorkspaceRequest BuildDefaultRunCardWorkspaceRequest(
		FName WorkspaceId,
		bool bIncludeProjectedBattleDeckCards)
	{
		FRunCardWorkspaceRequest Request;
		Request.WorkspaceId = WorkspaceId;
		Request.Kind = ERunCardWorkspaceKind::DefaultExploration;
		Request.bIncludeProjectedBattleDeckCards = bIncludeProjectedBattleDeckCards;
		return Request;
	}

	FRunCardWorkspaceRequest BuildRunCardWorkspaceRequestFromMenuLeaseRequest(
		const FWacomRunMenuCardLeaseRequest& Request)
	{
		FRunCardWorkspaceRequest WorkspaceRequest;
		WorkspaceRequest.WorkspaceId = Request.LeaseId;
		WorkspaceRequest.Kind = ERunCardWorkspaceKind::OwnedCardsFilter;
		WorkspaceRequest.AllowedCardDefinitions = Request.AllowedCardDefinitions;
		WorkspaceRequest.AllowedCardIds = Request.AllowedCardIds;
		WorkspaceRequest.ExplicitCardInstanceIds = Request.ExplicitCardInstanceIds;
		WorkspaceRequest.RequiredKeywords = Request.RequiredKeywords;
		WorkspaceRequest.BlockedKeywords = Request.BlockedKeywords;
		WorkspaceRequest.bIncludeBackpack = Request.bIncludeBackpack;
		WorkspaceRequest.bIncludeBattleDeck = Request.bIncludeBattleDeck;
		WorkspaceRequest.bIncludeBurdenZone = Request.bIncludeBurdenZone;
		WorkspaceRequest.bIncludeSpecialZones = Request.bIncludeSpecialZones;
		WorkspaceRequest.bAllowAllOwnedCardsWhenNoFilter =
			Request.bAllowAllOwnedCardsWhenNoFilter;
		return WorkspaceRequest;
	}

	void BuildLayerEntriesFromRunCardWorkspaceSnapshot(
		const FRunCardWorkspaceSnapshot& Snapshot,
		TArray<FWacomFirstPersonCardLayerEntry>& OutEntries)
	{
		OutEntries.Reset();
		OutEntries.Reserve(Snapshot.Entries.Num());
		const EWacomFirstPersonCardInteractionIntent InteractionIntent =
			ResolveRunFirstPersonCardInteractionIntent(Snapshot.Kind);
		const bool bAllowLockedFaceInspection =
			Snapshot.Kind == ERunCardWorkspaceKind::DefaultExploration;
		for (const FRunCardWorkspaceEntry& WorkspaceEntry : Snapshot.Entries)
		{
			if (!WorkspaceEntry.Instance.InstanceId.IsValid()
				|| !WorkspaceEntry.Instance.Definition)
			{
				continue;
			}
			OutEntries.Add(
				BuildRunFirstPersonCardLayerEntryFromWorkspaceEntry(
					WorkspaceEntry,
					InteractionIntent,
					bAllowLockedFaceInspection));
		}
	}

	FString BuildRunMenuCardLeaseProviderDebugSummary(
		const FWacomRunMenuCardLeaseRequest& Request,
		const FWacomRunMenuCardLeaseResult& Result)
	{
		return FString::Printf(
			TEXT("RunMenuCardLeaseProvider{LeaseId=%s SourceId=%s LeaseSet=%s Reject=%s Candidates=%d Considered=%d Filters{Definitions=%d CardIds=%d Instances=%d Required=%s Blocked=%s AllowAllWhenEmpty=%s Include{Backpack=%s BattleDeck=%s Burden=%s Special=%s}}}"),
			*Result.LeaseId.ToString(),
			*Result.SourceId.ToString(),
			Result.bLeaseSet ? TEXT("true") : TEXT("false"),
			*Result.RejectReason.ToString(),
			Result.CandidateCount,
			Result.ConsideredCount,
			Request.AllowedCardDefinitions.Num(),
			Request.AllowedCardIds.Num(),
			Request.ExplicitCardInstanceIds.Num(),
			*Request.RequiredKeywords.ToStringSimple(),
			*Request.BlockedKeywords.ToStringSimple(),
			Request.bAllowAllOwnedCardsWhenNoFilter ? TEXT("true") : TEXT("false"),
			Request.bIncludeBackpack ? TEXT("true") : TEXT("false"),
			Request.bIncludeBattleDeck ? TEXT("true") : TEXT("false"),
			Request.bIncludeBurdenZone ? TEXT("true") : TEXT("false"),
			Request.bIncludeSpecialZones ? TEXT("true") : TEXT("false"));
	}
}

UWacomRunFirstPersonCardSourceComponent::UWacomRunFirstPersonCardSourceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PresentationPrewarm = MakeShared<FWacomFirstPersonCardPresentationPrewarmController>();
	RunFirstPersonCardLayerSourceId = WacomFirstPersonCardLayerSourceIds::RunDefault();
}

UWacomRunFirstPersonCardSourceComponent::~UWacomRunFirstPersonCardSourceComponent() = default;

void UWacomRunFirstPersonCardSourceComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TickPresentationPrewarm(DeltaTime);
}

void UWacomRunFirstPersonCardSourceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearRunFirstPersonCardLayerWithResult(TEXT("EndPlay"), /*bClearMenuContext*/ true);
	UnbindRunSession();
	Super::EndPlay(EndPlayReason);
}

void UWacomRunFirstPersonCardSourceComponent::BindRunSession(URunSession* InRunSession)
{
	if (BoundRunSession == InRunSession)
	{
		if (bRuntimeSourceActive)
		{
			ReconcileRunFirstPersonCardLayer(
				/*bAllowDefaultSourceRevisionSkip*/ false,
				/*bAllowProviderLeaseRevisionSkip*/ false);
		}
		return;
	}

	UnbindRunSession();
	ResetDefaultSourceRefreshKey();
	ResetProviderLeaseRefreshKey();
	BoundRunSession = InRunSession;
	if (BoundRunSession)
	{
		BoundRunSession->OnRunStateChangedNative.AddUObject(
			this,
			&UWacomRunFirstPersonCardSourceComponent::HandleRunStateChanged);
	}

	if (bRuntimeSourceActive)
	{
		ReconcileRunFirstPersonCardLayer(
			/*bAllowDefaultSourceRevisionSkip*/ false,
			/*bAllowProviderLeaseRevisionSkip*/ false);
	}
}

void UWacomRunFirstPersonCardSourceComponent::SetRunFirstPersonCardLayerActive(bool bInActive)
{
	if (bRuntimeSourceActive == bInActive)
	{
		if (bRuntimeSourceActive)
		{
			ReconcileRunFirstPersonCardLayer(
				/*bAllowDefaultSourceRevisionSkip*/ false,
				/*bAllowProviderLeaseRevisionSkip*/ false);
		}
		return;
	}

	bRuntimeSourceActive = bInActive;
	ResetDefaultSourceRefreshKey();
	ResetProviderLeaseRefreshKey();
	if (bRuntimeSourceActive)
	{
		ReconcileRunFirstPersonCardLayer(
			/*bAllowDefaultSourceRevisionSkip*/ false,
			/*bAllowProviderLeaseRevisionSkip*/ false);
	}
	else
	{
		ClearRunFirstPersonCardLayer();
	}
}

bool UWacomRunFirstPersonCardSourceComponent::RefreshRunFirstPersonCardLayer()
{
	return ReconcileRunFirstPersonCardLayer(
		/*bAllowDefaultSourceRevisionSkip*/ false,
		/*bAllowProviderLeaseRevisionSkip*/ false);
}

bool UWacomRunFirstPersonCardSourceComponent::ReconcileRunFirstPersonCardLayer(
	bool bAllowDefaultSourceRevisionSkip,
	bool bAllowProviderLeaseRevisionSkip)
{
	return RefreshRunFirstPersonCardLayerInternal(
		bAllowDefaultSourceRevisionSkip,
		bAllowProviderLeaseRevisionSkip);
}

bool UWacomRunFirstPersonCardSourceComponent::RefreshRunFirstPersonCardLayerInternal(
	bool bAllowDefaultSourceRevisionSkip,
	bool bAllowProviderLeaseRevisionSkip)
{
	if (!bRuntimeSourceActive)
	{
		ResetBattleDeckRefreshDebugCounts();
		ResetDefaultSourceRefreshKey();
		ResetProviderLeaseRefreshKey();
		ClearReconcileBlocks();
		LastRefreshResult = TEXT("Inactive");
		LogDebugState(TEXT("RefreshSkipped"));
		return false;
	}
	if (!bEnableRunFirstPersonCardLayer)
	{
		ResetBattleDeckRefreshDebugCounts();
		ResetDefaultSourceRefreshKey();
		ResetProviderLeaseRefreshKey();
		ClearReconcileBlocks();
		LastRefreshResult = TEXT("Disabled");
		ClearRunFirstPersonCardLayerWithResult(LastRefreshResult, /*bClearMenuContext*/ false);
		LogDebugState(TEXT("RefreshSkipped"));
		return false;
	}
	if (!ActiveMenuLeaseId.IsNone())
	{
		ResetBattleDeckRefreshDebugCounts();
		ResetDefaultSourceRefreshKey();
		ClearDefaultSourceReconcileBlock();
		return RefreshActiveMenuLease(bAllowProviderLeaseRevisionSkip);
	}
	if (bSuppressedByGameMenu)
	{
		ResetBattleDeckRefreshDebugCounts();
		ResetDefaultSourceRefreshKey();
		ResetProviderLeaseRefreshKey();
		ClearReconcileBlocks();
		return WriteSuppressedRuntimeCardLayerWithResult(TEXT("SuppressedByGameMenu"));
	}

	return RefreshDefaultBattleDeckSource(bAllowDefaultSourceRevisionSkip);
}

bool UWacomRunFirstPersonCardSourceComponent::RefreshDefaultBattleDeckSource(
	bool bAllowRevisionSkip)
{
	if (!BoundRunSession)
	{
		ResetBattleDeckRefreshDebugCounts();
		ResetDefaultSourceRefreshKey();
		ClearRunFirstPersonCardLayerWithResult(TEXT("MissingRunSession"), /*bClearMenuContext*/ false);
		MarkDefaultSourceReconcileBlocked(TEXT("MissingRunSession"));
		LastRefreshResult = TEXT("MissingRunSession");
		LogDebugState(TEXT("RefreshFailed"));
		return false;
	}

	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchor();
	bLastHadAnchor = Anchor != nullptr;
	if (!Anchor)
	{
		ResetPresentationPrewarm();
		ResetBattleDeckRefreshDebugCounts();
		ResetDefaultSourceRefreshKey();
		RestoreMenuLeaseInteractionOverrides();
		MarkDefaultSourceReconcileBlocked(TEXT("MissingAnchor"));
		LastRefreshResult = TEXT("MissingAnchor");
		LogDebugState(TEXT("RefreshFailed"));
		return false;
	}

	if (bAllowRevisionSkip && CanSkipDefaultSourceRefresh(*Anchor))
	{
		ClearDefaultSourceReconcileBlock();
		LastRefreshResult = TEXT("SkippedUnchangedRevision");
#if WITH_AUTOMATION_TESTS
		++DefaultSourceRefreshCountersForTest.RevisionSkipCount;
#endif
		LogDebugState(TEXT("RefreshSkipped"));
		return true;
	}

	TArray<FWacomFirstPersonCardLayerEntry> Entries;
#if WITH_AUTOMATION_TESTS
	++DefaultSourceRefreshCountersForTest.DataBuildCount;
#endif
	const FRunCardWorkspaceRequest WorkspaceRequest =
		BuildDefaultRunCardWorkspaceRequest(
			RunFirstPersonCardLayerSourceId,
			bIncludeProjectedRunBattleDeckCards);
	const FRunCardWorkspaceSnapshot WorkspaceSnapshot =
		BoundRunSession->BuildRunCardWorkspaceSnapshot(WorkspaceRequest);
	LastBattleDeckPhysicalCount = WorkspaceSnapshot.PhysicalBattleDeckCount;
	LastBattleDeckProjectedCount = WorkspaceSnapshot.ProjectedBattleDeckCount;
	BuildLayerEntriesFromRunCardWorkspaceSnapshot(WorkspaceSnapshot, Entries);
	LastEntryCount = Entries.Num();

	FWacomFirstPersonCardLayerPresentationFrame Frame =
		BuildDefaultSourcePresentationFrame(*Anchor, MoveTemp(Entries));
	StageRuntimeCardLayerFrame(
		*Anchor,
		Frame);
	StoreRunCardWorkspaceMetadata(WorkspaceSnapshot);
	StoreDefaultSourceRefreshKey();
	ClearDefaultSourceReconcileBlock();
	LastRefreshResult = TEXT("Refreshed");
	LogDebugState(TEXT("Refreshed"));
	return true;
}

bool UWacomRunFirstPersonCardSourceComponent::TryBuildCurrentDefaultSourceRefreshKey(
	FDefaultSourceRefreshKey& OutKey) const
{
	if (!BoundRunSession)
	{
		return false;
	}

	OutKey.bIsValid = true;
	OutKey.BackpackStorageRevision =
		BoundRunSession->GetBackpackStorageSnapshotRevision();
	OutKey.SourceId = RunFirstPersonCardLayerSourceId;
	OutKey.bIncludeProjectedCards = bIncludeProjectedRunBattleDeckCards;
	return true;
}

bool UWacomRunFirstPersonCardSourceComponent::CanSkipDefaultSourceRefresh(
	const UWacomFirstPersonCardAnchorComponent& Anchor) const
{
	if (!Anchor.HasRuntimeCardLayerData()
		|| Anchor.GetRuntimeCardLayerSourceId() != RunFirstPersonCardLayerSourceId)
	{
		return false;
	}

	FDefaultSourceRefreshKey CurrentKey;
	return TryBuildCurrentDefaultSourceRefreshKey(CurrentKey)
		&& AreDefaultSourceRefreshKeysEquivalent(
			LastDefaultSourceRefreshKey,
			CurrentKey);
}

void UWacomRunFirstPersonCardSourceComponent::StoreDefaultSourceRefreshKey()
{
	FDefaultSourceRefreshKey CurrentKey;
	if (!TryBuildCurrentDefaultSourceRefreshKey(CurrentKey))
	{
		ResetDefaultSourceRefreshKey();
		return;
	}

	LastDefaultSourceRefreshKey = MoveTemp(CurrentKey);
}

void UWacomRunFirstPersonCardSourceComponent::ResetDefaultSourceRefreshKey()
{
	LastDefaultSourceRefreshKey.Reset();
}

bool UWacomRunFirstPersonCardSourceComponent::TryBuildCurrentProviderLeaseRefreshKey(
	FProviderLeaseRefreshKey& OutKey) const
{
	if (!BoundRunSession
		|| ActiveMenuLeaseId.IsNone()
		|| ActiveMenuLeaseSourceId.IsNone())
	{
		return false;
	}

	OutKey.bIsValid = true;
	OutKey.BackpackStorageRevision =
		BoundRunSession->GetBackpackStorageSnapshotRevision();
	OutKey.LeaseId = ActiveMenuLeaseId;
	OutKey.SourceId = ActiveMenuLeaseSourceId;
	OutKey.ProviderRequest = ActiveMenuLeaseProviderRequest;
	return true;
}

bool UWacomRunFirstPersonCardSourceComponent::CanSkipProviderLeaseRefresh(
	const UWacomFirstPersonCardAnchorComponent& Anchor) const
{
	if (!Anchor.HasRuntimeCardLayerData()
		|| Anchor.GetRuntimeCardLayerSourceId() != ActiveMenuLeaseSourceId)
	{
		return false;
	}

	FProviderLeaseRefreshKey CurrentKey;
	return TryBuildCurrentProviderLeaseRefreshKey(CurrentKey)
		&& AreProviderLeaseRefreshKeysEquivalent(
			LastProviderLeaseRefreshKey,
			CurrentKey);
}

void UWacomRunFirstPersonCardSourceComponent::StoreProviderLeaseRefreshKey()
{
	FProviderLeaseRefreshKey CurrentKey;
	if (!TryBuildCurrentProviderLeaseRefreshKey(CurrentKey))
	{
		ResetProviderLeaseRefreshKey();
		return;
	}

	LastProviderLeaseRefreshKey = MoveTemp(CurrentKey);
}

void UWacomRunFirstPersonCardSourceComponent::ResetProviderLeaseRefreshKey()
{
	LastProviderLeaseRefreshKey.Reset();
}

bool UWacomRunFirstPersonCardSourceComponent::AreDefaultSourceRefreshKeysEquivalent(
	const FDefaultSourceRefreshKey& Left,
	const FDefaultSourceRefreshKey& Right) const
{
	return Left.bIsValid
		&& Right.bIsValid
		&& Left.BackpackStorageRevision == Right.BackpackStorageRevision
		&& Left.SourceId == Right.SourceId
		&& Left.bIncludeProjectedCards == Right.bIncludeProjectedCards;
}

bool UWacomRunFirstPersonCardSourceComponent::AreProviderLeaseRefreshKeysEquivalent(
	const FProviderLeaseRefreshKey& Left,
	const FProviderLeaseRefreshKey& Right) const
{
	return Left.bIsValid
		&& Right.bIsValid
		&& Left.BackpackStorageRevision == Right.BackpackStorageRevision
		&& Left.LeaseId == Right.LeaseId
		&& Left.SourceId == Right.SourceId
		&& AreRunMenuCardLeaseRequestsEquivalent(
			Left.ProviderRequest,
			Right.ProviderRequest);
}

bool UWacomRunFirstPersonCardSourceComponent::AreRunMenuCardLeaseRequestsEquivalent(
	const FWacomRunMenuCardLeaseRequest& Left,
	const FWacomRunMenuCardLeaseRequest& Right) const
{
	return Left.LeaseId == Right.LeaseId
		&& Left.SourceId == Right.SourceId
		&& Left.AllowedCardDefinitions == Right.AllowedCardDefinitions
		&& Left.AllowedCardIds == Right.AllowedCardIds
		&& Left.ExplicitCardInstanceIds == Right.ExplicitCardInstanceIds
		&& Left.RequiredKeywords == Right.RequiredKeywords
		&& Left.BlockedKeywords == Right.BlockedKeywords
		&& Left.bIncludeBackpack == Right.bIncludeBackpack
		&& Left.bIncludeBattleDeck == Right.bIncludeBattleDeck
		&& Left.bIncludeBurdenZone == Right.bIncludeBurdenZone
		&& Left.bIncludeSpecialZones == Right.bIncludeSpecialZones
		&& Left.bAllowAllOwnedCardsWhenNoFilter
			== Right.bAllowAllOwnedCardsWhenNoFilter;
}

void UWacomRunFirstPersonCardSourceComponent::ResetBattleDeckRefreshDebugCounts()
{
	LastBattleDeckPhysicalCount = 0;
	LastBattleDeckProjectedCount = 0;
	LastEntryCount = 0;
	bLastHadAnchor = false;
}

void UWacomRunFirstPersonCardSourceComponent::StoreRunCardWorkspaceMetadata(
	const FRunCardWorkspaceSnapshot& Snapshot)
{
	CurrentWorkspaceEntriesByCardId.Reset();
	CurrentWorkspaceId = Snapshot.WorkspaceId;
	CurrentWorkspaceKind = Snapshot.Kind;
	for (const FRunCardWorkspaceEntry& Entry : Snapshot.Entries)
	{
		if (Entry.Instance.InstanceId.IsValid())
		{
			CurrentWorkspaceEntriesByCardId.Add(Entry.Instance.InstanceId, Entry);
		}
	}
}

void UWacomRunFirstPersonCardSourceComponent::ClearRunCardWorkspaceMetadata()
{
	CurrentWorkspaceEntriesByCardId.Reset();
	CurrentWorkspaceId = NAME_None;
	CurrentWorkspaceKind = ERunCardWorkspaceKind::DefaultExploration;
}

FWacomFirstPersonCardLayerPresentationFrame
UWacomRunFirstPersonCardSourceComponent::BuildDefaultSourcePresentationFrame(
	const UWacomFirstPersonCardAnchorComponent& Anchor,
	TArray<FWacomFirstPersonCardLayerEntry>&& Entries) const
{
	return BuildRunHandEnteredPresentationFrame(
		Anchor,
		RunFirstPersonCardLayerSourceId,
		MoveTemp(Entries));
}

FWacomFirstPersonCardLayerPresentationFrame
UWacomRunFirstPersonCardSourceComponent::BuildRunHandEnteredPresentationFrame(
	const UWacomFirstPersonCardAnchorComponent& Anchor,
	FName SourceId,
	TArray<FWacomFirstPersonCardLayerEntry>&& Entries) const
{
	FWacomFirstPersonCardLayerPresentationFrame Frame;
	Frame.SourceId = SourceId;
	Frame.Entries = MoveTemp(Entries);
	const TSet<FGuid> RunHandEnteredCardIds =
		DetermineRunHandEnteredCardIds(Anchor, SourceId, Frame.Entries);
	Frame.TransitionHints =
		BuildRunHandEnteredTransitionHints(Frame.Entries, RunHandEnteredCardIds);
	Frame.CommitMode = ResolveRunCardLayerFrameCommitMode(Frame);
	return Frame;
}

FWacomFirstPersonCardLayerPresentationFrame
UWacomRunFirstPersonCardSourceComponent::BuildSuppressedPresentationFrame() const
{
	FWacomFirstPersonCardLayerPresentationFrame Frame;
	Frame.SourceId = RunFirstPersonCardLayerSuppressedSourceId;
	Frame.CommitMode = EWacomFirstPersonCardLayerFrameCommitMode::Suppressed;
	return Frame;
}

TSet<FGuid> UWacomRunFirstPersonCardSourceComponent::DetermineRunHandEnteredCardIds(
	const UWacomFirstPersonCardAnchorComponent& Anchor,
	FName SourceId,
	const TArray<FWacomFirstPersonCardLayerEntry>& Entries) const
{
	TSet<FGuid> CardIdsToAnimate;
	if (Entries.IsEmpty())
	{
		return CardIdsToAnimate;
	}

	const bool bAnimateAll =
		!Anchor.HasRuntimeCardLayerData()
		|| Anchor.GetRuntimeCardLayerSourceId() != SourceId
		|| Anchor.GetRuntimeCardLayerCardCount() == 0;
	if (bAnimateAll)
	{
		for (const FWacomFirstPersonCardLayerEntry& Entry : Entries)
		{
			if (Entry.CardInstanceId.IsValid())
			{
				CardIdsToAnimate.Add(Entry.CardInstanceId);
			}
		}
		return CardIdsToAnimate;
	}

	TSet<FGuid> ExistingCardIds;
	for (const FWacomFirstPersonCardLayerEntry& ExistingEntry : Anchor.GetRuntimeCardLayerEntries())
	{
		if (ExistingEntry.CardInstanceId.IsValid())
		{
			ExistingCardIds.Add(ExistingEntry.CardInstanceId);
		}
	}

	for (const FWacomFirstPersonCardLayerEntry& Entry : Entries)
	{
		if (Entry.CardInstanceId.IsValid() && !ExistingCardIds.Contains(Entry.CardInstanceId))
		{
			CardIdsToAnimate.Add(Entry.CardInstanceId);
		}
	}
	return CardIdsToAnimate;
}

TArray<FWacomFirstPersonCardLayerTransitionHint>
UWacomRunFirstPersonCardSourceComponent::BuildRunHandEnteredTransitionHints(
	const TArray<FWacomFirstPersonCardLayerEntry>& Entries,
	const TSet<FGuid>& CardIdsToAnimate) const
{
	TArray<FGuid> OrderedAnimatedCardIds;
	OrderedAnimatedCardIds.Reserve(CardIdsToAnimate.Num());
	TSet<FGuid> SeenCardIds;
	for (const FWacomFirstPersonCardLayerEntry& Entry : Entries)
	{
		if (!Entry.CardInstanceId.IsValid()
			|| !CardIdsToAnimate.Contains(Entry.CardInstanceId)
			|| SeenCardIds.Contains(Entry.CardInstanceId))
		{
			continue;
		}
		SeenCardIds.Add(Entry.CardInstanceId);
		OrderedAnimatedCardIds.Add(Entry.CardInstanceId);
	}

	TArray<FWacomFirstPersonCardLayerTransitionHint> Hints;
	Hints.Reserve(OrderedAnimatedCardIds.Num());
	const int32 SequenceCount = OrderedAnimatedCardIds.Num();
	for (int32 Index = 0; Index < OrderedAnimatedCardIds.Num(); ++Index)
	{
		FWacomFirstPersonCardLayerTransitionHint Hint;
		Hint.CardInstanceId = OrderedAnimatedCardIds[Index];
		Hint.TransitionKind = EWacomFirstPersonCardSlotTransitionKind::RunHandEntered;
		Hint.SequenceIndex = Index;
		Hint.SequenceCount = SequenceCount;
		Hints.Add(Hint);
	}
	return Hints;
}

void UWacomRunFirstPersonCardSourceComponent::ClearRunFirstPersonCardLayer()
{
	ClearRunFirstPersonCardLayerWithResult(TEXT("Cleared"), /*bClearMenuContext*/ true);
}

void UWacomRunFirstPersonCardSourceComponent::ResetRunFirstPersonCardLayerMenuContext()
{
	// A menu-source revision may still be waiting on presentation assets. Cancel
	// that generation before selecting the replacement source so a late async
	// callback cannot restore the stale lease frame over the default source.
	ResetPresentationPrewarm();
	bSuppressedByGameMenu = false;
	ActiveMenuLeaseId = NAME_None;
	ActiveMenuLeaseSourceId = NAME_None;
	ActiveMenuLeaseEntries.Reset();
	ActiveMenuLeaseProviderRequest = FWacomRunMenuCardLeaseRequest();
	ResetDefaultSourceRefreshKey();
	ResetProviderLeaseRefreshKey();
	ClearReconcileBlocks();

	if (bRuntimeSourceActive)
	{
		ReconcileRunFirstPersonCardLayer(
			/*bAllowDefaultSourceRevisionSkip*/ false,
			/*bAllowProviderLeaseRevisionSkip*/ false);
	}
}

bool UWacomRunFirstPersonCardSourceComponent::RefreshActiveMenuLease(bool bAllowRevisionSkip)
{
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchor();
	bLastHadAnchor = Anchor != nullptr;
	if (!Anchor)
	{
		ResetPresentationPrewarm();
		ResetProviderLeaseRefreshKey();
		RestoreMenuLeaseInteractionOverrides();
		LastEntryCount = 0;
		MarkMenuLeaseReconcileBlocked(TEXT("MissingAnchorForMenuLease"));
		LastRefreshResult = TEXT("MissingAnchorForMenuLease");
		LogDebugState(TEXT("RefreshFailed"));
		return false;
	}

	if (bAllowRevisionSkip && CanSkipProviderLeaseRefresh(*Anchor))
	{
		ClearMenuLeaseReconcileBlock();
		LastRefreshResult = TEXT("MenuLeaseProviderSkippedUnchangedRevision");
#if WITH_AUTOMATION_TESTS
		++ProviderLeaseRefreshCountersForTest.RevisionSkipCount;
#endif
		LogDebugState(TEXT("MenuLeaseProviderRefreshSkipped"));
		return true;
	}

#if WITH_AUTOMATION_TESTS
	++ProviderLeaseRefreshCountersForTest.DataBuildCount;
#endif
	if (!RebuildActiveMenuLeaseFromProviderRequest())
	{
		return false;
	}

	LastEntryCount = ActiveMenuLeaseEntries.Num();
	FWacomFirstPersonCardLayerPresentationFrame Frame =
		BuildRunHandEnteredPresentationFrame(
			*Anchor,
			ActiveMenuLeaseSourceId,
			TArray<FWacomFirstPersonCardLayerEntry>(ActiveMenuLeaseEntries));
	StageRuntimeCardLayerFrame(*Anchor, Frame);
	StoreProviderLeaseRefreshKey();
	ClearMenuLeaseReconcileBlock();
	LastRefreshResult = TEXT("MenuLeaseRefreshed");
	LogDebugState(TEXT("MenuLeaseRefreshed"));
	return true;
}

bool UWacomRunFirstPersonCardSourceComponent::RebuildActiveMenuLeaseFromProviderRequest()
{
	FWacomRunMenuCardLeaseRequest Request = ActiveMenuLeaseProviderRequest;
	if (ActiveMenuLeaseId.IsNone() || ActiveMenuLeaseSourceId.IsNone())
	{
		ResetProviderLeaseRefreshKey();
		return false;
	}
	if (!BoundRunSession)
	{
		ResetProviderLeaseRefreshKey();
		LastEntryCount = 0;
		MarkMenuLeaseReconcileBlocked(TEXT("MissingRunSessionForMenuLease"));
		LastRefreshResult = TEXT("MissingRunSessionForMenuLease");
		LogDebugState(TEXT("MenuLeaseProviderRefreshFailed"));
		return false;
	}

	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	const FRunCardWorkspaceRequest WorkspaceRequest =
		BuildRunCardWorkspaceRequestFromMenuLeaseRequest(Request);
	const FRunCardWorkspaceSnapshot WorkspaceSnapshot =
		BoundRunSession->BuildRunCardWorkspaceSnapshot(WorkspaceRequest);
	BuildLayerEntriesFromRunCardWorkspaceSnapshot(WorkspaceSnapshot, Entries);

	FWacomRunMenuCardLeaseResult Result;
	Result.LeaseId = ActiveMenuLeaseId;
	Result.SourceId = ActiveMenuLeaseSourceId;
	Result.CandidateCount = Entries.Num();
	Result.ConsideredCount = WorkspaceSnapshot.ConsideredCount;
	if (Entries.Num() == 0)
	{
		const FName PreviousLeaseSourceId = ActiveMenuLeaseSourceId;
		ActiveMenuLeaseId = NAME_None;
		ActiveMenuLeaseSourceId = NAME_None;
		ActiveMenuLeaseEntries.Reset();
		ActiveMenuLeaseProviderRequest = FWacomRunMenuCardLeaseRequest();
		ResetProviderLeaseRefreshKey();
		ClearMenuLeaseReconcileBlock();
		Result.bLeaseSet = false;
		Result.RejectReason = WorkspaceSnapshot.RejectReason.IsNone()
			? FName(TEXT("NoMatchingCandidates"))
			: WorkspaceSnapshot.RejectReason;
		Result.DebugSummary = WorkspaceSnapshot.DebugSummary;
		StoreLastMenuLeaseProviderResult(Result);
		ClearRunCardWorkspaceMetadata();
		if (UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchor())
		{
			ClearRuntimeCardLayerEntries(*Anchor, PreviousLeaseSourceId);
			bLastHadAnchor = true;
		}
		LastEntryCount = 0;
		LastRefreshResult = TEXT("NoMatchingCandidates");
		LogDebugState(TEXT("MenuLeaseProviderCleared"));
		if (bSuppressedByGameMenu)
		{
			WriteSuppressedRuntimeCardLayerWithResult(TEXT("NoMatchingCandidates"));
		}
		return false;
	}

	ActiveMenuLeaseEntries = MoveTemp(Entries);
	Result.bLeaseSet = true;
	Result.DebugSummary = WorkspaceSnapshot.DebugSummary;
	StoreRunCardWorkspaceMetadata(WorkspaceSnapshot);
	StoreLastMenuLeaseProviderResult(Result);
	return true;
}

bool UWacomRunFirstPersonCardSourceComponent::WriteSuppressedRuntimeCardLayerWithResult(FName Result)
{
	// Suppression is an immediate ownership boundary. Pending default/lease
	// frames must never become visible after the game menu has taken control.
	ResetPresentationPrewarm();
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchor();
	bLastHadAnchor = Anchor != nullptr;
	if (!Anchor)
	{
		RestoreMenuLeaseInteractionOverrides();
		LastEntryCount = 0;
		LastRefreshResult = Result;
		LogDebugState(TEXT("SuppressedMissingAnchor"));
		ClearRunCardWorkspaceMetadata();
		return false;
	}

	if (Anchor->HasRuntimeCardLayerData()
		&& Anchor->GetRuntimeCardLayerSourceId() == RunFirstPersonCardLayerSuppressedSourceId
		&& Anchor->GetRuntimeCardLayerCardCount() == 0)
	{
		LastEntryCount = 0;
		LastWrittenRuntimeSourceId = RunFirstPersonCardLayerSuppressedSourceId;
		LastRefreshResult = Result;
		LogDebugState(TEXT("SuppressedUnchanged"));
		ClearRunCardWorkspaceMetadata();
		return false;
	}

	FWacomFirstPersonCardLayerPresentationFrame Frame = BuildSuppressedPresentationFrame();
	WriteRuntimeCardLayerFrame(
		*Anchor,
		Frame);
	LastEntryCount = 0;
	LastWrittenRuntimeSourceId = RunFirstPersonCardLayerSuppressedSourceId;
	LastRefreshResult = Result;
	ClearRunCardWorkspaceMetadata();
	LogDebugState(TEXT("Suppressed"));
	return false;
}

bool UWacomRunFirstPersonCardSourceComponent::ClearVisibleRuntimeCardLayerWithResult(FName Result)
{
	ResetDefaultSourceRefreshKey();
	ResetProviderLeaseRefreshKey();
	ResetBattleDeckRefreshDebugCounts();
	if (UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchor())
	{
		ClearKnownRuntimeSources(*Anchor);
		bLastHadAnchor = true;
	}
	else
	{
		bLastHadAnchor = false;
	}
	RestoreMenuLeaseInteractionOverrides();
	ClearRunCardWorkspaceMetadata();
	LastEntryCount = 0;
	LastWrittenRuntimeSourceId = NAME_None;
	LastRefreshResult = Result;
	LogDebugState(TEXT("Cleared"));
	return false;
}

void UWacomRunFirstPersonCardSourceComponent::ClearRunFirstPersonCardLayerWithResult(
	FName Result,
	bool bClearMenuContext)
{
	ResetPresentationPrewarm();
	if (bClearMenuContext)
	{
		bSuppressedByGameMenu = false;
		ActiveMenuLeaseId = NAME_None;
		ActiveMenuLeaseSourceId = NAME_None;
		ActiveMenuLeaseEntries.Reset();
		ActiveMenuLeaseProviderRequest = FWacomRunMenuCardLeaseRequest();
		ClearReconcileBlocks();
	}
	ClearVisibleRuntimeCardLayerWithResult(Result);
}

void UWacomRunFirstPersonCardSourceComponent::SetRunFirstPersonCardLayerSuppressedByGameMenu(bool bSuppressed)
{
	if (bSuppressedByGameMenu == bSuppressed)
	{
		ReconcileRunFirstPersonCardLayer(
			/*bAllowDefaultSourceRevisionSkip*/ false,
			/*bAllowProviderLeaseRevisionSkip*/ false);
		return;
	}

	bSuppressedByGameMenu = bSuppressed;
	ResetDefaultSourceRefreshKey();
	ResetProviderLeaseRefreshKey();
	ReconcileRunFirstPersonCardLayer(
		/*bAllowDefaultSourceRevisionSkip*/ false,
		/*bAllowProviderLeaseRevisionSkip*/ false);
}

void UWacomRunFirstPersonCardSourceComponent::SetRunFirstPersonCardLayerWorldActivitySuppressed(
	bool bSuppressed,
	bool bAnimate)
{
	bWorldActivitySuppressed = bSuppressed;
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchor();
	if (!Anchor || !Anchor->HasRuntimeCardLayerData()
		|| Anchor->GetRuntimeCardLayerSourceId() != RunFirstPersonCardLayerSourceId)
	{
		return;
	}
	FWacomFirstPersonCardLayerSourceLifecycleFrame LifecycleFrame;
	LifecycleFrame.SourceId = RunFirstPersonCardLayerSourceId;
	LifecycleFrame.bSetInteractionEnabled = true;
	LifecycleFrame.bInteractionEnabled = !bSuppressed
		&& bRuntimeSourceActive
		&& bSuppressedByGameMenu == false
		&& ActiveMenuLeaseId.IsNone();
	LifecycleFrame.bCancelActiveDrag = bSuppressed;
	Anchor->ApplyRuntimeCardLayerSourceLifecycleFrame(LifecycleFrame);
	Anchor->SetFirstPersonCardLayerWorldActivitySuppressed(
		bSuppressed,
		bAnimate);
}

bool UWacomRunFirstPersonCardSourceComponent::SetRunFirstPersonCardLayerMenuLeaseFromRunCards(
	const FWacomRunMenuCardLeaseRequest& Request,
	FWacomRunMenuCardLeaseResult& OutResult)
{
	OutResult = FWacomRunMenuCardLeaseResult();
	OutResult.LeaseId = Request.LeaseId;
	OutResult.SourceId = Request.SourceId;

	auto RejectWith = [this, &Request, &OutResult](FName Reason)
	{
		OutResult.bLeaseSet = false;
		OutResult.RejectReason = Reason;
		OutResult.DebugSummary = BuildRunMenuCardLeaseProviderDebugSummary(Request, OutResult);
		StoreLastMenuLeaseProviderResult(OutResult);
		LastRefreshResult = Reason;
		LogDebugState(TEXT("MenuLeaseProviderRejected"));
		return false;
	};

	if (Request.LeaseId.IsNone() || Request.SourceId.IsNone())
	{
		return RejectWith(TEXT("InvalidMenuLease"));
	}
	if (!ActiveMenuLeaseId.IsNone() && ActiveMenuLeaseId != Request.LeaseId)
	{
		return RejectWith(TEXT("MenuLeaseConflict"));
	}
	if (!BoundRunSession)
	{
		return RejectWith(TEXT("MissingRunSession"));
	}
	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	const FRunCardWorkspaceRequest WorkspaceRequest =
		BuildRunCardWorkspaceRequestFromMenuLeaseRequest(Request);
	const FRunCardWorkspaceSnapshot WorkspaceSnapshot =
		BoundRunSession->BuildRunCardWorkspaceSnapshot(WorkspaceRequest);
	BuildLayerEntriesFromRunCardWorkspaceSnapshot(WorkspaceSnapshot, Entries);

	OutResult.ConsideredCount = WorkspaceSnapshot.ConsideredCount;
	OutResult.CandidateCount = Entries.Num();
	if (Entries.Num() == 0)
	{
		if (ActiveMenuLeaseId == Request.LeaseId)
		{
			ClearRunFirstPersonCardLayerMenuLease(Request.LeaseId);
		}
		return RejectWith(
			WorkspaceSnapshot.RejectReason.IsNone()
				? FName(TEXT("NoMatchingCandidates"))
				: WorkspaceSnapshot.RejectReason);
	}

	ActiveMenuLeaseId = Request.LeaseId;
	ActiveMenuLeaseSourceId = Request.SourceId;
	ActiveMenuLeaseEntries = Entries;
	ActiveMenuLeaseProviderRequest = Request;
	ResetDefaultSourceRefreshKey();
	ResetProviderLeaseRefreshKey();
	StoreRunCardWorkspaceMetadata(WorkspaceSnapshot);
	ReconcileRunFirstPersonCardLayer(
		/*bAllowDefaultSourceRevisionSkip*/ false,
		/*bAllowProviderLeaseRevisionSkip*/ false);
	const bool bLeaseSet = true;
	OutResult.bLeaseSet = bLeaseSet;
	OutResult.RejectReason = NAME_None;
	OutResult.DebugSummary = WorkspaceSnapshot.DebugSummary;
	StoreLastMenuLeaseProviderResult(OutResult);
	LogDebugState(TEXT("MenuLeaseProviderSet"));
	return bLeaseSet;
}

bool UWacomRunFirstPersonCardSourceComponent::ClearRunFirstPersonCardLayerMenuLease(FName LeaseId)
{
	if (LeaseId.IsNone() || ActiveMenuLeaseId.IsNone() || ActiveMenuLeaseId != LeaseId)
	{
		LastRefreshResult = TEXT("MenuLeaseClearIgnored");
		LogDebugState(TEXT("MenuLeaseClearIgnored"));
		return false;
	}

	// Invalidate a lease frame that may still be loading before the default
	// source is reconciled. The replacement source starts its own generation.
	ResetPresentationPrewarm();
	ActiveMenuLeaseId = NAME_None;
	ActiveMenuLeaseSourceId = NAME_None;
	ActiveMenuLeaseEntries.Reset();
	ActiveMenuLeaseProviderRequest = FWacomRunMenuCardLeaseRequest();
	ClearRunCardWorkspaceMetadata();
	ResetDefaultSourceRefreshKey();
	ResetProviderLeaseRefreshKey();

	ReconcileRunFirstPersonCardLayer(
		/*bAllowDefaultSourceRevisionSkip*/ false,
		/*bAllowProviderLeaseRevisionSkip*/ false);
	return true;
}

FWacomRunFirstPersonCardSourceDebugView
UWacomRunFirstPersonCardSourceComponent::GetRunFirstPersonCardSourceDebugView() const
{
	FWacomRunFirstPersonCardSourceDebugView View;
	View.SourceId = RunFirstPersonCardLayerSourceId;
	View.bEnabled = bEnableRunFirstPersonCardLayer;
	View.bActive = bRuntimeSourceActive;
	View.bSuppressedByGameMenu = bSuppressedByGameMenu;
	View.bWorldActivitySuppressed = bWorldActivitySuppressed;
	View.bHasActiveMenuLease = !ActiveMenuLeaseId.IsNone();
	View.ActiveMenuLeaseId = ActiveMenuLeaseId;
	View.ActiveMenuLeaseSourceId = ActiveMenuLeaseSourceId;
	View.ActiveMenuLeaseEntryCount = ActiveMenuLeaseEntries.Num();
	View.bHasRunSession = BoundRunSession != nullptr;
	View.bHasAnchor = ResolveFirstPersonCardAnchor() != nullptr;
	View.BattleDeckPhysicalCount = LastBattleDeckPhysicalCount;
	View.BattleDeckProjectedCount = LastBattleDeckProjectedCount;
	View.EntryCount = LastEntryCount;
	View.LastRefreshResult = LastRefreshResult;
	View.LastMenuLeaseProviderLeaseId = LastMenuLeaseProviderLeaseId;
	View.LastMenuLeaseProviderSourceId = LastMenuLeaseProviderSourceId;
	View.LastMenuLeaseProviderResult = LastMenuLeaseProviderResult;
	View.LastMenuLeaseProviderCandidateCount = LastMenuLeaseProviderCandidateCount;
	View.LastMenuLeaseProviderConsideredCount = LastMenuLeaseProviderConsideredCount;
	View.LastMenuLeaseProviderDebugSummary = LastMenuLeaseProviderDebugSummary;
	View.bHasPendingDefaultSourceReconcile = bHasPendingDefaultSourceReconcile;
	View.PendingDefaultSourceBlockReason = PendingDefaultSourceBlockReason;
	View.bHasPendingMenuLeaseReconcile = bHasPendingMenuLeaseReconcile;
	View.PendingMenuLeaseBlockReason = PendingMenuLeaseBlockReason;
	if (PresentationPrewarm)
	{
		const FWacomFirstPersonCardPresentationPrewarmDebugView PrewarmView =
			PresentationPrewarm->BuildDebugView();
		View.PresentationPrewarmState = static_cast<int32>(PrewarmView.State);
		View.PresentationPrewarmGeneration = static_cast<int32>(PrewarmView.Generation);
		View.PresentationRequiredAssetCount = PrewarmView.RequiredAssetCount;
		View.PresentationOptionalAssetCount = PrewarmView.OptionalAssetCount;
		View.PresentationPrewarmElapsedSeconds = PrewarmView.ElapsedSeconds;
	}
	View.bPresentationSourceFrozenForPrewarm = PendingPresentationFrame.bValid;
	return View;
}

FString UWacomRunFirstPersonCardSourceComponent::GetRunFirstPersonCardSourceDebugSummary() const
{
	const FWacomRunFirstPersonCardSourceDebugView View = GetRunFirstPersonCardSourceDebugView();
	return FString::Printf(
		TEXT("RunFirstPersonCardSource{Owner=%s SourceId=%s Enabled=%s Active=%s SuppressedByGameMenu=%s HasLease=%s LeaseId=%s LeaseSource=%s LeaseEntries=%d HasRun=%s HasAnchor=%s Physical=%d Projected=%d Entries=%d Last=%s}"),
		*GetNameSafe(GetOwner()),
		*View.SourceId.ToString(),
		View.bEnabled ? TEXT("true") : TEXT("false"),
		View.bActive ? TEXT("true") : TEXT("false"),
		View.bSuppressedByGameMenu ? TEXT("true") : TEXT("false"),
		View.bHasActiveMenuLease ? TEXT("true") : TEXT("false"),
		*View.ActiveMenuLeaseId.ToString(),
		*View.ActiveMenuLeaseSourceId.ToString(),
		View.ActiveMenuLeaseEntryCount,
		View.bHasRunSession ? TEXT("true") : TEXT("false"),
		View.bHasAnchor ? TEXT("true") : TEXT("false"),
		View.BattleDeckPhysicalCount,
		View.BattleDeckProjectedCount,
		View.EntryCount,
		*View.LastRefreshResult.ToString())
		+ FString::Printf(
			TEXT(" Pending{Default=%s DefaultReason=%s MenuLease=%s MenuLeaseReason=%s}"),
			View.bHasPendingDefaultSourceReconcile ? TEXT("true") : TEXT("false"),
			*View.PendingDefaultSourceBlockReason.ToString(),
			View.bHasPendingMenuLeaseReconcile ? TEXT("true") : TEXT("false"),
			*View.PendingMenuLeaseBlockReason.ToString())
		+ FString::Printf(
			TEXT(" Provider{LeaseId=%s SourceId=%s Result=%s Candidates=%d Considered=%d Debug=%s}"),
			*View.LastMenuLeaseProviderLeaseId.ToString(),
			*View.LastMenuLeaseProviderSourceId.ToString(),
			*View.LastMenuLeaseProviderResult.ToString(),
			View.LastMenuLeaseProviderCandidateCount,
			View.LastMenuLeaseProviderConsideredCount,
			*View.LastMenuLeaseProviderDebugSummary)
		+ FString::Printf(
			TEXT(" Prewarm{State=%d Generation=%d Frozen=%s Required=%d Optional=%d Elapsed=%.3f}"),
			View.PresentationPrewarmState,
			View.PresentationPrewarmGeneration,
			View.bPresentationSourceFrozenForPrewarm ? TEXT("true") : TEXT("false"),
			View.PresentationRequiredAssetCount,
			View.PresentationOptionalAssetCount,
			View.PresentationPrewarmElapsedSeconds);
}

void UWacomRunFirstPersonCardSourceComponent::LogRunFirstPersonCardSourceDebugSummary() const
{
	UE_LOG(LogTemp, Display,
		TEXT("[WacomRunFirstPersonCardSource] %s"),
		*GetRunFirstPersonCardSourceDebugSummary());
}

bool UWacomRunFirstPersonCardSourceComponent::IsDefaultRunFirstPersonCardLayerSource(
	FName SourceId) const
{
	return !SourceId.IsNone()
		&& SourceId == RunFirstPersonCardLayerSourceId;
}

bool UWacomRunFirstPersonCardSourceComponent::IsActiveMenuLeaseSource(
	FName SourceId) const
{
	return !SourceId.IsNone()
		&& !ActiveMenuLeaseSourceId.IsNone()
		&& SourceId == ActiveMenuLeaseSourceId;
}

bool UWacomRunFirstPersonCardSourceComponent::IsSuppressedRunFirstPersonCardLayerSource(
	FName SourceId) const
{
	return !SourceId.IsNone()
		&& SourceId == RunFirstPersonCardLayerSuppressedSourceId;
}

bool UWacomRunFirstPersonCardSourceComponent::CanHandleRunFirstPersonCardLayerSource(
	FName SourceId) const
{
	if (SourceId.IsNone()
		|| SourceId == WacomFirstPersonCardLayerSourceIds::BattleHand()
		|| IsSuppressedRunFirstPersonCardLayerSource(SourceId))
	{
		return false;
	}

	if (IsDefaultRunFirstPersonCardLayerSource(SourceId))
	{
		return !HasActiveMenuLease();
	}

	return HasActiveMenuLease()
		&& IsActiveMenuLeaseSource(SourceId);
}

bool UWacomRunFirstPersonCardSourceComponent::FindCurrentRunFirstPersonCardWorkspaceEntry(
	FGuid CardInstanceId,
	FRunCardWorkspaceEntry& OutEntry) const
{
	if (!CardInstanceId.IsValid())
	{
		return false;
	}

	if (const FRunCardWorkspaceEntry* Found =
		CurrentWorkspaceEntriesByCardId.Find(CardInstanceId))
	{
		OutEntry = *Found;
		return true;
	}

	return false;
}

bool UWacomRunFirstPersonCardSourceComponent::BuildRunFirstPersonCardEntries(
	const URunSession& Run,
	TArray<FWacomFirstPersonCardLayerEntry>& OutEntries) const
{
	OutEntries.Reset();
	LastBattleDeckPhysicalCount = 0;
	LastBattleDeckProjectedCount = 0;

	const FRunCardWorkspaceRequest WorkspaceRequest =
		BuildDefaultRunCardWorkspaceRequest(
			RunFirstPersonCardLayerSourceId,
			bIncludeProjectedRunBattleDeckCards);
	const FRunCardWorkspaceSnapshot WorkspaceSnapshot =
		Run.BuildRunCardWorkspaceSnapshot(WorkspaceRequest);
	LastBattleDeckPhysicalCount = WorkspaceSnapshot.PhysicalBattleDeckCount;
	LastBattleDeckProjectedCount = WorkspaceSnapshot.ProjectedBattleDeckCount;
	BuildLayerEntriesFromRunCardWorkspaceSnapshot(WorkspaceSnapshot, OutEntries);

	LastEntryCount = OutEntries.Num();
	return OutEntries.Num() > 0;
}

#if WITH_AUTOMATION_TESTS
void UWacomRunFirstPersonCardSourceComponent::
ResetRunFirstPersonCardSourcePerfCountersForTest()
{
	DefaultSourceRefreshCountersForTest.Reset();
	ProviderLeaseRefreshCountersForTest.Reset();
}

void UWacomRunFirstPersonCardSourceComponent::SetActiveProviderLeaseRequestForTest(
	const FWacomRunMenuCardLeaseRequest& Request)
{
	ActiveMenuLeaseProviderRequest = Request;
}
#endif

UWacomFirstPersonCardAnchorComponent*
UWacomRunFirstPersonCardSourceComponent::ResolveFirstPersonCardAnchor() const
{
	const AActor* Owner = GetOwner();
	const AWacomPlayerController* PC = Cast<AWacomPlayerController>(Owner);
	const APawn* Pawn = PC ? PC->GetPawn() : Cast<APawn>(Owner);
	const AWacomPlayerCharacter* Character = Cast<AWacomPlayerCharacter>(Pawn);
	return Character ? Character->GetFirstPersonCardAnchorComponent() : nullptr;
}

void UWacomRunFirstPersonCardSourceComponent::WriteRuntimeCardLayerFrame(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	const FWacomFirstPersonCardLayerPresentationFrame& Frame)
{
	const bool bEnableMenuLeaseDragProbe =
		!ActiveMenuLeaseId.IsNone()
		&& Frame.SourceId == ActiveMenuLeaseSourceId;
	const bool bEnableRunWorldCardDropDrag =
		ActiveMenuLeaseId.IsNone()
		&& Frame.SourceId == RunFirstPersonCardLayerSourceId;
	const bool bEnableRuntimeInteraction =
		(bEnableMenuLeaseDragProbe || bEnableRunWorldCardDropDrag)
		&& !bWorldActivitySuppressed;
	ApplyMenuLeaseInteractionOverrides(Anchor, bEnableRuntimeInteraction);
	FWacomFirstPersonCardLayerSourceLifecycleFrame LifecycleFrame =
		FWacomFirstPersonCardLayerSourceLifecycleFrame::FromPresentationFrame(Frame);
	LifecycleFrame.bSetInteractionEnabled = true;
	LifecycleFrame.bInteractionEnabled = bEnableRuntimeInteraction;
	Anchor.ApplyRuntimeCardLayerSourceLifecycleFrame(LifecycleFrame);
	if (Frame.SourceId == RunFirstPersonCardLayerSourceId)
	{
		Anchor.SetFirstPersonCardLayerWorldActivitySuppressed(
			bWorldActivitySuppressed,
			/*bAnimate*/ true);
	}
	LastWrittenRuntimeSourceId = Frame.SourceId;
#if WITH_AUTOMATION_TESTS
	if (Frame.SourceId == RunFirstPersonCardLayerSourceId)
	{
		++DefaultSourceRefreshCountersForTest.RuntimeApplyCount;
	}
	else if (!ActiveMenuLeaseSourceId.IsNone()
		&& Frame.SourceId == ActiveMenuLeaseSourceId)
	{
		++ProviderLeaseRefreshCountersForTest.RuntimeApplyCount;
	}
#endif
}

bool UWacomRunFirstPersonCardSourceComponent::StageRuntimeCardLayerFrame(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	const FWacomFirstPersonCardLayerPresentationFrame& Frame)
{
	if (!PresentationPrewarm)
	{
		PresentationPrewarm = MakeShared<FWacomFirstPersonCardPresentationPrewarmController>();
	}
	const FWacomFirstPersonCardPresentationAssetCollection Collection =
		FWacomFirstPersonCardPresentationAssetCollector::Collect(Anchor);
	const uint32 AssetSetHash = BuildPresentationAssetSetHash(Collection);

	if (ActivePresentationAssetSetHash == AssetSetHash)
	{
		PresentationPrewarm->Tick(0.0f);
		if (PresentationPrewarm->IsGateResolved())
		{
			WriteRuntimeCardLayerFrame(Anchor, Frame);
			return true;
		}
	}
	else
	{
		FWacomFirstPersonCardPresentationPrewarmRequest Request;
		Request.ScopeId = TEXT("Run");
		Request.RequiredVisualAssets = Collection.RequiredVisualAssets;
		Request.OptionalAudioAssets = Collection.OptionalAudioAssets;
		Request.TimeoutSeconds = 1.5f;
		ActivePresentationAssetSetHash = AssetSetHash;
		PendingPresentationFrame.Generation = PresentationPrewarm->Begin(Request);
	}

	PendingPresentationFrame.Anchor = &Anchor;
	PendingPresentationFrame.Frame = Frame;
	PendingPresentationFrame.Generation = PresentationPrewarm->GetGeneration();
	PendingPresentationFrame.bValid = true;
	if (Anchor.HasRuntimeCardLayerData())
	{
		const FName VisibleSourceId = Anchor.GetRuntimeCardLayerSourceId();
		if (!VisibleSourceId.IsNone())
		{
			// Keep the current visual source in place while its replacement loads,
			// but freeze card interaction so stale menu/default ownership cannot
			// submit actions during the handoff.
			FWacomFirstPersonCardLayerSourceLifecycleFrame FreezeFrame;
			FreezeFrame.SourceId = VisibleSourceId;
			FreezeFrame.bSetInteractionEnabled = true;
			FreezeFrame.bInteractionEnabled = false;
			FreezeFrame.bCancelActiveDrag = true;
			Anchor.ApplyRuntimeCardLayerSourceLifecycleFrame(FreezeFrame);
		}
	}
	SetComponentTickEnabled(true);
	TickPresentationPrewarm(0.0f);
	return true;
}

void UWacomRunFirstPersonCardSourceComponent::TickPresentationPrewarm(float DeltaTime)
{
	if (!PresentationPrewarm)
	{
		SetComponentTickEnabled(false);
		return;
	}
	PresentationPrewarm->Tick(DeltaTime);
	uint32 ResolvedGeneration = 0;
	EWacomFirstPersonCardPresentationPrewarmState ResolvedState =
		EWacomFirstPersonCardPresentationPrewarmState::Inactive;
	if (PresentationPrewarm->ConsumeGateResolution(ResolvedGeneration, ResolvedState)
		&& PendingPresentationFrame.bValid
		&& PendingPresentationFrame.Generation == ResolvedGeneration)
	{
		if (UWacomFirstPersonCardAnchorComponent* Anchor = PendingPresentationFrame.Anchor.Get())
		{
			WriteRuntimeCardLayerFrame(*Anchor, PendingPresentationFrame.Frame);
		}
		PendingPresentationFrame = FPendingPresentationFrame();
	}
	const FWacomFirstPersonCardPresentationPrewarmDebugView View =
		PresentationPrewarm->BuildDebugView();
	SetComponentTickEnabled(
		PendingPresentationFrame.bValid
		|| (View.State == EWacomFirstPersonCardPresentationPrewarmState::TimedOut
			&& !View.bRequiredLoadCompleted));
}

void UWacomRunFirstPersonCardSourceComponent::ResetPresentationPrewarm()
{
	PendingPresentationFrame = FPendingPresentationFrame();
	ActivePresentationAssetSetHash = 0;
	if (PresentationPrewarm)
	{
		PresentationPrewarm->Reset();
	}
	SetComponentTickEnabled(false);
}

void UWacomRunFirstPersonCardSourceComponent::ClearRuntimeCardLayerEntries(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	FName SourceId)
{
	if (Anchor.GetRuntimeCardLayerSourceId() == SourceId)
	{
		FWacomFirstPersonCardLayerSourceLifecycleFrame LifecycleFrame;
		LifecycleFrame.SourceId = SourceId;
		LifecycleFrame.bSetInteractionEnabled = true;
		LifecycleFrame.bInteractionEnabled = false;
		LifecycleFrame.bCancelActiveDrag = true;
		LifecycleFrame.ClearMode =
			EWacomFirstPersonCardLayerSourceClearMode::RuntimeData;
		Anchor.ApplyRuntimeCardLayerSourceLifecycleFrame(LifecycleFrame);
		RestoreMenuLeaseInteractionOverrides();
		return;
	}
	FWacomFirstPersonCardLayerSourceLifecycleFrame LifecycleFrame;
	LifecycleFrame.SourceId = SourceId;
	LifecycleFrame.ClearMode =
		EWacomFirstPersonCardLayerSourceClearMode::RuntimeData;
	Anchor.ApplyRuntimeCardLayerSourceLifecycleFrame(LifecycleFrame);
}

void UWacomRunFirstPersonCardSourceComponent::ApplyMenuLeaseInteractionOverrides(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	bool bMenuLeaseSource)
{
	if (!bMenuLeaseSource)
	{
		RestoreMenuLeaseInteractionOverrides();
		return;
	}
}

void UWacomRunFirstPersonCardSourceComponent::RestoreMenuLeaseInteractionOverrides()
{
}

void UWacomRunFirstPersonCardSourceComponent::ClearKnownRuntimeSources(
	UWacomFirstPersonCardAnchorComponent& Anchor)
{
	ClearRuntimeCardLayerEntries(Anchor, RunFirstPersonCardLayerSourceId);
	if (!ActiveMenuLeaseSourceId.IsNone())
	{
		ClearRuntimeCardLayerEntries(Anchor, ActiveMenuLeaseSourceId);
	}
	if (!LastWrittenRuntimeSourceId.IsNone()
		&& LastWrittenRuntimeSourceId != RunFirstPersonCardLayerSourceId
		&& LastWrittenRuntimeSourceId != ActiveMenuLeaseSourceId)
	{
		ClearRuntimeCardLayerEntries(Anchor, LastWrittenRuntimeSourceId);
	}
}

void UWacomRunFirstPersonCardSourceComponent::UnbindRunSession()
{
	ResetPresentationPrewarm();
	if (BoundRunSession)
	{
		BoundRunSession->OnRunStateChangedNative.RemoveAll(this);
	}
	BoundRunSession = nullptr;
	ResetDefaultSourceRefreshKey();
	ResetProviderLeaseRefreshKey();
}

void UWacomRunFirstPersonCardSourceComponent::HandleRunStateChanged()
{
	if (bRuntimeSourceActive)
	{
		ReconcileRunFirstPersonCardLayer(
			/*bAllowDefaultSourceRevisionSkip*/ true,
			/*bAllowProviderLeaseRevisionSkip*/ true);
	}
}

void UWacomRunFirstPersonCardSourceComponent::MarkDefaultSourceReconcileBlocked(FName Reason)
{
	bHasPendingDefaultSourceReconcile = true;
	PendingDefaultSourceBlockReason = Reason;
}

void UWacomRunFirstPersonCardSourceComponent::ClearDefaultSourceReconcileBlock()
{
	bHasPendingDefaultSourceReconcile = false;
	PendingDefaultSourceBlockReason = NAME_None;
}

void UWacomRunFirstPersonCardSourceComponent::MarkMenuLeaseReconcileBlocked(FName Reason)
{
	bHasPendingMenuLeaseReconcile = true;
	PendingMenuLeaseBlockReason = Reason;
}

void UWacomRunFirstPersonCardSourceComponent::ClearMenuLeaseReconcileBlock()
{
	bHasPendingMenuLeaseReconcile = false;
	PendingMenuLeaseBlockReason = NAME_None;
}

void UWacomRunFirstPersonCardSourceComponent::ClearReconcileBlocks()
{
	ClearDefaultSourceReconcileBlock();
	ClearMenuLeaseReconcileBlock();
}

void UWacomRunFirstPersonCardSourceComponent::LogDebugState(const TCHAR* Prefix) const
{
	if (!bLogRunFirstPersonCardLayer)
	{
		return;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[WacomRunFirstPersonCardSource] %s %s"),
		Prefix,
		*GetRunFirstPersonCardSourceDebugSummary());
}

void UWacomRunFirstPersonCardSourceComponent::StoreLastMenuLeaseProviderResult(
	const FWacomRunMenuCardLeaseResult& Result)
{
	LastMenuLeaseProviderLeaseId = Result.LeaseId;
	LastMenuLeaseProviderSourceId = Result.SourceId;
	LastMenuLeaseProviderResult = Result.bLeaseSet
		? TEXT("LeaseSet")
		: Result.RejectReason;
	LastMenuLeaseProviderCandidateCount = Result.CandidateCount;
	LastMenuLeaseProviderConsideredCount = Result.ConsideredCount;
	LastMenuLeaseProviderDebugSummary = Result.DebugSummary;
}
