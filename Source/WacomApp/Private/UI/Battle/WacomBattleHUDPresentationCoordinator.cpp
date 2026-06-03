// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDPresentationCoordinator.h"

#include "UI/Battle/BattleHUD.h"

#include "Components/Widget.h"
#include "Events/BattleEvent.h"
#include "Session/BattleSession.h"
#include "UI/Battle/ActionPanel.h"
#include "UI/Battle/BattlePresentationStackWidget.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattleEventPresentationQueue.h"
#include "UI/Battle/WacomBattleHUDCommandFlow.h"
#include "UI/Battle/WacomBattleHUDTargetingFlow.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

namespace
{
	constexpr float BattlePresentationStackExitSeconds = 0.16f;
}

FWacomBattleHUDPresentationCoordinator::FWacomBattleHUDPresentationCoordinator(UBattleHUD& InHUD)
	: HUD(InHUD)
{
}

FWacomBattleHUDPresentationCoordinator::~FWacomBattleHUDPresentationCoordinator()
{
	if (BattleEventPresentationQueue)
	{
		BattleEventPresentationQueue->Clear();
		BattleEventPresentationQueue.Reset();
	}
	BattlePresentationStackExitTimerHandles.Reset();
	BattlePresentationStackExitingEntryIds.Reset();
	BattlePresentationStackEntries.Reset();
	PendingTurnBoundaryCommand = EWacomBattleHUDTurnBoundaryCommand::None;
}

int32 FWacomBattleHUDPresentationCoordinator::AppendStackEntry(
	const FWacomBattleCombatLogCommandContext& CommandContext,
	const FBattleSnapshot& PreCommandSnapshot)
{
	if (CommandContext.CommandKind != EWacomBattleCombatLogCommandKind::PlayCard
		|| !CommandContext.CardInstanceId.IsValid())
	{
		return INDEX_NONE;
	}

	const FHandCardSnapshot* CardSnapshot = nullptr;
	for (const FHandCardSnapshot& Candidate : PreCommandSnapshot.Hand.Cards)
	{
		if (Candidate.InstanceId == CommandContext.CardInstanceId)
		{
			CardSnapshot = &Candidate;
			break;
		}
	}
	if (!CardSnapshot || !CardSnapshot->Definition)
	{
		return INDEX_NONE;
	}

	FWacomBattlePresentationStackEntryView Entry;
	Entry.EntryId = NextBattlePresentationStackEntryId++;
	Entry.CardInstanceId = CommandContext.CardInstanceId;
	Entry.CardViewData = UWacomCardPresentationBuilder::BuildCardViewData(CardSnapshot->Definition);
	BattlePresentationStackEntries.Add(Entry);
	SyncStackWidget();
	RefreshCommandAvailabilityWidgets();
	return Entry.EntryId;
}

void FWacomBattleHUDPresentationCoordinator::BeginStackEntryExit(int32 EntryId)
{
	if (EntryId == INDEX_NONE)
	{
		return;
	}

	FWacomBattlePresentationStackEntryView* FoundEntry = BattlePresentationStackEntries.FindByPredicate(
		[EntryId](const FWacomBattlePresentationStackEntryView& Candidate)
		{
			return Candidate.EntryId == EntryId;
		});
	if (!FoundEntry)
	{
		return;
	}

	if (!FoundEntry->bIsExiting)
	{
		FoundEntry->bIsExiting = true;
		BattlePresentationStackExitingEntryIds.AddUnique(EntryId);
		SyncStackWidget();
	}

	if (UWorld* World = GetWorld())
	{
		FTimerHandle& TimerHandle = BattlePresentationStackExitTimerHandles.FindOrAdd(EntryId);
		World->GetTimerManager().ClearTimer(TimerHandle);
		World->GetTimerManager().SetTimer(
			TimerHandle,
			FTimerDelegate::CreateRaw(this, &FWacomBattleHUDPresentationCoordinator::FinishStackEntryExit, EntryId),
			BattlePresentationStackExitSeconds,
			false);
		return;
	}

	FinishStackEntryExit(EntryId);
}

