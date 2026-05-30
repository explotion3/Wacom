// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattleHUD.h"

#define LOCTEXT_NAMESPACE "WacomBattleHUD"
#include "UI/Battle/ActionPanel.h"
#include "Actors/WacomBattle3DHandPresenter.h"
#include "Actors/WacomBattleCardVisualActor.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "UI/Battle/EnemyInfoBar.h"
#include "UI/Battle/EquipmentBar.h"
#include "UI/Battle/EventToast.h"
#include "UI/Battle/BattleEventLogPanel.h"
#include "UI/Battle/HandPanel.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/Battle/CardWidget.h"
#include "UI/Battle/BattleHUDFallbackLayoutBuilder.h"
#include "UI/Battle/WacomBattleEventPresentationQueue.h"
#include "UI/Battle/WacomBattlePresentationTargetRegistry.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/Battle/WacomBattleHUDCommandFlow.h"
#include "UI/Battle/WacomBattleHUDEventFlow.h"
#include "UI/Battle/WacomBattleHUDTargetingFlow.h"
#include "UI/Battle/WacomKnockdownChoiceDialog.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Common/PileCountView.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomUITags.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "Input/WacomInputContextCoordinatorSubsystem.h"
#include "InputCoreTypes.h"
#include "UObject/UObjectIterator.h"

#include "Input/UIActionBindingHandle.h"
#include "Input/CommonUIInputSettings.h"

#include "Events/BattleEvent.h"
#include "Cards/CardDefinition.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Types/WacomEnums.h"

namespace
{
	const TCHAR* CardDetailPanelPath = TEXT("/Game/Wacom/UI/Card/WBP_CardDetailPanel.WBP_CardDetailPanel_C");
	const FName FirstPersonBattleHandLayerSourceId(TEXT("BattleHand"));

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
		case EWacomBattleTargetRejectReason::UnsupportedHandAnchorTarget: return TEXT("UnsupportedHandAnchorTarget");
		case EWacomBattleTargetRejectReason::UnsupportedZoneTarget: return TEXT("UnsupportedZoneTarget");
		default: return TEXT("Unknown");
		}
	}

	EWacomBattleCardDropRejectReason MapTargetValidationRejectReason(
		EWacomBattleTargetRejectReason RejectReason)
	{
		switch (RejectReason)
		{
		case EWacomBattleTargetRejectReason::None:
			return EWacomBattleCardDropRejectReason::None;
		case EWacomBattleTargetRejectReason::InvalidTarget:
			return EWacomBattleCardDropRejectReason::MissingTarget;
		case EWacomBattleTargetRejectReason::SourceCardInvalid:
		case EWacomBattleTargetRejectReason::SourceCardNotInHand:
		case EWacomBattleTargetRejectReason::SourceCardMissingDefinition:
			return EWacomBattleCardDropRejectReason::SourceCardInvalid;
		case EWacomBattleTargetRejectReason::UnsupportedWorldTarget:
		case EWacomBattleTargetRejectReason::InvalidWorldTarget:
			return EWacomBattleCardDropRejectReason::InvalidWorldTarget;
		case EWacomBattleTargetRejectReason::UnsupportedCardTarget:
		case EWacomBattleTargetRejectReason::TargetCardInvalid:
		case EWacomBattleTargetRejectReason::TargetCardNotInHand:
		case EWacomBattleTargetRejectReason::UnsupportedHandAnchorTarget:
			return EWacomBattleCardDropRejectReason::UnsupportedCardTarget;
		case EWacomBattleTargetRejectReason::SelfTarget:
			return EWacomBattleCardDropRejectReason::SelfTarget;
		case EWacomBattleTargetRejectReason::UnsupportedZoneTarget:
			return EWacomBattleCardDropRejectReason::UnsupportedZoneTarget;
		default:
			return EWacomBattleCardDropRejectReason::MissingTarget;
		}
	}

	bool ContainsHandCardId(const FBattleSnapshot& Snapshot, const FGuid& CardInstanceId)
	{
		if (!CardInstanceId.IsValid())
		{
			return false;
		}

		for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
		{
			if (CardSnapshot.InstanceId == CardInstanceId)
			{
				return true;
			}
		}
		return false;
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
	if (HandPanel)       { ChildBattleWidgets.Add(HandPanel); }
	if (EnemyInfoBar)    { ChildBattleWidgets.Add(EnemyInfoBar); }
	if (ActionPanel)     { ChildBattleWidgets.Add(ActionPanel); }
	if (EquipmentBar)    { ChildBattleWidgets.Add(EquipmentBar); }
	if (EventLogPanel)   { ChildBattleWidgets.Add(EventLogPanel); }

	if (HandPanel)
	{
		HandPanel->OnCardHoveredNative.RemoveAll(this);
		HandPanel->OnCardUnhoveredNative.RemoveAll(this);
		HandPanel->OnCardHoveredNative.AddUObject(this, &UBattleHUD::HandleHandCardHovered);
		HandPanel->OnCardUnhoveredNative.AddUObject(this, &UBattleHUD::HandleHandCardUnhovered);
	}

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

	SyncLegacyHandPanelVisibility();
}

void UBattleHUD::NativeDestruct()
{
	ClearBattlePresentationQueue();
	ClearFirstPersonBattleHandLayer();
	ClearBattleEnemyPartWorldTargets();
	DestroyBattle3DHandPresenter();
	ClearBattlePresentationTargetRegistry();
	ReleaseAllPlayerControllerInteractionEvents();
	if (HandPanel)
	{
		SyncLegacyHandPanelVisibility();
		HandPanel->OnCardHoveredNative.RemoveAll(this);
		HandPanel->OnCardUnhoveredNative.RemoveAll(this);
	}
	HideCardDetailPanel();
	CardDetailPanel = nullptr;
	if (FirstPersonCardDetailPanel)
	{
		FirstPersonCardDetailPanel->RemoveFromParent();
		FirstPersonCardDetailPanel = nullptr;
	}
	bHasLastBattleSnapshot = false;
	LastBattleSnapshot = FBattleSnapshot();
	bHasLastFirstPersonCardTransitionSnapshot = false;
	LastFirstPersonCardTransitionSnapshot = FBattleSnapshot();
	ClearPendingFirstPersonCardTransitionEvents();
	Super::NativeDestruct();
}

void UBattleHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TickCardDetailMotion(InDeltaTime);
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
			HandPanelSize,
			HandPanelBottomOffset,
			&EnemyInfoBar,
			&PlayerStatusBar,
			&HandPanel,
			&ActionPanel,
			&EquipmentBar,
			&DrawPileView,
			&DiscardPileView,
			&ExhaustPileView,
			&EventToast,
			&EventLogPanel,
			&CardDetailLayer});
	}
	return Super::RebuildWidget();
}

void UBattleHUD::NativeRefreshFromSnapshot(const FBattleSnapshot& Snap)
{
	HideCardDetailPanel();
	const bool bCanBuildFirstPersonTransitionHints =
		bHasLastFirstPersonCardTransitionSnapshot
		&& LastFirstPersonCardTransitionSnapshot.Phase != EBattlePhase::BattleEnd
		&& Snap.Phase != EBattlePhase::BattleEnd;
	const TArray<FWacomFirstPersonCardLayerTransitionHint> FirstPersonTransitionHints =
		bCanBuildFirstPersonTransitionHints
			? BuildFirstPersonCardTransitionHints(LastFirstPersonCardTransitionSnapshot, Snap)
			: TArray<FWacomFirstPersonCardLayerTransitionHint>();
	ClearPendingFirstPersonCardTransitionEvents();
	LastBattleSnapshot = Snap;
	bHasLastBattleSnapshot = true;
	LastFirstPersonCardTransitionSnapshot = Snap;
	bHasLastFirstPersonCardTransitionSnapshot = true;

	if (bEnable3DHandPrototype)
	{
		if (AWacomBattle3DHandPresenter* Presenter = EnsureBattle3DHandPresenter())
		{
			Presenter->RefreshFromSnapshot(Snap);
			SyncBattle3DHandPresenterTargeting();
		}
	}
	else
	{
		DestroyBattle3DHandPresenter();
	}

	SyncFirstPersonBattleHandLayer(Snap, FirstPersonTransitionHints);
	SyncLegacyHandPanelVisibility();

	// 战斗结束 → 切到 BattleEnd 状态，并广播一次
	if (Snap.Phase == EBattlePhase::BattleEnd)
	{
		ClearPendingFirstPersonCardTransitionEvents();
		bHasLastBattleSnapshot = false;
		bHasLastFirstPersonCardTransitionSnapshot = false;
		SetUIState(EBattleUIState::BattleEnd);
		SyncLegacyHandPanelVisibility();

		if (!bHasBroadcastBattleEnd)
		{
			bHasBroadcastBattleEnd = true;
			OnBattleEndedNative.Broadcast(Snap.Outcome);
		}
	}

	// 手动刷新 PileCountView（它们不是 BattleWidget，不走递归）
	if (DrawPileView)    { DrawPileView->SetCount(Snap.PileCounts.DrawCount); }
	if (DiscardPileView) { DiscardPileView->SetCount(Snap.PileCounts.DiscardCount); }
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
		ClearFirstPersonBattleHandLayer();
		ClearBattleEnemyPartWorldTargets();
		ClearPendingFirstPersonCardTransitionEvents();
		DestroyBattle3DHandPresenter();
		ClearBattlePresentationTargetRegistry();
		ReleaseAllPlayerControllerInteractionEvents();
	}

	// 新 Session 接入时，重置状态机到 Idle。
	UIState = EBattleUIState::Idle;
	PendingTargetingCardId.Invalidate();
	bHasBroadcastBattleEnd = false;
	bHasLastBattleSnapshot = false;
	LastBattleSnapshot = FBattleSnapshot();
	bHasLastFirstPersonCardTransitionSnapshot = false;
	LastFirstPersonCardTransitionSnapshot = FBattleSnapshot();
	ClearPendingFirstPersonCardTransitionEvents();
	HideCardDetailPanel();
	BattleEventLogHistory.Reset();
	SyncBattleEventLogPanel();

	if (bEnable3DHandPrototype && NewSession)
	{
		EnsureBattle3DHandPresenter();
	}
	else
	{
		DestroyBattle3DHandPresenter();
	}

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

void UBattleHUD::ToggleBattleEventLog()
{
	SetBattleEventLogOpen(!IsBattleEventLogOpen());
}

void UBattleHUD::SetBattleEventLogOpen(bool bOpen)
{
	if (EventLogPanel)
	{
		EventLogPanel->SetDrawerOpen(bOpen);
	}
}

bool UBattleHUD::IsBattleEventLogOpen() const
{
	return EventLogPanel && EventLogPanel->IsDrawerOpen();
}

bool UBattleHUD::IsBattlePresentationBusy() const
{
	return IsBattlePresentationQueueBusy();
}

