// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattleHUD.h"

#define LOCTEXT_NAMESPACE "WacomBattleHUD"
#include "UI/Battle/ActionPanel.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "UI/Battle/EquipmentBar.h"
#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/Battle/BattlePresentationStackWidget.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/Battle/BattleHUDFallbackLayoutBuilder.h"
#include "UI/Battle/WacomBattlePresentationTargetRegistry.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/Battle/WacomBattleHUDCardDetailController.h"
#include "UI/Battle/WacomBattleHUDCombatLogController.h"
#include "UI/Battle/WacomBattleHUDCommandFlow.h"
#include "UI/Battle/WacomBattleHUDEventFlow.h"
#include "UI/Battle/WacomBattleHUDFirstPersonHandBridge.h"
#include "UI/Battle/WacomBattleHUDPresentationCoordinator.h"
#include "UI/Battle/WacomBattleHUDTargetingFlow.h"
#include "UI/Battle/WacomBattleHUDSceneEnemyTargetCoordinator.h"
#include "UI/Battle/WacomKnockdownChoiceDialog.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Common/PileCountView.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomUITags.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "InputCoreTypes.h"

#include "Input/UIActionBindingHandle.h"
#include "Input/CommonUIInputSettings.h"

#include "Events/BattleEvent.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Types/WacomEnums.h"

namespace
{
	const TCHAR* CardDetailPanelPath = TEXT("/Game/Wacom/UI/Card/WBP_CardDetailPanel.WBP_CardDetailPanel_C");

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
		default: return TEXT("Unknown");
		}
	}

	FText BuildDiscardPileCountDisplayText(const FPileCountsSnapshot& PileCounts)
	{
		if (PileCounts.PlayedCount <= 0)
		{
			return FText::AsNumber(PileCounts.DiscardCount);
		}

		return FText::Format(
			LOCTEXT("DiscardPileWithPlayedCountFormat", "{0}+{1}"),
			FText::AsNumber(PileCounts.DiscardCount),
			FText::AsNumber(PileCounts.PlayedCount));
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

UBattleHUD::~UBattleHUD() = default;

void UBattleHUD::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// Widget Tree 尚未构造，ChildBattleWidgets 登记移到 NativeConstruct。
}

void UBattleHUD::NativeConstruct()
{
	Super::NativeConstruct();

	if (!CardDetailPanelClass)
	{
		if (UClass* LoadedPanelClass = LoadClass<UWacomCardDetailPanel>(nullptr, CardDetailPanelPath))
		{
			CardDetailPanelClass = LoadedPanelClass;
		}
		else
		{
			CardDetailPanelClass = UWacomCardDetailPanel::StaticClass();
		}
	}

	// 此时 RebuildWidget 已执行，BindWidget 字段都填好了。
	// 登记子 BattleWidget，让基类 SetSession / RefreshFromSnapshot 能递归到它们。
	ChildBattleWidgets.Reset();
	if (PlayerStatusBar) { ChildBattleWidgets.Add(PlayerStatusBar); }
	if (ActionPanel)     { ChildBattleWidgets.Add(ActionPanel); }
	if (EquipmentBar)    { ChildBattleWidgets.Add(EquipmentBar); }
	if (CombatLogFeed)   { ChildBattleWidgets.Add(CombatLogFeed); }
	if (BattlePresentationStack) { ChildBattleWidgets.Add(BattlePresentationStack); }

	// 如果 Session 在 RebuildWidget 之前就被 SetSession 过了，
	// 子 Widget 还没拿到 Session。这里补一次。
	if (UBattleSession* S = GetSession())
	{
		for (const TObjectPtr<UWacomBattleWidgetBase>& Child : ChildBattleWidgets)
		{
			if (Child) { Child->SetSession(S); }
		}
		RefreshFromSnapshot(S->BuildSnapshot());
	}
}

void UBattleHUD::NativeDestruct()
{
	if (PresentationCoordinator)
	{
		PresentationCoordinator->Shutdown();
		PresentationCoordinator.Reset();
	}
	ClearFirstPersonBattleHandLayer();
	ClearBattleEnemyPartWorldTargets();
	ClearBattlePresentationTargetRegistry();
	HideCardDetailPanel();
	GetCardDetailController().RemoveFirstPersonPanelFromViewport();
	bHasLastBattleSnapshot = false;
	LastBattleSnapshot = FBattleSnapshot();
	GetFirstPersonHandBridge().ClearTransitionSnapshot();
	ClearPendingFirstPersonCardTransitionEvents();
	ClearBattleSceneEnemyPartHoverProbe(TEXT("HUDDestruct"));
	Super::NativeDestruct();
}

void UBattleHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TickCardDetailMotion(InDeltaTime);
	TickBattleSceneEnemyPartHoverProbe(InDeltaTime);
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
			&ActionPanel,
			&EquipmentBar,
			&DrawPileView,
			&DiscardPileView,
			&ExhaustPileView,
			&CombatLogFeed,
			&BattlePresentationStack});
	}
	return Super::RebuildWidget();
}

void UBattleHUD::NativeRefreshFromSnapshot(const FBattleSnapshot& Snap)
{
	HideCardDetailPanel();
	const TArray<FWacomFirstPersonCardLayerTransitionHint> FirstPersonTransitionHints =
		GetFirstPersonHandBridge().BuildTransitionHintsForRefresh(Snap);
	ClearPendingFirstPersonCardTransitionEvents();
	LastBattleSnapshot = Snap;
	bHasLastBattleSnapshot = true;
	GetFirstPersonHandBridge().SetTransitionSnapshot(Snap);

	SyncFirstPersonBattleHandLayer(Snap, FirstPersonTransitionHints);

	// 战斗结束 → 切到 BattleEnd 状态，并广播一次
	if (Snap.Phase == EBattlePhase::BattleEnd)
	{
		ClearPendingFirstPersonCardTransitionEvents();
		ClearBattlePresentationStack();
		ClearPendingTurnBoundaryCommand();
		ClearBattleSceneEnemyPartHoverProbe(TEXT("BattleEnd"));
		bHasLastBattleSnapshot = false;
		GetFirstPersonHandBridge().ClearTransitionSnapshot();
		SetUIState(EBattleUIState::BattleEnd);

		if (!bHasBroadcastBattleEnd)
		{
			bHasBroadcastBattleEnd = true;
			OnBattleEndedNative.Broadcast(Snap.Outcome);
		}
	}

	// 手动刷新 PileCountView（它们不是 BattleWidget，不走递归）
	if (DrawPileView)    { DrawPileView->SetCount(Snap.PileCounts.DrawCount); }
	if (DiscardPileView)
	{
		DiscardPileView->SetCount(Snap.PileCounts.DiscardCount);
		DiscardPileView->SetCountDisplayText(BuildDiscardPileCountDisplayText(Snap.PileCounts));
	}
	if (ExhaustPileView) { ExhaustPileView->SetCount(Snap.PileCounts.ExhaustCount); }

	// 递归下发 Snapshot 给子 Widget
	Super::NativeRefreshFromSnapshot(Snap);
	SyncBattleEnemyPartWorldTargets(Snap);
}

void UBattleHUD::NativeOnSessionChanged(UBattleSession* OldSession, UBattleSession* NewSession)
{
	Super::NativeOnSessionChanged(OldSession, NewSession);
	if (OldSession != NewSession)
	{
		ClearBattlePresentationQueue();
		ClearBattlePresentationStack();
		ClearPendingTurnBoundaryCommand();
		ClearFirstPersonBattleHandLayer();
		SetBattleSceneEnemyHosts({});
		ClearBattleSceneEnemyPartHoverProbe(TEXT("SessionChanged"));
		ClearPendingFirstPersonCardTransitionEvents();
		ClearBattlePresentationTargetRegistry();
	}

	// 新 Session 接入时，重置状态机到 Idle。
	UIState = EBattleUIState::Idle;
	PendingTargetingCardId.Invalidate();
	bHasBroadcastBattleEnd = false;
	bHasLastBattleSnapshot = false;
	LastBattleSnapshot = FBattleSnapshot();
	GetFirstPersonHandBridge().ClearTransitionSnapshot();
	ClearPendingFirstPersonCardTransitionEvents();
	HideCardDetailPanel();
	GetCombatLogController().Clear();
	ClearBattlePresentationStack();
	ClearPendingTurnBoundaryCommand();
	ClearBattleSceneEnemyPartHoverProbe(TEXT("SessionChanged"));

	if (NewSession)
	{
		ConsumeAndLogEvents();
	}
}

TOptional<FUIInputConfig> UBattleHUD::GetDesiredInputConfig() const
{
	// All：鼠标可见 + 游戏输入透传。这样 UI 可点击，同时 Controller 战斗快捷键仍工作。
	return FUIInputConfig(ECommonInputMode::All, EMouseCaptureMode::NoCapture);
}

FBattleTargetSelectionView UBattleHUD::BuildTargetSelectionView() const
{
	return FWacomBattleHUDTargetingFlow::BuildTargetSelectionView(*this);
}

