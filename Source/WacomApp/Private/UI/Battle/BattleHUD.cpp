// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattleHUD.h"

#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "GameFramework/WacomPlayerController.h"
#include "Session/BattleSession.h"
#include "InputCoreTypes.h"
#include "Input/CommonUIInputSettings.h"
#include "UI/Battle/BattleCommandBarWidget.h"
#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/Battle/BattleHUDFallbackLayoutBuilder.h"
#include "UI/Battle/BattlePresentationStackWidget.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/Battle/WacomBattleHUDCardDetailController.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"
#include "UI/Battle/WacomBattleHUDEnemyInspectionCoordinator.h"
#include "UI/Battle/WacomBattleHUDResultApplicator.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Common/PileCountView.h"

#include "Blueprint/WidgetTree.h"

namespace
{
	const TCHAR* LexToString(EWacomBattleCardDropIntentKind IntentKind)
	{
		switch (IntentKind)
		{
		case EWacomBattleCardDropIntentKind::None: return TEXT("None");
		case EWacomBattleCardDropIntentKind::PlayCardNoTarget: return TEXT("PlayCardNoTarget");
		case EWacomBattleCardDropIntentKind::PlayCardWorldTarget: return TEXT("PlayCardWorldTarget");
		case EWacomBattleCardDropIntentKind::PlayCardCardTarget: return TEXT("PlayCardCardTarget");
		case EWacomBattleCardDropIntentKind::ProbeCardTarget: return TEXT("ProbeCardTarget");
		case EWacomBattleCardDropIntentKind::Reject: return TEXT("Reject");
		default: return TEXT("Unknown");
		}
	}

	const TCHAR* LexToString(EWacomBattleCardDropRejectReason RejectReason)
	{
		switch (RejectReason)
		{
		case EWacomBattleCardDropRejectReason::None: return TEXT("None");
		case EWacomBattleCardDropRejectReason::UIBlocked: return TEXT("UIBlocked");
		case EWacomBattleCardDropRejectReason::MissingSession: return TEXT("MissingSession");
		case EWacomBattleCardDropRejectReason::SourceCardInvalid: return TEXT("SourceCardInvalid");
		case EWacomBattleCardDropRejectReason::SourceCardNotPlayable: return TEXT("SourceCardNotPlayable");
		case EWacomBattleCardDropRejectReason::NotArmed: return TEXT("NotArmed");
		case EWacomBattleCardDropRejectReason::MissingTarget: return TEXT("MissingTarget");
		case EWacomBattleCardDropRejectReason::InvalidWorldTarget: return TEXT("InvalidWorldTarget");
		case EWacomBattleCardDropRejectReason::UnsupportedCardTarget: return TEXT("UnsupportedCardTarget");
		case EWacomBattleCardDropRejectReason::UnsupportedZoneTarget: return TEXT("UnsupportedZoneTarget");
		case EWacomBattleCardDropRejectReason::SelfTarget: return TEXT("SelfTarget");
		default: return TEXT("Unknown");
		}
	}

	const TCHAR* LexToString(EWacomBattleTargetRejectReason RejectReason)
	{
		switch (RejectReason)
		{
		case EWacomBattleTargetRejectReason::None: return TEXT("None");
		case EWacomBattleTargetRejectReason::InvalidTarget: return TEXT("InvalidTarget");
		case EWacomBattleTargetRejectReason::SourceCardInvalid: return TEXT("SourceCardInvalid");
		case EWacomBattleTargetRejectReason::SourceCardNotInHand: return TEXT("SourceCardNotInHand");
		case EWacomBattleTargetRejectReason::SourceCardMissingDefinition: return TEXT("SourceCardMissingDefinition");
		case EWacomBattleTargetRejectReason::SourceCardFrozen: return TEXT("SourceCardFrozen");
		case EWacomBattleTargetRejectReason::UnsupportedWorldTarget: return TEXT("UnsupportedWorldTarget");
		case EWacomBattleTargetRejectReason::InvalidWorldTarget: return TEXT("InvalidWorldTarget");
		case EWacomBattleTargetRejectReason::UnsupportedCardTarget: return TEXT("UnsupportedCardTarget");
		case EWacomBattleTargetRejectReason::TargetCardInvalid: return TEXT("TargetCardInvalid");
		case EWacomBattleTargetRejectReason::TargetCardNotInHand: return TEXT("TargetCardNotInHand");
		case EWacomBattleTargetRejectReason::SelfTarget: return TEXT("SelfTarget");
		case EWacomBattleTargetRejectReason::UnsupportedNormalHandCardTarget: return TEXT("UnsupportedNormalHandCardTarget");
		case EWacomBattleTargetRejectReason::UnsupportedHandAnchorTarget: return TEXT("UnsupportedHandAnchorTarget");
		case EWacomBattleTargetRejectReason::MissingRequiredTargetKeyword: return TEXT("MissingRequiredTargetKeyword");
		case EWacomBattleTargetRejectReason::BlockedTargetKeyword: return TEXT("BlockedTargetKeyword");
		case EWacomBattleTargetRejectReason::UnsupportedZoneTarget: return TEXT("UnsupportedZoneTarget");
		case EWacomBattleTargetRejectReason::TargetIdentityMismatch: return TEXT("TargetIdentityMismatch");
		case EWacomBattleTargetRejectReason::NotEnoughInitiative: return TEXT("NotEnoughInitiative");
		default: return TEXT("Unknown");
		}
	}
}

FString FWacomBattleCardDropResolveResult::ToDebugString() const
{
	return FString::Printf(
		TEXT("CardDrop{Source=%s Intent=%s Reject=%s CanSubmit=%s Target=%s HasPos=%s Pos=%s ValidationReject=%s Validation=%s}"),
		*SourceCardInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		LexToString(IntentKind),
		LexToString(RejectReason),
		bCanSubmit ? TEXT("true") : TEXT("false"),
		*TargetHandle.ToString(),
		bHasFeedbackTargetScreenPosition ? TEXT("true") : TEXT("false"),
		*FeedbackTargetScreenPosition.ToString(),
		LexToString(TargetValidationRejectReason),
		*TargetValidationDebugSummary);
}

