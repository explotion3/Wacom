// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDRuntime.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Blueprint/WidgetTree.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Enemies/EnemyPartDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/Battle/BattleHUDFallbackLayoutBuilder.h"
#include "UI/Battle/BattlePresentationStackWidget.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/Battle/WacomBattleCardPresentationHelper.h"
#include "UI/Battle/WacomBattleEnemyPartDragPredictionTypes.h"
#include "UI/Battle/WacomBattleHUDCardDetailController.h"
#include "UI/Battle/WacomBattleHUDCombatLogController.h"
#include "UI/Battle/WacomBattleHUDCommandController.h"
#include "UI/Battle/WacomBattleHUDCommandBarPresenter.h"
#include "UI/Battle/WacomBattleHUDFirstPersonHandBridge.h"
#include "UI/Battle/WacomBattleHUDPresentationCoordinator.h"
#include "UI/Battle/WacomBattleHUDSceneEnemyTargetCoordinator.h"
#include "UI/Battle/WacomBattleHUDSnapshotPresenter.h"
#include "UI/Battle/WacomBattleHUDTargetingController.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/Battle/WacomKnockdownChoiceDialog.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomUITags.h"

#define LOCTEXT_NAMESPACE "WacomBattleHUDRuntime"

namespace
{
	const TCHAR* CardDetailPanelPath = TEXT("/Game/Wacom/UI/Card/WBP_CardDetailPanel.WBP_CardDetailPanel_C");

	const TCHAR* HUDEventTypeToString(EBattleEventType Type)
	{
		switch (Type)
		{
		case EBattleEventType::BattleStarted:             return TEXT("BattleStarted");
		case EBattleEventType::TurnStarted:               return TEXT("TurnStarted");
		case EBattleEventType::CardsDrawn:                return TEXT("CardsDrawn");
		case EBattleEventType::HandZoneChanged:           return TEXT("HandZoneChanged");
		case EBattleEventType::CardPlayed:                return TEXT("CardPlayed");
		case EBattleEventType::InitiativeHit:             return TEXT("InitiativeHit");
		case EBattleEventType::ResistanceResolved:        return TEXT("ResistanceResolved");
		case EBattleEventType::PerfectReleaseResolved:    return TEXT("PerfectReleaseResolved");
		case EBattleEventType::DamageDealt:               return TEXT("DamageDealt");
		case EBattleEventType::StatusApplied:             return TEXT("StatusApplied");
		case EBattleEventType::CardStatusChanged:         return TEXT("CardStatusChanged");
		case EBattleEventType::EnemyInitiativeChanged:    return TEXT("EnemyInitiativeChanged");
		case EBattleEventType::InitiativePushed:          return TEXT("InitiativePushed");
		case EBattleEventType::WaitPerformed:             return TEXT("WaitPerformed");
		case EBattleEventType::EnemyPartActed:            return TEXT("EnemyPartActed");
		case EBattleEventType::EnemyIntentSelected:       return TEXT("EnemyIntentSelected");
		case EBattleEventType::EnemyPhaseChanged:         return TEXT("EnemyPhaseChanged");
		case EBattleEventType::EnemyPartHpEmptied:        return TEXT("EnemyPartHpEmptied");
		case EBattleEventType::EnemyKnockdown:            return TEXT("EnemyKnockdown");
		case EBattleEventType::KnockdownChoiceRequested:  return TEXT("KnockdownChoiceRequested");
		case EBattleEventType::KnockdownChoiceMade:       return TEXT("KnockdownChoiceMade");
		case EBattleEventType::TurnEnded:                 return TEXT("TurnEnded");
		case EBattleEventType::PassiveTriggered:          return TEXT("PassiveTriggered");
		case EBattleEventType::HandLimitDiscarded:        return TEXT("HandLimitDiscarded");
		case EBattleEventType::CardDiscarded:             return TEXT("CardDiscarded");
		case EBattleEventType::CardExhausted:             return TEXT("CardExhausted");
		case EBattleEventType::CardGained:                return TEXT("CardGained");
		case EBattleEventType::CardsRetained:             return TEXT("CardsRetained");
		case EBattleEventType::BattleEnded:               return TEXT("BattleEnded");
		default:                                          return TEXT("?");
		}
	}

	void LogRawBattleEvents(const TArray<FBattleEvent>& Events)
	{
		for (const FBattleEvent& Event : Events)
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("[BattleHUD] [#%d] %-22s Amount=%d Count=%d Actor=%s Card=%s Tag=%s"),
				Event.Sequence,
				HUDEventTypeToString(Event.Type),
				Event.Amount,
				Event.Count,
				*Event.ActorInstanceId.ToString(EGuidFormats::Short),
				*Event.CardInstanceId.ToString(EGuidFormats::Short),
				*Event.Tag.ToString());
		}
	}

}

FWacomBattleHUDRuntimeHost::FWacomBattleHUDRuntimeHost(UBattleHUD& InHUD)
	: HUD(InHUD)
{
}

UObject* FWacomBattleHUDRuntimeHost::AsObject() const
{
	return &HUD;
}

UWorld* FWacomBattleHUDRuntimeHost::GetWorld() const
{
	return HUD.GetWorld();
}

UGameInstance* FWacomBattleHUDRuntimeHost::GetGameInstance() const
{
	return HUD.GetGameInstance();
}

APlayerController* FWacomBattleHUDRuntimeHost::GetOwningPlayer() const
{
	return HUD.GetOwningPlayer();
}

UBattleSession* FWacomBattleHUDRuntimeHost::GetSession() const
{
	return HUD.GetInjectedBattleSession();
}

void FWacomBattleHUDRuntimeHost::RebuildChildBattleWidgets()
{
	HUD.RebuildChildBattleWidgetsForRuntime();
}

void FWacomBattleHUDRuntimeHost::RefreshChildBattleWidgetsFromSnapshot(const FBattleSnapshot& Snapshot)
{
	HUD.RefreshChildBattleWidgetsFromSnapshotForRuntime(Snapshot);
}

void FWacomBattleHUDRuntimeHost::BroadcastBattleEnd(EBattleOutcome Outcome)
{
	HUD.OnBattleEndedNative.Broadcast(Outcome);
}

void FWacomBattleHUDRuntimeHost::NotifyUIStateChanged(
	EBattleUIState OldState,
	EBattleUIState NewState)
{
	HUD.NativeOnUIStateChanged(OldState, NewState);
	HUD.BP_OnUIStateChanged(OldState, NewState);
}