int32 UBattleHUD::GetBattleCombatLogBlockCount() const
{
	return GetCombatLogController().GetBlockCount();
}

bool UBattleHUD::IsBattlePresentationBusy() const
{
	return PresentationCoordinator && PresentationCoordinator->IsBusy();
}

bool UBattleHUD::CanSubmitPlayerActionCommand() const
{
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

bool UBattleHUD::HasPendingTurnBoundaryCommand() const
{
	return PresentationCoordinator && PresentationCoordinator->HasPendingTurnBoundaryCommand();
}

FText UBattleHUD::GetPendingTurnBoundaryCommandText() const
{
	return PresentationCoordinator
		? PresentationCoordinator->GetPendingTurnBoundaryCommandText()
		: FText::GetEmpty();
}

void UBattleHUD::SetBattleSceneEnemyHosts(const TArray<AWacomBattleEnemyActor*>& InHosts)
{
	GetSceneEnemyTargetCoordinator().SetSceneEnemyHosts(InHosts);
}

bool UBattleHUD::IsBattleSceneEnemyHostInCurrentRegistry(const AWacomBattleEnemyActor* Host) const
{
	return GetSceneEnemyTargetCoordinator().IsSceneEnemyHostInCurrentRegistry(Host);
}

bool UBattleHUD::IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(
	const FWacomInteractionTargetHandle& TargetHandle) const
{
	return GetSceneEnemyTargetCoordinator().IsWorldTargetInCurrentRegistry(TargetHandle);
}

// ================ 状态机 ================

void UBattleHUD::SetUIState(EBattleUIState NewState)
{
	if (UIState == NewState) { return; }
	const EBattleUIState OldState = UIState;
	UIState = NewState;
	NativeOnUIStateChanged(OldState, NewState);
	BP_OnUIStateChanged(OldState, NewState);
}

void UBattleHUD::NativeOnUIStateChanged(EBattleUIState /*OldState*/, EBattleUIState NewState)
{
	if (NewState != EBattleUIState::Idle)
	{
		HideCardDetailPanel();
	}
	if (NewState == EBattleUIState::BattleEnd)
	{
		ClearBattleSceneEnemyPartHoverProbe(TEXT("BattleEnd"));
	}

	// 状态变化时，让 first-person hand / ActionPanel 重新刷一次（高亮/启用状态）。
	UBattleSession* S = GetSession();
	if (!S) { return; }
	const FBattleSnapshot Snap = S->BuildSnapshot();
	if (Snap.Phase == EBattlePhase::BattleEnd)
	{
		LastBattleSnapshot = FBattleSnapshot();
		bHasLastBattleSnapshot = false;
		GetFirstPersonHandBridge().ClearTransitionSnapshot();
	}
	else
	{
		LastBattleSnapshot = Snap;
		bHasLastBattleSnapshot = true;
	}
	if (ActionPanel)  { ActionPanel->RefreshFromSnapshot(Snap); }
	SyncFirstPersonBattleHandLayer(Snap);
	SyncBattleEnemyPartWorldTargets(Snap);
}

UWacomFirstPersonCardAnchorComponent* UBattleHUD::ResolveFirstPersonCardAnchor() const
{
	return GetFirstPersonHandBridge().ResolveAnchor();
}

UWacomFirstPersonCardAnchorComponent* UBattleHUD::ResolveActiveFirstPersonCardAnchor() const
{
	return GetFirstPersonHandBridge().ResolveActiveAnchor();
}

void UBattleHUD::SyncFirstPersonBattleHandLayer(
	const FBattleSnapshot& Snap,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints)
{
	GetFirstPersonHandBridge().SyncLayer(Snap, TransitionHints);
}

void UBattleHUD::ClearFirstPersonBattleHandLayer()
{
	GetFirstPersonHandBridge().ClearLayer();
}

bool UBattleHUD::ShouldUseFirstPersonBattleHandLayer() const
{
	return GetFirstPersonHandBridge().ShouldUseFirstPersonBattleHandLayer();
}

bool UBattleHUD::ShouldEnableFirstPersonBattleHandInteraction() const
{
	return GetFirstPersonHandBridge().ShouldEnableFirstPersonBattleHandInteraction();
}

void UBattleHUD::BindFirstPersonBattleHandLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor)
{
	GetFirstPersonHandBridge().BindLayerInteractions(Anchor);
}

void UBattleHUD::UnbindFirstPersonBattleHandLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor)
{
	GetFirstPersonHandBridge().UnbindLayerInteractions(Anchor);
}

void UBattleHUD::HandleFirstPersonCardLayerCardClicked(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetFirstPersonHandBridge().HandleCardClicked(CardInstanceId, SlotView);
}