UBattleHUD::~UBattleHUD()
{
	delete BattleHUDRuntime;
	BattleHUDRuntime = nullptr;
}

void UBattleHUD::BeginBattleEntryPresentation()
{
	GetBattleHUDRuntime().GetResultApplicator().BeginBattleEntryPresentation();
}

void UBattleHUD::AttachInitializedBattleSession(
	UBattleSession* InSession,
	FBattleInitializationResult Initialization)
{
	GetBattleHUDRuntime().GetResultApplicator().AttachInitializedBattleSession(
		InSession,
		MoveTemp(Initialization));
}

void UBattleHUD::ReleaseBattleEntryPresentation()
{
	GetBattleHUDRuntime().GetResultApplicator().ReleaseBattleEntryPresentation();
}

void UBattleHUD::SetInjectedBattleSession(UBattleSession* InSession)
{
	FWacomBattleHUDRuntime& Runtime = GetBattleHUDRuntime();
	Runtime.GetResultApplicator().HandleSessionChanged(GetInjectedBattleSession(), InSession);
	Super::SetInjectedBattleSession(InSession);
}

FWacomBattleHUDRuntime& UBattleHUD::GetBattleHUDRuntime()
{
	if (!BattleHUDRuntime)
	{
		BattleHUDRuntime = new FWacomBattleHUDRuntime(*this);
	}
	return *BattleHUDRuntime;
}

const FWacomBattleHUDRuntime& UBattleHUD::GetBattleHUDRuntime() const
{
	return const_cast<UBattleHUD*>(this)->GetBattleHUDRuntime();
}

void UBattleHUD::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UBattleHUD::NativeConstruct()
{
	Super::NativeConstruct();
	if (CommandBar)
	{
		CommandBar->OnBattleCommandRequested.RemoveAll(this);
		CommandBar->OnBattleCommandRequested.AddDynamic(this, &UBattleHUD::HandleCommandBarCommandRequested);
	}
	BindCombatLogFeedForRuntime();
	BindPileDetailsRequestsForRuntime();
	GetBattleHUDRuntime().NativeConstruct();
}

void UBattleHUD::NativeDestruct()
{
	if (CommandBar)
	{
		CommandBar->OnBattleCommandRequested.RemoveAll(this);
	}
	UnbindCombatLogFeedForRuntime();
	UnbindPileDetailsRequestsForRuntime();
	if (BattleHUDRuntime)
	{
		BattleHUDRuntime->NativeDestruct();
	}
	Super::NativeDestruct();
}

void UBattleHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	GetBattleHUDRuntime().NativeTick(InDeltaTime);
}

FReply UBattleHUD::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (AWacomPlayerController* PC = Cast<AWacomPlayerController>(GetOwningPlayer()))
		{
			if (PC->TryRouteBattleSceneTargetClick(/*bRequireTargetSelect*/true))
			{
				return FReply::Handled();
			}
		}
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

TSharedRef<SWidget> UBattleHUD::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		FBattleHUDFallbackLayoutBuilder::Build(FBattleHUDFallbackLayoutBuilderContext{
			this,
			WidgetTree,
			&PlayerStatusBar,
			&CommandBar,
			&DrawPileView,
			&DiscardPileView,
			&ExhaustPileView,
			&DrawPileMotionAnchor,
			&DiscardPileMotionAnchor,
			&PlayTargetMotionAnchor,
			&CombatLogFeed,
			&BattlePresentationStack});
	}
	return Super::RebuildWidget();
}

void UBattleHUD::NativeRefreshFromSnapshot(const FBattleSnapshot& Snap)
{
	GetBattleHUDRuntime().NativeRefreshFromSnapshot(Snap);
}

void UBattleHUD::NativeOnSessionChanged(UBattleSession* OldSession, UBattleSession* NewSession)
{
	Super::NativeOnSessionChanged(OldSession, NewSession);
	GetBattleHUDRuntime().NativeOnSessionChanged(OldSession, NewSession);
}

TOptional<FUIInputConfig> UBattleHUD::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::All, EMouseCaptureMode::NoCapture);
}

void UBattleHUD::NativeOnUIStateChanged(EBattleUIState /*OldState*/, EBattleUIState NewState)
{
	GetBattleHUDRuntime().NativeOnUIStateChanged(NewState);
}

void UBattleHUD::RebuildChildBattleWidgetsForRuntime()
{
	ChildBattleWidgets.Reset();
	if (PlayerStatusBar) { ChildBattleWidgets.Add(PlayerStatusBar); }
	if (CombatLogFeed) { ChildBattleWidgets.Add(CombatLogFeed); }
	if (BattlePresentationStack) { ChildBattleWidgets.Add(BattlePresentationStack); }
	BindCombatLogFeedForRuntime();
	BindPileDetailsRequestsForRuntime();

	if (UBattleSession* CurrentSession = GetInjectedBattleSession())
	{
		for (const TObjectPtr<UWacomBattleWidgetBase>& Child : ChildBattleWidgets)
		{
			if (Child)
			{
				Child->SetInjectedBattleSession(CurrentSession);
			}
		}
	}
}

void UBattleHUD::RefreshChildBattleWidgetsFromSnapshotForRuntime(const FBattleSnapshot& Snap)
{
	Super::NativeRefreshFromSnapshot(Snap);
}