void UBattleHUD::SetBattleHandPresentationMode(EWacomBattleHandPresentationMode NewMode)
{
	if (BattleHandPresentationMode == NewMode)
	{
		return;
	}

	BattleHandPresentationMode = NewMode;
	if (BattleHandPresentationMode == EWacomBattleHandPresentationMode::LegacyHandPanel)
	{
		ClearFirstPersonBattleHandLayer();
		return;
	}

	if (UBattleSession* S = GetSession())
	{
		SyncFirstPersonBattleHandLayer(S->BuildSnapshot());
	}
	else
	{
		ClearFirstPersonBattleHandLayer();
	}
	SyncLegacyHandPanelVisibility();
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

	// 状态变化时，让 HandPanel / EnemyInfoBar / ActionPanel 重新刷一次（高亮/启用状态）。
	UBattleSession* S = GetSession();
	SyncBattle3DHandPresenterTargeting();
	if (!S) { return; }
	const FBattleSnapshot Snap = S->BuildSnapshot();
	if (Snap.Phase == EBattlePhase::BattleEnd)
	{
		LastBattleSnapshot = FBattleSnapshot();
		bHasLastBattleSnapshot = false;
		LastFirstPersonCardTransitionSnapshot = FBattleSnapshot();
		bHasLastFirstPersonCardTransitionSnapshot = false;
	}
	else
	{
		LastBattleSnapshot = Snap;
		bHasLastBattleSnapshot = true;
	}
	if (HandPanel)    { HandPanel->RefreshFromSnapshot(Snap); }
	if (EnemyInfoBar) { EnemyInfoBar->RefreshFromSnapshot(Snap); }
	if (ActionPanel)  { ActionPanel->RefreshFromSnapshot(Snap); }
	SyncFirstPersonBattleHandLayer(Snap);
	SyncBattleEnemyPartWorldTargets(Snap);
	SyncLegacyHandPanelVisibility();
}

UWacomFirstPersonCardAnchorComponent* UBattleHUD::ResolveFirstPersonCardAnchor() const
{
	const APlayerController* PC = GetOwningPlayer();
	const AWacomPlayerCharacter* Character = PC ? Cast<AWacomPlayerCharacter>(PC->GetPawn()) : nullptr;
	return Character ? Character->GetFirstPersonCardAnchorComponent() : nullptr;
}

UWacomFirstPersonCardAnchorComponent* UBattleHUD::ResolveActiveFirstPersonCardAnchor() const
{
	if (UWacomFirstPersonCardAnchorComponent* LastAnchor = LastFirstPersonBattleHandAnchor.Get())
	{
		return LastAnchor;
	}

	return ResolveFirstPersonCardAnchor();
}

void UBattleHUD::SyncFirstPersonBattleHandLayer(
	const FBattleSnapshot& Snap,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints)
{
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchor();
	const bool bCanShowBattleHand =
		ShouldUseFirstPersonBattleHandLayer()
		&& GetSession()
		&& Snap.Phase != EBattlePhase::BattleEnd
		&& UIState != EBattleUIState::BattleEnd
		&& Anchor;
	if (!bCanShowBattleHand)
	{
		ClearFirstPersonBattleHandLayer();
		return;
	}

	bFirstPersonBattleHandLayerRuntimeActive = true;
	TArray<FWacomFirstPersonCardLayerEntry> CardEntries;
	CardEntries.Reserve(Snap.Hand.Cards.Num());
	for (const FHandCardSnapshot& CardSnapshot : Snap.Hand.Cards)
	{
		FWacomCardViewData Data = UWacomCardPresentationBuilder::BuildCardViewData(CardSnapshot.Definition);
		Data.Cost = CardSnapshot.RuntimeCost;
		Data.bShowCost = CardSnapshot.Definition != nullptr;
		Data.bDisabled = !CardSnapshot.bIsPlayable;

		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardInstanceId = CardSnapshot.InstanceId;
		Entry.CardViewData = MoveTemp(Data);
		Entry.Zone = CardSnapshot.Zone;
		Entry.bIsHandAnchor = CardSnapshot.bIsHandAnchor;
		Entry.bIsPlayable = CardSnapshot.bIsPlayable;
		Entry.TargetMode = CardSnapshot.Definition
			? CardSnapshot.Definition->TargetMode
			: ECardTargetMode::None;
		Entry.bIsPendingTargeting =
			IsInTargetSelect()
			&& PendingTargetingCardId.IsValid()
			&& CardSnapshot.InstanceId == PendingTargetingCardId;
		CardEntries.Add(MoveTemp(Entry));
	}

	Anchor->SetRuntimeCardLayerTransitionHints(FirstPersonBattleHandLayerSourceId, TransitionHints);
	Anchor->SetRuntimeCardLayerEntries(FirstPersonBattleHandLayerSourceId, CardEntries);
	Anchor->SetBattleHandInteractionPrototypeEnabled(ShouldEnableFirstPersonBattleHandInteraction());
	BindFirstPersonBattleHandLayerInteractions(Anchor);
	LastFirstPersonBattleHandAnchor = Anchor;
	SyncLegacyHandPanelVisibility();
}

void UBattleHUD::ClearFirstPersonBattleHandLayer()
{
	bFirstPersonBattleHandLayerRuntimeActive = false;
	if (UWacomFirstPersonCardAnchorComponent* LastAnchor = LastFirstPersonBattleHandAnchor.Get())
	{
		LastAnchor->SetBattleHandInteractionPrototypeEnabled(false);
		LastAnchor->CancelFirstPersonCardDragGesture(true);
		UnbindFirstPersonBattleHandLayerInteractions(LastAnchor);
		LastAnchor->ClearRuntimeCardLayerData(FirstPersonBattleHandLayerSourceId);
	}
	if (UWacomFirstPersonCardAnchorComponent* CurrentAnchor = ResolveFirstPersonCardAnchor())
	{
		CurrentAnchor->SetBattleHandInteractionPrototypeEnabled(false);
		CurrentAnchor->CancelFirstPersonCardDragGesture(true);
		UnbindFirstPersonBattleHandLayerInteractions(CurrentAnchor);
		CurrentAnchor->ClearRuntimeCardLayerData(FirstPersonBattleHandLayerSourceId);
	}
	ClearFirstPersonCardDragCameraLookOverride();
	ClearFirstPersonCardDragTargetFeedback();
	LastFirstPersonBattleHandAnchor.Reset();
	ForceHideCardDetailHost(ECardDetailHost::FirstPersonViewport);
	ClearPendingFirstPersonCardTransitionEvents();
	SyncLegacyHandPanelVisibility();
}

void UBattleHUD::SyncLegacyHandPanelVisibility()
{
	if (!HandPanel)
	{
		return;
	}

	if (ShouldHideLegacyHandPanel())
	{
		if (!bLegacyHandPanelHiddenByFirstPersonLayer)
		{
			CaptureLegacyHandPanelVisibilityIfNeeded();
		}
		HandPanel->SetVisibility(ESlateVisibility::Collapsed);
		bLegacyHandPanelHiddenByFirstPersonLayer = true;
		return;
	}

	if (bLegacyHandPanelHiddenByFirstPersonLayer && bHasCachedLegacyHandPanelVisibility)
	{
		HandPanel->SetVisibility(CachedLegacyHandPanelVisibility);
	}
	bLegacyHandPanelHiddenByFirstPersonLayer = false;
}

bool UBattleHUD::ShouldHideLegacyHandPanel() const
{
	return BattleHandPresentationMode == EWacomBattleHandPresentationMode::FirstPersonHandOnly
		&& ShouldEnableFirstPersonBattleHandInteraction()
		&& bFirstPersonBattleHandLayerRuntimeActive
		&& GetSession()
		&& UIState != EBattleUIState::BattleEnd
		&& LastFirstPersonBattleHandAnchor.IsValid();
}

bool UBattleHUD::ShouldUseFirstPersonBattleHandLayer() const
{
	return BattleHandPresentationMode == EWacomBattleHandPresentationMode::FirstPersonHandWithLegacyFallback
		|| BattleHandPresentationMode == EWacomBattleHandPresentationMode::FirstPersonHandOnly;
}

bool UBattleHUD::ShouldEnableFirstPersonBattleHandInteraction() const
{
	return ShouldUseFirstPersonBattleHandLayer();
}

void UBattleHUD::CaptureLegacyHandPanelVisibilityIfNeeded()
{
	if (!HandPanel)
	{
		return;
	}

	CachedLegacyHandPanelVisibility = HandPanel->GetVisibility();
	bHasCachedLegacyHandPanelVisibility = true;
}

void UBattleHUD::BindFirstPersonBattleHandLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor)
{
	if (!Anchor)
	{
		return;
	}

	Anchor->OnFirstPersonCardLayerCardClicked.RemoveAll(this);
	Anchor->OnFirstPersonCardLayerCardHovered.RemoveAll(this);
	Anchor->OnFirstPersonCardLayerCardUnhovered.RemoveAll(this);
	Anchor->OnFirstPersonCardLayerHoveredCardLayoutUpdated.RemoveAll(this);
	Anchor->OnFirstPersonCardLayerDragStarted.RemoveAll(this);
	Anchor->OnFirstPersonCardLayerDragUpdated.RemoveAll(this);
	Anchor->OnFirstPersonCardLayerDragReleased.RemoveAll(this);
	Anchor->OnFirstPersonCardLayerDragCancelled.RemoveAll(this);
	Anchor->OnFirstPersonCardLayerCardClicked.AddUObject(this, &UBattleHUD::HandleFirstPersonCardLayerCardClicked);
	Anchor->OnFirstPersonCardLayerCardHovered.AddUObject(this, &UBattleHUD::HandleFirstPersonCardLayerCardHovered);
	Anchor->OnFirstPersonCardLayerCardUnhovered.AddUObject(this, &UBattleHUD::HandleFirstPersonCardLayerCardUnhovered);
	Anchor->OnFirstPersonCardLayerHoveredCardLayoutUpdated.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerHoveredCardLayoutUpdated);
	Anchor->OnFirstPersonCardLayerDragStarted.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerDragStarted);
	Anchor->OnFirstPersonCardLayerDragUpdated.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerDragUpdated);
	Anchor->OnFirstPersonCardLayerDragReleased.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerDragReleased);
	Anchor->OnFirstPersonCardLayerDragCancelled.AddUObject(
		this,
		&UBattleHUD::HandleFirstPersonCardLayerDragCancelled);
}

void UBattleHUD::UnbindFirstPersonBattleHandLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor)
{
	if (!Anchor)
	{
		return;
	}

	Anchor->OnFirstPersonCardLayerCardClicked.RemoveAll(this);
	Anchor->OnFirstPersonCardLayerCardHovered.RemoveAll(this);
	Anchor->OnFirstPersonCardLayerCardUnhovered.RemoveAll(this);
	Anchor->OnFirstPersonCardLayerHoveredCardLayoutUpdated.RemoveAll(this);
	Anchor->OnFirstPersonCardLayerDragStarted.RemoveAll(this);
	Anchor->OnFirstPersonCardLayerDragUpdated.RemoveAll(this);
	Anchor->OnFirstPersonCardLayerDragReleased.RemoveAll(this);
	Anchor->OnFirstPersonCardLayerDragCancelled.RemoveAll(this);
}

void UBattleHUD::HandleFirstPersonCardLayerCardClicked(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& /*SlotView*/)
{
	if (!ShouldEnableFirstPersonBattleHandInteraction())
	{
		return;
	}
	OnCardClickedByUser(CardInstanceId);
}

void UBattleHUD::HandleFirstPersonCardLayerCardHovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (!ShouldEnableFirstPersonBattleHandInteraction()
		|| UIState != EBattleUIState::Idle
		|| !CardInstanceId.IsValid()
		|| !SlotView.bProjected)
	{
		ForceHideCardDetailHost(ECardDetailHost::FirstPersonViewport);
		return;
	}

	const FHandCardSnapshot* CardSnapshot = FindLastBattleHandCardSnapshot(CardInstanceId);
	if (!CardSnapshot || !CardSnapshot->Definition)
	{
		ForceHideCardDetailHost(ECardDetailHost::FirstPersonViewport);
		return;
	}

	CurrentFirstPersonCardDetailSourceId = CardInstanceId;
	if (ShowFirstPersonCardDetailAtSlot(
		UWacomCardPresentationBuilder::BuildCardDetailViewData(CardSnapshot->Definition),
		SlotView))
	{
		ForceHideCardDetailHost(ECardDetailHost::LegacyHandPanel);
		CurrentCardDetailSource.Reset();
	}
	else
	{
		CurrentFirstPersonCardDetailSourceId.Invalidate();
	}
}