void UBattleHUD::HandleFirstPersonCardLayerCardHovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetFirstPersonHandBridge().HandleCardHovered(CardInstanceId, SlotView);
}

void UBattleHUD::HandleFirstPersonCardLayerCardUnhovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetFirstPersonHandBridge().HandleCardUnhovered(CardInstanceId, SlotView);
}

void UBattleHUD::HandleFirstPersonCardLayerHoveredCardLayoutUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetFirstPersonHandBridge().HandleHoveredCardLayoutUpdated(CardInstanceId, SlotView);
}

void UBattleHUD::HandleFirstPersonCardLayerDragStarted(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	GetFirstPersonHandBridge().HandleDragStarted(CardInstanceId, DragView);
}

void UBattleHUD::HandleFirstPersonCardLayerDragUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	GetFirstPersonHandBridge().HandleDragUpdated(CardInstanceId, DragView);
}

void UBattleHUD::HandleFirstPersonCardLayerDragReleased(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	GetFirstPersonHandBridge().HandleDragReleased(CardInstanceId, DragView);
}

void UBattleHUD::HandleFirstPersonCardLayerDragCancelled(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	GetFirstPersonHandBridge().HandleDragCancelled(CardInstanceId, DragView);
}

void UBattleHUD::HandleFirstPersonCardLayerPointerMoved(
	const FWacomFirstPersonCardPointerView& PointerView)
{
	GetFirstPersonHandBridge().HandlePointerMoved(PointerView);
}

void UBattleHUD::HandleFirstPersonCardLayerPointerLeft()
{
	GetFirstPersonHandBridge().HandlePointerLeft();
}

void UBattleHUD::ApplyFirstPersonCardDragCameraLookOverride(
	const FWacomFirstPersonCardDragView& DragView)
{
	GetFirstPersonHandBridge().ApplyDragCameraLookOverride(DragView);
}

void UBattleHUD::ClearFirstPersonCardDragCameraLookOverride()
{
	GetFirstPersonHandBridge().ClearDragCameraLookOverride();
}

void UBattleHUD::UpdateFirstPersonCardDragTargetFeedback(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	GetFirstPersonHandBridge().UpdateDragTargetFeedback(CardInstanceId, DragView);
}

void UBattleHUD::ClearFirstPersonCardDragTargetFeedback()
{
	GetFirstPersonHandBridge().ClearDragTargetFeedback();
}

bool UBattleHUD::IsFirstPersonCardDragActiveForBattleSceneHover() const
{
	return GetFirstPersonHandBridge().IsFirstPersonCardDragActiveForBattleSceneHover();
}

FWacomBattleCardDropResolveResult UBattleHUD::ResolveFirstPersonCardDropIntent(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView) const
{
	return GetFirstPersonHandBridge().ResolveDropIntent(CardInstanceId, DragView);
}

TArray<FWacomFirstPersonCardTargetAffordance> UBattleHUD::BuildFirstPersonCardTargetAffordances(
	const FGuid& SourceCardId,
	const FBattleSnapshot& Snapshot,
	const UBattleSession& BattleSession) const
{
	return GetFirstPersonHandBridge().BuildCardTargetAffordances(SourceCardId, Snapshot, BattleSession);
}

UWacomBattleEnemyPartWorldTargetBridgeComponent* UBattleHUD::ResolveBattleEnemyPartWorldTargetBridge(
	const FWacomInteractionTargetHandle& TargetHandle) const
{
	return GetSceneEnemyTargetCoordinator().ResolveWorldTargetBridge(TargetHandle);
}

bool UBattleHUD::ProbeFirstPersonCardDragTarget(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	FWacomInteractionTargetHandle& OutTargetHandle,
	bool& bOutValidTarget) const
{
	return GetFirstPersonHandBridge().ProbeDragTarget(CardInstanceId, DragView, OutTargetHandle, bOutValidTarget);
}

bool UBattleHUD::ShouldShowFirstPersonDragInspectDetail(
	const FWacomFirstPersonCardDragView& DragView) const
{
	return GetFirstPersonHandBridge().ShouldShowDragInspectDetail(DragView);
}

// ================ 子 Widget 交互入口 ================

void UBattleHUD::OnCardClickedByUser(const FGuid& CardInstanceId)
{
	FWacomBattleHUDTargetingFlow::HandleCardClicked(*this, CardInstanceId);
}

void UBattleHUD::OnEnemyPartClickedByUser(const FGuid& PartInstanceId)
{
	FWacomBattleHUDTargetingFlow::HandleEnemyPartClicked(*this, PartInstanceId);
}