void UBattleHUD::BindFirstPersonCardLayerInteractionsForRuntime(
	UWacomFirstPersonCardAnchorComponent& Anchor)
{
	UnbindFirstPersonCardLayerInteractionsForRuntime(Anchor);
	Anchor.OnFirstPersonCardLayerCardHovered.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerCardHovered);
	Anchor.OnFirstPersonCardLayerCardUnhovered.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerCardUnhovered);
	Anchor.OnFirstPersonCardLayerHoveredCardLayoutUpdated.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerHoveredCardLayoutUpdated);
	Anchor.OnFirstPersonCardLayerCardTargetHovered.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerCardTargetHovered);
	Anchor.OnFirstPersonCardLayerCardTargetUnhovered.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerCardTargetUnhovered);
	Anchor.OnFirstPersonCardLayerHoveredCardTargetUpdated.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerHoveredCardTargetUpdated);
	Anchor.OnFirstPersonCardLayerPointerMoved.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerPointerMoved);
	Anchor.OnFirstPersonCardLayerPointerLeft.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerPointerLeft);
	Anchor.OnFirstPersonCardLayerDragStarted.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerDragStarted);
	Anchor.OnFirstPersonCardLayerDragUpdated.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerDragUpdated);
	Anchor.OnFirstPersonCardLayerDragReleased.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerDragReleased);
	Anchor.OnFirstPersonCardLayerDragCancelled.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerDragCancelled);
	Anchor.OnFirstPersonCardLayerFaceInspectLocked.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerFaceInspectLocked);
	Anchor.OnFirstPersonCardLayerFaceChanged.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerFaceChanged);
	Anchor.OnFirstPersonCardLayerFaceInspectClosed.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerFaceInspectClosed);
	Anchor.OnFirstPersonCardLayerPileTransferProgress.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerPileTransferProgress);
	Anchor.OnFirstPersonCardLayerEnterTransitionStarted.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerEnterTransitionStarted);
}

void UBattleHUD::UnbindFirstPersonCardLayerInteractionsForRuntime(
	UWacomFirstPersonCardAnchorComponent& Anchor)
{
	Anchor.OnFirstPersonCardLayerCardHovered.RemoveAll(this);
	Anchor.OnFirstPersonCardLayerCardUnhovered.RemoveAll(this);
	Anchor.OnFirstPersonCardLayerHoveredCardLayoutUpdated.RemoveAll(this);
	Anchor.OnFirstPersonCardLayerCardTargetHovered.RemoveAll(this);
	Anchor.OnFirstPersonCardLayerCardTargetUnhovered.RemoveAll(this);
	Anchor.OnFirstPersonCardLayerHoveredCardTargetUpdated.RemoveAll(this);
	Anchor.OnFirstPersonCardLayerPointerMoved.RemoveAll(this);
	Anchor.OnFirstPersonCardLayerPointerLeft.RemoveAll(this);
	Anchor.OnFirstPersonCardLayerDragStarted.RemoveAll(this);
	Anchor.OnFirstPersonCardLayerDragUpdated.RemoveAll(this);
	Anchor.OnFirstPersonCardLayerDragReleased.RemoveAll(this);
	Anchor.OnFirstPersonCardLayerDragCancelled.RemoveAll(this);
	Anchor.OnFirstPersonCardLayerFaceInspectLocked.RemoveAll(this);
	Anchor.OnFirstPersonCardLayerFaceChanged.RemoveAll(this);
	Anchor.OnFirstPersonCardLayerFaceInspectClosed.RemoveAll(this);
	Anchor.OnFirstPersonCardLayerPileTransferProgress.RemoveAll(this);
	Anchor.OnFirstPersonCardLayerEnterTransitionStarted.RemoveAll(this);
}

EBattleUIState UBattleHUD::GetUIState() const
{
	return GetBattleHUDRuntime().GetUIState();
}

bool UBattleHUD::IsInTargetSelect() const
{
	return GetBattleHUDRuntime().IsInTargetSelect();
}

FGuid UBattleHUD::GetPendingTargetingCardId() const
{
	return GetBattleHUDRuntime().GetPendingTargetingCardId();
}

FBattleTargetSelectionView UBattleHUD::BuildTargetSelectionView() const
{
	return GetBattleHUDRuntime().BuildTargetSelectionView();
}

int32 UBattleHUD::GetBattleCombatLogBlockCount() const
{
	return GetBattleHUDRuntime().GetBattleCombatLogBlockCount();
}

const TArray<FWacomBattleCombatLogBlockView>& UBattleHUD::GetBattleCombatLogHistory() const
{
	return GetBattleHUDRuntime().GetBattleCombatLogHistory();
}

const TArray<FWacomBattleCombatLogTurnSectionView>&
UBattleHUD::GetBattleCombatLogDetailsHistory() const
{
	return GetBattleHUDRuntime().GetBattleCombatLogDetailsHistory();
}

bool UBattleHUD::IsBattlePresentationBusy() const
{
	return GetBattleHUDRuntime().IsBattlePresentationBusy();
}

bool UBattleHUD::CanSubmitPlayerActionCommand() const
{
	return GetBattleHUDRuntime().CanSubmitPlayerActionCommand();
}

bool UBattleHUD::IsBattleInputReady() const
{
	return GetBattleHUDRuntime().IsBattleInputReady();
}

bool UBattleHUD::IsFirstPersonBattleHandSuppressedForEntry() const
{
	return GetBattleHUDRuntime().IsFirstPersonBattleHandSuppressedForEntry();
}

bool UBattleHUD::HasPendingTurnBoundaryCommand() const
{
	return GetBattleHUDRuntime().HasPendingTurnBoundaryCommand();
}

FText UBattleHUD::GetPendingTurnBoundaryCommandText() const
{
	return GetBattleHUDRuntime().GetPendingTurnBoundaryCommandText();
}

void UBattleHUD::SetBattleSceneEnemyHosts(const TArray<AWacomBattleEnemyActor*>& InHosts)
{
	GetBattleHUDRuntime().SetBattleSceneEnemyHosts(InHosts);
}

bool UBattleHUD::IsBattleSceneEnemyHostInCurrentRegistry(const AWacomBattleEnemyActor* Host) const
{
	return GetBattleHUDRuntime().IsBattleSceneEnemyHostInCurrentRegistry(Host);
}

bool UBattleHUD::IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(
	const FWacomInteractionTargetHandle& TargetHandle) const
{
	return GetBattleHUDRuntime().IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(TargetHandle);
}

void UBattleHUD::SetUIState(EBattleUIState NewState)
{
	GetBattleHUDRuntime().SetUIState(NewState);
}

void UBattleHUD::OnCardClickedByUser(const FGuid& /*CardInstanceId*/)
{
	// Deprecated compatibility shim. First-person hand submission is handled by drag/release.
}