UPlayerStatusBar* FWacomBattleHUDRuntimeHost::GetPlayerStatusBar() const { return HUD.PlayerStatusBar; }
UBattleCommandBarWidget* FWacomBattleHUDRuntimeHost::GetCommandBar() const { return HUD.CommandBar; }
UPileCountView* FWacomBattleHUDRuntimeHost::GetDrawPileView() const { return HUD.DrawPileView; }
UPileCountView* FWacomBattleHUDRuntimeHost::GetDiscardPileView() const { return HUD.DiscardPileView; }
UPileCountView* FWacomBattleHUDRuntimeHost::GetExhaustPileView() const { return HUD.ExhaustPileView; }
UWidget* FWacomBattleHUDRuntimeHost::GetDrawPileMotionAnchor() const { return HUD.DrawPileMotionAnchor; }
UWidget* FWacomBattleHUDRuntimeHost::GetDiscardPileMotionAnchor() const { return HUD.DiscardPileMotionAnchor; }
UWidget* FWacomBattleHUDRuntimeHost::GetPlayTargetMotionAnchor() const { return HUD.PlayTargetMotionAnchor; }
UBattleCombatLogFeedWidget* FWacomBattleHUDRuntimeHost::GetCombatLogFeed() const { return HUD.CombatLogFeed; }
UBattlePresentationStackWidget* FWacomBattleHUDRuntimeHost::GetBattlePresentationStack() const { return HUD.BattlePresentationStack; }
TObjectPtr<UWacomCardDetailPanel>& FWacomBattleHUDRuntimeHost::GetFirstPersonCardDetailPanelSlot() const { return HUD.FirstPersonCardDetailPanel; }
TSubclassOf<UWacomCardDetailPanel> FWacomBattleHUDRuntimeHost::GetCardDetailPanelClass() const { return HUD.CardDetailPanelClass; }
void FWacomBattleHUDRuntimeHost::SetCardDetailPanelClass(TSubclassOf<UWacomCardDetailPanel> PanelClass) { HUD.CardDetailPanelClass = PanelClass; }

float FWacomBattleHUDRuntimeHost::GetCardDetailPanelPadding() const { return HUD.CardDetailPanelPadding; }
FVector2D FWacomBattleHUDRuntimeHost::GetCardDetailPanelEstimatedSize() const { return HUD.CardDetailPanelEstimatedSize; }
bool FWacomBattleHUDRuntimeHost::IsCardDetailReadabilityPolishEnabled() const { return HUD.bEnableCardDetailReadabilityPolish; }
float FWacomBattleHUDRuntimeHost::GetCardDetailHoverDelaySeconds() const { return HUD.CardDetailHoverDelaySeconds; }
float FWacomBattleHUDRuntimeHost::GetCardDetailFadeInSpeed() const { return HUD.CardDetailFadeInSpeed; }
float FWacomBattleHUDRuntimeHost::GetCardDetailFadeOutSpeed() const { return HUD.CardDetailFadeOutSpeed; }
float FWacomBattleHUDRuntimeHost::GetCardDetailFollowSpeed() const { return HUD.CardDetailFollowSpeed; }
float FWacomBattleHUDRuntimeHost::GetCardDetailPositionResetDistancePixels() const { return HUD.CardDetailPositionResetDistancePixels; }
float FWacomBattleHUDRuntimeHost::GetCardDetailAppearStartScale() const { return HUD.CardDetailAppearStartScale; }
float FWacomBattleHUDRuntimeHost::GetCardDetailSideSwitchHysteresisPixels() const { return HUD.CardDetailSideSwitchHysteresisPixels; }
int32 FWacomBattleHUDRuntimeHost::GetBattleCombatLogMaxBlocks() const { return HUD.BattleCombatLogMaxBlocks; }
float FWacomBattleHUDRuntimeHost::GetCardPresentationStackMinimumHoldSeconds() const { return HUD.CardPresentationStackMinimumHoldSeconds; }
int32 FWacomBattleHUDRuntimeHost::GetFirstPersonCardDetailViewportZOrder() const { return HUD.FirstPersonCardDetailViewportZOrder; }
FVector2D FWacomBattleHUDRuntimeHost::GetFirstPersonCardDetailAnchorBaseSize() const { return HUD.FirstPersonCardDetailAnchorBaseSize; }
float FWacomBattleHUDRuntimeHost::GetBattleSceneEnemyPartHoverProbeIntervalSeconds() const { return HUD.BattleSceneEnemyPartHoverProbeIntervalSeconds; }

void FWacomBattleHUDRuntimeHost::BindFirstPersonCardLayerInteractions(
	UWacomFirstPersonCardAnchorComponent& Anchor)
{
	HUD.BindFirstPersonCardLayerInteractionsForRuntime(Anchor);
}

void FWacomBattleHUDRuntimeHost::UnbindFirstPersonCardLayerInteractions(
	UWacomFirstPersonCardAnchorComponent& Anchor)
{
	HUD.UnbindFirstPersonCardLayerInteractionsForRuntime(Anchor);
}

void FWacomBattleHUDRuntimeHost::PushKnockdownChoiceDialog(
	const FKnockdownChoiceView& ChoiceView)
{
	UGameInstance* GameInstance = GetGameInstance();
	UWacomGameUIManagerSubsystem* UIManager =
		GameInstance ? GameInstance->GetSubsystem<UWacomGameUIManagerSubsystem>() : nullptr;
	if (!UIManager)
	{
		return;
	}

	UCommonActivatableWidget* Pushed = UIManager->PushContentToLayer(
		WacomUITags::UI_Layer_Modal.GetTag(),
		UWacomKnockdownChoiceDialog::StaticClass());
	UWacomKnockdownChoiceDialog* Dialog = Cast<UWacomKnockdownChoiceDialog>(Pushed);
	if (!Dialog)
	{
		return;
	}

	Dialog->SetContext(&HUD, ChoiceView);
}

FWacomBattleHUDRuntime::FWacomBattleHUDRuntime(UBattleHUD& InHUD)
	: RuntimeHost(InHUD)
{
}

FWacomBattleHUDRuntime::~FWacomBattleHUDRuntime() = default;

void FWacomBattleHUDRuntime::NativeConstruct()
{
	if (!RuntimeHost.GetCardDetailPanelClass())
	{
		if (UClass* LoadedPanelClass = LoadClass<UWacomCardDetailPanel>(nullptr, CardDetailPanelPath))
		{
			RuntimeHost.SetCardDetailPanelClass(LoadedPanelClass);
		}
		else
		{
			RuntimeHost.SetCardDetailPanelClass(UWacomCardDetailPanel::StaticClass());
		}
	}

	RuntimeHost.RebuildChildBattleWidgets();
	if (UBattleSession* Session = GetSession())
	{
		RuntimeHost.RebuildChildBattleWidgets();
		NativeRefreshFromSnapshot(Session->BuildSnapshot());
	}
	else
	{
		GetCommandBarPresenter().RefreshEmpty();
	}
}

void FWacomBattleHUDRuntime::NativeDestruct()
{
	bBattleInputReady = true;
	bFirstPersonBattleHandSuppressedForEntry = false;
	if (PresentationCoordinator)
	{
		PresentationCoordinator->Shutdown();
		PresentationCoordinator.Reset();
	}
	ClearActionPreview();
	ClearFirstPersonBattleHandLayer();
	ClearBattleEnemyPartWorldTargets();
	ClearBattlePresentationTargetRegistry();
	HideCardDetailPanel();
	if (CardDetailController)
	{
		CardDetailController->RemoveFirstPersonPanelFromViewport();
	}
	ClearLastBattleSnapshot();
	GetFirstPersonHandBridge().ClearTransitionSnapshot();
	ClearPendingFirstPersonCardTransitionEvents();
	ClearBattleSceneEnemyPartHoverProbe(TEXT("HUDDestruct"));
}