void UBattleHUD::HandleFirstPersonCardLayerCardUnhovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& /*SlotView*/)
{
	if (IsFirstPersonCardInspectDetailActiveForSource(CardInstanceId))
	{
		return;
	}
	HideFirstPersonCardDetailPanelForSource(CardInstanceId);
}

void UBattleHUD::HandleFirstPersonCardLayerHoveredCardLayoutUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (!ShouldEnableFirstPersonBattleHandInteraction()
		|| UIState != EBattleUIState::Idle
		|| !CardInstanceId.IsValid()
		|| CurrentFirstPersonCardDetailSourceId != CardInstanceId
		|| !SlotView.bProjected)
	{
		return;
	}

	CardDetailMotionState.ActiveFirstPersonSlot = SlotView;
	CardDetailMotionState.bHasActiveFirstPersonSlot = true;
	PositionFirstPersonCardDetailPanelBesideSlot(SlotView);
}

void UBattleHUD::HandleFirstPersonCardLayerDragStarted(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (!ShouldEnableFirstPersonBattleHandInteraction() || !CardInstanceId.IsValid())
	{
		return;
	}

	if (ShouldShowFirstPersonDragInspectDetail(DragView))
	{
		const FHandCardSnapshot* CardSnapshot = FindLastBattleHandCardSnapshot(CardInstanceId);
		if (CardSnapshot && CardSnapshot->Definition)
		{
			CurrentFirstPersonCardDetailSourceId = CardInstanceId;
			if (ShowFirstPersonCardDetailAtSlot(
				UWacomCardPresentationBuilder::BuildCardDetailViewData(CardSnapshot->Definition),
				DragView.SourceSlotView))
			{
				ForceHideCardDetailHost(ECardDetailHost::LegacyHandPanel);
				CurrentCardDetailSource.Reset();
			}
		}
	}
	else
	{
		ForceHideCardDetailHost(ECardDetailHost::FirstPersonViewport);
	}

	HandleFirstPersonCardLayerDragUpdated(CardInstanceId, DragView);
}

void UBattleHUD::HandleFirstPersonCardLayerDragUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (!ShouldEnableFirstPersonBattleHandInteraction() || !CardInstanceId.IsValid())
	{
		return;
	}

	ApplyFirstPersonCardDragCameraLookOverride(DragView);

	if (ShouldShowFirstPersonDragInspectDetail(DragView)
		&& CurrentFirstPersonCardDetailSourceId == CardInstanceId
		&& DragView.SourceSlotView.bProjected)
	{
		CardDetailMotionState.ActiveFirstPersonSlot = DragView.SourceSlotView;
		CardDetailMotionState.bHasActiveFirstPersonSlot = true;
		PositionFirstPersonCardDetailPanelBesideSlot(DragView.SourceSlotView);
		return;
	}
	else if (DragView.GestureState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
		|| DragView.GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit
		|| DragView.GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard)
	{
		ForceHideCardDetailHost(ECardDetailHost::FirstPersonViewport);
	}

	UpdateFirstPersonCardDragTargetFeedback(CardInstanceId, DragView);
}

void UBattleHUD::HandleFirstPersonCardLayerDragReleased(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (!ShouldEnableFirstPersonBattleHandInteraction() || !CardInstanceId.IsValid())
	{
		return;
	}

	const FWacomFirstPersonCardDragView ReleaseDragView = DragView;
	const FWacomBattleCardDropResolveResult DropResult =
		ResolveFirstPersonCardDropIntent(CardInstanceId, ReleaseDragView);

	ClearFirstPersonCardDragCameraLookOverride();
	if (UWacomBattleEnemyPartWorldTargetBridgeComponent* PreviewBridge = CurrentFirstPersonDragPreviewBridge.Get())
	{
		PreviewBridge->ClearDragTargetPreviewState();
	}
	CurrentFirstPersonDragPreviewBridge.Reset();
	ForceHideCardDetailHost(ECardDetailHost::FirstPersonViewport);

	if (!DropResult.bCanSubmit)
	{
		return;
	}

	switch (DropResult.IntentKind)
	{
	case EWacomBattleCardDropIntentKind::PlayCardNoTarget:
		SubmitPlayCard(CardInstanceId, FGuid());
		return;

	case EWacomBattleCardDropIntentKind::PlayCardWorldTarget:
		SubmitPlayCard(CardInstanceId, DropResult.TargetHandle.WorldTargetId);
		return;

	case EWacomBattleCardDropIntentKind::PlayCardCardTarget:
		SubmitPlayCardOnHandCard(CardInstanceId, DropResult.TargetHandle.CardInstanceId);
		return;

	default:
		return;
	}
}

void UBattleHUD::HandleFirstPersonCardLayerDragCancelled(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& /*DragView*/)
{
	ClearFirstPersonCardDragCameraLookOverride();
	ClearFirstPersonCardDragTargetFeedback();
	HideFirstPersonCardDetailPanelForSource(CardInstanceId);
	if (UWacomFirstPersonCardAnchorComponent* Anchor = ResolveActiveFirstPersonCardAnchor())
	{
		Anchor->SetFirstPersonCardDragFeedbackTarget(
			FWacomInteractionTargetHandle(),
			false,
			EWacomFirstPersonCardDragTargetFeedbackState::None);
	}
}

void UBattleHUD::ApplyFirstPersonCardDragCameraLookOverride(
	const FWacomFirstPersonCardDragView& DragView)
{
	if (!DragView.bHasPointerViewportPosition)
	{
		return;
	}

	const UWacomFirstPersonCardAnchorComponent* Anchor = ResolveActiveFirstPersonCardAnchor();
	if (!Anchor
		|| !Anchor->bAllowCameraLookDuringCardDrag
		|| Anchor->CardDragCameraLookScale <= 0.0f)
	{
		ClearFirstPersonCardDragCameraLookOverride();
		return;
	}

	AWacomPlayerCharacter* Character = nullptr;
	if (const APlayerController* PC = GetOwningPlayer())
	{
		Character = Cast<AWacomPlayerCharacter>(PC->GetPawn());
	}
	UWacomBattleCameraLookComponent* BattleCamera = Character
		? Character->GetBattleCameraLookComponent()
		: nullptr;
	if (!BattleCamera || !BattleCamera->IsBattleCameraLookActive())
	{
		return;
	}

	BattleCamera->SetCursorLookOverrideNormalized(
		DragView.PointerNormalizedViewportPosition,
		Anchor->CardDragCameraLookScale,
		Anchor->CardDragCameraLookInterpSpeedOverride);
}

void UBattleHUD::ClearFirstPersonCardDragCameraLookOverride()
{
	const APlayerController* PC = GetOwningPlayer();
	AWacomPlayerCharacter* Character = PC ? Cast<AWacomPlayerCharacter>(PC->GetPawn()) : nullptr;
	if (UWacomBattleCameraLookComponent* BattleCamera = Character ? Character->GetBattleCameraLookComponent() : nullptr)
	{
		BattleCamera->ClearCursorLookOverride();
	}
}

void UBattleHUD::UpdateFirstPersonCardDragTargetFeedback(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	const FWacomBattleCardDropResolveResult DropResult =
		ResolveFirstPersonCardDropIntent(CardInstanceId, DragView);
	TArray<FWacomFirstPersonCardTargetAffordance> CardTargetAffordances;
	if (DragView.GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard)
	{
		if (const UBattleSession* CurrentSession = GetSession())
		{
			const FBattleSnapshot CurrentSnapshot = CurrentSession->BuildSnapshot();
			CardTargetAffordances = BuildFirstPersonCardTargetAffordances(
				CardInstanceId,
				CurrentSnapshot,
				*CurrentSession);
		}
	}

	EWacomFirstPersonCardDragTargetFeedbackState FeedbackState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	TOptional<FVector2D> FeedbackTargetPosition;
	UWacomBattleEnemyPartWorldTargetBridgeComponent* PreviewBridge = nullptr;

	switch (DropResult.IntentKind)
	{
	case EWacomBattleCardDropIntentKind::PlayCardNoTarget:
		FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::CommitReady;
		break;

	case EWacomBattleCardDropIntentKind::PlayCardWorldTarget:
		FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget;
		break;

	case EWacomBattleCardDropIntentKind::PlayCardCardTarget:
		FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget;
		break;

	case EWacomBattleCardDropIntentKind::ProbeCardTarget:
		FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::CardProbe;
		break;

	case EWacomBattleCardDropIntentKind::Reject:
		if (DropResult.TargetHandle.TargetKind == EWacomInteractionTargetKind::Card)
		{
			FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget;
		}
		else if (DragView.GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard
			|| DropResult.TargetHandle.TargetKind == EWacomInteractionTargetKind::World)
		{
			FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::Invalid;
		}
		break;

	case EWacomBattleCardDropIntentKind::None:
	default:
		break;
	}

	if (DropResult.bHasFeedbackTargetScreenPosition)
	{
		FeedbackTargetPosition = DropResult.FeedbackTargetScreenPosition;
	}
	if (DropResult.TargetHandle.TargetKind == EWacomInteractionTargetKind::World)
	{
		PreviewBridge = ResolveBattleEnemyPartWorldTargetBridge(DropResult.TargetHandle);
	}

	if (CurrentFirstPersonDragPreviewBridge.Get() != PreviewBridge)
	{
		if (UWacomBattleEnemyPartWorldTargetBridgeComponent* PreviousBridge =
			CurrentFirstPersonDragPreviewBridge.Get())
		{
			PreviousBridge->ClearDragTargetPreviewState();
		}
		CurrentFirstPersonDragPreviewBridge = PreviewBridge;
	}
	if (PreviewBridge)
	{
		PreviewBridge->SetDragTargetPreviewState(FeedbackState);
	}

	if (UWacomFirstPersonCardAnchorComponent* Anchor = ResolveActiveFirstPersonCardAnchor())
	{
		Anchor->SetFirstPersonCardDragFeedbackTarget(
			DropResult.TargetHandle,
			DropResult.bCanSubmit,
			FeedbackState,
			FeedbackTargetPosition,
			DropResult.ToDebugString(),
			CardTargetAffordances);
	}
}

void UBattleHUD::ClearFirstPersonCardDragTargetFeedback()
{
	if (UWacomBattleEnemyPartWorldTargetBridgeComponent* PreviewBridge = CurrentFirstPersonDragPreviewBridge.Get())
	{
		PreviewBridge->ClearDragTargetPreviewState();
	}
	CurrentFirstPersonDragPreviewBridge.Reset();
	if (UWacomFirstPersonCardAnchorComponent* Anchor = ResolveActiveFirstPersonCardAnchor())
	{
		Anchor->SetFirstPersonCardDragFeedbackTarget(
			FWacomInteractionTargetHandle(),
			false,
			EWacomFirstPersonCardDragTargetFeedbackState::None);
	}
}