void UBattleHUD::OnEnemyPartClickedByUser(const FWacomInteractionTargetHandle& TargetHandle)
{
	GetBattleHUDRuntime().OnEnemyPartClickedByUser(TargetHandle);
}

void UBattleHUD::OnWaitRequested()
{
	GetBattleHUDRuntime().OnWaitRequested();
}

void UBattleHUD::OnEndTurnRequested()
{
	GetBattleHUDRuntime().OnEndTurnRequested();
}

bool UBattleHUD::TryStartFirstPersonBattleHandDragByIndex(
	int32 OneBasedIndex,
	const TOptional<FVector2D>& InitialPointerWidgetPosition)
{
	return GetBattleHUDRuntime().TryStartFirstPersonBattleHandDragByIndex(
		OneBasedIndex,
		InitialPointerWidgetPosition);
}

void UBattleHUD::CancelTargetSelect()
{
	GetBattleHUDRuntime().CancelTargetSelect();
}

bool UBattleHUD::TryCloseEnemyInspection()
{
	return GetBattleHUDRuntime().TryCloseEnemyInspection();
}

bool UBattleHUD::IsEnemyInspectionOpen() const
{
	return GetBattleHUDRuntime().GetEnemyInspectionCoordinator().IsInspectionOpen();
}

void UBattleHUD::HandleCommandBarCommandRequested(EWacomBattleCommandId CommandId)
{
	switch (CommandId)
	{
	case EWacomBattleCommandId::Wait:
		OnWaitRequested();
		break;
	case EWacomBattleCommandId::EndTurn:
		OnEndTurnRequested();
		break;
	case EWacomBattleCommandId::CancelTargetSelect:
		CancelTargetSelect();
		break;
	case EWacomBattleCommandId::None:
	default:
		break;
	}
}

void UBattleHUD::OnKnockdownChoiceSelected(EKnockdownChoice Choice)
{
	TrySubmitKnockdownChoice(Choice);
}

bool UBattleHUD::TrySubmitKnockdownChoice(EKnockdownChoice Choice)
{
	return GetBattleHUDRuntime().TrySubmitKnockdownChoice(Choice);
}

void UBattleHUD::SubmitPlayCard(const FGuid& CardId, const FGuid& TargetPartId)
{
	GetBattleHUDRuntime().SubmitPlayCard(CardId, TargetPartId);
}

void UBattleHUD::SubmitPlayCardOnHandCard(const FGuid& CardId, const FGuid& TargetCardId)
{
	GetBattleHUDRuntime().SubmitPlayCardOnHandCard(CardId, TargetCardId);
}

void UBattleHUD::AppendBattleCombatLogBlock(const FWacomBattleCombatLogBlockView& Block)
{
	GetBattleHUDRuntime().AppendBattleCombatLogBlock(Block);
}

void UBattleHUD::StoreFirstPersonCardTransitionEvents(const TArray<FBattleEvent>& Events)
{
	GetBattleHUDRuntime().StoreFirstPersonCardTransitionEvents(Events);
}

void UBattleHUD::ClearPendingFirstPersonCardTransitionEvents()
{
	GetBattleHUDRuntime().ClearPendingFirstPersonCardTransitionEvents();
}

void UBattleHUD::RecordFirstPersonPlayCommit(
	const FGuid& CardInstanceId,
	const FBattlePartSlotIdentity& TargetPartKey,
	const TOptional<FVector2D>& TargetWidgetPosition)
{
	GetBattleHUDRuntime().RecordFirstPersonPlayCommit(
		CardInstanceId,
		TargetPartKey,
		TargetWidgetPosition);
}

TArray<FWacomFirstPersonCardLayerTransitionHint> UBattleHUD::BuildFirstPersonCardTransitionHints(
	const FBattleSnapshot& PreviousSnapshot,
	const FBattleSnapshot& NextSnapshot) const
{
	return GetBattleHUDRuntime().BuildFirstPersonCardTransitionHints(PreviousSnapshot, NextSnapshot);
}

int32 UBattleHUD::AppendBattlePresentationStackEntry(
	const FWacomBattleCombatLogCommandContext& CommandContext,
	const FBattleSnapshot& PreCommandSnapshot)
{
	return GetBattleHUDRuntime().AppendBattlePresentationStackEntry(CommandContext, PreCommandSnapshot);
}

void UBattleHUD::BeginBattlePresentationStackEntryExit(int32 EntryId)
{
	GetBattleHUDRuntime().BeginBattlePresentationStackEntryExit(EntryId);
}

void UBattleHUD::FinishBattlePresentationStackEntryExit(int32 EntryId)
{
	GetBattleHUDRuntime().FinishBattlePresentationStackEntryExit(EntryId);
}

void UBattleHUD::ClearBattlePresentationStack()
{
	GetBattleHUDRuntime().ClearBattlePresentationStack();
}

bool UBattleHUD::HasBattlePresentationStackEntries() const
{
	return GetBattleHUDRuntime().HasBattlePresentationStackEntries();
}

void UBattleHUD::EnqueueBattlePresentationEvents(
	const TArray<FBattleEvent>& Events,
	int32 PresentationStackEntryId)
{
	GetBattleHUDRuntime().EnqueueBattlePresentationEvents(Events, PresentationStackEntryId);
}

void UBattleHUD::ClearBattlePresentationQueue()
{
	GetBattleHUDRuntime().ClearBattlePresentationQueue();
}

bool UBattleHUD::IsBattlePresentationQueueBusy() const
{
	return GetBattleHUDRuntime().IsBattlePresentationQueueBusy();
}

void UBattleHUD::QueuePendingTurnBoundaryCommand(EWacomBattleHUDTurnBoundaryCommand Command)
{
	GetBattleHUDRuntime().QueuePendingTurnBoundaryCommand(Command);
}

void UBattleHUD::ClearPendingTurnBoundaryCommand()
{
	GetBattleHUDRuntime().ClearPendingTurnBoundaryCommand();
}

void UBattleHUD::TryExecutePendingTurnBoundaryCommand()
{
	GetBattleHUDRuntime().TryExecutePendingTurnBoundaryCommand();
}