void FWacomBattleHUDRuntime::NativeTick(float DeltaTime)
{
	TickCardDetailMotion(DeltaTime);
	TickBattleSceneEnemyPartHoverProbe(DeltaTime);
	GetFirstPersonHandBridge().TickPendingPresentationFrames(DeltaTime);
}

void FWacomBattleHUDRuntime::NativeRefreshFromSnapshot(
	const FBattleSnapshot& Snapshot)
{
	GetSnapshotPresenter().RefreshFromSnapshot(Snapshot);
	if (Snapshot.Phase == EBattlePhase::BattleEnd && !bHasBroadcastBattleEnd)
	{
		ClearActionPreview();
		bHasBroadcastBattleEnd = true;
		RuntimeHost.BroadcastBattleEnd(Snapshot.Outcome);
	}
}

void FWacomBattleHUDRuntime::NativeOnSessionChanged(
	UBattleSession* OldSession,
	UBattleSession* NewSession)
{
	if (OldSession != NewSession)
	{
		bBattleInputReady = true;
		bFirstPersonBattleHandSuppressedForEntry = false;
		ClearBattlePresentationQueue();
		ClearBattlePresentationStack();
		ClearPendingTurnBoundaryCommand();
		ClearFirstPersonBattleHandLayer();
		ClearActionPreview();
		SetBattleSceneEnemyHosts({});
		ClearBattleSceneEnemyPartHoverProbe(TEXT("SessionChanged"));
		ClearPendingFirstPersonCardTransitionEvents();
		ClearBattlePresentationTargetRegistry();
	}

	UIState = EBattleUIState::Idle;
	bBattleInputReady = true;
	bFirstPersonBattleHandSuppressedForEntry = false;
	PendingTargetingCardId.Invalidate();
	bHasBroadcastBattleEnd = false;
	ClearLastBattleSnapshot();
	GetFirstPersonHandBridge().ClearTransitionSnapshot();
	ClearPendingFirstPersonCardTransitionEvents();
	HideCardDetailPanel();
	GetCombatLogController().Clear();
	ClearBattlePresentationStack();
	ClearPendingTurnBoundaryCommand();
	ClearActionPreview();
	ClearBattleSceneEnemyPartHoverProbe(TEXT("SessionChanged"));

	if (NewSession)
	{
		ConsumeAndLogEvents();
		RefreshCommandBarFromSnapshot(NewSession->BuildSnapshot());
	}
	else
	{
		GetCommandBarPresenter().RefreshEmpty();
	}
}

void FWacomBattleHUDRuntime::NativeOnUIStateChanged(
	EBattleUIState NewState)
{
	if (NewState != EBattleUIState::Idle)
	{
		HideCardDetailPanel();
	}
	if (NewState == EBattleUIState::BattleEnd)
	{
		ClearActionPreview();
		ClearBattleSceneEnemyPartHoverProbe(TEXT("BattleEnd"));
	}

	UBattleSession* Session = GetSession();
	if (!Session)
	{
		return;
	}
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	if (Snapshot.Phase == EBattlePhase::BattleEnd)
	{
		ClearLastBattleSnapshot();
		GetFirstPersonHandBridge().ClearTransitionSnapshot();
	}
	else
	{
		SetLastBattleSnapshot(Snapshot);
	}
	RefreshCommandBarFromSnapshot(Snapshot);
	SyncFirstPersonBattleHandLayer(Snapshot);
	SyncBattleEnemyPartWorldTargets(Snapshot);
}

void FWacomBattleHUDRuntime::SetBattleInputReady(bool bReady)
{
	if (bBattleInputReady == bReady)
	{
		return;
	}

	bBattleInputReady = bReady;
	RefreshCommandBarFromCurrentSnapshot();
}

void FWacomBattleHUDRuntime::SetUIState(EBattleUIState NewState)
{
	if (UIState == NewState)
	{
		return;
	}
	const EBattleUIState OldState = UIState;
	UIState = NewState;
	RuntimeHost.NotifyUIStateChanged(OldState, NewState);
}

void FWacomBattleHUDRuntime::SetFirstPersonBattleHandSuppressedForEntry(bool bSuppressed)
{
	if (bFirstPersonBattleHandSuppressedForEntry == bSuppressed)
	{
		return;
	}

	bFirstPersonBattleHandSuppressedForEntry = bSuppressed;
	if (bFirstPersonBattleHandSuppressedForEntry)
	{
		GetFirstPersonHandBridge().SuppressLayerForEntry();
	}
}

void FWacomBattleHUDRuntime::SetLastBattleSnapshot(
	const FBattleSnapshot& Snapshot)
{
	LastBattleSnapshot = Snapshot;
	bHasLastBattleSnapshot = true;
}

void FWacomBattleHUDRuntime::ClearLastBattleSnapshot()
{
	LastBattleSnapshot = FBattleSnapshot();
	bHasLastBattleSnapshot = false;
}

FBattleTargetSelectionView FWacomBattleHUDRuntime::BuildTargetSelectionView() const
{
	return const_cast<FWacomBattleHUDRuntime*>(this)->GetTargetingController().BuildTargetSelectionView();
}

int32 FWacomBattleHUDRuntime::GetBattleCombatLogBlockCount() const
{
	return GetCombatLogController().GetBlockCount();
}

bool FWacomBattleHUDRuntime::IsBattlePresentationBusy() const
{
	return PresentationCoordinator && PresentationCoordinator->IsBusy();
}

bool FWacomBattleHUDRuntime::IsBattlePresentationPlanBusy() const
{
	return PresentationCoordinator && PresentationCoordinator->IsPresentationPlanBusy();
}

bool FWacomBattleHUDRuntime::CanSubmitPlayerActionCommand() const
{
	if (!bBattleInputReady)
	{
		return false;
	}
	if (PresentationCoordinator && PresentationCoordinator->IsPresentationPlanBusy())
	{
		return false;
	}
	if (UIState == EBattleUIState::BattleEnd)
	{
		return false;
	}
	if (HasPendingTurnBoundaryCommand())
	{
		return false;
	}

	const UBattleSession* CurrentSession = GetSession();
	if (!CurrentSession)
	{
		return false;
	}

	const FBattleSnapshot Snapshot = CurrentSession->BuildSnapshot();
	return Snapshot.Phase == EBattlePhase::PlayerAction;
}

bool FWacomBattleHUDRuntime::HasPendingTurnBoundaryCommand() const
{
	return PresentationCoordinator && PresentationCoordinator->HasPendingTurnBoundaryCommand();
}

FText FWacomBattleHUDRuntime::GetPendingTurnBoundaryCommandText() const
{
	return PresentationCoordinator
		? PresentationCoordinator->GetPendingTurnBoundaryCommandText()
		: FText::GetEmpty();
}