FWacomBattleCardDropResolveResult UBattleHUD::ResolveFirstPersonCardDropIntent(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView) const
{
	FWacomBattleCardDropResolveResult Result;
	Result.SourceCardInstanceId = CardInstanceId;

	if (!CardInstanceId.IsValid())
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::SourceCardInvalid;
		return Result;
	}

	if (UIState == EBattleUIState::BattleEnd
		|| UIState == EBattleUIState::Resolving
		|| IsBattlePresentationQueueBusy())
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::UIBlocked;
		return Result;
	}

	const UBattleSession* CurrentSession = GetSession();
	if (!CurrentSession)
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::MissingSession;
		return Result;
	}

	const FBattleSnapshot CurrentSnapshot = CurrentSession->BuildSnapshot();
	const FHandCardSnapshot* CardSnapshot = nullptr;
	for (const FHandCardSnapshot& Candidate : CurrentSnapshot.Hand.Cards)
	{
		if (Candidate.InstanceId == CardInstanceId)
		{
			CardSnapshot = &Candidate;
			break;
		}
	}
	if (!CardSnapshot || !CardSnapshot->Definition)
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::SourceCardInvalid;
		return Result;
	}
	if (!CardSnapshot->bIsPlayable)
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::SourceCardNotPlayable;
		return Result;
	}

	if (DragView.GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit)
	{
		if (DragView.bCommitArmed)
		{
			Result.IntentKind = EWacomBattleCardDropIntentKind::PlayCardNoTarget;
			Result.bCanSubmit = true;
			return Result;
		}

		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::NotArmed;
		return Result;
	}

	if (DragView.GestureState != EWacomFirstPersonCardGestureState::AimingTargetedCard)
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::NotArmed;
		return Result;
	}

	FWacomInteractionTargetHandle CandidateTarget;
	bool bIgnoredValidTarget = false;
	bool bHasTarget = false;
	if (DragView.CurrentTarget.IsValid()
		&& (DragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Card
			|| DragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Zone))
	{
		CandidateTarget = DragView.CurrentTarget;
		bHasTarget = true;
	}
	else
	{
		bHasTarget = ProbeFirstPersonCardDragTarget(
			CardInstanceId,
			DragView,
			CandidateTarget,
			bIgnoredValidTarget);
	}
	Result.TargetHandle = CandidateTarget;
	if (!CandidateTarget.ScreenPosition.IsNearlyZero())
	{
		Result.bHasFeedbackTargetScreenPosition = true;
		Result.FeedbackTargetScreenPosition = CandidateTarget.ScreenPosition;
	}

	if (!bHasTarget || !CandidateTarget.IsValid())
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::MissingTarget;
		return Result;
	}

	switch (CandidateTarget.TargetKind)
	{
	case EWacomInteractionTargetKind::World:
	{
		const FWacomBattleTargetValidationResult Validation =
			CurrentSession->ValidateTargetWithCard(CardInstanceId, CandidateTarget);
		Result.TargetValidationRejectReason = Validation.RejectReason;
		Result.TargetValidationDebugSummary = Validation.DebugSummary;
		if (Validation.bCanTarget && CandidateTarget.WorldTargetId.IsValid())
		{
			Result.IntentKind = EWacomBattleCardDropIntentKind::PlayCardWorldTarget;
			Result.bCanSubmit = true;
		}
		else
		{
			Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
			Result.RejectReason = MapTargetValidationRejectReason(Validation.RejectReason);
		}
		return Result;
	}

	case EWacomInteractionTargetKind::Card:
	{
		const FWacomBattleTargetValidationResult Validation =
			CurrentSession->ValidateTargetWithCard(CardInstanceId, CandidateTarget);
		Result.TargetValidationRejectReason = Validation.RejectReason;
		Result.TargetValidationDebugSummary = Validation.DebugSummary;
		if (Validation.RejectReason == EWacomBattleTargetRejectReason::SelfTarget)
		{
			Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
			Result.RejectReason = EWacomBattleCardDropRejectReason::SelfTarget;
			return Result;
		}
		if (Validation.bCanTarget)
		{
			Result.IntentKind = EWacomBattleCardDropIntentKind::PlayCardCardTarget;
			Result.bCanSubmit = true;
			return Result;
		}

		if (CardSnapshot->Definition->TargetMode == ECardTargetMode::HandCard)
		{
			Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
			Result.RejectReason = MapTargetValidationRejectReason(Validation.RejectReason);
			return Result;
		}

		Result.IntentKind = EWacomBattleCardDropIntentKind::ProbeCardTarget;
		Result.RejectReason = EWacomBattleCardDropRejectReason::UnsupportedCardTarget;
		Result.TargetValidationRejectReason = EWacomBattleTargetRejectReason::UnsupportedCardTarget;
		return Result;
	}

	case EWacomInteractionTargetKind::Zone:
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::UnsupportedZoneTarget;
		return Result;

	case EWacomInteractionTargetKind::None:
	default:
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::MissingTarget;
		return Result;
	}
}

TArray<FWacomFirstPersonCardTargetAffordance> UBattleHUD::BuildFirstPersonCardTargetAffordances(
	const FGuid& SourceCardId,
	const FBattleSnapshot& Snapshot,
	const UBattleSession& BattleSession) const
{
	TArray<FWacomFirstPersonCardTargetAffordance> Affordances;
	if (!SourceCardId.IsValid())
	{
		return Affordances;
	}

	const FHandCardSnapshot* SourceSnapshot = nullptr;
	for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
	{
		if (CardSnapshot.InstanceId == SourceCardId)
		{
			SourceSnapshot = &CardSnapshot;
			break;
		}
	}
	if (!SourceSnapshot
		|| !SourceSnapshot->Definition
		|| SourceSnapshot->Definition->TargetMode != ECardTargetMode::HandCard)
	{
		return Affordances;
	}

	Affordances.Reserve(FMath::Max(0, Snapshot.Hand.Cards.Num() - 1));
	for (const FHandCardSnapshot& TargetCard : Snapshot.Hand.Cards)
	{
		if (!TargetCard.InstanceId.IsValid() || TargetCard.InstanceId == SourceCardId)
		{
			continue;
		}

		FWacomInteractionTargetHandle TargetHandle =
			FWacomInteractionTargetHandle::ForCardTarget(TargetCard.InstanceId, const_cast<UBattleHUD*>(this));
		const FWacomBattleTargetValidationResult Validation =
			BattleSession.ValidateTargetWithCard(SourceCardId, TargetHandle);

		FWacomFirstPersonCardTargetAffordance Affordance;
		Affordance.CardInstanceId = TargetCard.InstanceId;
		Affordance.bCanSubmit = Validation.bCanTarget;
		Affordance.FeedbackState = Validation.bCanTarget
			? EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
			: EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget;
		Affordance.DebugSummary = Validation.DebugSummary;
		Affordances.Add(MoveTemp(Affordance));
	}
	return Affordances;
}

UWacomBattleEnemyPartWorldTargetBridgeComponent* UBattleHUD::ResolveBattleEnemyPartWorldTargetBridge(
	const FWacomInteractionTargetHandle& TargetHandle) const
{
	const UWacomInteractionTargetComponent* InteractionTarget =
		Cast<UWacomInteractionTargetComponent>(TargetHandle.SourceObject.Get());
	const AActor* Owner = InteractionTarget ? InteractionTarget->GetOwner() : nullptr;
	return Owner ? Owner->FindComponentByClass<UWacomBattleEnemyPartWorldTargetBridgeComponent>() : nullptr;
}

bool UBattleHUD::ProbeFirstPersonCardDragTarget(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	FWacomInteractionTargetHandle& OutTargetHandle,
	bool& bOutValidTarget) const
{
	OutTargetHandle = FWacomInteractionTargetHandle();
	bOutValidTarget = false;

	if (!CardInstanceId.IsValid())
	{
		return false;
	}

	if (DragView.CurrentTarget.IsValid()
		&& DragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Card)
	{
		if (DragView.CurrentTarget.CardInstanceId == CardInstanceId)
		{
			return false;
		}
		OutTargetHandle = DragView.CurrentTarget;
		return true;
	}

	const AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(GetOwningPlayer());
	const bool bProbed = DragView.bHasPointerViewportPosition
		? WacomPC && WacomPC->TryProbeBattleSceneInteractionTargetAtWidgetPosition(
			DragView.PointerViewportPosition,
			OutTargetHandle)
		: WacomPC && WacomPC->TryProbeBattleSceneInteractionTarget(OutTargetHandle);
	if (!bProbed)
	{
		return false;
	}

	const UBattleSession* CurrentSession = GetSession();
	bOutValidTarget = CurrentSession && CurrentSession->ValidateTargetWithCard(CardInstanceId, OutTargetHandle).bCanTarget;
	return OutTargetHandle.IsValid();
}

bool UBattleHUD::ShouldShowFirstPersonDragInspectDetail(
	const FWacomFirstPersonCardDragView& DragView) const
{
	const UWacomFirstPersonCardAnchorComponent* Anchor = LastFirstPersonBattleHandAnchor.Get();
	if (!Anchor)
	{
		Anchor = ResolveFirstPersonCardAnchor();
	}
	if (!Anchor || !Anchor->bShowDetailDuringCardInspect)
	{
		return false;
	}

	return DragView.GestureState == EWacomFirstPersonCardGestureState::Inspecting;
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
	const bool bLegacyVisible = CardDetailPanel && CardDetailPanel->GetVisibility() != ESlateVisibility::Collapsed;
	const bool bFirstPersonVisible = FirstPersonCardDetailPanel
		&& FirstPersonCardDetailPanel->GetVisibility() != ESlateVisibility::Collapsed;
	return bLegacyVisible || bFirstPersonVisible;
}

FText UBattleHUD::GetCardDetailPanelNameText() const
{
	if (FirstPersonCardDetailPanel && FirstPersonCardDetailPanel->GetVisibility() != ESlateVisibility::Collapsed)
	{
		return FirstPersonCardDetailPanel->GetNameText();
	}
	return CardDetailPanel ? CardDetailPanel->GetNameText() : FText::GetEmpty();
}

void UBattleHUD::HandleHandCardHovered(UCardWidget* SourceWidget)
{
	ShowCardDetailForCardWidget(SourceWidget);
}

void UBattleHUD::HandleHandCardUnhovered(UCardWidget* SourceWidget)
{
	HideCardDetailPanelForSource(SourceWidget);
}

bool UBattleHUD::ShowCardDetailForCardWidget(UCardWidget* SourceWidget)
{
	if (!SourceWidget || !SourceWidget->GetCardSnapshot().Definition || UIState != EBattleUIState::Idle)
	{
		HideCardDetailPanel();
		return false;
	}

	EnsureCardDetailLayer();
	if (!CardDetailLayer)
	{
		return false;
	}

	const FGeometry& LayerGeometry = CardDetailLayer->GetCachedGeometry();
	const FGeometry& SourceGeometry = SourceWidget->GetCachedGeometry();
	const FVector2D AnchorPosition = LayerGeometry.AbsoluteToLocal(SourceGeometry.GetAbsolutePosition());
	const FVector2D AnchorSize = SourceGeometry.GetLocalSize();
	CurrentCardDetailSource = SourceWidget;
	if (ShowCardDetailAtAnchor(
		UWacomCardPresentationBuilder::BuildCardDetailViewData(SourceWidget->GetCardSnapshot().Definition),
		AnchorPosition,
		AnchorSize))
	{
		CurrentFirstPersonCardDetailSourceId.Invalidate();
		ForceHideCardDetailHost(ECardDetailHost::FirstPersonViewport);
		return true;
	}
	CurrentCardDetailSource.Reset();
	return false;
}

bool UBattleHUD::ShowCardDetailAtAnchor(
	const FWacomCardDetailViewData& DetailData,
	const FVector2D& AnchorPosition,
	const FVector2D& AnchorSize)
{
	UWacomCardDetailPanel* Panel = EnsureCardDetailPanel();
	if (!Panel)
	{
		return false;
	}

	Panel->SetCardDetailData(DetailData);
	PositionCardDetailPanelBesideAnchor(AnchorPosition, AnchorSize);
	if (!bEnableCardDetailReadabilityPolish)
	{
		Panel->SetRenderOpacity(1.0f);
		Panel->SetRenderTransform(FWidgetTransform());
		Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
		return true;
	}

	Panel->SetIsEnabled(true);
	if (!BeginCardDetailMotionShow(ECardDetailHost::LegacyHandPanel))
	{
		ForceHideCardDetailHost(ECardDetailHost::LegacyHandPanel);
		return false;
	}
	return true;
}

