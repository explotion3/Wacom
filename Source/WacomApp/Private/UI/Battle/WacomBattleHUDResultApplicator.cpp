// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDResultApplicator.h"

#include "Events/BattleEvent.h"
#include "Session/BattleInitializationResult.h"
#include "Session/BattleResolution.h"
#include "Session/BattleSession.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleHUDCombatLogController.h"
#include "UI/Battle/WacomBattleHUDPresentationCoordinator.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"
#include "UI/Battle/WacomBattleHUDFirstPersonHandBridge.h"
#include "UI/Card/WacomFirstPersonCardPresentationAssetCollector.h"
#include "UI/Card/WacomFirstPersonCardPresentationPrewarmController.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"

namespace
{
	void LogResultApplicatorRawBattleEvents(const TArray<FBattleEvent>& Events)
	{
		for (const FBattleEvent& Event : Events)
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("[BattleHUD] [#%d] Type=%d Amount=%d Count=%d Actor=%s Card=%s Tag=%s"),
				Event.Sequence,
				static_cast<int32>(Event.Type),
				Event.Amount,
				Event.Count,
				*Event.ActorInstanceId.ToString(EGuidFormats::Short),
				*Event.CardInstanceId.ToString(EGuidFormats::Short),
				*Event.Tag.ToString());
		}
	}
}

FWacomBattleHUDResultApplicator::FWacomBattleHUDResultApplicator(FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
	, CardPresentationPrewarm(MakeUnique<FWacomFirstPersonCardPresentationPrewarmController>())
{
}

FWacomBattleHUDResultApplicator::~FWacomBattleHUDResultApplicator() = default;

void FWacomBattleHUDResultApplicator::BeginBattleEntryPresentation()
{
	++PresentationGeneration;
	BoundSession.Reset();
	PendingInitializationSnapshot = FBattleSnapshot();
	LastAppliedStateVersion = INDEX_NONE;
	bEntryPresentationActive = true;
	bInitializationApplied = false;
	bInitialTurnActivityPresented = false;
	bCameraStageReady = false;
	bPrewarmGateReady = false;
	ResetCardPresentationPrewarm();
	Runtime.SetBattleInputReady(false);
	Runtime.SetFirstPersonBattleHandSuppressedForEntry(true);
}

void FWacomBattleHUDResultApplicator::AttachInitializedBattleSession(
	UBattleSession* Session,
	FBattleInitializationResult Initialization)
{
	if (!bEntryPresentationActive || bInitializationApplied)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleHUD] Reject initialization attach outside an open generation (Generation=%llu)."),
			PresentationGeneration);
		return;
	}

	if (!Session || !Initialization.IsOk())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleHUD] Reject failed initialization attach (Generation=%llu Session=%s Code=%d)."),
			PresentationGeneration,
			*GetNameSafe(Session),
			static_cast<int32>(Initialization.Status.Code));
		CancelEntryPresentation();
		return;
	}

	const bool bReinitializingBoundSession = Runtime.GetSession() == Session;
	BoundSession = Session;
	bBindingSessionInternally = true;
	Runtime.Host().GetHUD().SetInjectedBattleSession(Session);
	if (bReinitializingBoundSession)
	{
		// A successful reinitialize on the same UObject is still a new battle.
		// Run the normal HUD cleanup even though the pointer did not change.
		Runtime.NativeOnSessionChanged(Session, Session);
	}
	bBindingSessionInternally = false;

	// Session cleanup is broad. Reassert the active generation's entry gates
	// before its snapshot can reach the first-person hand layer.
	Runtime.SetBattleInputReady(false);
	Runtime.SetFirstPersonBattleHandSuppressedForEntry(true);
	BeginCardPresentationPrewarm();

	Runtime.StoreFirstPersonCardTransitionEvents(Initialization.Events);
	Runtime.NativeRefreshFromSnapshot(Initialization.PostSnapshot);

	const FWacomBattleCombatLogCommandContext SystemContext =
		UWacomBattleCombatLogBuilder::BuildSystemCommandContext(Initialization.PostSnapshot);
	LogResultApplicatorRawBattleEvents(Initialization.Events);
	Runtime.GetCombatLogController().AppendBlock(
		SystemContext,
		Initialization.Events,
		Initialization.PostSnapshot,
		Initialization.PostSnapshot);
	Runtime.EnqueueBattlePresentationEvents(Initialization.Events, INDEX_NONE);

	PendingInitializationSnapshot = Initialization.PostSnapshot;
	LastAppliedStateVersion = Initialization.PostSnapshot.Version;
	bInitializationApplied = true;
	Tick(0.0f);
}