void UBattleHUD::OnWaitRequested()
{
	FWacomBattleHUDCommandFlow::SubmitWait(*this);
}

void UBattleHUD::OnEndTurnRequested()
{
	FWacomBattleHUDCommandFlow::SubmitEndTurn(*this);
}

void UBattleHUD::CancelTargetSelect()
{
	FWacomBattleHUDTargetingFlow::CancelTargetSelect(*this);
}

void UBattleHUD::OnKnockdownChoiceSelected(EKnockdownChoice Choice)
{
	FWacomBattleHUDCommandFlow::SubmitKnockdownChoice(*this, Choice);
}

// ================ 内部 ================

void UBattleHUD::SubmitPlayCard(const FGuid& CardId, const FGuid& TargetPartId)
{
	FWacomBattleHUDCommandFlow::SubmitPlayCard(*this, CardId, TargetPartId);
}

void UBattleHUD::SubmitPlayCardOnHandCard(const FGuid& CardId, const FGuid& TargetCardId)
{
	FWacomBattleHUDCommandFlow::SubmitPlayCardOnHandCard(*this, CardId, TargetCardId);
}

void UBattleHUD::AfterCommand()
{
	FWacomBattleHUDCommandFlow::AfterCommand(*this);
}

FVector2D UBattleHUD::ComputeCardDetailPanelPositionBeside(
	const FVector2D& AnchorPosition,
	const FVector2D& AnchorSize,
	const FVector2D& LayerSize,
	const FVector2D& PanelSize,
	float Padding)
{
	const float SafePadding = FMath::Max(0.0f, Padding);
	const float MaxX = FMath::Max(0.0f, LayerSize.X - PanelSize.X);
	const float MaxY = FMath::Max(0.0f, LayerSize.Y - PanelSize.Y);

	const float LeftX = AnchorPosition.X - PanelSize.X - SafePadding;
	const float RightX = AnchorPosition.X + AnchorSize.X + SafePadding;
	const float DesiredX = LeftX >= 0.0f ? LeftX : RightX;
	const float DesiredY = AnchorPosition.Y + (AnchorSize.Y - PanelSize.Y) * 0.5f;

	return FVector2D(
		FMath::Clamp(DesiredX, 0.0f, MaxX),
		FMath::Clamp(DesiredY, 0.0f, MaxY));
}

bool UBattleHUD::IsCardDetailPanelVisible() const
{
	return GetCardDetailController().IsVisible();
}

FText UBattleHUD::GetCardDetailPanelNameText() const
{
	return GetCardDetailController().GetNameText();
}

void UBattleHUD::HideCardDetailPanel()
{
	GetCardDetailController().HideAll();
}

void UBattleHUD::HideFirstPersonCardDetailPanelForSource(const FGuid& CardInstanceId)
{
	GetCardDetailController().HideFirstPersonForSource(CardInstanceId);
}

bool UBattleHUD::IsFirstPersonCardInspectDetailActiveForSource(const FGuid& CardInstanceId) const
{
	return GetCardDetailController().IsFirstPersonInspectDetailActiveForSource(CardInstanceId);
}

UWacomCardDetailPanel* UBattleHUD::EnsureFirstPersonCardDetailPanel()
{
	return GetCardDetailController().EnsureFirstPersonPanel();
}

bool UBattleHUD::ShowFirstPersonCardDetailAtSlot(
	const FWacomCardDetailViewData& DetailData,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	return GetCardDetailController().ShowFirstPersonAtSlot(DetailData, SlotView);
}

void UBattleHUD::PositionFirstPersonCardDetailPanelBesideSlot(
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetCardDetailController().PositionFirstPersonBesideSlot(SlotView);
}

void UBattleHUD::HideFirstPersonCardDetailPanel()
{
	GetCardDetailController().HideFirstPerson();
}

void UBattleHUD::TickCardDetailMotion(float DeltaTime)
{
	GetCardDetailController().TickMotion(DeltaTime);
}

void UBattleHUD::ForceHideCardDetailHost(ECardDetailHost Host)
{
	GetCardDetailController().ForceHideHost(Host);
}

bool UBattleHUD::ComputeFirstPersonCardDetailTarget(
	const FWacomFirstPersonCardLayerSlotView& SlotView,
	FVector2D& OutPosition)
{
	return GetCardDetailController().ComputeFirstPersonTarget(SlotView, OutPosition);
}

