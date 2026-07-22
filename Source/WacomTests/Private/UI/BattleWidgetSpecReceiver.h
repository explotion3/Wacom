// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerController.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/Battle/BattleCommandBarWidget.h"
#include "UI/Battle/BattlePresentationStackWidget.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/Battle/WacomKnockdownChoiceDialog.h"
#include "UI/Battle/WacomBattleCombatLogDetailsScreen.h"
#include "UI/Battle/WacomBattleSecondaryPanelScreenBase.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Common/PileCountView.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Presentation/BattlePresentationJournal.h"
#include "BattleWidgetSpecReceiver.generated.h"

struct FWacomBattleSceneTargetClickTestAccess;

UCLASS()
class UWacomBattleCombatLogDetailsScreenTest : public UWacomBattleCombatLogDetailsScreen
{
	GENERATED_BODY()

public:
	TOptional<FUIInputConfig> GetDesiredInputConfigForTest() const
	{
		return GetDesiredInputConfig();
	}

	FReply PressKeyForTest(const FKey& Key)
	{
		return NativeOnKeyDown(
			FGeometry(),
			FKeyEvent(Key, FModifierKeysState(), 0, false, 0, 0));
	}

	FReply PressMouseButtonForTest(const FKey& Key)
	{
		const TSet<FKey> PressedButtons = { Key };
		return NativeOnMouseButtonDown(
			FGeometry(),
			FPointerEvent(
				0,
				0,
				FVector2D::ZeroVector,
				FVector2D::ZeroVector,
				PressedButtons,
				Key,
				0.0f,
				FModifierKeysState()));
	}

	void ClickBackdropForTest()
	{
		if (BackdropButton)
		{
			BackdropButton->OnClicked.Broadcast();
		}
	}

	void ClickCloseForTest()
	{
		if (CloseButton)
		{
			CloseButton->OnClicked.Broadcast();
		}
	}

	void SetDetailsCheckedForTest(bool bChecked)
	{
		if (DetailsToggle)
		{
			DetailsToggle->OnCheckStateChanged.Broadcast(bChecked);
		}
	}
};
UCLASS()
class AWacomBattleHUDLocalPlayerControllerTest : public APlayerController
{
	GENERATED_BODY()

public:
	virtual bool IsLocalController() const override
	{
		return true;
	}
};

UCLASS()
class AWacomBattleSceneClickRouterPlayerControllerTest : public AWacomPlayerController
{
	GENERATED_BODY()

public:
	virtual bool IsLocalController() const override
	{
		return true;
	}

private:
	friend struct FWacomBattleSceneTargetClickTestAccess;

	void SetBattleSceneClickHUDForTest(UBattleHUD* InHUD)
	{
		BattleSceneClickHUDOverride = InHUD;
	}

	void SetBattleSceneClickHitForTest(AActor* InActor, UPrimitiveComponent* InComponent = nullptr)
	{
		bHasBattleSceneClickHitOverride = true;
		BattleSceneClickHitOverride = FHitResult();
		BattleSceneClickHitOverride.HitObjectHandle = FActorInstanceHandle(InActor);
		BattleSceneClickHitOverride.Component = InComponent;
	}

	void ClearBattleSceneClickHitForTest()
	{
		bHasBattleSceneClickHitOverride = false;
		BattleSceneClickHitOverride = FHitResult();
	}

	bool RouteBattleSceneTargetClickForTest()
	{
		return TryRouteBattleSceneTargetClick();
	}

	bool ProbeBattleSceneTargetForTest(FWacomInteractionTargetHandle& OutHandle) const
	{
		return TryProbeBattleSceneInteractionTarget(OutHandle);
	}

	bool ProbeBattleSceneTargetAtWidgetPositionForTest(
		const FVector2D& WidgetPosition,
		FWacomInteractionTargetHandle& OutHandle) const
	{
		return TryProbeBattleSceneInteractionTargetAtWidgetPosition(WidgetPosition, OutHandle);
	}