void FWacomBattleHUDResultApplicator::ReleaseBattleEntryPresentation()
{
	if (!bEntryPresentationActive || !bInitializationApplied)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleHUD] Reject battle-entry release before attach or after release (Generation=%llu)."),
			PresentationGeneration);
		return;
	}

	if (!BoundSession.IsValid() || Runtime.GetSession() != BoundSession.Get())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleHUD] Reject battle-entry release for a stale session (Generation=%llu)."),
			PresentationGeneration);
		CancelEntryPresentation();
		return;
	}

	bCameraStageReady = true;
	TryReleaseBattleEntryPresentation();
}

void FWacomBattleHUDResultApplicator::Tick(float DeltaTime)
{
	if (!CardPresentationPrewarm)
	{
		return;
	}
	if (!bEntryPresentationActive)
	{
		const FWacomFirstPersonCardPresentationPrewarmDebugView View =
			CardPresentationPrewarm->BuildDebugView();
		if (View.State == EWacomFirstPersonCardPresentationPrewarmState::TimedOut
			&& !View.bRequiredLoadCompleted)
		{
			CardPresentationPrewarm->Tick(DeltaTime);
		}
		return;
	}
	CardPresentationPrewarm->Tick(DeltaTime);
	uint32 ResolvedGeneration = 0;
	EWacomFirstPersonCardPresentationPrewarmState ResolvedState =
		EWacomFirstPersonCardPresentationPrewarmState::Inactive;
	if (CardPresentationPrewarm->ConsumeGateResolution(ResolvedGeneration, ResolvedState)
		&& ResolvedGeneration == PrewarmGeneration)
	{
		bPrewarmGateReady = true;
	}
	TryReleaseBattleEntryPresentation();
}

void FWacomBattleHUDResultApplicator::BeginCardPresentationPrewarm()
{
	if (!CardPresentationPrewarm)
	{
		CardPresentationPrewarm = MakeUnique<FWacomFirstPersonCardPresentationPrewarmController>();
	}
	FWacomFirstPersonCardPresentationPrewarmRequest Request;
	Request.ScopeId = TEXT("Battle");
	Request.TimeoutSeconds = 1.5f;
	if (UWacomFirstPersonCardAnchorComponent* Anchor =
		Runtime.GetFirstPersonHandBridge().ResolveAnchor())
	{
		const FWacomFirstPersonCardPresentationAssetCollection Collection =
			FWacomFirstPersonCardPresentationAssetCollector::Collect(*Anchor);
		Request.RequiredVisualAssets = Collection.RequiredVisualAssets;
		Request.OptionalAudioAssets = Collection.OptionalAudioAssets;
	}
	PrewarmGeneration = CardPresentationPrewarm->Begin(Request);
}

void FWacomBattleHUDResultApplicator::TryReleaseBattleEntryPresentation()
{
	if (!bEntryPresentationActive || !bInitializationApplied
		|| !bCameraStageReady || !bPrewarmGateReady)
	{
		return;
	}
	if (!BoundSession.IsValid() || Runtime.GetSession() != BoundSession.Get())
	{
		CancelEntryPresentation();
		return;
	}
	Runtime.SetFirstPersonBattleHandSuppressedForEntry(false);
	Runtime.SetBattleInputReady(true);
	Runtime.NativeRefreshFromSnapshot(PendingInitializationSnapshot);
	if (!bInitialTurnActivityPresented)
	{
		Runtime.GetCombatLogController().PresentInitialTurnActivity(
			PendingInitializationSnapshot.TurnNumber);
		bInitialTurnActivityPresented = true;
	}
	bEntryPresentationActive = false;
	PendingInitializationSnapshot = FBattleSnapshot();
}