void UBattleHUD::HideCardDetailPanel()
{
	ForceHideAllCardDetails();
}

void UBattleHUD::HideCardDetailPanelForSource(UCardWidget* SourceWidget)
{
	if (!IsCardDetailMotionSource(ECardDetailHost::LegacyHandPanel, SourceWidget))
	{
		return;
	}
	RequestCardDetailMotionHide(ECardDetailHost::LegacyHandPanel, !bEnableCardDetailReadabilityPolish);
}

void UBattleHUD::HideFirstPersonCardDetailPanelForSource(const FGuid& CardInstanceId)
{
	if (!IsCardDetailMotionSource(ECardDetailHost::FirstPersonViewport, CardInstanceId))
	{
		return;
	}
	RequestCardDetailMotionHide(ECardDetailHost::FirstPersonViewport, !bEnableCardDetailReadabilityPolish);
}

bool UBattleHUD::IsFirstPersonCardInspectDetailActiveForSource(const FGuid& CardInstanceId) const
{
	if (!CardInstanceId.IsValid()
		|| CurrentFirstPersonCardDetailSourceId != CardInstanceId
		|| CardDetailMotionState.ActiveHost != ECardDetailHost::FirstPersonViewport
		|| CardDetailMotionState.ActiveFirstPersonSourceId != CardInstanceId
		|| !CardDetailMotionState.bHasActiveFirstPersonSlot)
	{
		return false;
	}

	return CardDetailMotionState.ActiveFirstPersonSlot.GestureState == EWacomFirstPersonCardGestureState::Inspecting;
}

UWacomCardDetailPanel* UBattleHUD::EnsureCardDetailPanel()
{
	EnsureCardDetailLayer();
	if (!CardDetailLayer)
	{
		return nullptr;
	}

	if (CardDetailPanel)
	{
		return CardDetailPanel;
	}

	UClass* PanelClass = CardDetailPanelClass
		? CardDetailPanelClass.Get()
		: UWacomCardDetailPanel::StaticClass();
	if (APlayerController* OwningPlayer = GetOwningPlayer();
		OwningPlayer && OwningPlayer->IsLocalController() && OwningPlayer->GetLocalPlayer())
	{
		CardDetailPanel = CreateWidget<UWacomCardDetailPanel>(OwningPlayer, PanelClass);
	}
	if (!CardDetailPanel)
	{
		if (UWorld* World = GetWorld())
		{
			CardDetailPanel = CreateWidget<UWacomCardDetailPanel>(World, PanelClass);
		}
	}
	if (!CardDetailPanel)
	{
		CardDetailPanel = NewObject<UWacomCardDetailPanel>(this, PanelClass);
	}
	if (!CardDetailPanel)
	{
		return nullptr;
	}

	CardDetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	CardDetailPanel->SetIsEnabled(true);
	CardDetailPanel->SetRenderOpacity(1.0f);
	CardDetailPanel->SetRenderTransform(FWidgetTransform());
	if (UCanvasPanelSlot* DetailSlot = CardDetailLayer->AddChildToCanvas(CardDetailPanel))
	{
		DetailSlot->SetAutoSize(false);
		DetailSlot->SetSize(CardDetailPanelEstimatedSize);
		DetailSlot->SetZOrder(1);
	}
	return CardDetailPanel;
}

UWacomCardDetailPanel* UBattleHUD::EnsureFirstPersonCardDetailPanel()
{
	if (FirstPersonCardDetailPanel)
	{
		return FirstPersonCardDetailPanel;
	}

	UClass* PanelClass = CardDetailPanelClass
		? CardDetailPanelClass.Get()
		: UWacomCardDetailPanel::StaticClass();
	if (APlayerController* OwningPlayer = GetOwningPlayer();
		OwningPlayer && OwningPlayer->IsLocalController() && OwningPlayer->GetLocalPlayer())
	{
		FirstPersonCardDetailPanel = CreateWidget<UWacomCardDetailPanel>(OwningPlayer, PanelClass);
	}
	if (!FirstPersonCardDetailPanel)
	{
		if (UWorld* World = GetWorld())
		{
			FirstPersonCardDetailPanel = CreateWidget<UWacomCardDetailPanel>(World, PanelClass);
		}
	}
	if (!FirstPersonCardDetailPanel)
	{
		FirstPersonCardDetailPanel = NewObject<UWacomCardDetailPanel>(this, PanelClass);
	}
	if (!FirstPersonCardDetailPanel)
	{
		return nullptr;
	}

	FirstPersonCardDetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	FirstPersonCardDetailPanel->SetIsEnabled(true);
	FirstPersonCardDetailPanel->SetRenderOpacity(1.0f);
	FirstPersonCardDetailPanel->SetRenderTransform(FWidgetTransform());
	FirstPersonCardDetailPanel->AddToViewport(FirstPersonCardDetailViewportZOrder);
	return FirstPersonCardDetailPanel;
}

void UBattleHUD::EnsureCardDetailLayer()
{
	if (CardDetailLayer)
	{
		return;
	}

	if (UCanvasPanel* RootCanvas = WidgetTree ? Cast<UCanvasPanel>(WidgetTree->RootWidget) : nullptr)
	{
		CardDetailLayer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CardDetailLayer_Runtime"));
		CardDetailLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* DetailLayerSlot = RootCanvas->AddChildToCanvas(CardDetailLayer))
		{
			DetailLayerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			DetailLayerSlot->SetOffsets(FMargin(0.0f));
			DetailLayerSlot->SetAutoSize(false);
			DetailLayerSlot->SetZOrder(10);
		}
	}
	else
	{
		if (!bLoggedMissingCardDetailLayer)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] CardDetailLayer 未绑定，且 RootWidget 不是 CanvasPanel，战斗手牌悬浮详情不会显示"));
			bLoggedMissingCardDetailLayer = true;
		}
	}
}

void UBattleHUD::PositionCardDetailPanelNear(UCardWidget* SourceWidget)
{
	if (!SourceWidget || !CardDetailLayer || !CardDetailPanel)
	{
		return;
	}

	const FGeometry& LayerGeometry = CardDetailLayer->GetCachedGeometry();
	const FGeometry& SourceGeometry = SourceWidget->GetCachedGeometry();
	const FVector2D AnchorPosition = LayerGeometry.AbsoluteToLocal(SourceGeometry.GetAbsolutePosition());
	const FVector2D AnchorSize = SourceGeometry.GetLocalSize();
	PositionCardDetailPanelBesideAnchor(AnchorPosition, AnchorSize);
}

void UBattleHUD::PositionCardDetailPanelBesideAnchor(const FVector2D& AnchorPosition, const FVector2D& AnchorSize)
{
	if (!CardDetailLayer || !CardDetailPanel)
	{
		return;
	}

	const FGeometry& LayerGeometry = CardDetailLayer->GetCachedGeometry();
	const FVector2D LayerSize = LayerGeometry.GetLocalSize();
	const FVector2D Position = bEnableCardDetailReadabilityPolish
		? ComputeCardDetailPanelPositionBesideStable(
			AnchorPosition,
			AnchorSize,
			LayerSize,
			CardDetailPanelEstimatedSize,
			CardDetailPanelPadding)
		: ComputeCardDetailPanelPositionBeside(
			AnchorPosition,
			AnchorSize,
			LayerSize,
			CardDetailPanelEstimatedSize,
			CardDetailPanelPadding);

	if (bEnableCardDetailReadabilityPolish)
	{
		CardDetailMotionState.TargetPosition = Position;
		CardDetailMotionState.bHasTargetPosition = true;
		return;
	}

	if (UCanvasPanelSlot* DetailSlot = Cast<UCanvasPanelSlot>(CardDetailPanel->Slot))
	{
		DetailSlot->SetPosition(Position);
		DetailSlot->SetSize(CardDetailPanelEstimatedSize);
	}
}

bool UBattleHUD::ShowFirstPersonCardDetailAtSlot(
	const FWacomCardDetailViewData& DetailData,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	UWacomCardDetailPanel* Panel = EnsureFirstPersonCardDetailPanel();
	if (!Panel)
	{
		return false;
	}

	if (!Panel->IsInViewport())
	{
		Panel->AddToViewport(FirstPersonCardDetailViewportZOrder);
	}

	Panel->SetCardDetailData(DetailData);
	PositionFirstPersonCardDetailPanelBesideSlot(SlotView);
	Panel->SetDesiredSizeInViewport(CardDetailPanelEstimatedSize);
	Panel->SetIsEnabled(true);
	CardDetailMotionState.ActiveFirstPersonSlot = SlotView;
	CardDetailMotionState.bHasActiveFirstPersonSlot = true;
	if (!bEnableCardDetailReadabilityPolish)
	{
		Panel->SetRenderOpacity(1.0f);
		Panel->SetRenderTransform(FWidgetTransform());
		Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
		return true;
	}

	if (!BeginCardDetailMotionShow(ECardDetailHost::FirstPersonViewport))
	{
		ForceHideCardDetailHost(ECardDetailHost::FirstPersonViewport);
		return false;
	}
	return true;
}

void UBattleHUD::PositionFirstPersonCardDetailPanelBesideSlot(
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (!FirstPersonCardDetailPanel)
	{
		return;
	}

	const FVector2D AnchorSize = FirstPersonCardDetailAnchorBaseSize * FMath::Max(0.01f, SlotView.RenderScale);
	const FVector2D AnchorPosition = SlotView.ScreenPosition - AnchorSize * 0.5f;
	const FVector2D Position = bEnableCardDetailReadabilityPolish
		? ComputeCardDetailPanelPositionBesideStable(
			AnchorPosition,
			AnchorSize,
			GetFirstPersonCardDetailViewportSize(),
			CardDetailPanelEstimatedSize,
			CardDetailPanelPadding)
		: ComputeCardDetailPanelPositionBeside(
			AnchorPosition,
			AnchorSize,
			GetFirstPersonCardDetailViewportSize(),
			CardDetailPanelEstimatedSize,
			CardDetailPanelPadding);

	FirstPersonCardDetailPanel->SetDesiredSizeInViewport(CardDetailPanelEstimatedSize);
	FirstPersonCardDetailPanel->SetAlignmentInViewport(FVector2D::ZeroVector);
	if (bEnableCardDetailReadabilityPolish)
	{
		CardDetailMotionState.TargetPosition = Position;
		CardDetailMotionState.bHasTargetPosition = true;
		return;
	}

	FirstPersonCardDetailPanel->SetPositionInViewport(Position, false);
	LastFirstPersonCardDetailPanelPosition = Position;
}

void UBattleHUD::HideFirstPersonCardDetailPanel()
{
	RequestCardDetailMotionHide(ECardDetailHost::FirstPersonViewport, !bEnableCardDetailReadabilityPolish);
}