	bool InputRightMousePressedForTest()
	{
		FInputKeyEventArgs Args;
		Args.Key = EKeys::RightMouseButton;
		Args.Event = IE_Pressed;
		return InputKey(Args);
	}

	bool InputLeftMouseReleasedForTest()
	{
		FInputKeyEventArgs Args;
		Args.Key = EKeys::LeftMouseButton;
		Args.Event = IE_Released;
		return InputKey(Args);
	}

	void PressWaitShortcutForTest()
	{
		OnWaitPressed();
	}

	void PressEndTurnShortcutForTest()
	{
		OnEndTurnPressed();
	}

protected:
	virtual bool CanRouteBattleSceneTargetClick(UBattleHUD*& OutHUD) const override
	{
		OutHUD = BattleSceneClickHUDOverride.Get();
		return OutHUD != nullptr;
	}

	virtual bool BuildBattleSceneClickHitResult(FHitResult& OutHitResult) const override
	{
		if (!bHasBattleSceneClickHitOverride)
		{
			return false;
		}

		OutHitResult = BattleSceneClickHitOverride;
		return OutHitResult.GetActor() || OutHitResult.GetComponent();
	}

	virtual bool BuildBattleSceneInteractionTargetHitResultAtWidgetPosition(
		const FVector2D& WidgetPosition,
		FHitResult& OutHitResult) const override
	{
		if (!bHasBattleSceneClickHitOverride)
		{
			return false;
		}

		OutHitResult = BattleSceneClickHitOverride;
		OutHitResult.Location = FVector(WidgetPosition.X, WidgetPosition.Y, 0.0f);
		return OutHitResult.GetActor() || OutHitResult.GetComponent();
	}

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<UBattleHUD> BattleSceneClickHUDOverride;

	FHitResult BattleSceneClickHitOverride;
	bool bHasBattleSceneClickHitOverride = false;
};

UCLASS()
class UWacomBattleCommandBarTestProbe : public UBattleCommandBarWidget
{
	GENERATED_BODY()

public:
	bool IsWaitCommandEnabledForTest() const
	{
		return IsCommandEnabled(EWacomBattleCommandId::Wait);
	}

	bool IsEndTurnCommandEnabledForTest() const
	{
		return IsCommandEnabled(EWacomBattleCommandId::EndTurn);
	}

	FText GetWaitValueTextForTest() const
	{
		return GetCurrentViewData().WaitValueText;
	}

	FText GetPendingCommandTextForTest() const
	{
		return GetCurrentViewData().PendingCommandText;
	}

	void CreateAuthoredCommandButtonsForTest()
	{
		WaitButton = NewObject<UWacomBattleCommandButtonWidget>(this);
		EndTurnButton = NewObject<UWacomBattleCommandButtonWidget>(this);
		if (WaitButton)
		{
			WaitButton->TakeWidget();
		}
		if (EndTurnButton)
		{
			EndTurnButton->TakeWidget();
		}
	}

	UWacomBattleCommandButtonWidget* GetAuthoredWaitButtonForTest() const
	{
		return WaitButton;
	}

	UWacomBattleCommandButtonWidget* GetAuthoredEndTurnButtonForTest() const
	{
		return EndTurnButton;
	}
};

UCLASS()
class UWacomBattleHUDDetailTest : public UBattleHUD
{
	GENERATED_BODY()

public:
	virtual APlayerController* GetOwningPlayer() const override
	{
		return OwningPlayerOverride ? OwningPlayerOverride.Get() : Super::GetOwningPlayer();
	}

	virtual UWorld* GetWorld() const override
	{
		return WorldOverride ? WorldOverride.Get() : Super::GetWorld();
	}

	void SetOwningPlayerForTest(APlayerController* InPlayerController)
	{
		OwningPlayerOverride = InPlayerController;
	}