void FWacomBattleHUDPresentationCoordinator::FinishStackEntryExit(int32 EntryId)
{
	if (EntryId == INDEX_NONE)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (FTimerHandle* TimerHandle = BattlePresentationStackExitTimerHandles.Find(EntryId))
		{
			World->GetTimerManager().ClearTimer(*TimerHandle);
		}
	}
	BattlePresentationStackExitTimerHandles.Remove(EntryId);
	BattlePresentationStackExitingEntryIds.Remove(EntryId);

	const int32 Removed = BattlePresentationStackEntries.RemoveAll(
		[EntryId](const FWacomBattlePresentationStackEntryView& Entry)
		{
			return Entry.EntryId == EntryId;
		});
	if (Removed <= 0)
	{
		return;
	}

	SyncStackWidget();
	RefreshCommandAvailabilityWidgets();
	TryExecutePendingTurnBoundaryCommand();
}

void FWacomBattleHUDPresentationCoordinator::ClearStack()
{
	if (UWorld* World = GetWorld())
	{
		for (TPair<int32, FTimerHandle>& Pair : BattlePresentationStackExitTimerHandles)
		{
			World->GetTimerManager().ClearTimer(Pair.Value);
		}
	}
	BattlePresentationStackExitTimerHandles.Reset();
	BattlePresentationStackExitingEntryIds.Reset();
	BattlePresentationStackEntries.Reset();
	SyncStackWidget();
}

void FWacomBattleHUDPresentationCoordinator::EnqueueEvents(
	const TArray<FBattleEvent>& Events,
	int32 PresentationStackEntryId)
{
	if (Events.IsEmpty())
	{
		if (PresentationStackEntryId != INDEX_NONE)
		{
			BeginStackEntryExit(PresentationStackEntryId);
		}
		return;
	}

	if (!BattleEventPresentationQueue)
	{
		BattleEventPresentationQueue = MakeShared<FWacomBattleEventPresentationQueue>(*this);
	}

	BattleEventPresentationQueue->EnqueueEvents(
		Events,
		PresentationStackEntryId,
		HUD.CardPresentationStackMinimumHoldSeconds);
}

void FWacomBattleHUDPresentationCoordinator::ClearQueue()
{
	if (BattleEventPresentationQueue)
	{
		BattleEventPresentationQueue->Clear();
		BattleEventPresentationQueue.Reset();
	}
	ClearStack();
	ClearPendingTurnBoundaryCommand();
}

bool FWacomBattleHUDPresentationCoordinator::IsQueueBusy() const
{
	return BattleEventPresentationQueue && BattleEventPresentationQueue->IsBusy();
}

void FWacomBattleHUDPresentationCoordinator::QueuePendingTurnBoundaryCommand(
	EWacomBattleHUDTurnBoundaryCommand Command)
{
	if (Command == EWacomBattleHUDTurnBoundaryCommand::None
		|| PendingTurnBoundaryCommand != EWacomBattleHUDTurnBoundaryCommand::None)
	{
		return;
	}

	HUD.ClearBattleSceneEnemyPartHoverProbe(TEXT("PendingTurnBoundary"));
	PendingTurnBoundaryCommand = Command;
	if (HUD.UIState == EBattleUIState::TargetSelect)
	{
		HUD.PendingTargetingCardId.Invalidate();
		HUD.SetUIState(EBattleUIState::Idle);
	}
	RefreshCommandAvailabilityWidgets();
	TryExecutePendingTurnBoundaryCommand();
}

void FWacomBattleHUDPresentationCoordinator::ClearPendingTurnBoundaryCommand()
{
	if (PendingTurnBoundaryCommand == EWacomBattleHUDTurnBoundaryCommand::None)
	{
		return;
	}

	PendingTurnBoundaryCommand = EWacomBattleHUDTurnBoundaryCommand::None;
	RefreshCommandAvailabilityWidgets();
}

FText FWacomBattleHUDPresentationCoordinator::GetPendingTurnBoundaryCommandText() const
{
	switch (PendingTurnBoundaryCommand)
	{
	case EWacomBattleHUDTurnBoundaryCommand::Wait:
		return NSLOCTEXT("BattleHUD", "PendingTurnBoundaryWait", "等待排队中");
	case EWacomBattleHUDTurnBoundaryCommand::EndTurn:
		return NSLOCTEXT("BattleHUD", "PendingTurnBoundaryEndTurn", "结束回合排队中");
	case EWacomBattleHUDTurnBoundaryCommand::None:
	default:
		return FText::GetEmpty();
	}
}