void UBattleHUD::TickCardDetailMotion(float DeltaTime)
{
	if (!bEnableCardDetailReadabilityPolish)
	{
		return;
	}

	FCardDetailMotionState& State = CardDetailMotionState;
	if (State.ActiveHost == ECardDetailHost::None && !State.bPendingShow)
	{
		return;
	}

	const float SafeDeltaTime = FMath::Max(0.0f, DeltaTime);
	if (State.bPendingShow)
	{
		State.PendingElapsedSeconds += SafeDeltaTime;
		UpdateCardDetailMotionTarget(State.ActiveHost);
		if (State.PendingElapsedSeconds < FMath::Max(0.0f, CardDetailHoverDelaySeconds))
		{
			return;
		}

		State.bPendingShow = false;
		State.bWantsVisible = true;
		State.VisualOpacity = 0.0f;
		State.bResetPosition = true;
		if (UWacomCardDetailPanel* Panel = GetCardDetailPanelForHost(State.ActiveHost))
		{
			Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
			Panel->SetRenderOpacity(0.0f);
		}
	}

	if (!UpdateCardDetailMotionTarget(State.ActiveHost))
	{
		ForceHideCardDetailHost(State.ActiveHost);
		return;
	}

	UWacomCardDetailPanel* Panel = GetCardDetailPanelForHost(State.ActiveHost);
	if (!Panel)
	{
		ForceHideCardDetailHost(State.ActiveHost);
		return;
	}

	if (State.bWantsVisible && Panel->GetVisibility() == ESlateVisibility::Collapsed)
	{
		Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	const FVector2D TargetPosition = State.bHasTargetPosition ? State.TargetPosition : FVector2D::ZeroVector;
	if (!State.bHasVisualPosition
		|| State.bResetPosition
		|| FVector2D::Distance(State.VisualPosition, TargetPosition) > FMath::Max(0.0f, CardDetailPositionResetDistancePixels))
	{
		State.VisualPosition = TargetPosition;
		State.bHasVisualPosition = true;
		State.bResetPosition = false;
	}
	else if (CardDetailFollowSpeed <= 0.0f)
	{
		State.VisualPosition = TargetPosition;
	}
	else
	{
		State.VisualPosition = FMath::Vector2DInterpTo(
			State.VisualPosition,
			TargetPosition,
			SafeDeltaTime,
			CardDetailFollowSpeed);
	}

	const float TargetOpacity = State.bWantsVisible ? 1.0f : 0.0f;
	const float OpacitySpeed = State.bWantsVisible ? CardDetailFadeInSpeed : CardDetailFadeOutSpeed;
	State.VisualOpacity = OpacitySpeed <= 0.0f
		? TargetOpacity
		: FMath::FInterpTo(State.VisualOpacity, TargetOpacity, SafeDeltaTime, OpacitySpeed);

	if (!State.bWantsVisible && State.VisualOpacity <= 0.01f)
	{
		CollapseCardDetailHost(State.ActiveHost);
		State = FCardDetailMotionState();
		return;
	}

	ApplyCardDetailMotionVisual(State.ActiveHost, State.VisualPosition, State.VisualOpacity);
}

bool UBattleHUD::BeginCardDetailMotionShow(ECardDetailHost Host)
{
	if (!bEnableCardDetailReadabilityPolish)
	{
		return UpdateCardDetailMotionTarget(Host);
	}

	FCardDetailMotionState& State = CardDetailMotionState;
	const ECardDetailHost PreviousHost = State.ActiveHost;
	if (PreviousHost != ECardDetailHost::None && PreviousHost != Host)
	{
		CollapseCardDetailHost(PreviousHost);
		State.bResetPosition = true;
		State.StableSide = 0;
	}

	State.ActiveHost = Host;
	State.bHasTargetPosition = false;
	switch (Host)
	{
	case ECardDetailHost::LegacyHandPanel:
		State.ActiveLegacySource = CurrentCardDetailSource;
		State.ActiveFirstPersonSourceId.Invalidate();
		State.bHasActiveFirstPersonSlot = false;
		break;
	case ECardDetailHost::FirstPersonViewport:
		State.ActiveLegacySource.Reset();
		State.ActiveFirstPersonSourceId = CurrentFirstPersonCardDetailSourceId;
		break;
	default:
		break;
	}

	if (!UpdateCardDetailMotionTarget(Host))
	{
		return false;
	}

	RequestCardDetailMotionShow(Host);
	return true;
}

void UBattleHUD::RequestCardDetailMotionShow(ECardDetailHost Host)
{
	if (!bEnableCardDetailReadabilityPolish)
	{
		if (UWacomCardDetailPanel* Panel = GetCardDetailPanelForHost(Host))
		{
			Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
			Panel->SetRenderOpacity(1.0f);
		}
		return;
	}

	FCardDetailMotionState& State = CardDetailMotionState;
	const bool bWasShowing = State.VisualOpacity > 0.01f || State.bWantsVisible;
	State.ActiveHost = Host;
	State.bPendingShow = !bWasShowing && CardDetailHoverDelaySeconds > 0.0f;
	State.PendingElapsedSeconds = 0.0f;
	State.bWantsVisible = !State.bPendingShow;
	if (State.bPendingShow)
	{
		if (UWacomCardDetailPanel* Panel = GetCardDetailPanelForHost(Host))
		{
			Panel->SetVisibility(ESlateVisibility::Collapsed);
			Panel->SetRenderOpacity(0.0f);
		}
		State.VisualOpacity = 0.0f;
		State.bResetPosition = true;
		return;
	}

	if (!State.bHasVisualPosition)
	{
		State.VisualOpacity = 0.0f;
		State.bResetPosition = true;
	}
	if (UWacomCardDetailPanel* Panel = GetCardDetailPanelForHost(Host))
	{
		Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UBattleHUD::RequestCardDetailMotionHide(ECardDetailHost Host, bool bImmediate)
{
	if (bImmediate || !bEnableCardDetailReadabilityPolish)
	{
		ForceHideCardDetailHost(Host);
		return;
	}

	FCardDetailMotionState& State = CardDetailMotionState;
	if (State.ActiveHost != Host)
	{
		return;
	}

	State.bPendingShow = false;
	State.PendingElapsedSeconds = 0.0f;
	State.bWantsVisible = false;
	switch (Host)
	{
	case ECardDetailHost::LegacyHandPanel:
		CurrentCardDetailSource.Reset();
		break;
	case ECardDetailHost::FirstPersonViewport:
		CurrentFirstPersonCardDetailSourceId.Invalidate();
		LastFirstPersonCardDetailPanelPosition = FVector2D::ZeroVector;
		break;
	default:
		break;
	}
}

void UBattleHUD::ForceHideCardDetailHost(ECardDetailHost Host)
{
	CollapseCardDetailHost(Host);
	if (CardDetailMotionState.ActiveHost == Host)
	{
		CardDetailMotionState = FCardDetailMotionState();
	}
	if (Host == ECardDetailHost::LegacyHandPanel)
	{
		CurrentCardDetailSource.Reset();
	}
	else if (Host == ECardDetailHost::FirstPersonViewport)
	{
		CurrentFirstPersonCardDetailSourceId.Invalidate();
		LastFirstPersonCardDetailPanelPosition = FVector2D::ZeroVector;
	}
}

void UBattleHUD::ForceHideAllCardDetails()
{
	CollapseCardDetailHost(ECardDetailHost::LegacyHandPanel);
	CollapseCardDetailHost(ECardDetailHost::FirstPersonViewport);
	CardDetailMotionState = FCardDetailMotionState();
	CurrentCardDetailSource.Reset();
	CurrentFirstPersonCardDetailSourceId.Invalidate();
	LastFirstPersonCardDetailPanelPosition = FVector2D::ZeroVector;
}

UWacomCardDetailPanel* UBattleHUD::GetCardDetailPanelForHost(ECardDetailHost Host) const
{
	switch (Host)
	{
	case ECardDetailHost::LegacyHandPanel:
		return CardDetailPanel;
	case ECardDetailHost::FirstPersonViewport:
		return FirstPersonCardDetailPanel;
	default:
		return nullptr;
	}
}

bool UBattleHUD::UpdateCardDetailMotionTarget(ECardDetailHost Host)
{
	switch (Host)
	{
	case ECardDetailHost::LegacyHandPanel:
	{
		FVector2D Position = FVector2D::ZeroVector;
		if (!ComputeLegacyCardDetailTarget(CardDetailMotionState.ActiveLegacySource.Get(), Position))
		{
			return false;
		}
		CardDetailMotionState.TargetPosition = Position;
		CardDetailMotionState.bHasTargetPosition = true;
		return true;
	}
	case ECardDetailHost::FirstPersonViewport:
	{
		if (!CardDetailMotionState.bHasActiveFirstPersonSlot)
		{
			return false;
		}
		FVector2D Position = FVector2D::ZeroVector;
		if (!ComputeFirstPersonCardDetailTarget(CardDetailMotionState.ActiveFirstPersonSlot, Position))
		{
			return false;
		}
		CardDetailMotionState.TargetPosition = Position;
		CardDetailMotionState.bHasTargetPosition = true;
		return true;
	}
	default:
		return false;
	}
}

bool UBattleHUD::ComputeLegacyCardDetailTarget(UCardWidget* SourceWidget, FVector2D& OutPosition)
{
	if (!SourceWidget || !CardDetailLayer || !CardDetailPanel)
	{
		return false;
	}

	const FGeometry& LayerGeometry = CardDetailLayer->GetCachedGeometry();
	const FGeometry& SourceGeometry = SourceWidget->GetCachedGeometry();
	FVector2D LayerSize = LayerGeometry.GetLocalSize();
	if (LayerSize.X <= 0.0f || LayerSize.Y <= 0.0f)
	{
		LayerSize = GetFirstPersonCardDetailViewportSize();
	}

	const FVector2D AnchorPosition = LayerGeometry.AbsoluteToLocal(SourceGeometry.GetAbsolutePosition());
	const FVector2D AnchorSize = SourceGeometry.GetLocalSize();
	OutPosition = ComputeCardDetailPanelPositionBesideStable(
		AnchorPosition,
		AnchorSize,
		LayerSize,
		CardDetailPanelEstimatedSize,
		CardDetailPanelPadding);
	return true;
}

bool UBattleHUD::ComputeFirstPersonCardDetailTarget(
	const FWacomFirstPersonCardLayerSlotView& SlotView,
	FVector2D& OutPosition)
{
	if (!FirstPersonCardDetailPanel || !SlotView.bProjected)
	{
		return false;
	}

	const FVector2D ViewportSize = GetFirstPersonCardDetailViewportSize();
	if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
	{
		return false;
	}

	const FVector2D AnchorSize = FirstPersonCardDetailAnchorBaseSize * FMath::Max(0.01f, SlotView.RenderScale);
	const FVector2D AnchorPosition = SlotView.ScreenPosition - AnchorSize * 0.5f;
	OutPosition = ComputeCardDetailPanelPositionBesideStable(
		AnchorPosition,
		AnchorSize,
		ViewportSize,
		CardDetailPanelEstimatedSize,
		CardDetailPanelPadding);
	return true;
}

FVector2D UBattleHUD::ComputeCardDetailPanelPositionBesideStable(
	const FVector2D& AnchorPosition,
	const FVector2D& AnchorSize,
	const FVector2D& LayerSize,
	const FVector2D& PanelSize,
	float DetailPadding)
{
	const float SafePadding = FMath::Max(0.0f, DetailPadding);
	const float Hysteresis = FMath::Max(0.0f, CardDetailSideSwitchHysteresisPixels);
	const float MaxX = FMath::Max(0.0f, LayerSize.X - PanelSize.X);
	const float MaxY = FMath::Max(0.0f, LayerSize.Y - PanelSize.Y);
	const float LeftX = AnchorPosition.X - PanelSize.X - SafePadding;
	const float RightX = AnchorPosition.X + AnchorSize.X + SafePadding;

	int32 DesiredSide = CardDetailMotionState.StableSide;
	if (DesiredSide < 0 && LeftX < -Hysteresis)
	{
		DesiredSide = 0;
	}
	else if (DesiredSide > 0 && RightX > MaxX + Hysteresis)
	{
		DesiredSide = 0;
	}

	if (DesiredSide == 0)
	{
		DesiredSide = LeftX >= 0.0f ? -1 : 1;
	}
	CardDetailMotionState.StableSide = DesiredSide;

	const float DesiredX = DesiredSide < 0 ? LeftX : RightX;
	const float DesiredY = AnchorPosition.Y + (AnchorSize.Y - PanelSize.Y) * 0.5f;
	return FVector2D(
		FMath::Clamp(DesiredX, 0.0f, MaxX),
		FMath::Clamp(DesiredY, 0.0f, MaxY));
}

void UBattleHUD::ApplyCardDetailMotionVisual(ECardDetailHost Host, const FVector2D& Position, float Opacity)
{
	UWacomCardDetailPanel* Panel = GetCardDetailPanelForHost(Host);
	if (!Panel)
	{
		return;
	}

	Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	Panel->SetRenderOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));
	const float StartScale = FMath::Clamp(CardDetailAppearStartScale, 0.5f, 1.0f);
	const float Scale = FMath::Lerp(StartScale, 1.0f, FMath::Clamp(Opacity, 0.0f, 1.0f));
	FWidgetTransform Transform = Panel->GetRenderTransform();
	Transform.Scale = FVector2D(Scale, Scale);
	Panel->SetRenderTransform(Transform);
	Panel->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	if (Host == ECardDetailHost::LegacyHandPanel)
	{
		if (UCanvasPanelSlot* DetailSlot = Cast<UCanvasPanelSlot>(Panel->Slot))
		{
			DetailSlot->SetPosition(Position);
			DetailSlot->SetSize(CardDetailPanelEstimatedSize);
		}
	}
	else if (Host == ECardDetailHost::FirstPersonViewport)
	{
		Panel->SetDesiredSizeInViewport(CardDetailPanelEstimatedSize);
		Panel->SetAlignmentInViewport(FVector2D::ZeroVector);
		Panel->SetPositionInViewport(Position, false);
		LastFirstPersonCardDetailPanelPosition = Position;
	}
}

void UBattleHUD::CollapseCardDetailHost(ECardDetailHost Host)
{
	if (UWacomCardDetailPanel* Panel = GetCardDetailPanelForHost(Host))
	{
		Panel->SetVisibility(ESlateVisibility::Collapsed);
		Panel->SetRenderOpacity(0.0f);
		Panel->SetRenderTransform(FWidgetTransform());
	}
}

bool UBattleHUD::IsCardDetailMotionSource(ECardDetailHost Host, UCardWidget* SourceWidget) const
{
	if (!SourceWidget || Host != ECardDetailHost::LegacyHandPanel)
	{
		return false;
	}
	return CurrentCardDetailSource.Get() == SourceWidget
		|| CardDetailMotionState.ActiveLegacySource.Get() == SourceWidget;
}

bool UBattleHUD::IsCardDetailMotionSource(ECardDetailHost Host, const FGuid& CardInstanceId) const
{
	if (!CardInstanceId.IsValid() || Host != ECardDetailHost::FirstPersonViewport)
	{
		return false;
	}
	return CurrentFirstPersonCardDetailSourceId == CardInstanceId
		|| CardDetailMotionState.ActiveFirstPersonSourceId == CardInstanceId;
}

FVector2D UBattleHUD::GetFirstPersonCardDetailViewportSize() const
{
	FVector2D ViewportPixelSize = FVector2D::ZeroVector;
	if (const UWorld* World = GetWorld())
	{
		if (const UGameViewportClient* GameViewport = World->GetGameViewport())
		{
			GameViewport->GetViewportSize(ViewportPixelSize);
		}
	}

	if (ViewportPixelSize.X <= 0.0f || ViewportPixelSize.Y <= 0.0f)
	{
		if (const APlayerController* PC = GetOwningPlayer())
		{
			int32 ViewportX = 0;
			int32 ViewportY = 0;
			PC->GetViewportSize(ViewportX, ViewportY);
			ViewportPixelSize = FVector2D(ViewportX, ViewportY);
		}
	}

	if (ViewportPixelSize.X <= 0.0f || ViewportPixelSize.Y <= 0.0f)
	{
		ViewportPixelSize = FVector2D(1920.0f, 1080.0f);
	}

	float ViewportScale = 1.0f;
	if (const APlayerController* PC = GetOwningPlayer())
	{
		ViewportScale = UWidgetLayoutLibrary::GetViewportScale(PC);
	}
	ViewportScale = FMath::Max(0.01f, ViewportScale);
	return ViewportPixelSize / ViewportScale;
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

AWacomBattle3DHandPresenter* UBattleHUD::EnsureBattle3DHandPresenter()
{
	if (!bEnable3DHandPrototype || !GetSession())
	{
		DestroyBattle3DHandPresenter();
		return nullptr;
	}

	if (IsValid(Battle3DHandPresenter))
	{
		return Battle3DHandPresenter;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TSubclassOf<AWacomBattle3DHandPresenter> PresenterClass = Battle3DHandPresenterClass;
	if (!PresenterClass)
	{
		PresenterClass = AWacomBattle3DHandPresenter::StaticClass();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwningPlayer();
	SpawnParams.ObjectFlags |= RF_Transient;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWacomBattle3DHandPresenter* Presenter = World->SpawnActor<AWacomBattle3DHandPresenter>(
		PresenterClass.Get(),
		FTransform::Identity,
		SpawnParams);
	if (!Presenter)
	{
		return nullptr;
	}

	Battle3DHandPresenter = Presenter;
	AcquirePlayerControllerClickEvents();
	AcquirePlayerControllerMouseOverEvents();
	Presenter->SetOwningBattleHUD(this);
	if (Battle3DCardActorClass)
	{
		Presenter->CardActorClass = Battle3DCardActorClass;
	}
	SyncBattle3DHandPresenterTargeting();
	return Presenter;
}

void UBattleHUD::DestroyBattle3DHandPresenter()
{
	if (IsValid(Battle3DHandPresenter))
	{
		Battle3DHandPresenter->Destroy();
	}
	Battle3DHandPresenter = nullptr;
	ReleasePlayerControllerClickEvents();
	ReleasePlayerControllerMouseOverEvents();
}

void UBattleHUD::SyncBattle3DHandPresenterTargeting()
{
	if (!bEnable3DHandPrototype || !IsValid(Battle3DHandPresenter))
	{
		return;
	}

	Battle3DHandPresenter->SetTargetSelectionView(BuildTargetSelectionView());
}

void UBattleHUD::SyncBattleEnemyPartWorldTargets(const FBattleSnapshot& Snap)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (Snap.Phase == EBattlePhase::None || Snap.Phase == EBattlePhase::BattleEnd)
	{
		ClearBattleEnemyPartWorldTargets();
		return;
	}

	const FBattleTargetSelectionView TargetSelectionView = BuildTargetSelectionView();
	for (TObjectIterator<UWacomBattleEnemyPartWorldTargetBridgeComponent> It; It; ++It)
	{
		UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge = *It;
		if (!Bridge || Bridge->GetWorld() != World)
		{
			continue;
		}

		Bridge->SyncFromBattleHUD(*this, Snap, TargetSelectionView);
	}
}

void UBattleHUD::ClearBattleEnemyPartWorldTargets()
{
	ClearFirstPersonCardDragTargetFeedback();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TObjectIterator<UWacomBattleEnemyPartWorldTargetBridgeComponent> It; It; ++It)
	{
		UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge = *It;
		if (Bridge && Bridge->GetWorld() == World)
		{
			Bridge->ClearBattleBinding();
		}
	}
}

void UBattleHUD::AcquirePlayerControllerClickEvents()
{
	APlayerController* PC = GetOwningPlayer();
	ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
	UWacomInputContextCoordinatorSubsystem* InputCoordinator =
		LP ? LP->GetSubsystem<UWacomInputContextCoordinatorSubsystem>() : nullptr;
	if (!InputCoordinator)
	{
		++PlayerControllerClickEventAcquireCount;
		ApplyFallbackPlayerControllerInteractionEvents();
		return;
	}
	InputCoordinator->InitializeForPlayerController(PC);
	InputCoordinator->AcquirePlayerControllerInteractionEvents(this, /*bClickEvents*/ true, /*bMouseOverEvents*/ false);
	++PlayerControllerClickEventAcquireCount;
}

void UBattleHUD::ReleasePlayerControllerClickEvents()
{
	if (PlayerControllerClickEventAcquireCount <= 0)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
	UWacomInputContextCoordinatorSubsystem* InputCoordinator =
		LP ? LP->GetSubsystem<UWacomInputContextCoordinatorSubsystem>() : nullptr;
	if (InputCoordinator)
	{
		InputCoordinator->ReleasePlayerControllerInteractionEvents(this, /*bClickEvents*/ true, /*bMouseOverEvents*/ false);
	}
	--PlayerControllerClickEventAcquireCount;
	if (!InputCoordinator)
	{
		RestoreFallbackPlayerControllerInteractionEvents();
	}
}

void UBattleHUD::AcquirePlayerControllerMouseOverEvents()
{
	APlayerController* PC = GetOwningPlayer();
	ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
	UWacomInputContextCoordinatorSubsystem* InputCoordinator =
		LP ? LP->GetSubsystem<UWacomInputContextCoordinatorSubsystem>() : nullptr;
	if (!InputCoordinator)
	{
		++PlayerControllerMouseOverEventAcquireCount;
		ApplyFallbackPlayerControllerInteractionEvents();
		return;
	}
	InputCoordinator->InitializeForPlayerController(PC);
	InputCoordinator->AcquirePlayerControllerInteractionEvents(this, /*bClickEvents*/ false, /*bMouseOverEvents*/ true);
	++PlayerControllerMouseOverEventAcquireCount;
}

void UBattleHUD::ReleasePlayerControllerMouseOverEvents()
{
	if (PlayerControllerMouseOverEventAcquireCount <= 0)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
	UWacomInputContextCoordinatorSubsystem* InputCoordinator =
		LP ? LP->GetSubsystem<UWacomInputContextCoordinatorSubsystem>() : nullptr;
	if (InputCoordinator)
	{
		InputCoordinator->ReleasePlayerControllerInteractionEvents(this, /*bClickEvents*/ false, /*bMouseOverEvents*/ true);
	}
	--PlayerControllerMouseOverEventAcquireCount;
	if (!InputCoordinator)
	{
		RestoreFallbackPlayerControllerInteractionEvents();
	}
}

void UBattleHUD::ReleaseAllPlayerControllerInteractionEvents()
{
	APlayerController* PC = GetOwningPlayer();
	ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
	if (UWacomInputContextCoordinatorSubsystem* InputCoordinator =
		LP ? LP->GetSubsystem<UWacomInputContextCoordinatorSubsystem>() : nullptr)
	{
		InputCoordinator->ReleasePlayerControllerInteractionEvents(this);
	}
	PlayerControllerClickEventAcquireCount = 0;
	PlayerControllerMouseOverEventAcquireCount = 0;
	RestoreFallbackPlayerControllerInteractionEvents();
}

void UBattleHUD::ApplyFallbackPlayerControllerInteractionEvents()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	if (!bHasFallbackPlayerControllerInteractionEventState)
	{
		bFallbackSavedPlayerControllerClickEvents = PC->bEnableClickEvents;
		bFallbackSavedPlayerControllerMouseOverEvents = PC->bEnableMouseOverEvents;
		bHasFallbackPlayerControllerInteractionEventState = true;
	}

	PC->bEnableClickEvents = bFallbackSavedPlayerControllerClickEvents || PlayerControllerClickEventAcquireCount > 0;
	PC->bEnableMouseOverEvents = bFallbackSavedPlayerControllerMouseOverEvents || PlayerControllerMouseOverEventAcquireCount > 0;
}

void UBattleHUD::RestoreFallbackPlayerControllerInteractionEvents()
{
	if (!bHasFallbackPlayerControllerInteractionEventState)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		bHasFallbackPlayerControllerInteractionEventState = false;
		bFallbackSavedPlayerControllerClickEvents = false;
		bFallbackSavedPlayerControllerMouseOverEvents = false;
		return;
	}

	if (PlayerControllerClickEventAcquireCount > 0 || PlayerControllerMouseOverEventAcquireCount > 0)
	{
		PC->bEnableClickEvents = bFallbackSavedPlayerControllerClickEvents || PlayerControllerClickEventAcquireCount > 0;
		PC->bEnableMouseOverEvents = bFallbackSavedPlayerControllerMouseOverEvents || PlayerControllerMouseOverEventAcquireCount > 0;
		return;
	}

	PC->bEnableClickEvents = bFallbackSavedPlayerControllerClickEvents;
	PC->bEnableMouseOverEvents = bFallbackSavedPlayerControllerMouseOverEvents;
	bHasFallbackPlayerControllerInteractionEventState = false;
	bFallbackSavedPlayerControllerClickEvents = false;
	bFallbackSavedPlayerControllerMouseOverEvents = false;
}