void UBattleHUD::ClearBattlePresentationTargetRegistry()
{
	GetBattleHUDRuntime().ClearBattlePresentationTargetRegistry();
}

FWacomBattlePresentationTargetRegistry& UBattleHUD::GetBattlePresentationTargetRegistry()
{
	return GetBattleHUDRuntime().GetBattlePresentationTargetRegistry();
}

void UBattleHUD::RegisterBattlePresentationTarget(
	const FBattlePartSlotIdentity& TargetPartKey,
	UObject* Owner,
	TFunction<void(const FWacomBattlePresentationTargetCue&)> Handler)
{
	GetBattleHUDRuntime().RegisterBattlePresentationTarget(TargetPartKey, Owner, MoveTemp(Handler));
}

void UBattleHUD::UnregisterBattlePresentationTargetsForOwner(const UObject* Owner)
{
	GetBattleHUDRuntime().UnregisterBattlePresentationTargetsForOwner(Owner);
}

bool UBattleHUD::IsBattlePresentationTargetRegisteredForOwner(const UObject* Owner) const
{
	return GetBattleHUDRuntime().IsBattlePresentationTargetRegisteredForOwner(Owner);
}

void UBattleHUD::PlayBattlePresentationCue(const FWacomBattlePresentationTargetCue& Cue)
{
	GetBattleHUDRuntime().PlayBattlePresentationCue(Cue);
}

void UBattleHUD::PushPendingKnockdownChoiceDialog()
{
	GetBattleHUDRuntime().PushPendingKnockdownChoiceDialog();
}

void UBattleHUD::AdvanceBattlePresentationQueueOnce()
{
	GetBattleHUDRuntime().AdvanceBattlePresentationQueueOnce();
}

bool UBattleHUD::IsCardDetailPanelVisible() const
{
	return GetBattleHUDRuntime().GetCardDetailController().IsVisible();
}

FText UBattleHUD::GetCardDetailPanelNameText() const
{
	return GetBattleHUDRuntime().GetCardDetailController().GetNameText();
}

void UBattleHUD::HideCardDetailPanel()
{
	GetBattleHUDRuntime().HideCardDetailPanel();
}

void UBattleHUD::HideFirstPersonCardDetailPanelForSource(const FGuid& CardInstanceId)
{
	GetBattleHUDRuntime().HideFirstPersonCardDetailPanelForSource(CardInstanceId);
}

UWacomCardDetailPanel* UBattleHUD::EnsureFirstPersonCardDetailPanel()
{
	return GetBattleHUDRuntime().EnsureFirstPersonCardDetailPanel();
}

bool UBattleHUD::ShowFirstPersonCardDetailAtSlot(
	const FWacomCardDetailViewData& DetailData,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	return GetBattleHUDRuntime().ShowFirstPersonCardDetailAtSlot(DetailData, SlotView);
}

void UBattleHUD::PositionFirstPersonCardDetailPanelBesideSlot(
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetBattleHUDRuntime().PositionFirstPersonCardDetailPanelBesideSlot(SlotView);
}

void UBattleHUD::HideFirstPersonCardDetailPanel()
{
	GetBattleHUDRuntime().HideFirstPersonCardDetailPanel();
}

void UBattleHUD::TickCardDetailMotion(float DeltaTime)
{
	GetBattleHUDRuntime().TickCardDetailMotion(DeltaTime);
}

void UBattleHUD::ForceHideCardDetailHost(EWacomBattleHUDCardDetailHost Host)
{
	GetBattleHUDRuntime().ForceHideCardDetailHost(Host);
}

bool UBattleHUD::ComputeFirstPersonCardDetailTarget(
	const FWacomFirstPersonCardLayerSlotView& SlotView,
	FVector2D& OutPosition)
{
	return GetBattleHUDRuntime().ComputeFirstPersonCardDetailTarget(SlotView, OutPosition);
}

FVector2D UBattleHUD::ComputeCardDetailPanelPositionBesideStable(
	const FVector2D& AnchorPosition,
	const FVector2D& AnchorSize,
	const FVector2D& LayerSize,
	const FVector2D& PanelSize,
	float DetailPadding)
{
	return GetBattleHUDRuntime().ComputeCardDetailPanelPositionBesideStable(
		AnchorPosition,
		AnchorSize,
		LayerSize,
		PanelSize,
		DetailPadding);
}

FVector2D UBattleHUD::GetFirstPersonCardDetailViewportSize() const
{
	return GetBattleHUDRuntime().GetFirstPersonCardDetailViewportSize();
}

void UBattleHUD::SetFirstPersonCardDetailSource(const FGuid& CardInstanceId)
{
	GetBattleHUDRuntime().SetFirstPersonCardDetailSource(CardInstanceId);
}

void UBattleHUD::ClearFirstPersonCardDetailSource()
{
	GetBattleHUDRuntime().ClearFirstPersonCardDetailSource();
}

bool UBattleHUD::IsCurrentFirstPersonCardDetailSource(const FGuid& CardInstanceId) const
{
	return GetBattleHUDRuntime().IsCurrentFirstPersonCardDetailSource(CardInstanceId);
}

void UBattleHUD::UpdateFirstPersonCardDetailSlot(const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetBattleHUDRuntime().UpdateFirstPersonCardDetailSlot(SlotView);
}

FVector2D UBattleHUD::GetLastFirstPersonCardDetailPanelPosition() const
{
	return GetBattleHUDRuntime().GetLastFirstPersonCardDetailPanelPosition();
}

const FHandCardSnapshot* UBattleHUD::FindLastBattleHandCardSnapshot(const FGuid& CardInstanceId) const
{
	if (!GetBattleHUDRuntime().HasLastBattleSnapshot() || !CardInstanceId.IsValid())
	{
		return nullptr;
	}
	for (const FHandCardSnapshot& CardSnapshot : GetBattleHUDRuntime().GetLastBattleSnapshot().Hand.Cards)
	{
		if (CardSnapshot.InstanceId == CardInstanceId)
		{
			return &CardSnapshot;
		}
	}
	return nullptr;
}