void FWacomBattleHUDPresentationCoordinator::TryExecutePendingTurnBoundaryCommand()
{
	if (PendingTurnBoundaryCommand == EWacomBattleHUDTurnBoundaryCommand::None
		|| HasStackEntries()
		|| IsQueueBusy())
	{
		return;
	}

	UBattleSession* CurrentSession = HUD.GetSession();
	if (!CurrentSession)
	{
		ClearPendingTurnBoundaryCommand();
		return;
	}

	const FBattleSnapshot Snapshot = CurrentSession->BuildSnapshot();
	if (Snapshot.Phase == EBattlePhase::BattleEnd
		|| Snapshot.Phase == EBattlePhase::PendingKnockdownChoice
		|| Snapshot.Phase != EBattlePhase::PlayerAction)
	{
		ClearPendingTurnBoundaryCommand();
		return;
	}

	const EWacomBattleHUDTurnBoundaryCommand CommandToExecute = PendingTurnBoundaryCommand;
	PendingTurnBoundaryCommand = EWacomBattleHUDTurnBoundaryCommand::None;
	RefreshCommandAvailabilityWidgets();
	ExecuteTurnBoundaryCommandNow(CommandToExecute);
}

void FWacomBattleHUDPresentationCoordinator::HandleQueueStarted()
{
	HUD.HideCardDetailPanel();
}

void FWacomBattleHUDPresentationCoordinator::HandleQueueFinished()
{
	UBattleSession* CurrentSession = HUD.GetSession();
	if (!CurrentSession)
	{
		return;
	}

	const FBattleSnapshot Snapshot = CurrentSession->BuildSnapshot();
	if (Snapshot.Phase == EBattlePhase::BattleEnd)
	{
		HUD.SetUIState(EBattleUIState::BattleEnd);
		return;
	}

	TryExecutePendingTurnBoundaryCommand();
}

void FWacomBattleHUDPresentationCoordinator::HandleBattleEndStep()
{
	if (UBattleSession* CurrentSession = HUD.GetSession())
	{
		HUD.RefreshFromSnapshot(CurrentSession->BuildSnapshot());
	}
}

void FWacomBattleHUDPresentationCoordinator::HandleKnockdownChoiceDialogStep()
{
	HUD.PushPendingKnockdownChoiceDialog();
}

void FWacomBattleHUDPresentationCoordinator::HandleTargetCueStep(
	const FWacomBattlePresentationTargetCue& Cue)
{
	HUD.PlayBattlePresentationCue(Cue);
}

void FWacomBattleHUDPresentationCoordinator::HandleCardStackBoundaryStep(int32 EntryId)
{
	BeginStackEntryExit(EntryId);
}

UWorld* FWacomBattleHUDPresentationCoordinator::GetWorld() const
{
	return HUD.GetWorld();
}

#if WITH_AUTOMATION_TESTS
void FWacomBattleHUDPresentationCoordinator::AdvanceQueueOnce()
{
	if (BattleEventPresentationQueue)
	{
		BattleEventPresentationQueue->AdvanceForTest();
	}
}
#endif

void FWacomBattleHUDPresentationCoordinator::SyncStackWidget()
{
	if (HUD.BattlePresentationStack)
	{
		HUD.BattlePresentationStack->SetPresentationStackEntries(BattlePresentationStackEntries);
	}
}

void FWacomBattleHUDPresentationCoordinator::ExecuteTurnBoundaryCommandNow(
	EWacomBattleHUDTurnBoundaryCommand Command)
{
	switch (Command)
	{
	case EWacomBattleHUDTurnBoundaryCommand::Wait:
		FWacomBattleHUDCommandFlow::SubmitWait(HUD);
		break;
	case EWacomBattleHUDTurnBoundaryCommand::EndTurn:
		FWacomBattleHUDCommandFlow::SubmitEndTurn(HUD);
		break;
	case EWacomBattleHUDTurnBoundaryCommand::None:
	default:
		break;
	}
}

void FWacomBattleHUDPresentationCoordinator::RefreshCommandAvailabilityWidgets()
{
	UBattleSession* CurrentSession = HUD.GetSession();
	if (!CurrentSession)
	{
		return;
	}

	const FBattleSnapshot Snapshot = CurrentSession->BuildSnapshot();
	if (HUD.ActionPanel)
	{
		HUD.ActionPanel->RefreshFromSnapshot(Snapshot);
	}
	HUD.SyncFirstPersonBattleHandLayer(Snapshot);
	HUD.SyncBattleEnemyPartWorldTargets(Snapshot);
}