void FWacomBattleHUDRuntime::RefreshCommandBarFromSnapshot(
	const FBattleSnapshot& Snapshot)
{
	GetCommandBarPresenter().RefreshFromSnapshot(Snapshot);
}

void FWacomBattleHUDRuntime::RefreshCommandBarFromCurrentSnapshot()
{
	if (UBattleSession* Session = GetSession())
	{
		RefreshCommandBarFromSnapshot(Session->BuildSnapshot());
		return;
	}

	GetCommandBarPresenter().RefreshEmpty();
}

void FWacomBattleHUDRuntime::OnEnemyPartClickedByUser(
	const FWacomInteractionTargetHandle& TargetHandle)
{
	GetTargetingController().HandleEnemyPartClicked(TargetHandle);
}

void FWacomBattleHUDRuntime::OnWaitRequested()
{
	GetCommandController().SubmitWait();
}

void FWacomBattleHUDRuntime::OnEndTurnRequested()
{
	GetCommandController().SubmitEndTurn();
}

bool FWacomBattleHUDRuntime::TryStartFirstPersonBattleHandDragByIndex(
	int32 OneBasedIndex,
	const TOptional<FVector2D>& InitialPointerWidgetPosition)
{
	return GetFirstPersonHandBridge().TryStartDragByHandIndex(
		OneBasedIndex,
		InitialPointerWidgetPosition);
}

void FWacomBattleHUDRuntime::CancelTargetSelect()
{
	GetTargetingController().CancelTargetSelect();
}

void FWacomBattleHUDRuntime::OnKnockdownChoiceSelected(EKnockdownChoice Choice)
{
	GetCommandController().SubmitKnockdownChoice(Choice);
}

void FWacomBattleHUDRuntime::SubmitPlayCard(
	const FGuid& CardId,
	const FGuid& TargetPartId,
	const TOptional<FVector2D>& PresentationTargetWidgetPosition)
{
	GetCommandController().SubmitPlayCard(CardId, TargetPartId, PresentationTargetWidgetPosition);
}

void FWacomBattleHUDRuntime::SubmitPlayCardOnWorldTarget(
	const FGuid& CardId,
	const FWacomInteractionTargetHandle& TargetHandle,
	const TOptional<FVector2D>& PresentationTargetWidgetPosition)
{
	GetCommandController().SubmitPlayCardOnWorldTarget(
		CardId,
		TargetHandle,
		PresentationTargetWidgetPosition);
}

void FWacomBattleHUDRuntime::SubmitPlayCardOnHandCard(
	const FGuid& CardId,
	const FGuid& TargetCardId,
	const TOptional<FVector2D>& PresentationTargetWidgetPosition)
{
	GetCommandController().SubmitPlayCardOnHandCard(
		CardId,
		TargetCardId,
		PresentationTargetWidgetPosition);
}

void FWacomBattleHUDRuntime::AfterCommand()
{
	GetCommandController().AfterCommand();
}

void FWacomBattleHUDRuntime::ConsumeAndLogEvents()
{
	UBattleSession* Session = GetSession();
	if (!Session)
	{
		return;
	}

	const TArray<FBattleEvent> Events = Session->ConsumeEvents();
	(void)Session->ConsumePresentationJournal();
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FWacomBattleCombatLogCommandContext SystemContext =
		UWacomBattleCombatLogBuilder::BuildSystemCommandContext(Snapshot);
	LogRawBattleEvents(Events);
	StoreFirstPersonCardTransitionEvents(Events);
	GetCombatLogController().AppendBlock(SystemContext, Events, Snapshot, Snapshot);
	EnqueueBattlePresentationEvents(Events, INDEX_NONE);
}

bool FWacomBattleHUDRuntime::ConsumeAndLogEvents(
	const FWacomBattleCombatLogCommandContext& CommandContext,
	const FBattleSnapshot& PreCommandSnapshot,
	const FBattleResolution& Resolution)
{
	const TArray<FBattleEvent>& Events = Resolution.Events;
	const FBattlePresentationJournal& PresentationJournal = Resolution.PresentationJournal;
	const FBattleSnapshot& PostCommandSnapshot = Resolution.PostSnapshot;
	LogRawBattleEvents(Events);
	GetCombatLogController().AppendBlock(
		CommandContext,
		Events,
		PreCommandSnapshot,
		PostCommandSnapshot);
	if (CommandContext.CommandKind == EWacomBattleCombatLogCommandKind::EndTurn
		&& GetPresentationCoordinator().EnqueueEndTurnPresentationPlan(
			PresentationJournal,
			Events,
			PostCommandSnapshot))
	{
		return true;
	}

	StoreFirstPersonCardTransitionEvents(Events);
	const int32 PresentationStackEntryId =
		CommandContext.CommandKind == EWacomBattleCombatLogCommandKind::PlayCard
			? AppendBattlePresentationStackEntry(CommandContext, PreCommandSnapshot)
			: INDEX_NONE;
	EnqueueBattlePresentationEvents(Events, PresentationStackEntryId);
	return false;
}

FWacomBattleHUDCardDetailController& FWacomBattleHUDRuntime::GetCardDetailController()
{
	if (!CardDetailController)
	{
		CardDetailController = MakeShared<FWacomBattleHUDCardDetailController>(*this);
	}
	return *CardDetailController;
}

const FWacomBattleHUDCardDetailController& FWacomBattleHUDRuntime::GetCardDetailController() const
{
	return const_cast<FWacomBattleHUDRuntime*>(this)->GetCardDetailController();
}

FWacomBattleHUDCombatLogController& FWacomBattleHUDRuntime::GetCombatLogController()
{
	if (!CombatLogController)
	{
		CombatLogController = MakeShared<FWacomBattleHUDCombatLogController>(*this);
	}
	return *CombatLogController;
}

const FWacomBattleHUDCombatLogController& FWacomBattleHUDRuntime::GetCombatLogController() const
{
	return const_cast<FWacomBattleHUDRuntime*>(this)->GetCombatLogController();
}

FWacomBattleHUDFirstPersonHandBridge& FWacomBattleHUDRuntime::GetFirstPersonHandBridge()
{
	if (!FirstPersonHandBridge)
	{
		FirstPersonHandBridge = MakeShared<FWacomBattleHUDFirstPersonHandBridge>(*this);
	}
	return *FirstPersonHandBridge;
}

const FWacomBattleHUDFirstPersonHandBridge& FWacomBattleHUDRuntime::GetFirstPersonHandBridge() const
{
	return const_cast<FWacomBattleHUDRuntime*>(this)->GetFirstPersonHandBridge();
}

FWacomBattleHUDPresentationCoordinator& FWacomBattleHUDRuntime::GetPresentationCoordinator()
{
	if (!PresentationCoordinator)
	{
		PresentationCoordinator = MakeShared<FWacomBattleHUDPresentationCoordinator>(*this);
	}
	return *PresentationCoordinator;
}