FVector2D UBattleHUD::ComputeCardDetailPanelPositionBesideStable(
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

FVector2D UBattleHUD::GetFirstPersonCardDetailViewportSize() const
{
	return GetCardDetailController().GetFirstPersonViewportSize();
}

void UBattleHUD::SetFirstPersonCardDetailSource(const FGuid& CardInstanceId)
{
	GetCardDetailController().SetFirstPersonSource(CardInstanceId);
}

void UBattleHUD::ClearFirstPersonCardDetailSource()
{
	GetCardDetailController().ClearFirstPersonSource();
}

bool UBattleHUD::IsCurrentFirstPersonCardDetailSource(const FGuid& CardInstanceId) const
{
	return GetCardDetailController().IsCurrentFirstPersonSource(CardInstanceId);
}

void UBattleHUD::UpdateFirstPersonCardDetailSlot(const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetCardDetailController().UpdateFirstPersonSlot(SlotView);
}

FVector2D UBattleHUD::GetLastFirstPersonCardDetailPanelPosition() const
{
	return GetCardDetailController().GetLastFirstPersonPanelPosition();
}

const FHandCardSnapshot* UBattleHUD::FindLastBattleHandCardSnapshot(const FGuid& CardInstanceId) const
{
	if (!bHasLastBattleSnapshot || !CardInstanceId.IsValid())
	{
		return nullptr;
	}

	for (const FHandCardSnapshot& CardSnapshot : LastBattleSnapshot.Hand.Cards)
	{
		if (CardSnapshot.InstanceId == CardInstanceId)
		{
			return &CardSnapshot;
		}
	}
	return nullptr;
}

FWacomBattleHUDCardDetailController& UBattleHUD::GetCardDetailController()
{
	if (!CardDetailController)
	{
		CardDetailController = MakeShared<FWacomBattleHUDCardDetailController>(*this);
	}
	return *CardDetailController;
}

const FWacomBattleHUDCardDetailController& UBattleHUD::GetCardDetailController() const
{
	return const_cast<UBattleHUD*>(this)->GetCardDetailController();
}

FWacomBattleHUDCombatLogController& UBattleHUD::GetCombatLogController()
{
	if (!CombatLogController)
	{
		CombatLogController = MakeShared<FWacomBattleHUDCombatLogController>(*this);
	}
	return *CombatLogController;
}

const FWacomBattleHUDCombatLogController& UBattleHUD::GetCombatLogController() const
{
	return const_cast<UBattleHUD*>(this)->GetCombatLogController();
}

FWacomBattleHUDFirstPersonHandBridge& UBattleHUD::GetFirstPersonHandBridge()
{
	if (!FirstPersonHandBridge)
	{
		FirstPersonHandBridge = MakeShared<FWacomBattleHUDFirstPersonHandBridge>(*this);
	}
	return *FirstPersonHandBridge;
}

const FWacomBattleHUDFirstPersonHandBridge& UBattleHUD::GetFirstPersonHandBridge() const
{
	return const_cast<UBattleHUD*>(this)->GetFirstPersonHandBridge();
}

FWacomBattleHUDPresentationCoordinator& UBattleHUD::GetPresentationCoordinator()
{
	if (!PresentationCoordinator)
	{
		PresentationCoordinator = MakeShared<FWacomBattleHUDPresentationCoordinator>(*this);
	}
	return *PresentationCoordinator;
}

const FWacomBattleHUDPresentationCoordinator& UBattleHUD::GetPresentationCoordinator() const
{
	return const_cast<UBattleHUD*>(this)->GetPresentationCoordinator();
}

FWacomBattleHUDSceneEnemyTargetCoordinator& UBattleHUD::GetSceneEnemyTargetCoordinator()
{
	if (!SceneEnemyTargetCoordinator)
	{
		SceneEnemyTargetCoordinator = MakeShared<FWacomBattleHUDSceneEnemyTargetCoordinator>(*this);
	}
	return *SceneEnemyTargetCoordinator;
}

const FWacomBattleHUDSceneEnemyTargetCoordinator& UBattleHUD::GetSceneEnemyTargetCoordinator() const
{
	return const_cast<UBattleHUD*>(this)->GetSceneEnemyTargetCoordinator();
}

void UBattleHUD::RebuildBattleSceneEnemyPartWorldTargetRegistry()
{
	GetSceneEnemyTargetCoordinator().RebuildRegistry();
}

bool UBattleHUD::IsBattleSceneEnemyPartBridgeInCurrentRegistry(
	const UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge) const
{
	return GetSceneEnemyTargetCoordinator().IsBridgeInCurrentRegistry(Bridge);
}

void UBattleHUD::SyncBattleEnemyPartWorldTargets(const FBattleSnapshot& Snap)
{
	GetSceneEnemyTargetCoordinator().SyncWorldTargets(Snap);
}

void UBattleHUD::ClearBattleEnemyPartWorldTargets()
{
	GetSceneEnemyTargetCoordinator().ClearWorldTargets();
}

bool UBattleHUD::CanUpdateBattleSceneEnemyPartHoverProbe() const
{
	return GetSceneEnemyTargetCoordinator().CanUpdateHoverProbe();
}

FWacomBattleEnemyPartDragPredictionDebugInput
UBattleHUD::BuildBattleSceneEnemyPartHoverPredictionInput(
	const FWacomInteractionTargetHandle& TargetHandle) const
{
	return GetSceneEnemyTargetCoordinator().BuildHoverPredictionInput(TargetHandle);
}

void UBattleHUD::TickBattleSceneEnemyPartHoverProbe(float DeltaTime)
{
	GetSceneEnemyTargetCoordinator().TickHoverProbe(DeltaTime);
}

void UBattleHUD::UpdateBattleSceneEnemyPartHoverProbe()
{
	GetSceneEnemyTargetCoordinator().UpdateHoverProbe();
}

void UBattleHUD::ClearBattleSceneEnemyPartHoverProbe(FName Reason)
{
	GetSceneEnemyTargetCoordinator().ClearHoverProbe(Reason);
}

void UBattleHUD::ConsumeAndLogEvents()
{
	FWacomBattleHUDEventFlow::ConsumeAndLogEvents(*this);
}

void UBattleHUD::AppendBattleCombatLogBlock(const FWacomBattleCombatLogBlockView& Block)
{
	GetCombatLogController().AppendBlock(Block);
}

void UBattleHUD::StoreFirstPersonCardTransitionEvents(const TArray<FBattleEvent>& Events)
{
	GetFirstPersonHandBridge().StoreTransitionEvents(Events);
}

void UBattleHUD::ClearPendingFirstPersonCardTransitionEvents()
{
	GetFirstPersonHandBridge().ClearPendingTransitionEvents();
}

void UBattleHUD::RecordFirstPersonPlayCommit(
	const FGuid& CardInstanceId,
	const FGuid& TargetPartInstanceId)
{
	GetFirstPersonHandBridge().RecordPlayCommit(CardInstanceId, TargetPartInstanceId);
}

TArray<FWacomFirstPersonCardLayerTransitionHint> UBattleHUD::BuildFirstPersonCardTransitionHints(
	const FBattleSnapshot& PreviousSnapshot,
	const FBattleSnapshot& NextSnapshot) const
{
	return GetFirstPersonHandBridge().BuildTransitionHints(PreviousSnapshot, NextSnapshot);
}

int32 UBattleHUD::AppendBattlePresentationStackEntry(
	const FWacomBattleCombatLogCommandContext& CommandContext,
	const FBattleSnapshot& PreCommandSnapshot)
{
	return GetPresentationCoordinator().AppendStackEntry(CommandContext, PreCommandSnapshot);
}

void UBattleHUD::BeginBattlePresentationStackEntryExit(int32 EntryId)
{
	GetPresentationCoordinator().BeginStackEntryExit(EntryId);
}

void UBattleHUD::FinishBattlePresentationStackEntryExit(int32 EntryId)
{
	GetPresentationCoordinator().FinishStackEntryExit(EntryId);
}

void UBattleHUD::ClearBattlePresentationStack()
{
	if (PresentationCoordinator)
	{
		PresentationCoordinator->ClearStack();
	}
}

bool UBattleHUD::HasBattlePresentationStackEntries() const
{
	return PresentationCoordinator && PresentationCoordinator->HasStackEntries();
}

void UBattleHUD::EnqueueBattlePresentationEvents(
	const TArray<FBattleEvent>& Events,
	int32 PresentationStackEntryId)
{
	GetPresentationCoordinator().EnqueueEvents(Events, PresentationStackEntryId);
}

void UBattleHUD::ClearBattlePresentationQueue()
{
	if (PresentationCoordinator)
	{
		PresentationCoordinator->ClearQueue();
	}
}

bool UBattleHUD::IsBattlePresentationQueueBusy() const
{
	return PresentationCoordinator && PresentationCoordinator->IsQueueBusy();
}

void UBattleHUD::QueuePendingTurnBoundaryCommand(ETurnBoundaryCommand Command)
{
	switch (Command)
	{
	case ETurnBoundaryCommand::Wait:
		GetPresentationCoordinator().QueuePendingTurnBoundaryCommand(EWacomBattleHUDTurnBoundaryCommand::Wait);
		break;
	case ETurnBoundaryCommand::EndTurn:
		GetPresentationCoordinator().QueuePendingTurnBoundaryCommand(EWacomBattleHUDTurnBoundaryCommand::EndTurn);
		break;
	case ETurnBoundaryCommand::None:
	default:
		break;
	}
}

void UBattleHUD::ClearPendingTurnBoundaryCommand()
{
	if (PresentationCoordinator)
	{
		PresentationCoordinator->ClearPendingTurnBoundaryCommand();
	}
}

void UBattleHUD::TryExecutePendingTurnBoundaryCommand()
{
	if (PresentationCoordinator)
	{
		PresentationCoordinator->TryExecutePendingTurnBoundaryCommand();
	}
}

FWacomBattlePresentationTargetRegistry& UBattleHUD::GetBattlePresentationTargetRegistry()
{
	if (!BattlePresentationTargetRegistry)
	{
		BattlePresentationTargetRegistry = MakeShared<FWacomBattlePresentationTargetRegistry>();
	}
	return *BattlePresentationTargetRegistry;
}

void UBattleHUD::ClearBattlePresentationTargetRegistry()
{
	if (BattlePresentationTargetRegistry)
	{
		BattlePresentationTargetRegistry->Clear();
		BattlePresentationTargetRegistry.Reset();
	}
}

void UBattleHUD::RegisterBattlePresentationTarget(
	const FGuid& PartInstanceId,
	UObject* Owner,
	TFunction<void(const FWacomBattlePresentationTargetCue&)> Handler)
{
	GetBattlePresentationTargetRegistry().Register(PartInstanceId, Owner, MoveTemp(Handler));
}

void UBattleHUD::UnregisterBattlePresentationTargetsForOwner(const UObject* Owner)
{
	if (BattlePresentationTargetRegistry)
	{
		BattlePresentationTargetRegistry->UnregisterOwner(Owner);
	}
}

bool UBattleHUD::IsBattlePresentationTargetRegisteredForOwner(const UObject* Owner) const
{
	return BattlePresentationTargetRegistry
		&& BattlePresentationTargetRegistry->ContainsOwner(Owner);
}

void UBattleHUD::PlayBattlePresentationCue(const FWacomBattlePresentationTargetCue& Cue)
{
	if (BattlePresentationTargetRegistry)
	{
		BattlePresentationTargetRegistry->PlayCue(Cue);
	}
}

void UBattleHUD::PushPendingKnockdownChoiceDialog()
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

	Dialog->SetContext(this, ChoiceView);
}