void UBattleHUD::RebuildBattleSceneEnemyPartWorldTargetRegistry()
{
	GetBattleHUDRuntime().RebuildBattleSceneEnemyPartWorldTargetRegistry();
}

bool UBattleHUD::IsBattleSceneEnemyPartInCurrentRegistry(
	const UWacomBattleEnemyPartComponent* Part) const
{
	return GetBattleHUDRuntime().IsBattleSceneEnemyPartInCurrentRegistry(Part);
}

void UBattleHUD::SyncBattleEnemyPartWorldTargets(const FBattleSnapshot& Snap)
{
	GetBattleHUDRuntime().SyncBattleEnemyPartWorldTargets(Snap);
}

void UBattleHUD::ClearBattleEnemyPartWorldTargets()
{
	GetBattleHUDRuntime().ClearBattleEnemyPartWorldTargets();
}

bool UBattleHUD::CanUpdateBattleSceneEnemyPartHoverProbe() const
{
	return GetBattleHUDRuntime().CanUpdateBattleSceneEnemyPartHoverProbe();
}

void UBattleHUD::TickBattleSceneEnemyPartHoverProbe(float DeltaTime)
{
	GetBattleHUDRuntime().TickBattleSceneEnemyPartHoverProbe(DeltaTime);
}

void UBattleHUD::UpdateBattleSceneEnemyPartHoverProbe()
{
	GetBattleHUDRuntime().UpdateBattleSceneEnemyPartHoverProbe();
}

void UBattleHUD::ClearBattleSceneEnemyPartHoverProbe(FName Reason)
{
	GetBattleHUDRuntime().ClearBattleSceneEnemyPartHoverProbe(Reason);
}

UWacomFirstPersonCardAnchorComponent* UBattleHUD::ResolveFirstPersonCardAnchor() const
{
	return GetBattleHUDRuntime().ResolveFirstPersonCardAnchor();
}

UWacomFirstPersonCardAnchorComponent* UBattleHUD::ResolveActiveFirstPersonCardAnchor() const
{
	return GetBattleHUDRuntime().ResolveActiveFirstPersonCardAnchor();
}

void UBattleHUD::SyncFirstPersonBattleHandLayer(const FBattleSnapshot& Snap)
{
	GetBattleHUDRuntime().SyncFirstPersonBattleHandLayer(Snap);
}

void UBattleHUD::SyncFirstPersonBattleHandLayer(
	const FBattleSnapshot& Snap,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints)
{
	GetBattleHUDRuntime().SyncFirstPersonBattleHandLayer(Snap, TransitionHints);
}

void UBattleHUD::RefreshFromPresentationPhase(
	const FBattleSnapshot& Snap,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints)
{
	GetBattleHUDRuntime().RefreshFromPresentationPhase(Snap, TransitionHints);
}

void UBattleHUD::ClearFirstPersonBattleHandLayer()
{
	GetBattleHUDRuntime().ClearFirstPersonBattleHandLayer();
}

bool UBattleHUD::ShouldUseFirstPersonBattleHandLayer() const
{
	return GetBattleHUDRuntime().ShouldUseFirstPersonBattleHandLayer();
}

bool UBattleHUD::ShouldEnableFirstPersonBattleHandInteraction() const
{
	return GetBattleHUDRuntime().ShouldEnableFirstPersonBattleHandInteraction();
}

void UBattleHUD::BindFirstPersonBattleHandLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor)
{
	GetBattleHUDRuntime().BindFirstPersonBattleHandLayerInteractions(Anchor);
}

void UBattleHUD::UnbindFirstPersonBattleHandLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor)
{
	GetBattleHUDRuntime().UnbindFirstPersonBattleHandLayerInteractions(Anchor);
}

void UBattleHUD::HandleFirstPersonCardLayerCardHovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerCardHovered(CardInstanceId, SlotView);
}

void UBattleHUD::HandleFirstPersonCardLayerCardUnhovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerCardUnhovered(CardInstanceId, SlotView);
}

void UBattleHUD::HandleFirstPersonCardLayerHoveredCardLayoutUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerHoveredCardLayoutUpdated(CardInstanceId, SlotView);
}

void UBattleHUD::HandleFirstPersonCardLayerCardTargetHovered(
	const FWacomInteractionTargetHandle& CardTargetHandle,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerCardTargetHovered(CardTargetHandle, SlotView);
}

void UBattleHUD::HandleFirstPersonCardLayerCardTargetUnhovered(
	const FWacomInteractionTargetHandle& CardTargetHandle,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerCardTargetUnhovered(CardTargetHandle, SlotView);
}

void UBattleHUD::HandleFirstPersonCardLayerHoveredCardTargetUpdated(
	const FWacomInteractionTargetHandle& CardTargetHandle,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerHoveredCardTargetUpdated(CardTargetHandle, SlotView);
}

void UBattleHUD::HandleFirstPersonCardLayerPointerMoved(
	const FWacomFirstPersonCardPointerView& PointerView)
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerPointerMoved(PointerView);
}

void UBattleHUD::HandleFirstPersonCardLayerPointerLeft()
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerPointerLeft();
}

void UBattleHUD::HandleFirstPersonCardLayerDragStarted(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerDragStarted(CardInstanceId, DragView);
}

void UBattleHUD::HandleFirstPersonCardLayerDragUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerDragUpdated(CardInstanceId, DragView);
}

void UBattleHUD::HandleFirstPersonCardLayerDragReleased(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerDragReleased(CardInstanceId, DragView);
}

void UBattleHUD::HandleFirstPersonCardLayerDragCancelled(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerDragCancelled(CardInstanceId, DragView);
}

void UBattleHUD::HandleFirstPersonCardLayerFaceInspectLocked(
	const FGuid& CardInstanceId,
	const EWacomCardFaceContext FaceContext,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerFaceInspectLocked(
		CardInstanceId,
		FaceContext,
		SlotView);
}

void UBattleHUD::HandleFirstPersonCardLayerFaceChanged(
	const FGuid& CardInstanceId,
	const EWacomCardFaceContext FaceContext,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerFaceChanged(
		CardInstanceId,
		FaceContext,
		SlotView);
}