const FWacomBattleHUDPresentationCoordinator& FWacomBattleHUDRuntime::GetPresentationCoordinator() const
{
	return const_cast<FWacomBattleHUDRuntime*>(this)->GetPresentationCoordinator();
}

FWacomBattleHUDSceneEnemyTargetCoordinator& FWacomBattleHUDRuntime::GetSceneEnemyTargetCoordinator()
{
	if (!SceneEnemyTargetCoordinator)
	{
		SceneEnemyTargetCoordinator = MakeShared<FWacomBattleHUDSceneEnemyTargetCoordinator>(*this);
	}
	return *SceneEnemyTargetCoordinator;
}

const FWacomBattleHUDSceneEnemyTargetCoordinator& FWacomBattleHUDRuntime::GetSceneEnemyTargetCoordinator() const
{
	return const_cast<FWacomBattleHUDRuntime*>(this)->GetSceneEnemyTargetCoordinator();
}

FWacomBattleHUDCommandController& FWacomBattleHUDRuntime::GetCommandController()
{
	if (!CommandController)
	{
		CommandController = MakeUnique<FWacomBattleHUDCommandController>(*this);
	}
	return *CommandController;
}

FWacomBattleHUDCommandBarPresenter& FWacomBattleHUDRuntime::GetCommandBarPresenter()
{
	if (!CommandBarPresenter)
	{
		CommandBarPresenter = MakeUnique<FWacomBattleHUDCommandBarPresenter>(*this);
	}
	return *CommandBarPresenter;
}

FWacomBattleHUDTargetingController& FWacomBattleHUDRuntime::GetTargetingController()
{
	if (!TargetingController)
	{
		TargetingController = MakeUnique<FWacomBattleHUDTargetingController>(*this);
	}
	return *TargetingController;
}

FWacomBattleHUDSnapshotPresenter& FWacomBattleHUDRuntime::GetSnapshotPresenter()
{
	if (!SnapshotPresenter)
	{
		SnapshotPresenter = MakeUnique<FWacomBattleHUDSnapshotPresenter>(*this);
	}
	return *SnapshotPresenter;
}

void FWacomBattleHUDRuntime::HideCardDetailPanel()
{
	GetCardDetailController().HideAll();
}

void FWacomBattleHUDRuntime::HideFirstPersonCardDetailPanelForSource(const FGuid& CardInstanceId)
{
	GetCardDetailController().HideFirstPersonForSource(CardInstanceId);
}

bool FWacomBattleHUDRuntime::IsFirstPersonCardInspectDetailActiveForSource(
	const FGuid& CardInstanceId) const
{
	return GetCardDetailController().IsFirstPersonInspectDetailActiveForSource(CardInstanceId);
}

UWacomCardDetailPanel* FWacomBattleHUDRuntime::EnsureFirstPersonCardDetailPanel()
{
	return GetCardDetailController().EnsureFirstPersonPanel();
}

bool FWacomBattleHUDRuntime::ShowFirstPersonCardDetailAtSlot(
	const FWacomCardDetailViewData& DetailData,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	return GetCardDetailController().ShowFirstPersonAtSlot(DetailData, SlotView);
}

void FWacomBattleHUDRuntime::PositionFirstPersonCardDetailPanelBesideSlot(
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetCardDetailController().PositionFirstPersonBesideSlot(SlotView);
}

void FWacomBattleHUDRuntime::HideFirstPersonCardDetailPanel()
{
	GetCardDetailController().HideFirstPerson();
}

void FWacomBattleHUDRuntime::TickCardDetailMotion(float DeltaTime)
{
	GetCardDetailController().TickMotion(DeltaTime);
}

void FWacomBattleHUDRuntime::ForceHideCardDetailHost(EWacomBattleHUDCardDetailHost Host)
{
	GetCardDetailController().ForceHideHost(Host);
}

bool FWacomBattleHUDRuntime::ComputeFirstPersonCardDetailTarget(
	const FWacomFirstPersonCardLayerSlotView& SlotView,
	FVector2D& OutPosition)
{
	return GetCardDetailController().ComputeFirstPersonTarget(SlotView, OutPosition);
}

FVector2D FWacomBattleHUDRuntime::ComputeCardDetailPanelPositionBesideStable(
	const FVector2D& AnchorPosition,
	const FVector2D& AnchorSize,
	const FVector2D& LayerSize,
	const FVector2D& PanelSize,
	float DetailPadding)
{
	return GetCardDetailController().ComputeStablePosition(
		AnchorPosition,
		AnchorSize,
		LayerSize,
		PanelSize,
		DetailPadding);
}

FVector2D FWacomBattleHUDRuntime::GetFirstPersonCardDetailViewportSize() const
{
	return GetCardDetailController().GetFirstPersonViewportSize();
}

void FWacomBattleHUDRuntime::SetFirstPersonCardDetailSource(const FGuid& CardInstanceId)
{
	GetCardDetailController().SetFirstPersonSource(CardInstanceId);
}

void FWacomBattleHUDRuntime::ClearFirstPersonCardDetailSource()
{
	GetCardDetailController().ClearFirstPersonSource();
}

bool FWacomBattleHUDRuntime::IsCurrentFirstPersonCardDetailSource(const FGuid& CardInstanceId) const
{
	return GetCardDetailController().IsCurrentFirstPersonSource(CardInstanceId);
}

void FWacomBattleHUDRuntime::UpdateFirstPersonCardDetailSlot(
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetCardDetailController().UpdateFirstPersonSlot(SlotView);
}

FVector2D FWacomBattleHUDRuntime::GetLastFirstPersonCardDetailPanelPosition() const
{
	return GetCardDetailController().GetLastFirstPersonPanelPosition();
}

void FWacomBattleHUDRuntime::AppendBattleCombatLogBlock(
	const FWacomBattleCombatLogBlockView& Block)
{
	GetCombatLogController().AppendBlock(Block);
}

void FWacomBattleHUDRuntime::StoreFirstPersonCardTransitionEvents(
	const TArray<FBattleEvent>& Events)
{
	GetFirstPersonHandBridge().StoreTransitionEvents(Events);
}

void FWacomBattleHUDRuntime::ClearPendingFirstPersonCardTransitionEvents()
{
	GetFirstPersonHandBridge().ClearPendingTransitionEvents();
}

void FWacomBattleHUDRuntime::RecordFirstPersonPlayCommit(
	const FGuid& CardInstanceId,
	const FBattlePartSlotIdentity& TargetPartKey,
	const TOptional<FVector2D>& TargetWidgetPosition)
{
	GetFirstPersonHandBridge().RecordPlayCommit(
		CardInstanceId,
		TargetPartKey,
		TargetWidgetPosition);
}

TArray<FWacomFirstPersonCardLayerTransitionHint>
FWacomBattleHUDRuntime::BuildFirstPersonCardTransitionHints(
	const FBattleSnapshot& PreviousSnapshot,
	const FBattleSnapshot& NextSnapshot) const
{
	return GetFirstPersonHandBridge().BuildTransitionHints(PreviousSnapshot, NextSnapshot);
}