void FWacomBattleHUDResultApplicator::ResetCardPresentationPrewarm()
{
	if (CardPresentationPrewarm)
	{
		CardPresentationPrewarm->Reset();
	}
	PrewarmGeneration = 0;
}

int32 FWacomBattleHUDResultApplicator::GetCardPresentationPrewarmState() const
{
	return CardPresentationPrewarm
		? static_cast<int32>(CardPresentationPrewarm->BuildDebugView().State)
		: static_cast<int32>(EWacomFirstPersonCardPresentationPrewarmState::Inactive);
}

uint32 FWacomBattleHUDResultApplicator::GetCardPresentationPrewarmGeneration() const
{
	return CardPresentationPrewarm
		? CardPresentationPrewarm->BuildDebugView().Generation
		: 0;
}

int32 FWacomBattleHUDResultApplicator::GetCardPresentationRequiredAssetCount() const
{
	return CardPresentationPrewarm
		? CardPresentationPrewarm->BuildDebugView().RequiredAssetCount
		: 0;
}

int32 FWacomBattleHUDResultApplicator::GetCardPresentationOptionalAssetCount() const
{
	return CardPresentationPrewarm
		? CardPresentationPrewarm->BuildDebugView().OptionalAssetCount
		: 0;
}

float FWacomBattleHUDResultApplicator::GetCardPresentationPrewarmElapsedSeconds() const
{
	return CardPresentationPrewarm
		? CardPresentationPrewarm->BuildDebugView().ElapsedSeconds
		: 0.0f;
}

bool FWacomBattleHUDResultApplicator::IsEntryWaitingForCamera() const
{
	return bEntryPresentationActive && !bCameraStageReady;
}

bool FWacomBattleHUDResultApplicator::IsEntryWaitingForCardPresentationPrewarm() const
{
	return bEntryPresentationActive && !bPrewarmGateReady;
}

void FWacomBattleHUDResultApplicator::ApplyCommandResolution(
	const FWacomBattleCommandPresentationContext& Context,
	const FBattleResolution& Resolution)
{
	UBattleSession* Session = Runtime.GetSession();
	if (!ValidateCommandResolution(Session, Context, Resolution))
	{
		return;
	}

	// Advance first so re-entrant UI callbacks cannot replay this resolution.
	LastAppliedStateVersion = Resolution.VersionAfter;
	Runtime.HideCardDetailPanel();

	if (Context.PlayCardCommit.IsSet())
	{
		const FWacomBattlePlayCardCommitPresentation& Commit = Context.PlayCardCommit.GetValue();
		Runtime.RecordFirstPersonPlayCommit(
			Commit.CardInstanceId,
			Commit.TargetPartIdentity,
			Commit.TargetWidgetPosition);
		const FBattleCardTargetPreview& TargetPreview =
			Context.CombatLogContext.CardTargetPreview;
		if (TargetPreview.bHasPreview
			&& TargetPreview.TargetKind == EWacomBattleCardPreviewTargetKind::HandCard
			&& TargetPreview.TargetHandCardInstanceId.IsValid())
		{
			Runtime.RecordFirstPersonHandTargetImpact(
				TargetPreview.TargetHandCardInstanceId);
		}
		Runtime.ClearPendingTargetingCardId();
		Runtime.SetUIState(EBattleUIState::Idle);
	}

	LogResultApplicatorRawBattleEvents(Resolution.Events);
	Runtime.GetCombatLogController().AppendBlock(
		Context.CombatLogContext,
		Resolution.Events,
		Context.PreCommandSnapshot,
		Resolution.PostSnapshot);

	if (Context.CombatLogContext.CommandKind == EWacomBattleCombatLogCommandKind::EndTurn
		&& Runtime.GetPresentationCoordinator().EnqueueEndTurnPresentationPlan(
			Resolution.PresentationJournal,
			Resolution.Events,
			Resolution.PostSnapshot))
	{
		return;
	}
	const int32 PresentationStackEntryId =
		Context.CombatLogContext.CommandKind == EWacomBattleCombatLogCommandKind::PlayCard
			? Runtime.AppendBattlePresentationStackEntry(
				Context.CombatLogContext,
				Context.PreCommandSnapshot)
			: INDEX_NONE;
	if (Context.CombatLogContext.CommandKind == EWacomBattleCombatLogCommandKind::PlayCard
		&& Runtime.GetPresentationCoordinator().EnqueuePlayCardPresentationPlan(
			Context,
			Resolution,
			PresentationStackEntryId))
	{
		return;
	}
	if (Context.CombatLogContext.CommandKind != EWacomBattleCombatLogCommandKind::EndTurn
		&& Context.CombatLogContext.CommandKind != EWacomBattleCombatLogCommandKind::PlayCard
		&& Runtime.GetPresentationCoordinator().EnqueueResolvedCommandPresentationPlan(
			Resolution.PresentationJournal,
			Resolution.Events,
			Context.PreCommandSnapshot,
			Resolution.PostSnapshot,
			PresentationStackEntryId))
	{
		return;
	}

	Runtime.StoreFirstPersonCardTransitionEvents(Resolution.Events);
	Runtime.EnqueueBattlePresentationEvents(Resolution.Events, PresentationStackEntryId);
	Runtime.NativeRefreshFromSnapshot(Resolution.PostSnapshot);
}