void UBattleHUD::HandleFirstPersonCardLayerFaceInspectClosed(
	const FGuid& CardInstanceId,
	const EWacomCardFaceContext /*FaceContext*/,
	const FWacomFirstPersonCardLayerSlotView& /*SlotView*/)
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerFaceInspectClosed(CardInstanceId);
}

void UBattleHUD::HandleFirstPersonCardLayerPileTransferProgress(
	const FWacomFirstPersonCardPileTransferProgressView& Progress)
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerPileTransferProgress(Progress);
}

void UBattleHUD::RequestOpenCombatLogDetails()
{
	GetBattleHUDRuntime().RequestOpenCombatLogDetails();
	OnCombatLogDetailsRequestedNative.Broadcast();
}

void UBattleHUD::RequestOpenCardPileDetails(EWacomBattlePileDetailsTab InitialTab)
{
	GetBattleHUDRuntime().RequestOpenCardPileDetails(InitialTab);
}

void UBattleHUD::BindCombatLogFeedForRuntime()
{
	if (!CombatLogFeed)
	{
		return;
	}
	CombatLogFeed->OnCombatLogDetailsRequestedNative().RemoveAll(this);
	CombatLogFeed->OnCombatLogDetailsRequestedNative().AddUObject(
		this,
		&UBattleHUD::HandleCombatLogDetailsRequested);
}

void UBattleHUD::UnbindCombatLogFeedForRuntime()
{
	if (CombatLogFeed)
	{
		CombatLogFeed->OnCombatLogDetailsRequestedNative().RemoveAll(this);
	}
}

void UBattleHUD::HandleCombatLogDetailsRequested()
{
	RequestOpenCombatLogDetails();
}

void UBattleHUD::BindPileDetailsRequestsForRuntime()
{
	UnbindPileDetailsRequestsForRuntime();
	if (DrawPileView)
	{
		DrawPileView->OnPileDetailsRequestedNative().AddUObject(this, &UBattleHUD::HandleDrawPileDetailsRequested);
	}
	if (DiscardPileView)
	{
		DiscardPileView->OnPileDetailsRequestedNative().AddUObject(this, &UBattleHUD::HandleDiscardPileDetailsRequested);
	}
	if (ExhaustPileView)
	{
		ExhaustPileView->OnPileDetailsRequestedNative().AddUObject(this, &UBattleHUD::HandleExhaustPileDetailsRequested);
	}
}

void UBattleHUD::UnbindPileDetailsRequestsForRuntime()
{
	if (DrawPileView) { DrawPileView->OnPileDetailsRequestedNative().RemoveAll(this); }
	if (DiscardPileView) { DiscardPileView->OnPileDetailsRequestedNative().RemoveAll(this); }
	if (ExhaustPileView) { ExhaustPileView->OnPileDetailsRequestedNative().RemoveAll(this); }
}

void UBattleHUD::HandleDrawPileDetailsRequested()
{
	RequestOpenCardPileDetails(EWacomBattlePileDetailsTab::Draw);
}

void UBattleHUD::HandleDiscardPileDetailsRequested()
{
	RequestOpenCardPileDetails(EWacomBattlePileDetailsTab::Discard);
}

void UBattleHUD::HandleExhaustPileDetailsRequested()
{
	RequestOpenCardPileDetails(EWacomBattlePileDetailsTab::Exhaust);
}

void UBattleHUD::HandleFirstPersonCardLayerEnterTransitionStarted(
	const FWacomFirstPersonCardEnterTransitionStartedView& View)
{
	GetBattleHUDRuntime().HandleFirstPersonCardLayerEnterTransitionStarted(View);
}

#if WITH_AUTOMATION_TESTS
void UBattleHUD::PrepareDrawPileFeedbackForAutomationTest(
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints)
{
	GetBattleHUDRuntime().PrepareDrawPileFeedbackForPresentationFrame(TransitionHints);
}

void UBattleHUD::DispatchEnterTransitionStartedForAutomationTest(
	const FWacomFirstPersonCardEnterTransitionStartedView& View)
{
	HandleFirstPersonCardLayerEnterTransitionStarted(View);
}

void UBattleHUD::ResetDrawPileFeedbackForAutomationTest(int32 AuthoritativeDrawPileCount)
{
	GetBattleHUDRuntime().ResetDrawPileFeedback(AuthoritativeDrawPileCount);
}
#endif

void UBattleHUD::UpdateFirstPersonCardDragTargetFeedback(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	GetBattleHUDRuntime().UpdateFirstPersonCardDragTargetFeedback(CardInstanceId, DragView);
}

void UBattleHUD::ClearFirstPersonCardDragTargetFeedback()
{
	GetBattleHUDRuntime().ClearFirstPersonCardDragTargetFeedback();
}

bool UBattleHUD::IsFirstPersonCardDragActiveForBattleSceneHover() const
{
	return GetBattleHUDRuntime().IsFirstPersonCardDragActiveForBattleSceneHover();
}

FWacomBattleCardDropResolveResult UBattleHUD::ResolveFirstPersonCardDropIntent(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView) const
{
	return GetBattleHUDRuntime().ResolveFirstPersonCardDropIntent(CardInstanceId, DragView);
}

TArray<FWacomFirstPersonCardTargetAffordance> UBattleHUD::BuildFirstPersonCardTargetAffordances(
	const FGuid& SourceCardId,
	const FBattleSnapshot& Snapshot,
	const UBattleSession& BattleSession) const
{
	return GetBattleHUDRuntime().BuildFirstPersonCardTargetAffordances(SourceCardId, Snapshot, BattleSession);
}

UWacomBattleEnemyPartComponent* UBattleHUD::ResolveBattleEnemyPartComponent(
	const FWacomInteractionTargetHandle& TargetHandle) const
{
	return GetBattleHUDRuntime().ResolveBattleEnemyPartComponent(TargetHandle);
}

bool UBattleHUD::ProbeFirstPersonCardDragTarget(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	FWacomInteractionTargetHandle& OutTargetHandle,
	bool& bOutValidTarget) const
{
	return GetBattleHUDRuntime().ProbeFirstPersonCardDragTarget(CardInstanceId, DragView, OutTargetHandle, bOutValidTarget);
}