TArray<FWacomFirstPersonCardLayerFeedbackHint>
FWacomBattleHUDRuntime::BuildFirstPersonCardFeedbackHints(
	const FBattleSnapshot& NextSnapshot) const
{
	return GetFirstPersonHandBridge().BuildFeedbackHints(NextSnapshot);
}

int32 FWacomBattleHUDRuntime::AppendBattlePresentationStackEntry(
	const FWacomBattleCombatLogCommandContext& CommandContext,
	const FBattleSnapshot& PreCommandSnapshot)
{
	return GetPresentationCoordinator().AppendStackEntry(CommandContext, PreCommandSnapshot);
}

void FWacomBattleHUDRuntime::BeginBattlePresentationStackEntryExit(int32 EntryId)
{
	GetPresentationCoordinator().BeginStackEntryExit(EntryId);
}

void FWacomBattleHUDRuntime::FinishBattlePresentationStackEntryExit(int32 EntryId)
{
	GetPresentationCoordinator().FinishStackEntryExit(EntryId);
}

void FWacomBattleHUDRuntime::ClearBattlePresentationStack()
{
	if (PresentationCoordinator)
	{
		PresentationCoordinator->ClearStack();
	}
}

bool FWacomBattleHUDRuntime::HasBattlePresentationStackEntries() const
{
	return PresentationCoordinator && PresentationCoordinator->HasStackEntries();
}

void FWacomBattleHUDRuntime::EnqueueBattlePresentationEvents(
	const TArray<FBattleEvent>& Events,
	int32 PresentationStackEntryId)
{
	GetPresentationCoordinator().EnqueueEvents(Events, PresentationStackEntryId);
}

void FWacomBattleHUDRuntime::ClearBattlePresentationQueue()
{
	if (PresentationCoordinator)
	{
		PresentationCoordinator->ClearQueue();
	}
}

bool FWacomBattleHUDRuntime::IsBattlePresentationQueueBusy() const
{
	return PresentationCoordinator && PresentationCoordinator->IsQueueBusy();
}

void FWacomBattleHUDRuntime::QueuePendingTurnBoundaryCommand(
	EWacomBattleHUDTurnBoundaryCommand Command)
{
	GetPresentationCoordinator().QueuePendingTurnBoundaryCommand(Command);
}

void FWacomBattleHUDRuntime::ClearPendingTurnBoundaryCommand()
{
	if (PresentationCoordinator)
	{
		PresentationCoordinator->ClearPendingTurnBoundaryCommand();
	}
}

void FWacomBattleHUDRuntime::TryExecutePendingTurnBoundaryCommand()
{
	if (PresentationCoordinator)
	{
		PresentationCoordinator->TryExecutePendingTurnBoundaryCommand();
	}
}

FWacomBattlePresentationTargetRegistry& FWacomBattleHUDRuntime::GetBattlePresentationTargetRegistry()
{
	if (!BattlePresentationTargetRegistry)
	{
		BattlePresentationTargetRegistry = MakeShared<FWacomBattlePresentationTargetRegistry>();
	}
	return *BattlePresentationTargetRegistry;
}

void FWacomBattleHUDRuntime::ClearBattlePresentationTargetRegistry()
{
	if (BattlePresentationTargetRegistry)
	{
		BattlePresentationTargetRegistry->Clear();
		BattlePresentationTargetRegistry.Reset();
	}
}

void FWacomBattleHUDRuntime::RegisterBattlePresentationTarget(
	const FBattlePartSlotIdentity& TargetPartKey,
	UObject* Owner,
	TFunction<void(const FWacomBattlePresentationTargetCue&)> Handler)
{
	GetBattlePresentationTargetRegistry().Register(TargetPartKey, Owner, MoveTemp(Handler));
}

void FWacomBattleHUDRuntime::UnregisterBattlePresentationTargetsForOwner(const UObject* Owner)
{
	if (BattlePresentationTargetRegistry)
	{
		BattlePresentationTargetRegistry->UnregisterOwner(Owner);
	}
}

bool FWacomBattleHUDRuntime::IsBattlePresentationTargetRegisteredForOwner(const UObject* Owner) const
{
	return BattlePresentationTargetRegistry
		&& BattlePresentationTargetRegistry->ContainsOwner(Owner);
}

void FWacomBattleHUDRuntime::PlayBattlePresentationCue(
	const FWacomBattlePresentationTargetCue& Cue)
{
	if (BattlePresentationTargetRegistry)
	{
		BattlePresentationTargetRegistry->PlayCue(Cue);
	}
}

void FWacomBattleHUDRuntime::PushPendingKnockdownChoiceDialog()
{
	UBattleSession* CurrentSession = GetSession();
	if (!CurrentSession)
	{
		return;
	}

	const FKnockdownChoiceView ChoiceView = CurrentSession->BuildPendingKnockdownChoiceView();
	if (!ChoiceView.bHasPendingChoice)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleHUD] KnockdownChoiceRequested presentation step has no pending choice view"));
		return;
	}

	RuntimeHost.PushKnockdownChoiceDialog(ChoiceView);
}

void FWacomBattleHUDRuntime::AdvanceBattlePresentationQueueOnce()
{
#if WITH_AUTOMATION_TESTS
	GetPresentationCoordinator().AdvanceQueueOnce();
	GetPresentationCoordinator().AdvancePresentationPlanOnce();
#endif
}

void FWacomBattleHUDRuntime::SetBattleSceneEnemyHosts(
	const TArray<AWacomBattleEnemyActor*>& InHosts)
{
	GetSceneEnemyTargetCoordinator().SetSceneEnemyHosts(InHosts);
}

bool FWacomBattleHUDRuntime::IsBattleSceneEnemyHostInCurrentRegistry(
	const AWacomBattleEnemyActor* Host) const
{
	return GetSceneEnemyTargetCoordinator().IsSceneEnemyHostInCurrentRegistry(Host);
}

bool FWacomBattleHUDRuntime::IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(
	const FWacomInteractionTargetHandle& TargetHandle) const
{
	return GetSceneEnemyTargetCoordinator().IsWorldTargetInCurrentRegistry(TargetHandle);
}

void FWacomBattleHUDRuntime::RebuildBattleSceneEnemyPartWorldTargetRegistry()
{
	GetSceneEnemyTargetCoordinator().RebuildRegistry();
}

bool FWacomBattleHUDRuntime::IsBattleSceneEnemyPartBridgeInCurrentRegistry(
	const UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge) const
{
	return GetSceneEnemyTargetCoordinator().IsBridgeInCurrentRegistry(Bridge);
}

void FWacomBattleHUDRuntime::SyncBattleEnemyPartWorldTargets(
	const FBattleSnapshot& Snapshot)
{
	GetSceneEnemyTargetCoordinator().SyncWorldTargets(Snapshot);
}

void FWacomBattleHUDRuntime::ClearBattleEnemyPartWorldTargets()
{
	GetSceneEnemyTargetCoordinator().ClearWorldTargets();
}

bool FWacomBattleHUDRuntime::CanUpdateBattleSceneEnemyPartHoverProbe() const
{
	return GetSceneEnemyTargetCoordinator().CanUpdateHoverProbe();
}

