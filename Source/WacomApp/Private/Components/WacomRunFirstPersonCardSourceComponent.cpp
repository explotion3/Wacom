// Copyright Wacom. All Rights Reserved.

#include "Components/WacomRunFirstPersonCardSourceComponent.h"

#include "Cards/CardDefinition.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "RunStateTypes.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

UWacomRunFirstPersonCardSourceComponent::UWacomRunFirstPersonCardSourceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWacomRunFirstPersonCardSourceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearRunFirstPersonCardLayer();
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
	LastBattleDeckPhysicalCount = 0;
	LastBattleDeckProjectedCount = 0;
	LastEntryCount = 0;
	bLastHadAnchor = false;

	if (!bRuntimeSourceActive)
	{
		LastRefreshResult = TEXT("Inactive");
		LogDebugState(TEXT("RefreshSkipped"));
		return false;
	}
	if (!bEnableRunFirstPersonCardLayer)
	{
		LastRefreshResult = TEXT("Disabled");
		ClearRunFirstPersonCardLayerWithResult(LastRefreshResult);
		LogDebugState(TEXT("RefreshSkipped"));
		return false;
	}
	if (!BoundRunSession)
	{
		LastRefreshResult = TEXT("MissingRunSession");
		ClearRunFirstPersonCardLayerWithResult(LastRefreshResult);
		LogDebugState(TEXT("RefreshFailed"));
		return false;
	}

	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchor();
	bLastHadAnchor = Anchor != nullptr;
	if (!Anchor)
	{
		LastRefreshResult = TEXT("MissingAnchor");
		LogDebugState(TEXT("RefreshFailed"));
		return false;
	}

	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	BuildRunFirstPersonCardEntries(*BoundRunSession, Entries);
	LastEntryCount = Entries.Num();

	WriteRuntimeCardLayerEntries(*Anchor, Entries);
	LastRefreshResult = TEXT("Refreshed");
	LogDebugState(TEXT("Refreshed"));
	return true;
}

void UWacomRunFirstPersonCardSourceComponent::ClearRunFirstPersonCardLayer()
{
	ClearRunFirstPersonCardLayerWithResult(TEXT("Cleared"));
}

void UWacomRunFirstPersonCardSourceComponent::ClearRunFirstPersonCardLayerWithResult(FName Result)
{
	if (UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchor())
	{
		ClearRuntimeCardLayerEntries(*Anchor);
		bLastHadAnchor = true;
	}
	else
	{
		bLastHadAnchor = false;
	}
	LastEntryCount = 0;
	LastRefreshResult = Result;
	LogDebugState(TEXT("Cleared"));
}

FWacomRunFirstPersonCardSourceDebugView
UWacomRunFirstPersonCardSourceComponent::GetRunFirstPersonCardSourceDebugView() const
{
	FWacomRunFirstPersonCardSourceDebugView View;
	View.SourceId = RunFirstPersonCardLayerSourceId;
	View.bEnabled = bEnableRunFirstPersonCardLayer;
	View.bActive = bRuntimeSourceActive;
	View.bHasRunSession = BoundRunSession != nullptr;
	View.bHasAnchor = ResolveFirstPersonCardAnchor() != nullptr;
	View.BattleDeckPhysicalCount = LastBattleDeckPhysicalCount;
	View.BattleDeckProjectedCount = LastBattleDeckProjectedCount;
	View.EntryCount = LastEntryCount;
	View.LastRefreshResult = LastRefreshResult;
	return View;
}

FString UWacomRunFirstPersonCardSourceComponent::GetRunFirstPersonCardSourceDebugSummary() const
{
	const FWacomRunFirstPersonCardSourceDebugView View = GetRunFirstPersonCardSourceDebugView();
	return FString::Printf(
		TEXT("RunFirstPersonCardSource{Owner=%s SourceId=%s Enabled=%s Active=%s HasRun=%s HasAnchor=%s Physical=%d Projected=%d Entries=%d Last=%s}"),
		*GetNameSafe(GetOwner()),
		*View.SourceId.ToString(),
		View.bEnabled ? TEXT("true") : TEXT("false"),
		View.bActive ? TEXT("true") : TEXT("false"),
		View.bHasRunSession ? TEXT("true") : TEXT("false"),
		View.bHasAnchor ? TEXT("true") : TEXT("false"),
		View.BattleDeckPhysicalCount,
		View.BattleDeckProjectedCount,
		View.EntryCount,
		*View.LastRefreshResult.ToString());
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
		OutEntries.Add(MoveTemp(Entry));
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
	const TArray<FWacomFirstPersonCardLayerEntry>& Entries)
{
	Anchor.SetRuntimeCardLayerEntries(RunFirstPersonCardLayerSourceId, Entries);
	Anchor.SetBattleHandInteractionPrototypeEnabled(false);
}

void UWacomRunFirstPersonCardSourceComponent::ClearRuntimeCardLayerEntries(
	UWacomFirstPersonCardAnchorComponent& Anchor)
{
	if (Anchor.GetRuntimeCardLayerSourceId() == RunFirstPersonCardLayerSourceId)
	{
		Anchor.SetBattleHandInteractionPrototypeEnabled(false);
		Anchor.CancelFirstPersonCardDragGesture(true);
	}
	Anchor.ClearRuntimeCardLayerData(RunFirstPersonCardLayerSourceId);
}

void UWacomRunFirstPersonCardSourceComponent::UnbindRunSession()
{
	if (BoundRunSession)
	{
		BoundRunSession->OnRunStateChangedNative.RemoveAll(this);
	}
	BoundRunSession = nullptr;
}

void UWacomRunFirstPersonCardSourceComponent::HandleRunStateChanged()
{
	if (bRuntimeSourceActive)
	{
		RefreshRunFirstPersonCardLayer();
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