bool UBattleHUD::ShouldShowFirstPersonDragInspectDetail(
	const FWacomFirstPersonCardDragView& DragView) const
{
	return GetBattleHUDRuntime().ShouldShowFirstPersonDragInspectDetail(DragView);
}

#if WITH_AUTOMATION_TESTS
void UBattleHUD::PlayBattlePresentationCueForTest(
	EBattleEventType SourceEventType,
	const FBattlePartSlotIdentity& TargetPartKey,
	int32 Amount)
{
	GetBattleHUDRuntime().PlayBattlePresentationCueForTest(SourceEventType, TargetPartKey, Amount);
}

void UBattleHUD::PlayTargetConfirmedCueForTest(const FBattlePartSlotIdentity& TargetPartKey)
{
	GetBattleHUDRuntime().PlayTargetConfirmedCueForTest(TargetPartKey);
}

bool UBattleHUD::EnqueueEndTurnPresentationPlanForTest(
	const FBattlePresentationJournal& Journal,
	const TArray<FBattleEvent>& Events,
	const FBattleSnapshot& PostCommandSnapshot)
{
	return GetBattleHUDRuntime().EnqueueEndTurnPresentationPlanForTest(Journal, Events, PostCommandSnapshot);
}

bool UBattleHUD::EnqueueCommandPresentationPlanForTest(
	const FBattlePresentationJournal& Journal,
	const TArray<FBattleEvent>& Events,
	const FBattleSnapshot& PreCommandSnapshot,
	const FBattleSnapshot& PostCommandSnapshot)
{
	return GetBattleHUDRuntime().EnqueueCommandPresentationPlanForTest(
		Journal,
		Events,
		PreCommandSnapshot,
		PostCommandSnapshot);
}

void UBattleHUD::PrimeDiscardPileReceiveFeedbackForAutomationTest(
	int32 EventSequence,
	int32 TotalCount,
	int32 InitialDiscardCount)
{
	GetBattleHUDRuntime().PrimeDiscardPileReceiveFeedbackForTest(
		EventSequence,
		TotalCount,
		InitialDiscardCount);
}

void UBattleHUD::PrimeReshufflePileFeedbackForAutomationTest(
	int32 EventSequence,
	int32 TotalCount,
	int32 InitialDrawCount,
	int32 FinalDrawCount,
	int32 InitialDiscardCount,
	int32 FinalDiscardCount,
	int32 PlayedCount)
{
	GetBattleHUDRuntime().PrimeReshufflePileFeedbackForTest(
		EventSequence,
		TotalCount,
		InitialDrawCount,
		FinalDrawCount,
		InitialDiscardCount,
		FinalDiscardCount,
		PlayedCount);
}

FWacomBattleHUDAutomationTestView UBattleHUD::GetAutomationTestViewForTest() const
{
	return GetBattleHUDRuntime().GetAutomationTestViewForTest();
}

TArray<FWacomFirstPersonCardLayerTransitionHint> UBattleHUD::BuildFirstPersonCardTransitionHintsForRefreshForTest(
	const FBattleSnapshot& NextSnapshot) const
{
	return GetBattleHUDRuntime().BuildFirstPersonCardTransitionHintsForRefreshForTest(NextSnapshot);
}

TArray<FWacomFirstPersonCardLayerFeedbackHint> UBattleHUD::BuildFirstPersonCardFeedbackHintsForTest(
	const FBattleSnapshot& NextSnapshot) const
{
	return GetBattleHUDRuntime().BuildFirstPersonCardFeedbackHintsForTest(NextSnapshot);
}

void UBattleHUD::SetFirstPersonCardTransitionSnapshotForTest(const FBattleSnapshot& Snapshot)
{
	GetBattleHUDRuntime().SetFirstPersonCardTransitionSnapshotForTest(Snapshot);
}

void UBattleHUD::SetTargetSelectionStateForAutomationTest(const FGuid& PendingCardId)
{
	GetBattleHUDRuntime().SetPendingTargetingCardId(PendingCardId);
	SetUIState(EBattleUIState::TargetSelect);
}

void UBattleHUD::ClearTargetSelectionStateForAutomationTest()
{
	GetBattleHUDRuntime().ClearPendingTargetingCardId();
	SetUIState(EBattleUIState::Idle);
}

void UBattleHUD::QueuePendingTurnBoundaryWaitForAutomationTest()
{
	QueuePendingTurnBoundaryCommand(EWacomBattleHUDTurnBoundaryCommand::Wait);
}

void UBattleHUD::SetBattleInputReadyForAutomationTest(bool bReady)
{
	GetBattleHUDRuntime().SetBattleInputReady(bReady);
}

void UBattleHUD::SetFirstPersonBattleHandSuppressedForAutomationTest(bool bSuppressed)
{
	GetBattleHUDRuntime().SetFirstPersonBattleHandSuppressedForEntry(bSuppressed);
}

void UBattleHUD::SetSecondaryPanelOpenForAutomationTest(bool bOpen)
{
	GetBattleHUDRuntime().SetSecondaryPanelOpen(bOpen);
}

void UBattleHUD::ApplyCommandResolutionForAutomationTest(
	UBattleSession* SourceSession,
	const FWacomBattleCombatLogCommandContext& LogContext,
	const FBattleSnapshot& PreCommandSnapshot,
	const FBattleResolution& Resolution,
	const FGuid& PlayCommitCardInstanceId)
{
	FWacomBattleCommandPresentationContext Context;
	Context.SourceSession = SourceSession;
	Context.CombatLogContext = LogContext;
	Context.PreCommandSnapshot = PreCommandSnapshot;
	if (PlayCommitCardInstanceId.IsValid())
	{
		FWacomBattlePlayCardCommitPresentation Commit;
		Commit.CardInstanceId = PlayCommitCardInstanceId;
		Context.PlayCardCommit = Commit;
	}
	GetBattleHUDRuntime().GetResultApplicator().ApplyCommandResolution(Context, Resolution);
}
#endif