void FWacomBattleHUDResultApplicator::HandleSessionChanged(
	UBattleSession* OldSession,
	UBattleSession* NewSession)
{
	(void)OldSession;
	if (bBindingSessionInternally && NewSession == BoundSession.Get())
	{
		return;
	}

	Runtime.SetFirstPersonBattleHandSuppressedForEntry(false);
	Runtime.SetBattleInputReady(true);
	BoundSession = NewSession;
	PendingInitializationSnapshot = FBattleSnapshot();
	bEntryPresentationActive = false;
	bInitializationApplied = false;
	bInitialTurnActivityPresented = false;
	bCameraStageReady = false;
	bPrewarmGateReady = false;
	ResetCardPresentationPrewarm();
	LastAppliedStateVersion = NewSession
		? NewSession->BuildSnapshot().Version
		: INDEX_NONE;
}

void FWacomBattleHUDResultApplicator::CancelEntryPresentation()
{
	UBattleSession* CurrentSession = Runtime.GetSession();
	BoundSession = CurrentSession;
	PendingInitializationSnapshot = FBattleSnapshot();
	bEntryPresentationActive = false;
	bInitializationApplied = false;
	bInitialTurnActivityPresented = false;
	bCameraStageReady = false;
	bPrewarmGateReady = false;
	ResetCardPresentationPrewarm();
	LastAppliedStateVersion = CurrentSession
		? CurrentSession->BuildSnapshot().Version
		: INDEX_NONE;
	Runtime.SetFirstPersonBattleHandSuppressedForEntry(false);
	Runtime.SetBattleInputReady(true);
}

bool FWacomBattleHUDResultApplicator::ValidateCommandResolution(
	UBattleSession* Session,
	const FWacomBattleCommandPresentationContext& Context,
	const FBattleResolution& Resolution) const
{
	if (!Resolution.IsOk())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleHUD] Reject failed command resolution (Code=%d Detail=%s)."),
			static_cast<int32>(Resolution.Status.Code),
			*Resolution.Status.Detail.ToString());
		return false;
	}

	if (!Session || BoundSession.Get() != Session || Context.SourceSession.Get() != Session)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] Reject command resolution for an unbound session."));
		return false;
	}

	if (Resolution.VersionBefore != LastAppliedStateVersion
		|| Resolution.VersionAfter != Resolution.VersionBefore + 1
		|| Resolution.PostSnapshot.Version != Resolution.VersionAfter)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleHUD] Reject non-contiguous command resolution (Last=%d Before=%d After=%d Snapshot=%d)."),
			LastAppliedStateVersion,
			Resolution.VersionBefore,
			Resolution.VersionAfter,
			Resolution.PostSnapshot.Version);
		return false;
	}

	return true;
}