void FWacomBattleHUDRuntime::ApplyActionPreviewPresentation(
	const FWacomBattleActionPreviewPresentation& Presentation,
	const bool bApplyScenePartPreview)
{
	ClearActionPreview();
	if (!Presentation.bHasPreview)
	{
		return;
	}

	if (Presentation.bHasProjectedPlayer)
	{
		if (UPlayerStatusBar* PlayerStatusBar = Host().GetPlayerStatusBar())
		{
			PlayerStatusBar->SetActionPreview(Presentation.ProjectedPlayer);
		}
	}

	GetSceneEnemyTargetCoordinator().ApplyActionPreviewToEnemyPanels(
		Presentation.ProjectedEnemyParts,
		bApplyScenePartPreview);
}

void FWacomBattleHUDRuntime::ClearActionPreview()
{
	if (UPlayerStatusBar* PlayerStatusBar = Host().GetPlayerStatusBar())
	{
		PlayerStatusBar->ClearActionPreview();
	}
	GetSceneEnemyTargetCoordinator().ClearActionPreviewFromEnemyPanels();
}

FWacomBattleEnemyPartDragPredictionDebugInput
FWacomBattleHUDRuntime::BuildBattleSceneEnemyPartHoverPredictionInput(
	const FWacomInteractionTargetHandle& TargetHandle) const
{
	return GetSceneEnemyTargetCoordinator().BuildHoverPredictionInput(TargetHandle);
}

void FWacomBattleHUDRuntime::TickBattleSceneEnemyPartHoverProbe(float DeltaTime)
{
	GetSceneEnemyTargetCoordinator().TickHoverProbe(DeltaTime);
}

void FWacomBattleHUDRuntime::UpdateBattleSceneEnemyPartHoverProbe()
{
	GetSceneEnemyTargetCoordinator().UpdateHoverProbe();
}

void FWacomBattleHUDRuntime::ClearBattleSceneEnemyPartHoverProbe(FName Reason)
{
	GetSceneEnemyTargetCoordinator().ClearHoverProbe(Reason);
}

UWacomFirstPersonCardAnchorComponent* FWacomBattleHUDRuntime::ResolveFirstPersonCardAnchor() const
{
	return GetFirstPersonHandBridge().ResolveAnchor();
}

UWacomFirstPersonCardAnchorComponent* FWacomBattleHUDRuntime::ResolveActiveFirstPersonCardAnchor() const
{
	return GetFirstPersonHandBridge().ResolveActiveAnchor();
}

void FWacomBattleHUDRuntime::SyncFirstPersonBattleHandLayer(
	const FBattleSnapshot& Snapshot)
{
	GetFirstPersonHandBridge().SyncLayer(Snapshot);
}

void FWacomBattleHUDRuntime::SyncFirstPersonBattleHandLayer(
	const FBattleSnapshot& Snapshot,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints)
{
	GetFirstPersonHandBridge().SyncLayer(Snapshot, TransitionHints);
}

void FWacomBattleHUDRuntime::SyncFirstPersonBattleHandLayer(
	const FBattleSnapshot& Snapshot,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints,
	const TArray<FWacomFirstPersonCardLayerFeedbackHint>& FeedbackHints)
{
	GetFirstPersonHandBridge().SyncLayer(Snapshot, TransitionHints, FeedbackHints);
}

void FWacomBattleHUDRuntime::RefreshFromPresentationPhase(
	const FBattleSnapshot& Snapshot,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints,
	const TArray<FWacomFirstPersonCardLayerFeedbackHint>& FeedbackHints)
{
	GetSnapshotPresenter().RefreshFromPresentationPhase(Snapshot, TransitionHints, FeedbackHints);
}

void FWacomBattleHUDRuntime::ClearFirstPersonBattleHandLayer()
{
	GetFirstPersonHandBridge().ClearLayer();
}

bool FWacomBattleHUDRuntime::ShouldUseFirstPersonBattleHandLayer() const
{
	return GetFirstPersonHandBridge().ShouldUseFirstPersonBattleHandLayer();
}

bool FWacomBattleHUDRuntime::ShouldEnableFirstPersonBattleHandInteraction() const
{
	return GetFirstPersonHandBridge().ShouldEnableFirstPersonBattleHandInteraction();
}

void FWacomBattleHUDRuntime::BindFirstPersonBattleHandLayerInteractions(
	UWacomFirstPersonCardAnchorComponent* Anchor)
{
	GetFirstPersonHandBridge().BindLayerInteractions(Anchor);
}

void FWacomBattleHUDRuntime::UnbindFirstPersonBattleHandLayerInteractions(
	UWacomFirstPersonCardAnchorComponent* Anchor)
{
	GetFirstPersonHandBridge().UnbindLayerInteractions(Anchor);
}

void FWacomBattleHUDRuntime::HandleFirstPersonCardLayerCardHovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetFirstPersonHandBridge().HandleCardHovered(CardInstanceId, SlotView);
}

void FWacomBattleHUDRuntime::HandleFirstPersonCardLayerCardUnhovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetFirstPersonHandBridge().HandleCardUnhovered(CardInstanceId, SlotView);
}

void FWacomBattleHUDRuntime::HandleFirstPersonCardLayerHoveredCardLayoutUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetFirstPersonHandBridge().HandleHoveredCardLayoutUpdated(CardInstanceId, SlotView);
}

void FWacomBattleHUDRuntime::HandleFirstPersonCardLayerCardTargetHovered(
	const FWacomInteractionTargetHandle& CardTargetHandle,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetFirstPersonHandBridge().HandleCardTargetHovered(CardTargetHandle, SlotView);
}

void FWacomBattleHUDRuntime::HandleFirstPersonCardLayerCardTargetUnhovered(
	const FWacomInteractionTargetHandle& CardTargetHandle,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetFirstPersonHandBridge().HandleCardTargetUnhovered(CardTargetHandle, SlotView);
}

void FWacomBattleHUDRuntime::HandleFirstPersonCardLayerHoveredCardTargetUpdated(
	const FWacomInteractionTargetHandle& CardTargetHandle,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetFirstPersonHandBridge().HandleHoveredCardTargetUpdated(CardTargetHandle, SlotView);
}

void FWacomBattleHUDRuntime::HandleFirstPersonCardLayerDragStarted(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	GetFirstPersonHandBridge().HandleDragStarted(CardInstanceId, DragView);
}

void FWacomBattleHUDRuntime::HandleFirstPersonCardLayerDragUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	GetFirstPersonHandBridge().HandleDragUpdated(CardInstanceId, DragView);
}

void FWacomBattleHUDRuntime::HandleFirstPersonCardLayerDragReleased(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	GetFirstPersonHandBridge().HandleDragReleased(CardInstanceId, DragView);
}

void FWacomBattleHUDRuntime::HandleFirstPersonCardLayerDragCancelled(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	GetFirstPersonHandBridge().HandleDragCancelled(CardInstanceId, DragView);
}