	void SetWorldForTest(UWorld* InWorld)
	{
		WorldOverride = InWorld;
	}

	void SetBattleSceneEnemyHostsForTest(const TArray<AWacomBattleEnemyActor*>& InHosts)
	{
		SetBattleSceneEnemyHosts(InHosts);
	}

	FWacomBattleHUDAutomationTestView AutomationViewForTest() const
	{
		return GetAutomationTestViewForTest();
	}

	bool IsPresentationPlanActiveForTest() const
	{
		return AutomationViewForTest().bPresentationPlanActive;
	}

	int32 GetPresentationPlanPendingPhaseCountForTest() const
	{
		return AutomationViewForTest().PresentationPlanPendingPhaseCount;
	}

	FName GetActivePresentationPlanPhaseNameForTest() const
	{
		return AutomationViewForTest().ActivePresentationPlanPhaseName;
	}

	TArray<FName> GetStartedPresentationPlanPhaseNamesForTest() const
	{
		if (const TArray<FName>* Names = AutomationViewForTest().PresentationPlanStartedPhaseNames)
		{
			return *Names;
		}
		return TArray<FName>();
	}

	TArray<FWacomFirstPersonCardLayerFeedbackHint> GetSubmittedPresentationPlanFeedbackHintsForTest() const
	{
		if (const TArray<FWacomFirstPersonCardLayerFeedbackHint>* Hints =
			AutomationViewForTest().PresentationPlanSubmittedFeedbackHints)
		{
			return *Hints;
		}
		return TArray<FWacomFirstPersonCardLayerFeedbackHint>();
	}

	int32 GetBattleSceneEnemyPartComponentCountForTest() const
	{
		return AutomationViewForTest().SceneEnemyPartComponentCount;
	}

	int32 GetBattleSceneEnemyTargetRegistryRevisionForTest() const
	{
		return AutomationViewForTest().SceneEnemyTargetRegistryRevision;
	}

	void RefreshFromSnapshotForTest(const FBattleSnapshot& Snapshot)
	{
		RefreshFromSnapshot(Snapshot);
	}

	void SyncFirstPersonBattleHandLayerForTest(const FBattleSnapshot& Snapshot)
	{
		SyncFirstPersonBattleHandLayer(Snapshot);
	}

	void StoreFirstPersonCardTransitionEventsForTest(const TArray<FBattleEvent>& Events)
	{
		StoreFirstPersonCardTransitionEvents(Events);
	}

	void PrepareDrawPileFeedbackForTest(
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints)
	{
		PrepareDrawPileFeedbackForAutomationTest(TransitionHints);
	}

	void DispatchEnterTransitionStartedForTest(
		const FWacomFirstPersonCardEnterTransitionStartedView& View)
	{
		DispatchEnterTransitionStartedForAutomationTest(View);
	}

	void ResetDrawPileFeedbackForTest(int32 AuthoritativeDrawPileCount)
	{
		ResetDrawPileFeedbackForAutomationTest(AuthoritativeDrawPileCount);
	}

	void RecordFirstPersonPlayCommitForTest(
		const FGuid& CardInstanceId,
		const FBattlePartSlotIdentity& TargetPartKey = FBattlePartSlotIdentity(),
		const TOptional<FVector2D>& TargetWidgetPosition = TOptional<FVector2D>())
	{
		RecordFirstPersonPlayCommit(CardInstanceId, TargetPartKey, TargetWidgetPosition);
	}

	TArray<FWacomFirstPersonCardLayerTransitionHint> BuildFirstPersonCardTransitionHintsForTest(
		const FBattleSnapshot& PreviousSnapshot,
		const FBattleSnapshot& NextSnapshot) const
	{
		return BuildFirstPersonCardTransitionHints(PreviousSnapshot, NextSnapshot);
	}