void UBattleHUD::AdvanceBattlePresentationQueueOnce()
{
#if WITH_AUTOMATION_TESTS
	GetPresentationCoordinator().AdvanceQueueOnce();
#endif
}

#if WITH_AUTOMATION_TESTS
void UBattleHUD::PlayBattlePresentationCueForTest(
	EBattleEventType SourceEventType,
	const FGuid& TargetPartInstanceId,
	int32 Amount)
{
	FWacomBattlePresentationTargetCue Cue;
	Cue.SourceEventType = SourceEventType;
	Cue.TargetPartInstanceId = TargetPartInstanceId;
	Cue.Amount = Amount;
	PlayBattlePresentationCue(Cue);
}

void UBattleHUD::PlayTargetConfirmedCueForTest(const FGuid& TargetPartInstanceId)
{
	FWacomBattlePresentationTargetCue Cue;
	Cue.CueKind = EWacomBattlePresentationTargetCueKind::TargetConfirmed;
	Cue.TargetPartInstanceId = TargetPartInstanceId;
	Cue.Duration = 0.10f;
	PlayBattlePresentationCue(Cue);
}

FWacomBattleHUDAutomationTestView UBattleHUD::GetAutomationTestViewForTest() const
{
	static const TArray<FWacomBattlePresentationStackEntryView> EmptyEntries;
	static const TArray<FWacomBattleCombatLogBlockView> EmptyHistory;

	FWacomBattleHUDAutomationTestView View;
	View.PresentationTargetCount = BattlePresentationTargetRegistry ? BattlePresentationTargetRegistry->Num() : 0;
	View.SceneEnemyPartWorldTargetBridgeCount = GetSceneEnemyTargetCoordinator().GetRegisteredBridgeCount();
	View.PresentationStackEntries = PresentationCoordinator ? &PresentationCoordinator->GetStackEntries() : &EmptyEntries;
	View.CombatLogHistory = CombatLogController ? &CombatLogController->GetHistory() : &EmptyHistory;
	return View;
}
#endif

#undef LOCTEXT_NAMESPACE