void UBattleHUD::ConsumeAndLogEvents()
{
	FWacomBattleHUDEventFlow::ConsumeAndLogEvents(*this);
}

void UBattleHUD::AppendBattleEventLogEntries(const TArray<FBattleEvent>& Events)
{
	FWacomBattleHUDEventFlow::AppendBattleEventLogEntries(*this, Events);
}

void UBattleHUD::StoreFirstPersonCardTransitionEvents(const TArray<FBattleEvent>& Events)
{
	for (const FBattleEvent& Event : Events)
	{
		switch (Event.Type)
		{
		case EBattleEventType::CardsDrawn:
		case EBattleEventType::CardGained:
		case EBattleEventType::CardPlayed:
		case EBattleEventType::HandLimitDiscarded:
		case EBattleEventType::CardDiscarded:
		case EBattleEventType::CardExhausted:
			PendingFirstPersonCardTransitionEvents.Add(Event);
			break;
		default:
			break;
		}
	}
}

void UBattleHUD::ClearPendingFirstPersonCardTransitionEvents()
{
	PendingFirstPersonCardTransitionEvents.Reset();
	PendingFirstPersonCardPlayCommitHints.Reset();
}

void UBattleHUD::RecordFirstPersonPlayCommit(
	const FGuid& CardInstanceId,
	const FGuid& TargetPartInstanceId)
{
	if (!CardInstanceId.IsValid())
	{
		return;
	}

	FWacomFirstPersonCardPlayCommitHint CommitHint;
	CommitHint.CardInstanceId = CardInstanceId;
	CommitHint.TargetPartInstanceId = TargetPartInstanceId;
	if (TargetPartInstanceId.IsValid())
	{
		CommitHint.bHasTargetWidgetPosition =
			TryGetEnemyPartWidgetCenterInViewport(TargetPartInstanceId, CommitHint.TargetWidgetPosition);

		FWacomBattlePresentationTargetCue Cue;
		Cue.CueKind = EWacomBattlePresentationTargetCueKind::TargetConfirmed;
		Cue.TargetPartInstanceId = TargetPartInstanceId;
		Cue.Duration = 0.10f;
		PlayBattlePresentationCue(Cue);
	}

	PendingFirstPersonCardPlayCommitHints.Add(CommitHint);
}