	TArray<FWacomFirstPersonCardLayerTransitionHint> BuildFirstPersonCardTransitionHintsForRefreshForTest(
		const FBattleSnapshot& NextSnapshot) const
	{
		return UBattleHUD::BuildFirstPersonCardTransitionHintsForRefreshForTest(NextSnapshot);
	}

	TArray<FWacomFirstPersonCardLayerFeedbackHint> BuildFirstPersonCardFeedbackHintsForTest(
		const FBattleSnapshot& NextSnapshot) const
	{
		return UBattleHUD::BuildFirstPersonCardFeedbackHintsForTest(NextSnapshot);
	}

	void SetFirstPersonCardTransitionSnapshotForTest(const FBattleSnapshot& Snapshot)
	{
		UBattleHUD::SetFirstPersonCardTransitionSnapshotForTest(Snapshot);
	}

	void ClearPendingFirstPersonCardTransitionEventsForTest()
	{
		ClearPendingFirstPersonCardTransitionEvents();
	}

	void ClearFirstPersonBattleHandLayerForTest()
	{
		ClearFirstPersonBattleHandLayer();
	}

	UWacomFirstPersonCardAnchorComponent* ResolveFirstPersonCardAnchorForTest() const
	{
		return ResolveFirstPersonCardAnchor();
	}

	void SetTargetSelectionStateForTest(const FGuid& PendingCardId)
	{
		SetTargetSelectionStateForAutomationTest(PendingCardId);
	}

	bool ShouldEnableFirstPersonBattleHandInteractionForTest() const
	{
		return ShouldEnableFirstPersonBattleHandInteraction();
	}

	void SetSecondaryPanelOpenForTest(bool bOpen)
	{
		SetSecondaryPanelOpenForAutomationTest(bOpen);
	}

	bool IsSecondaryPanelOpenForTest() const
	{
		return AutomationViewForTest().bSecondaryPanelOpen;
	}

	void ClearTargetSelectionStateForTest()
	{
		ClearTargetSelectionStateForAutomationTest();
	}

	void SetUIStateForTest(EBattleUIState NewState)
	{
		SetUIState(NewState);
	}

	void SetBattleInputReadyForTest(bool bReady)
	{
		SetBattleInputReadyForAutomationTest(bReady);
	}

	void SetFirstPersonBattleHandSuppressedForTest(bool bSuppressed)
	{
		SetFirstPersonBattleHandSuppressedForAutomationTest(bSuppressed);
	}

	FReply MouseLeftButtonUpForTest()
	{
		FPointerEvent MouseEvent(
			0,
			0,
			FVector2D::ZeroVector,
			FVector2D::ZeroVector,
			TSet<FKey>(),
			EKeys::LeftMouseButton,
			0.0f,
			FModifierKeysState());
		return NativeOnMouseButtonUp(FGeometry(), MouseEvent);
	}

	void SetCardDetailReadabilityPolishForTest(bool bEnabled)
	{
		bEnableCardDetailReadabilityPolish = bEnabled;
	}

	void SetCardDetailMotionSpeedsForTest(float FollowSpeed, float FadeInSpeed, float FadeOutSpeed)
	{
		CardDetailFollowSpeed = FollowSpeed;
		CardDetailFadeInSpeed = FadeInSpeed;
		CardDetailFadeOutSpeed = FadeOutSpeed;
	}

	void TickCardDetailMotionForTest(float DeltaTime)
	{
		NativeTick(FGeometry(), DeltaTime);
	}

	void TickBattleSceneEnemyPartHoverProbeForTest(float DeltaTime = 0.05f)
	{
		NativeTick(FGeometry(), DeltaTime);
	}

	void HideCardDetailForTest()
	{
		HideCardDetailPanel();
	}

	void HandleFirstPersonCardHoveredForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView)
	{
		HandleFirstPersonCardLayerCardHovered(CardInstanceId, SlotView);
	}

	void HandleFirstPersonCardUnhoveredForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView)
	{
		HandleFirstPersonCardLayerCardUnhovered(CardInstanceId, SlotView);
	}

	void HandleFirstPersonCardLayoutUpdatedForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView)
	{
		HandleFirstPersonCardLayerHoveredCardLayoutUpdated(CardInstanceId, SlotView);
	}

	void HandleFirstPersonCardDragUpdatedForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView)
	{
		HandleFirstPersonCardLayerDragUpdated(CardInstanceId, DragView);
	}

	void HandleFirstPersonCardDragReleasedForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView)
	{
		HandleFirstPersonCardLayerDragReleased(CardInstanceId, DragView);
	}

	void SetFirstPersonCardAnchorForTest(UWacomFirstPersonCardAnchorComponent* Anchor)
	{
		BindFirstPersonBattleHandLayerInteractions(Anchor);
	}

	FWacomBattleCardDropResolveResult ResolveFirstPersonCardDropIntentForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView) const
	{
		return ResolveFirstPersonCardDropIntent(CardInstanceId, DragView);
	}

	bool IsFirstPersonCardDetailPanelVisibleForTest() const
	{
		return FirstPersonCardDetailPanel
			&& FirstPersonCardDetailPanel->GetVisibility() != ESlateVisibility::Collapsed;
	}

	FText GetFirstPersonCardDetailPanelNameTextForTest() const
	{
		return FirstPersonCardDetailPanel ? FirstPersonCardDetailPanel->GetNameText() : FText::GetEmpty();
	}

	const FWacomCardDetailViewData& GetFirstPersonCardDetailDataForTest() const
	{
		if (FirstPersonCardDetailPanel)
		{
			return FirstPersonCardDetailPanel->GetCardDetailData();
		}

		static const FWacomCardDetailViewData EmptyData;
		return EmptyData;
	}

	FVector2D GetFirstPersonCardDetailPanelPositionForTest() const
	{
		return GetLastFirstPersonCardDetailPanelPosition();
	}

	float GetFirstPersonCardDetailPanelOpacityForTest() const
	{
		return FirstPersonCardDetailPanel ? FirstPersonCardDetailPanel->GetRenderOpacity() : 0.0f;
	}

	int32 GetFirstPersonCardDetailViewportZOrderForTest() const
	{
		return FirstPersonCardDetailViewportZOrder;
	}

	bool EnsureFirstPersonCardDetailPanelForTest()
	{
		return EnsureFirstPersonCardDetailPanel() != nullptr;
	}

	EBattleUIState GetUIStateForTest() const
	{
		return GetUIState();
	}

	bool HasLastBattleSnapshotForTest() const
	{
		return AutomationViewForTest().bHasLastBattleSnapshot;
	}

	int32 GetLastBattleSnapshotHandCountForTest() const
	{
		return AutomationViewForTest().LastBattleSnapshotHandCount;
	}

	int32 GetLastBattleSnapshotVersionForTest() const
	{
		return AutomationViewForTest().LastBattleSnapshotVersion;
	}

	void ApplyCommandResolutionForTest(
		const FWacomBattleCombatLogCommandContext& LogContext,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleResolution& Resolution,
		UBattleSession* SourceSession = nullptr,
		const FGuid& PlayCommitCardInstanceId = FGuid())
	{
		ApplyCommandResolutionForAutomationTest(
			SourceSession ? SourceSession : GetInjectedBattleSession(),
			LogContext,
			PreCommandSnapshot,
			Resolution,
			PlayCommitCardInstanceId);
	}

	bool HasLastBattleHandCardForTest(const FGuid& CardInstanceId) const
	{
		return FindLastBattleHandCardSnapshot(CardInstanceId) != nullptr;
	}

	void SetCombatLogFeedForTest(UBattleCombatLogFeedWidget* InFeed)
	{
		UnbindCombatLogFeedForRuntime();
		CombatLogFeed = InFeed;
		if (InFeed)
		{
			ChildBattleWidgets.AddUnique(InFeed);
		}
		BindCombatLogFeedForRuntime();
	}

	void SetPresentationStackForTest(UBattlePresentationStackWidget* InStack)
	{
		BattlePresentationStack = InStack;
		if (InStack)
		{
			ChildBattleWidgets.AddUnique(InStack);
		}
	}

	const TArray<FWacomBattlePresentationStackEntryView>& GetPresentationStackEntriesForTest() const
	{
		if (const TArray<FWacomBattlePresentationStackEntryView>* Entries =
			AutomationViewForTest().PresentationStackEntries)
		{
			return *Entries;
		}

		static const TArray<FWacomBattlePresentationStackEntryView> EmptyEntries;
		return EmptyEntries;
	}

	int32 GetPresentationStackEntryCountForTest() const
	{
		return GetPresentationStackEntriesForTest().Num();
	}

	bool HasPendingTurnBoundaryCommandForTest() const
	{
		return HasPendingTurnBoundaryCommand();
	}

	void QueuePendingTurnBoundaryWaitForTest()
	{
		QueuePendingTurnBoundaryWaitForAutomationTest();
	}

	void ClearPendingTurnBoundaryCommandForTest()
	{
		ClearPendingTurnBoundaryCommand();
	}

	void HandleFirstPersonCardDragStartedForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView)
	{
		HandleFirstPersonCardLayerDragStarted(CardInstanceId, DragView);
	}

	void HandleFirstPersonCardPointerMovedForTest(
		const FWacomFirstPersonCardPointerView& PointerView)
	{
		HandleFirstPersonCardLayerPointerMoved(PointerView);
	}

	void HandleFirstPersonCardPointerLeftForTest()
	{
		HandleFirstPersonCardLayerPointerLeft();
	}

	void HandleFirstPersonCardDragCancelledForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView)
	{
		HandleFirstPersonCardLayerDragCancelled(CardInstanceId, DragView);
	}

	void FinishPresentationStackEntryExitForTest(int32 EntryId)
	{
		FinishBattlePresentationStackEntryExit(EntryId);
	}

	TArray<FWacomBattleCombatLogBlockView> GetBattleCombatLogHistoryForTest() const
	{
		if (const TArray<FWacomBattleCombatLogBlockView>* History =
			AutomationViewForTest().CombatLogHistory)
		{
			return *History;
		}

		return TArray<FWacomBattleCombatLogBlockView>();
	}

	TArray<FWacomBattleCombatLogTurnSectionView> GetBattleCombatLogDetailsHistoryForTest() const
	{
		return GetBattleCombatLogDetailsHistory();
	}

	void AppendBattleCombatLogBlockForTest(const FWacomBattleCombatLogBlockView& Block)
	{
		AppendBattleCombatLogBlock(Block);
	}

	void SetCommandBarForTest(UBattleCommandBarWidget* InCommandBar)
	{
		CommandBar = InCommandBar;
		if (InCommandBar)
		{
			InCommandBar->OnBattleCommandRequested.RemoveAll(this);
			InCommandBar->OnBattleCommandRequested.AddDynamic(
				this,
				&UBattleHUD::HandleCommandBarCommandRequested);
		}
	}

	void SetPlayerStatusBarForTest(UPlayerStatusBar* InPlayerStatusBar)
	{
		PlayerStatusBar = InPlayerStatusBar;
		if (InPlayerStatusBar)
		{
			ChildBattleWidgets.AddUnique(InPlayerStatusBar);
		}
	}

	void CreatePileViewsForTest()
	{
		DrawPileView = NewObject<UPileCountView>(this);
		DiscardPileView = NewObject<UPileCountView>(this);
		ExhaustPileView = NewObject<UPileCountView>(this);
		if (DrawPileView) { DrawPileView->TakeWidget(); }
		if (DiscardPileView) { DiscardPileView->TakeWidget(); }
		if (ExhaustPileView) { ExhaustPileView->TakeWidget(); }
	}

	UPileCountView* GetDrawPileViewForTest() const
	{
		return DrawPileView;
	}

	UPileCountView* GetDiscardPileViewForTest() const
	{
		return DiscardPileView;
	}

	UPileCountView* GetExhaustPileViewForTest() const
	{
		return ExhaustPileView;
	}

	void EnqueueBattlePresentationEventsForTest(const TArray<FBattleEvent>& Events)
	{
		EnqueueBattlePresentationEvents(Events);
	}

	bool EnqueueEndTurnPresentationPlanForTest(
		const FBattlePresentationJournal& Journal,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PostCommandSnapshot)
	{
		return UBattleHUD::EnqueueEndTurnPresentationPlanForTest(
			Journal,
			Events,
			PostCommandSnapshot);
	}

	bool EnqueueCommandPresentationPlanForTest(
		const FBattlePresentationJournal& Journal,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot)
	{
		return UBattleHUD::EnqueueCommandPresentationPlanForTest(
			Journal,
			Events,
			PreCommandSnapshot,
			PostCommandSnapshot);
	}

	void ClearBattlePresentationQueueForTest()
	{
		ClearBattlePresentationQueue();
	}

	void NativeDestructForTest()
	{
		NativeDestruct();
	}

	void AdvanceBattlePresentationQueueForTest()
	{
		AdvanceBattlePresentationQueueOnce();
	}

	void PrimeDiscardPileReceiveFeedbackForTest(
		int32 EventSequence,
		int32 TotalCount,
		int32 InitialDiscardCount)
	{
		PrimeDiscardPileReceiveFeedbackForAutomationTest(
			EventSequence,
			TotalCount,
			InitialDiscardCount);
	}

	void PrimeReshufflePileFeedbackForTest(
		int32 EventSequence,
		int32 TotalCount,
		int32 InitialDrawCount,
		int32 FinalDrawCount,
		int32 InitialDiscardCount,
		int32 FinalDiscardCount,
		int32 PlayedCount)
	{
		PrimeReshufflePileFeedbackForAutomationTest(
			EventSequence,
			TotalCount,
			InitialDrawCount,
			FinalDrawCount,
			InitialDiscardCount,
			FinalDiscardCount,
			PlayedCount);
	}

	void HandlePileTransferProgressForTest(
		const FWacomFirstPersonCardPileTransferProgressView& Progress)
	{
		HandleFirstPersonCardLayerPileTransferProgress(Progress);
	}

	void PlayBattlePresentationCueForTest(
		EBattleEventType SourceEventType,
		const FBattlePartSlotIdentity& TargetPartKey,
		int32 Amount)
	{
		UBattleHUD::PlayBattlePresentationCueForTest(SourceEventType, TargetPartKey, Amount);
	}

	void PlayTargetConfirmedCueForTest(const FBattlePartSlotIdentity& TargetPartKey)
	{
		UBattleHUD::PlayTargetConfirmedCueForTest(TargetPartKey);
	}

	int32 GetBattlePresentationTargetCountForTest() const
	{
		return AutomationViewForTest().PresentationTargetCount;
	}

	UFUNCTION()
	void ClearPresentationQueueOnBattleEndedForTest(EBattleOutcome Outcome)
	{
		(void)Outcome;
		++BattleEndedCallbackCountForTest;
		ClearBattlePresentationQueue();
	}

	int32 GetBattleEndedCallbackCountForTest() const
	{
		return BattleEndedCallbackCountForTest;
	}

private:
	UPROPERTY(Transient)
	TObjectPtr<APlayerController> OwningPlayerOverride;

	UPROPERTY(Transient)
	TObjectPtr<UWorld> WorldOverride;

	int32 BattleEndedCallbackCountForTest = 0;
};