void FWacomBattleHUDRuntime::HandleFirstPersonCardLayerPointerMoved(
	const FWacomFirstPersonCardPointerView& PointerView)
{
	GetFirstPersonHandBridge().HandlePointerMoved(PointerView);
}

void FWacomBattleHUDRuntime::HandleFirstPersonCardLayerPointerLeft()
{
	GetFirstPersonHandBridge().HandlePointerLeft();
}

void FWacomBattleHUDRuntime::ApplyFirstPersonCardDragCameraLookOverride(
	const FWacomFirstPersonCardDragView& DragView)
{
	GetFirstPersonHandBridge().ApplyDragCameraLookOverride(DragView);
}

void FWacomBattleHUDRuntime::ClearFirstPersonCardDragCameraLookOverride()
{
	GetFirstPersonHandBridge().ClearDragCameraLookOverride();
}

void FWacomBattleHUDRuntime::UpdateFirstPersonCardDragTargetFeedback(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	GetFirstPersonHandBridge().UpdateDragTargetFeedback(CardInstanceId, DragView);
}

void FWacomBattleHUDRuntime::ClearFirstPersonCardDragTargetFeedback()
{
	GetFirstPersonHandBridge().ClearDragTargetFeedback();
}

bool FWacomBattleHUDRuntime::IsFirstPersonCardDragActiveForBattleSceneHover() const
{
	return GetFirstPersonHandBridge().IsFirstPersonCardDragActiveForBattleSceneHover();
}

FWacomBattleCardDropResolveResult FWacomBattleHUDRuntime::ResolveFirstPersonCardDropIntent(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView) const
{
	return GetFirstPersonHandBridge().ResolveDropIntent(CardInstanceId, DragView);
}

TArray<FWacomFirstPersonCardTargetAffordance>
FWacomBattleHUDRuntime::BuildFirstPersonCardTargetAffordances(
	const FGuid& SourceCardId,
	const FBattleSnapshot& Snapshot,
	const UBattleSession& BattleSession) const
{
	return GetFirstPersonHandBridge().BuildCardTargetAffordances(SourceCardId, Snapshot, BattleSession);
}

UWacomBattleEnemyPartWorldTargetBridgeComponent*
FWacomBattleHUDRuntime::ResolveBattleEnemyPartWorldTargetBridge(
	const FWacomInteractionTargetHandle& TargetHandle) const
{
	return GetSceneEnemyTargetCoordinator().ResolveWorldTargetBridge(TargetHandle);
}

UWacomBattleEnemyPartPresentationComponent*
FWacomBattleHUDRuntime::ResolveBattleEnemyPartWorldTargetPresentation(
	const FWacomInteractionTargetHandle& TargetHandle) const
{
	return GetSceneEnemyTargetCoordinator().ResolveWorldTargetPresentation(TargetHandle);
}

bool FWacomBattleHUDRuntime::ProbeFirstPersonCardDragTarget(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	FWacomInteractionTargetHandle& OutTargetHandle,
	bool& bOutValidTarget) const
{
	return GetFirstPersonHandBridge().ProbeDragTarget(CardInstanceId, DragView, OutTargetHandle, bOutValidTarget);
}

bool FWacomBattleHUDRuntime::ShouldShowFirstPersonDragInspectDetail(
	const FWacomFirstPersonCardDragView& DragView) const
{
	return GetFirstPersonHandBridge().ShouldShowDragInspectDetail(DragView);
}

#if WITH_AUTOMATION_TESTS
void FWacomBattleHUDRuntime::PlayBattlePresentationCueForTest(
	EBattleEventType SourceEventType,
	const FBattlePartSlotIdentity& TargetPartKey,
	int32 Amount)
{
	FWacomBattlePresentationTargetCue Cue;
	Cue.SourceEventType = SourceEventType;
	Cue.TargetPartKey = TargetPartKey;
	Cue.Amount = Amount;
	PlayBattlePresentationCue(Cue);
}

void FWacomBattleHUDRuntime::PlayTargetConfirmedCueForTest(
	const FBattlePartSlotIdentity& TargetPartKey)
{
	FWacomBattlePresentationTargetCue Cue;
	Cue.CueKind = EWacomBattlePresentationTargetCueKind::TargetConfirmed;
	Cue.TargetPartKey = TargetPartKey;
	Cue.Duration = 0.10f;
	PlayBattlePresentationCue(Cue);
}

bool FWacomBattleHUDRuntime::EnqueueEndTurnPresentationPlanForTest(
	const FBattlePresentationJournal& Journal,
	const TArray<FBattleEvent>& Events,
	const FBattleSnapshot& PostCommandSnapshot)
{
	return GetPresentationCoordinator().EnqueueEndTurnPresentationPlan(
		Journal,
		Events,
		PostCommandSnapshot);
}

FWacomBattleHUDAutomationTestView FWacomBattleHUDRuntime::GetAutomationTestViewForTest() const
{
	static const TArray<FWacomBattlePresentationStackEntryView> EmptyEntries;
	static const TArray<FWacomBattleCombatLogBlockView> EmptyHistory;

	FWacomBattleHUDAutomationTestView View;
	View.PresentationTargetCount = BattlePresentationTargetRegistry ? BattlePresentationTargetRegistry->Num() : 0;
	View.SceneEnemyPartWorldTargetBridgeCount = GetSceneEnemyTargetCoordinator().GetRegisteredBridgeCount();
	View.PresentationStackEntries = PresentationCoordinator ? &PresentationCoordinator->GetStackEntries() : &EmptyEntries;
	View.CombatLogHistory = CombatLogController ? &CombatLogController->GetHistory() : &EmptyHistory;
	View.bHasLastBattleSnapshot = bHasLastBattleSnapshot;
	View.LastBattleSnapshotHandCount = bHasLastBattleSnapshot ? LastBattleSnapshot.Hand.Cards.Num() : 0;
	if (PresentationCoordinator)
	{
		View.bPresentationPlanActive = PresentationCoordinator->IsPresentationPlanBusy();
		View.PresentationPlanPendingPhaseCount = PresentationCoordinator->GetPendingPresentationPlanPhaseCount();
		View.ActivePresentationPlanPhaseName = PresentationCoordinator->GetActivePresentationPlanPhaseName();
		View.PresentationPlanStartedPhaseNames =
			&PresentationCoordinator->GetStartedPresentationPlanPhaseNamesForTest();
	}
	return View;
}

TArray<FWacomFirstPersonCardLayerTransitionHint>
FWacomBattleHUDRuntime::BuildFirstPersonCardTransitionHintsForRefreshForTest(
	const FBattleSnapshot& NextSnapshot) const
{
	return GetFirstPersonHandBridge().BuildTransitionHintsForRefresh(NextSnapshot);
}

void FWacomBattleHUDRuntime::SetFirstPersonCardTransitionSnapshotForTest(
	const FBattleSnapshot& Snapshot)
{
	GetFirstPersonHandBridge().SetTransitionSnapshot(Snapshot);
}
#endif

#undef LOCTEXT_NAMESPACE