TArray<FWacomFirstPersonCardLayerTransitionHint> UBattleHUD::BuildFirstPersonCardTransitionHints(
	const FBattleSnapshot& PreviousSnapshot,
	const FBattleSnapshot& NextSnapshot) const
{
	TArray<FWacomFirstPersonCardLayerTransitionHint> Hints;
	if (PendingFirstPersonCardTransitionEvents.IsEmpty()
		&& PendingFirstPersonCardPlayCommitHints.IsEmpty())
	{
		return Hints;
	}

	TSet<FGuid> NewCardIds;
	for (const FHandCardSnapshot& CardSnapshot : NextSnapshot.Hand.Cards)
	{
		if (CardSnapshot.InstanceId.IsValid()
			&& !ContainsHandCardId(PreviousSnapshot, CardSnapshot.InstanceId))
		{
			NewCardIds.Add(CardSnapshot.InstanceId);
		}
	}

	TSet<FGuid> RemovedCardIds;
	for (const FHandCardSnapshot& CardSnapshot : PreviousSnapshot.Hand.Cards)
	{
		if (CardSnapshot.InstanceId.IsValid()
			&& !ContainsHandCardId(NextSnapshot, CardSnapshot.InstanceId))
		{
			RemovedCardIds.Add(CardSnapshot.InstanceId);
		}
	}

	auto FindCommitHint = [this](const FGuid& CardInstanceId) -> const FWacomFirstPersonCardPlayCommitHint*
	{
		return PendingFirstPersonCardPlayCommitHints.FindByPredicate(
			[&CardInstanceId](const FWacomFirstPersonCardPlayCommitHint& CommitHint)
			{
				return CommitHint.CardInstanceId == CardInstanceId;
			});
	};

	auto AddHint = [&Hints, &FindCommitHint](
		const FGuid& CardInstanceId,
		EWacomFirstPersonCardSlotTransitionKind TransitionKind)
	{
		if (!CardInstanceId.IsValid() || TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Default)
		{
			return;
		}

		FWacomFirstPersonCardLayerTransitionHint Hint;
		Hint.CardInstanceId = CardInstanceId;
		Hint.TransitionKind = TransitionKind;
		if (TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Played)
		{
			if (const FWacomFirstPersonCardPlayCommitHint* CommitHint = FindCommitHint(CardInstanceId))
			{
				Hint.bPlayCommitFeedback = true;
				Hint.bHasPlayedExitTargetWidgetPosition = CommitHint->bHasTargetWidgetPosition;
				Hint.PlayedExitTargetWidgetPosition = CommitHint->TargetWidgetPosition;
			}
		}
		Hints.Add(Hint);
	};

	int32 DrawnCardHintBudget = 0;
	for (const FBattleEvent& Event : PendingFirstPersonCardTransitionEvents)
	{
		switch (Event.Type)
		{
		case EBattleEventType::CardGained:
			if (NewCardIds.Contains(Event.CardInstanceId))
			{
				AddHint(Event.CardInstanceId, EWacomFirstPersonCardSlotTransitionKind::Gained);
				NewCardIds.Remove(Event.CardInstanceId);
			}
			break;
		case EBattleEventType::CardPlayed:
			if (RemovedCardIds.Contains(Event.CardInstanceId))
			{
				AddHint(Event.CardInstanceId, EWacomFirstPersonCardSlotTransitionKind::Played);
				RemovedCardIds.Remove(Event.CardInstanceId);
			}
			break;
		case EBattleEventType::HandLimitDiscarded:
		case EBattleEventType::CardDiscarded:
		case EBattleEventType::CardExhausted:
			if (RemovedCardIds.Contains(Event.CardInstanceId))
			{
				AddHint(Event.CardInstanceId, EWacomFirstPersonCardSlotTransitionKind::Discarded);
				RemovedCardIds.Remove(Event.CardInstanceId);
			}
			break;
		case EBattleEventType::CardsDrawn:
			DrawnCardHintBudget += FMath::Max(0, Event.Count);
			break;
		default:
			break;
		}
	}

	for (const FHandCardSnapshot& CardSnapshot : NextSnapshot.Hand.Cards)
	{
		if (DrawnCardHintBudget <= 0)
		{
			break;
		}
		if (!NewCardIds.Contains(CardSnapshot.InstanceId))
		{
			continue;
		}

		AddHint(CardSnapshot.InstanceId, EWacomFirstPersonCardSlotTransitionKind::Drawn);
		NewCardIds.Remove(CardSnapshot.InstanceId);
		--DrawnCardHintBudget;
	}

	return Hints;
}

bool UBattleHUD::TryGetEnemyPartWidgetCenterInViewport(
	const FGuid& PartInstanceId,
	FVector2D& OutWidgetPosition) const
{
	return EnemyInfoBar && EnemyInfoBar->TryGetPartWidgetCenterInViewport(PartInstanceId, OutWidgetPosition);
}

void UBattleHUD::TrimBattleEventLogHistory()
{
	FWacomBattleHUDEventFlow::TrimBattleEventLogHistory(*this);
}

void UBattleHUD::SyncBattleEventLogPanel()
{
	FWacomBattleHUDEventFlow::SyncBattleEventLogPanel(*this);
}

void UBattleHUD::EnqueueBattlePresentationEvents(const TArray<FBattleEvent>& Events)
{
	if (Events.IsEmpty())
	{
		return;
	}

	if (!BattleEventPresentationQueue)
	{
		BattleEventPresentationQueue = MakeShared<FWacomBattleEventPresentationQueue>(*this);
	}

	BattleEventPresentationQueue->EnqueueEvents(Events);
}

void UBattleHUD::ClearBattlePresentationQueue()
{
	if (BattleEventPresentationQueue)
	{
		BattleEventPresentationQueue->Clear();
		BattleEventPresentationQueue.Reset();
	}
}

bool UBattleHUD::IsBattlePresentationQueueBusy() const
{
	return BattleEventPresentationQueue && BattleEventPresentationQueue->IsBusy();
}

TSharedPtr<FWacomBattleEventPresentationQueue> UBattleHUD::GetBattlePresentationQueueSelfKeepAlive() const
{
	return BattleEventPresentationQueue;
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

void UBattleHUD::EnqueueBattlePresentationToast(const FBattleEventPresentationView& View)
{
	if (EventToast)
	{
		EventToast->EnqueuePresentationView(View);
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

void UBattleHUD::HandleBattlePresentationQueueStarted()
{
	HideCardDetailPanel();
	if (UIState != EBattleUIState::BattleEnd)
	{
		SetUIState(EBattleUIState::Resolving);
	}
}

void UBattleHUD::HandleBattlePresentationQueueFinished()
{
	UBattleSession* CurrentSession = GetSession();
	if (!CurrentSession)
	{
		return;
	}

	const FBattleSnapshot Snapshot = CurrentSession->BuildSnapshot();
	if (Snapshot.Phase == EBattlePhase::BattleEnd)
	{
		SetUIState(EBattleUIState::BattleEnd);
		return;
	}

	if (UIState == EBattleUIState::Resolving)
	{
		SetUIState(EBattleUIState::Idle);
	}
}

void UBattleHUD::HandleBattlePresentationBattleEndStep()
{
	if (UBattleSession* CurrentSession = GetSession())
	{
		RefreshFromSnapshot(CurrentSession->BuildSnapshot());
	}
}

void UBattleHUD::AdvanceBattlePresentationQueueOnce()
{
#if WITH_AUTOMATION_TESTS
	if (BattleEventPresentationQueue)
	{
		BattleEventPresentationQueue->AdvanceForTest();
	}
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

int32 UBattleHUD::GetBattlePresentationTargetCountForTest() const
{
	return BattlePresentationTargetRegistry ? BattlePresentationTargetRegistry->Num() : 0;
}
#endif

void UBattleHUD::HandleBattleEventLogButtonClicked()
{
	ToggleBattleEventLog();
}

#undef LOCTEXT_NAMESPACE

