// Copyright Wacom. All Rights Reserved.

#include "Components/WacomRunFirstPersonCardSourceComponent.h"

#include "Cards/CardDefinition.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "RunStateTypes.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

namespace
{
	const FName RunFirstPersonCardLayerSuppressedSourceId(TEXT("RunFirstPersonMenuSuppressed"));

	FWacomFirstPersonCardLayerEntry BuildRunFirstPersonCardLayerEntryFromInstance(
		const FCardInstance& Instance)
	{
		FWacomCardViewData CardViewData =
			UWacomCardPresentationBuilder::BuildCardViewData(Instance.Definition);
		CardViewData.bShowCost = true;
		CardViewData.bDisabled = false;

		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardInstanceId = Instance.InstanceId;
		Entry.CardViewData = MoveTemp(CardViewData);
		Entry.Zone = EHandZone::None;
		Entry.bIsHandAnchor = false;
		Entry.bIsPlayable = true;
		Entry.bIsPendingTargeting = false;
		Entry.TargetMode = Instance.Definition
			? Instance.Definition->TargetMode
			: ECardTargetMode::None;
		return Entry;
	}

	bool ContainsDefinition(
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

	bool HasAnyRuntimeFilter(const FWacomRunMenuCardLeaseRequest& Request)
	{
		return Request.AllowedCardDefinitions.Num() > 0
			|| Request.AllowedCardIds.Num() > 0
			|| Request.ExplicitCardInstanceIds.Num() > 0
			|| !Request.RequiredKeywords.IsEmpty()
			|| !Request.BlockedKeywords.IsEmpty();
	}

	bool DoesInstanceMatchRunMenuCardLeaseRequest(
		const FCardInstance& Instance,
		const FWacomRunMenuCardLeaseRequest& Request)
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
				ContainsDefinition(Request.AllowedCardDefinitions, Definition);
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

	void AppendRunMenuLeaseCandidatesFromZone(
		const TArray<FCardInstance>& ZoneCards,
		const FWacomRunMenuCardLeaseRequest& Request,
		TArray<FWacomFirstPersonCardLayerEntry>& OutEntries,
		int32& InOutConsideredCount)
	{
		for (const FCardInstance& Instance : ZoneCards)
		{
			if (!Instance.InstanceId.IsValid() || !Instance.Definition)
			{
				continue;
			}

			++InOutConsideredCount;
			if (DoesInstanceMatchRunMenuCardLeaseRequest(Instance, Request))
			{
				OutEntries.Add(BuildRunFirstPersonCardLayerEntryFromInstance(Instance));
			}
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
	PrimaryComponentTick.bCanEverTick = false;
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
			RefreshRunFirstPersonCardLayer();
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
		RefreshRunFirstPersonCardLayer();
	}
}

void UWacomRunFirstPersonCardSourceComponent::SetRunFirstPersonCardLayerActive(bool bInActive)
{
	if (bRuntimeSourceActive == bInActive)
	{
		if (bRuntimeSourceActive)
		{
			RefreshRunFirstPersonCardLayer();
		}
		return;
	}

	bRuntimeSourceActive = bInActive;
	ResetDefaultSourceRefreshKey();
	ResetProviderLeaseRefreshKey();
	if (bRuntimeSourceActive)
	{
		RefreshRunFirstPersonCardLayer();
	}
	else
	{
		ClearRunFirstPersonCardLayer();
	}
}

bool UWacomRunFirstPersonCardSourceComponent::RefreshRunFirstPersonCardLayer()
{
	return RefreshRunFirstPersonCardLayerInternal(
		/*bAllowDefaultSourceRevisionSkip*/ false,
		/*bAllowProviderLeaseRevisionSkip*/ false);
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
		LastRefreshResult = TEXT("Inactive");
		LogDebugState(TEXT("RefreshSkipped"));
		return false;
	}
	if (!bEnableRunFirstPersonCardLayer)
	{
		ResetBattleDeckRefreshDebugCounts();
		ResetDefaultSourceRefreshKey();
		ResetProviderLeaseRefreshKey();
		LastRefreshResult = TEXT("Disabled");
		ClearRunFirstPersonCardLayerWithResult(LastRefreshResult, /*bClearMenuContext*/ false);
		LogDebugState(TEXT("RefreshSkipped"));
		return false;
	}
	if (!ActiveMenuLeaseId.IsNone())
	{
		ResetBattleDeckRefreshDebugCounts();
		ResetDefaultSourceRefreshKey();
		return RefreshActiveMenuLease(bAllowProviderLeaseRevisionSkip);
	}
	if (bSuppressedByGameMenu)
	{
		ResetBattleDeckRefreshDebugCounts();
		ResetDefaultSourceRefreshKey();
		ResetProviderLeaseRefreshKey();
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
		LastRefreshResult = TEXT("MissingRunSession");
		ClearRunFirstPersonCardLayerWithResult(LastRefreshResult, /*bClearMenuContext*/ false);
		LogDebugState(TEXT("RefreshFailed"));
		return false;
	}

	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchor();
	bLastHadAnchor = Anchor != nullptr;
	if (!Anchor)
	{
		ResetBattleDeckRefreshDebugCounts();
		ResetDefaultSourceRefreshKey();
		RestoreMenuLeaseInteractionOverrides();
		LastRefreshResult = TEXT("MissingAnchor");
		LogDebugState(TEXT("RefreshFailed"));
		return false;
	}

	if (bAllowRevisionSkip && CanSkipDefaultSourceRefresh(*Anchor))
	{
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
	BuildRunFirstPersonCardEntries(*BoundRunSession, Entries);
	LastEntryCount = Entries.Num();

	WriteRuntimeCardLayerEntries(*Anchor, RunFirstPersonCardLayerSourceId, Entries);
	LastWrittenRuntimeSourceId = RunFirstPersonCardLayerSourceId;
	StoreDefaultSourceRefreshKey();
	LastRefreshResult = TEXT("Refreshed");
#if WITH_AUTOMATION_TESTS
	++DefaultSourceRefreshCountersForTest.RuntimeApplyCount;
#endif
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
		|| !bActiveMenuLeaseBackedByProvider
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

void UWacomRunFirstPersonCardSourceComponent::ClearRunFirstPersonCardLayer()
{
	ClearRunFirstPersonCardLayerWithResult(TEXT("Cleared"), /*bClearMenuContext*/ true);
}

bool UWacomRunFirstPersonCardSourceComponent::RefreshActiveMenuLease(bool bAllowRevisionSkip)
{
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchor();
	bLastHadAnchor = Anchor != nullptr;
	if (!Anchor)
	{
		ResetProviderLeaseRefreshKey();
		RestoreMenuLeaseInteractionOverrides();
		LastEntryCount = 0;
		LastRefreshResult = TEXT("MissingAnchorForMenuLease");
		LogDebugState(TEXT("RefreshFailed"));
		return false;
	}

	if (bActiveMenuLeaseBackedByProvider)
	{
		if (bAllowRevisionSkip && CanSkipProviderLeaseRefresh(*Anchor))
		{
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
	}

	LastEntryCount = ActiveMenuLeaseEntries.Num();
	WriteRuntimeCardLayerEntries(*Anchor, ActiveMenuLeaseSourceId, ActiveMenuLeaseEntries);
	LastWrittenRuntimeSourceId = ActiveMenuLeaseSourceId;
	if (bActiveMenuLeaseBackedByProvider)
	{
		StoreProviderLeaseRefreshKey();
#if WITH_AUTOMATION_TESTS
		++ProviderLeaseRefreshCountersForTest.RuntimeApplyCount;
#endif
	}
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
		LastRefreshResult = TEXT("MissingRunSessionForMenuLease");
		LogDebugState(TEXT("MenuLeaseProviderRefreshFailed"));
		return false;
	}

	const FRunState& RunState = BoundRunSession->GetRunState();
	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	int32 ConsideredCount = 0;
	if (Request.bIncludeBackpack)
	{
		AppendRunMenuLeaseCandidatesFromZone(RunState.Backpack, Request, Entries, ConsideredCount);
	}
	if (Request.bIncludeBattleDeck)
	{
		AppendRunMenuLeaseCandidatesFromZone(RunState.BattleDeck, Request, Entries, ConsideredCount);
	}
	if (Request.bIncludeBurdenZone)
	{
		AppendRunMenuLeaseCandidatesFromZone(RunState.BurdenZone, Request, Entries, ConsideredCount);
	}
	if (Request.bIncludeSpecialZones)
	{
		for (const FSpecialZone& SpecialZone : RunState.SpecialZones)
		{
			AppendRunMenuLeaseCandidatesFromZone(
				SpecialZone.Cards,
				Request,
				Entries,
				ConsideredCount);
		}
	}

	FWacomRunMenuCardLeaseResult Result;
	Result.LeaseId = ActiveMenuLeaseId;
	Result.SourceId = ActiveMenuLeaseSourceId;
	Result.CandidateCount = Entries.Num();
	Result.ConsideredCount = ConsideredCount;
	if (Entries.Num() == 0)
	{
		const FName PreviousLeaseSourceId = ActiveMenuLeaseSourceId;
		ActiveMenuLeaseId = NAME_None;
		ActiveMenuLeaseSourceId = NAME_None;
		ActiveMenuLeaseEntries.Reset();
		bActiveMenuLeaseBackedByProvider = false;
		ActiveMenuLeaseProviderRequest = FWacomRunMenuCardLeaseRequest();
		ResetProviderLeaseRefreshKey();
		Result.bLeaseSet = false;
		Result.RejectReason = TEXT("NoMatchingCandidates");
		Result.DebugSummary = BuildRunMenuCardLeaseProviderDebugSummary(Request, Result);
		StoreLastMenuLeaseProviderResult(Result);
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
	Result.DebugSummary = BuildRunMenuCardLeaseProviderDebugSummary(Request, Result);
	StoreLastMenuLeaseProviderResult(Result);
	return true;
}

bool UWacomRunFirstPersonCardSourceComponent::WriteSuppressedRuntimeCardLayerWithResult(FName Result)
{
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchor();
	bLastHadAnchor = Anchor != nullptr;
	if (!Anchor)
	{
		RestoreMenuLeaseInteractionOverrides();
		LastEntryCount = 0;
		LastRefreshResult = Result;
		LogDebugState(TEXT("SuppressedMissingAnchor"));
		return false;
	}

	ClearRuntimeCardLayerEntries(*Anchor, RunFirstPersonCardLayerSourceId);
	if (!ActiveMenuLeaseSourceId.IsNone())
	{
		ClearRuntimeCardLayerEntries(*Anchor, ActiveMenuLeaseSourceId);
	}

	if (Anchor->HasRuntimeCardLayerData()
		&& Anchor->GetRuntimeCardLayerSourceId() == RunFirstPersonCardLayerSuppressedSourceId
		&& Anchor->GetRuntimeCardLayerCardCount() == 0)
	{
		LastEntryCount = 0;
		LastWrittenRuntimeSourceId = RunFirstPersonCardLayerSuppressedSourceId;
		LastRefreshResult = Result;
		LogDebugState(TEXT("SuppressedUnchanged"));
		return false;
	}

	TArray<FWacomFirstPersonCardLayerEntry> EmptyEntries;
	WriteRuntimeCardLayerEntries(*Anchor, RunFirstPersonCardLayerSuppressedSourceId, EmptyEntries);
	LastEntryCount = 0;
	LastWrittenRuntimeSourceId = RunFirstPersonCardLayerSuppressedSourceId;
	LastRefreshResult = Result;
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
	if (bClearMenuContext)
	{
		bSuppressedByGameMenu = false;
		ActiveMenuLeaseId = NAME_None;
		ActiveMenuLeaseSourceId = NAME_None;
		ActiveMenuLeaseEntries.Reset();
		bActiveMenuLeaseBackedByProvider = false;
		ActiveMenuLeaseProviderRequest = FWacomRunMenuCardLeaseRequest();
	}
	ClearVisibleRuntimeCardLayerWithResult(Result);
}

void UWacomRunFirstPersonCardSourceComponent::SetRunFirstPersonCardLayerSuppressedByGameMenu(bool bSuppressed)
{
	if (bSuppressedByGameMenu == bSuppressed)
	{
		RefreshRunFirstPersonCardLayer();
		return;
	}

	bSuppressedByGameMenu = bSuppressed;
	ResetDefaultSourceRefreshKey();
	ResetProviderLeaseRefreshKey();
	RefreshRunFirstPersonCardLayer();
}

bool UWacomRunFirstPersonCardSourceComponent::SetRunFirstPersonCardLayerMenuLease(
	FName LeaseId,
	FName SourceId,
	const TArray<FWacomFirstPersonCardLayerEntry>& Entries)
{
	if (LeaseId.IsNone() || SourceId.IsNone())
	{
		LastRefreshResult = TEXT("InvalidMenuLease");
		LogDebugState(TEXT("MenuLeaseRejected"));
		return false;
	}
	if (!ActiveMenuLeaseId.IsNone() && ActiveMenuLeaseId != LeaseId)
	{
		LastRefreshResult = TEXT("MenuLeaseConflict");
		LogDebugState(TEXT("MenuLeaseRejected"));
		return false;
	}

	ActiveMenuLeaseId = LeaseId;
	ActiveMenuLeaseSourceId = SourceId;
	ActiveMenuLeaseEntries = Entries;
	bActiveMenuLeaseBackedByProvider = false;
	ActiveMenuLeaseProviderRequest = FWacomRunMenuCardLeaseRequest();
	ResetDefaultSourceRefreshKey();
	ResetProviderLeaseRefreshKey();
	RefreshRunFirstPersonCardLayer();
	return true;
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
	if (!ResolveFirstPersonCardAnchor())
	{
		return RejectWith(TEXT("MissingAnchor"));
	}
	if (!HasAnyRuntimeFilter(Request) && !Request.bAllowAllOwnedCardsWhenNoFilter)
	{
		return RejectWith(TEXT("EmptyFilter"));
	}

	const FRunState& RunState = BoundRunSession->GetRunState();
	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	int32 ConsideredCount = 0;

	if (Request.bIncludeBackpack)
	{
		AppendRunMenuLeaseCandidatesFromZone(RunState.Backpack, Request, Entries, ConsideredCount);
	}
	if (Request.bIncludeBattleDeck)
	{
		AppendRunMenuLeaseCandidatesFromZone(RunState.BattleDeck, Request, Entries, ConsideredCount);
	}
	if (Request.bIncludeBurdenZone)
	{
		AppendRunMenuLeaseCandidatesFromZone(RunState.BurdenZone, Request, Entries, ConsideredCount);
	}
	if (Request.bIncludeSpecialZones)
	{
		for (const FSpecialZone& SpecialZone : RunState.SpecialZones)
		{
			AppendRunMenuLeaseCandidatesFromZone(SpecialZone.Cards, Request, Entries, ConsideredCount);
		}
	}

	OutResult.ConsideredCount = ConsideredCount;
	OutResult.CandidateCount = Entries.Num();
	if (Entries.Num() == 0)
	{
		if (ActiveMenuLeaseId == Request.LeaseId)
		{
			ClearRunFirstPersonCardLayerMenuLease(Request.LeaseId);
		}
		return RejectWith(TEXT("NoMatchingCandidates"));
	}

	const bool bLeaseSet =
		SetRunFirstPersonCardLayerMenuLease(Request.LeaseId, Request.SourceId, Entries);
	if (bLeaseSet)
	{
		ResetProviderLeaseRefreshKey();
		bActiveMenuLeaseBackedByProvider = true;
		ActiveMenuLeaseProviderRequest = Request;
		StoreProviderLeaseRefreshKey();
	}
	OutResult.bLeaseSet = bLeaseSet;
	OutResult.RejectReason = bLeaseSet
		? NAME_None
		: LastRefreshResult;
	OutResult.DebugSummary = BuildRunMenuCardLeaseProviderDebugSummary(Request, OutResult);
	StoreLastMenuLeaseProviderResult(OutResult);
	if (!bLeaseSet)
	{
		LogDebugState(TEXT("MenuLeaseProviderRejected"));
	}
	else
	{
		LogDebugState(TEXT("MenuLeaseProviderSet"));
	}
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

	const FName PreviousLeaseSourceId = ActiveMenuLeaseSourceId;
	ActiveMenuLeaseId = NAME_None;
	ActiveMenuLeaseSourceId = NAME_None;
	ActiveMenuLeaseEntries.Reset();
	bActiveMenuLeaseBackedByProvider = false;
	ActiveMenuLeaseProviderRequest = FWacomRunMenuCardLeaseRequest();
	ResetDefaultSourceRefreshKey();
	ResetProviderLeaseRefreshKey();

	if (UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchor())
	{
		ClearRuntimeCardLayerEntries(*Anchor, PreviousLeaseSourceId);
		bLastHadAnchor = true;
	}
	else
	{
		bLastHadAnchor = false;
	}

	RefreshRunFirstPersonCardLayer();
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
	View.bHasActiveMenuLease = !ActiveMenuLeaseId.IsNone();
	View.ActiveMenuLeaseId = ActiveMenuLeaseId;
	View.ActiveMenuLeaseSourceId = ActiveMenuLeaseSourceId;
	View.ActiveMenuLeaseEntryCount = ActiveMenuLeaseEntries.Num();
	View.bActiveMenuLeaseBackedByProvider = bActiveMenuLeaseBackedByProvider;
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
	return View;
}

FString UWacomRunFirstPersonCardSourceComponent::GetRunFirstPersonCardSourceDebugSummary() const
{
	const FWacomRunFirstPersonCardSourceDebugView View = GetRunFirstPersonCardSourceDebugView();
	return FString::Printf(
		TEXT("RunFirstPersonCardSource{Owner=%s SourceId=%s Enabled=%s Active=%s SuppressedByGameMenu=%s HasLease=%s ProviderBacked=%s LeaseId=%s LeaseSource=%s LeaseEntries=%d HasRun=%s HasAnchor=%s Physical=%d Projected=%d Entries=%d Last=%s}"),
		*GetNameSafe(GetOwner()),
		*View.SourceId.ToString(),
		View.bEnabled ? TEXT("true") : TEXT("false"),
		View.bActive ? TEXT("true") : TEXT("false"),
		View.bSuppressedByGameMenu ? TEXT("true") : TEXT("false"),
		View.bHasActiveMenuLease ? TEXT("true") : TEXT("false"),
		View.bActiveMenuLeaseBackedByProvider ? TEXT("true") : TEXT("false"),
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
			TEXT(" Provider{LeaseId=%s SourceId=%s Result=%s Candidates=%d Considered=%d Debug=%s}"),
			*View.LastMenuLeaseProviderLeaseId.ToString(),
			*View.LastMenuLeaseProviderSourceId.ToString(),
			*View.LastMenuLeaseProviderResult.ToString(),
			View.LastMenuLeaseProviderCandidateCount,
			View.LastMenuLeaseProviderConsideredCount,
			*View.LastMenuLeaseProviderDebugSummary);
}

void UWacomRunFirstPersonCardSourceComponent::LogRunFirstPersonCardSourceDebugSummary() const
{
	UE_LOG(LogTemp, Display,
		TEXT("[WacomRunFirstPersonCardSource] %s"),
		*GetRunFirstPersonCardSourceDebugSummary());
}

bool UWacomRunFirstPersonCardSourceComponent::BuildRunFirstPersonCardEntries(
	const URunSession& Run,
	TArray<FWacomFirstPersonCardLayerEntry>& OutEntries) const
{
	OutEntries.Reset();
	LastBattleDeckPhysicalCount = 0;
	LastBattleDeckProjectedCount = 0;

	const FRunBackpackStorageSnapshot Snapshot = Run.BuildBackpackStorageSnapshot();
	LastBattleDeckPhysicalCount = Snapshot.BattleDeckPhysicalCards.Num();
	LastBattleDeckProjectedCount = bIncludeProjectedRunBattleDeckCards
		? Snapshot.BattleDeckProjectedCards.Num()
		: 0;
	OutEntries.Reserve(LastBattleDeckPhysicalCount + LastBattleDeckProjectedCount);

	auto AppendCardView = [&OutEntries](const FRunStorageCardView& StorageView)
	{
		const FCardInstance& Instance = StorageView.Instance;
		if (!Instance.InstanceId.IsValid() || !Instance.Definition)
		{
			return;
		}
		OutEntries.Add(BuildRunFirstPersonCardLayerEntryFromInstance(Instance));
	};

	for (const FRunStorageCardView& StorageView : Snapshot.BattleDeckPhysicalCards)
	{
		AppendCardView(StorageView);
	}
	if (bIncludeProjectedRunBattleDeckCards)
	{
		for (const FRunStorageCardView& StorageView : Snapshot.BattleDeckProjectedCards)
		{
			AppendCardView(StorageView);
		}
	}

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

void UWacomRunFirstPersonCardSourceComponent::WriteRuntimeCardLayerEntries(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	FName SourceId,
	const TArray<FWacomFirstPersonCardLayerEntry>& Entries)
{
	const bool bEnableMenuLeaseDragProbe =
		!ActiveMenuLeaseId.IsNone()
		&& SourceId == ActiveMenuLeaseSourceId;
	const bool bEnableRunWorldCardDropDrag =
		ActiveMenuLeaseId.IsNone()
		&& SourceId == RunFirstPersonCardLayerSourceId;
	const bool bEnableRuntimeInteraction =
		bEnableMenuLeaseDragProbe || bEnableRunWorldCardDropDrag;
	ApplyMenuLeaseInteractionOverrides(Anchor, bEnableRuntimeInteraction);
	Anchor.SetRuntimeCardLayerEntries(SourceId, Entries);
	Anchor.SetBattleHandInteractionEnabled(bEnableRuntimeInteraction);
}

void UWacomRunFirstPersonCardSourceComponent::ClearRuntimeCardLayerEntries(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	FName SourceId)
{
	if (Anchor.GetRuntimeCardLayerSourceId() == SourceId)
	{
		Anchor.SetBattleHandInteractionEnabled(false);
		Anchor.CancelFirstPersonCardDragGesture(true);
		RestoreMenuLeaseInteractionOverrides();
	}
	Anchor.ClearRuntimeCardLayerData(SourceId);
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

	if (bHasMenuLeaseClickOverride && MenuLeaseClickOverrideAnchor.Get() == &Anchor)
	{
		Anchor.bEnableClickToPlayCard = false;
		return;
	}

	RestoreMenuLeaseInteractionOverrides();
	bMenuLeasePreviousClickToPlayCard = Anchor.bEnableClickToPlayCard;
	Anchor.bEnableClickToPlayCard = false;
	MenuLeaseClickOverrideAnchor = &Anchor;
	bHasMenuLeaseClickOverride = true;
}

void UWacomRunFirstPersonCardSourceComponent::RestoreMenuLeaseInteractionOverrides()
{
	if (bHasMenuLeaseClickOverride)
	{
		if (UWacomFirstPersonCardAnchorComponent* Anchor = MenuLeaseClickOverrideAnchor.Get())
		{
			Anchor->bEnableClickToPlayCard = bMenuLeasePreviousClickToPlayCard;
		}
	}

	bHasMenuLeaseClickOverride = false;
	bMenuLeasePreviousClickToPlayCard = true;
	MenuLeaseClickOverrideAnchor.Reset();
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
		RefreshRunFirstPersonCardLayerInternal(
			/*bAllowDefaultSourceRevisionSkip*/ true,
			/*bAllowProviderLeaseRevisionSkip*/ true);
	}
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
