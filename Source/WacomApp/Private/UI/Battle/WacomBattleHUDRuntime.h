// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Runtime/BattlePartSlotIdentity.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattlePresentationPlan.h"
#include "UI/Battle/WacomBattlePresentationTargetRegistry.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "Types/WacomEnums.h"
#include "Types/WacomInteractionTargetTypes.h"

class AWacomBattleEnemyActor;
class UBattleCombatLogFeedWidget;
class UBattleCommandBarWidget;
class UBattlePresentationStackWidget;
class UBattleSession;
class UPlayerStatusBar;
class UPileCountView;
class UWidget;
class UWacomBattleEnemyPartComponent;
class UWacomCardDetailPanel;
class UWacomFirstPersonCardAnchorComponent;
class UWacomGameUIManagerSubsystem;
class UWacomKnockdownChoiceDialog;
class FWacomBattleHUDCardDetailController;
class FWacomBattleHUDCombatLogController;
class FWacomBattleHUDCommandController;
class FWacomBattleHUDCommandBarPresenter;
class FWacomBattleHUDEnemyInspectionCoordinator;
class FWacomBattleDrawPileFeedbackController;
class FWacomBattleHUDFirstPersonHandBridge;
class FWacomBattleHUDPresentationCoordinator;
class FWacomBattleHUDResultApplicator;
class FWacomBattleHUDSceneEnemyTargetCoordinator;
class FWacomBattleSecondaryPanelCoordinator;
struct FWacomBattleActionPreviewPresentation;
class FWacomBattleHUDSnapshotPresenter;
class FWacomBattleHUDTargetingController;
struct FKnockdownChoiceView;
struct FBattleInitializationResult;
struct FBattleResolution;
struct FBattleSnapshot;
struct FWacomBattlePresentationTargetCue;
struct FWacomBattlePresentationProgress;
struct FWacomCardDetailViewData;
struct FWacomFirstPersonCardDragView;
struct FWacomFirstPersonCardLayerSlotView;
struct FWacomFirstPersonCardPointerView;
struct FWacomBattleDrawPileFeedbackBatch;
struct FWacomKnockdownChoiceDialogViewData;

enum class EWacomBattleHUDCardDetailHost : uint8
{
	None,
	FirstPersonViewport,
};

enum class EWacomBattleHUDTurnBoundaryCommand : uint8
{
	None,
	Wait,
	EndTurn,
};

class FWacomBattleHUDRuntime;

class FWacomBattleHUDRuntimeHost
{
public:
	explicit FWacomBattleHUDRuntimeHost(UBattleHUD& InHUD);

	UBattleHUD& GetHUD() const { return HUD; }
	UObject* AsObject() const;
	UWorld* GetWorld() const;
	UGameInstance* GetGameInstance() const;
	APlayerController* GetOwningPlayer() const;
	UBattleSession* GetSession() const;

	void RebuildChildBattleWidgets();
	void RefreshChildBattleWidgetsFromSnapshot(const FBattleSnapshot& Snapshot);
	void BroadcastBattleEnd(EBattleOutcome Outcome);
	void NotifyUIStateChanged(EBattleUIState OldState, EBattleUIState NewState);

	UPlayerStatusBar* GetPlayerStatusBar() const;
	UBattleCommandBarWidget* GetCommandBar() const;
	UPileCountView* GetDrawPileView() const;
	UPileCountView* GetDiscardPileView() const;
	UPileCountView* GetExhaustPileView() const;
	UWidget* GetDrawPileMotionAnchor() const;
	UWidget* GetDiscardPileMotionAnchor() const;
	UWidget* GetPlayTargetMotionAnchor() const;
	UBattleCombatLogFeedWidget* GetCombatLogFeed() const;
	UBattlePresentationStackWidget* GetBattlePresentationStack() const;
	TObjectPtr<UWacomCardDetailPanel>& GetFirstPersonCardDetailPanelSlot() const;
	TSubclassOf<UWacomCardDetailPanel> GetCardDetailPanelClass() const;
	void SetCardDetailPanelClass(TSubclassOf<UWacomCardDetailPanel> PanelClass);

	float GetCardDetailPanelPadding() const;
	FVector2D GetCardDetailPanelEstimatedSize() const;
	bool IsCardDetailReadabilityPolishEnabled() const;
	float GetCardDetailHoverDelaySeconds() const;
	float GetCardDetailFadeInSpeed() const;
	float GetCardDetailFadeOutSpeed() const;
	float GetCardDetailFollowSpeed() const;
	float GetCardDetailPositionResetDistancePixels() const;
	float GetCardDetailAppearStartScale() const;
	float GetCardDetailSideSwitchHysteresisPixels() const;
	int32 GetBattleCombatLogMaxBlocks() const;
	float GetCardPresentationStackMinimumHoldSeconds() const;
	int32 GetFirstPersonCardDetailViewportZOrder() const;
	FVector2D GetFirstPersonCardDetailAnchorBaseSize() const;
	float GetBattleSceneEnemyPartHoverProbeIntervalSeconds() const;

	void BindFirstPersonCardLayerInteractions(UWacomFirstPersonCardAnchorComponent& Anchor);
	void UnbindFirstPersonCardLayerInteractions(UWacomFirstPersonCardAnchorComponent& Anchor);

	TSubclassOf<UWacomKnockdownChoiceDialog> GetKnockdownChoiceDialogClass() const;
	void SetKnockdownChoiceDialogClass(
		TSubclassOf<UWacomKnockdownChoiceDialog> DialogClass);
	void PushKnockdownChoiceDialog(
		const FWacomKnockdownChoiceDialogViewData& DialogViewData);

private:
	UBattleHUD& HUD;
};

class FWacomBattleHUDRuntime
{
public:
	explicit FWacomBattleHUDRuntime(UBattleHUD& InHUD);
	~FWacomBattleHUDRuntime();

	FWacomBattleHUDRuntimeHost& Host() { return RuntimeHost; }
	const FWacomBattleHUDRuntimeHost& Host() const { return RuntimeHost; }

	void NativeConstruct();
	void NativeDestruct();
	void NativeTick(float DeltaTime);
	void NativeRefreshFromSnapshot(const FBattleSnapshot& Snapshot);
	void NativeOnSessionChanged(UBattleSession* OldSession, UBattleSession* NewSession);
	void NativeOnUIStateChanged(EBattleUIState NewState);

	EBattleUIState GetUIState() const { return UIState; }
	void SetUIState(EBattleUIState NewState);
	bool IsInTargetSelect() const { return UIState == EBattleUIState::TargetSelect; }
	FGuid GetPendingTargetingCardId() const { return PendingTargetingCardId; }
	void SetPendingTargetingCardId(const FGuid& CardInstanceId) { PendingTargetingCardId = CardInstanceId; }
	void ClearPendingTargetingCardId() { PendingTargetingCardId.Invalidate(); }

	void SetBattleInputReady(bool bReady);
	bool IsBattleInputReady() const { return bBattleInputReady; }
	void SetFirstPersonBattleHandSuppressedForEntry(bool bSuppressed);
	bool IsFirstPersonBattleHandSuppressedForEntry() const { return bFirstPersonBattleHandSuppressedForEntry; }

	bool HasLastBattleSnapshot() const { return bHasLastBattleSnapshot; }
	const FBattleSnapshot& GetLastBattleSnapshot() const { return LastBattleSnapshot; }
	void SetLastBattleSnapshot(const FBattleSnapshot& Snapshot);
	void ClearLastBattleSnapshot();

	UBattleSession* GetSession() const { return RuntimeHost.GetSession(); }
	APlayerController* GetOwningPlayer() const { return RuntimeHost.GetOwningPlayer(); }
	UWorld* GetWorld() const { return RuntimeHost.GetWorld(); }

	FBattleTargetSelectionView BuildTargetSelectionView() const;
	FBattleTargetSelectionView BuildTargetSelectionView(const FBattleSnapshot& Snapshot) const;
	int32 GetBattleCombatLogBlockCount() const;
	const TArray<FWacomBattleCombatLogBlockView>& GetBattleCombatLogHistory() const;
	const TArray<FWacomBattleCombatLogTurnSectionView>& GetBattleCombatLogDetailsHistory() const;
	bool IsBattlePresentationBusy() const;
	bool IsBattlePresentationPlanBusy() const;
	bool CanSubmitPlayerActionCommand() const;
	void SetSecondaryPanelOpen(bool bOpen);
	bool SetFirstPersonBattleHandPresentationVisible(bool bVisible);
	bool IsSecondaryPanelOpen() const { return bSecondaryPanelOpen; }
	bool RequestOpenCombatLogDetails();
	bool RequestOpenCardPileDetails(EWacomBattlePileDetailsTab InitialTab);
	void RefreshPileDetailsInteractionState();
	bool HasPendingTurnBoundaryCommand() const;
	FText GetPendingTurnBoundaryCommandText() const;
	void RefreshCommandBarFromSnapshot(const FBattleSnapshot& Snapshot);
	void RefreshCommandBarFromCurrentSnapshot();

	void OnEnemyPartClickedByUser(const FWacomInteractionTargetHandle& TargetHandle);
	void OnWaitRequested();
	void OnEndTurnRequested();
	bool TryStartFirstPersonBattleHandDragByIndex(
		int32 OneBasedIndex,
		const TOptional<FVector2D>& InitialPointerWidgetPosition);
	void CancelTargetSelect();
	void OnKnockdownChoiceSelected(EKnockdownChoice Choice);
	bool TrySubmitKnockdownChoice(EKnockdownChoice Choice);

	void SubmitPlayCard(
		const FGuid& CardId,
		const FGuid& TargetPartId,
		const TOptional<FVector2D>& PresentationTargetWidgetPosition = TOptional<FVector2D>());
	void SubmitPlayCardOnWorldTarget(
		const FGuid& CardId,
		const FWacomInteractionTargetHandle& TargetHandle,
		const TOptional<FVector2D>& PresentationTargetWidgetPosition = TOptional<FVector2D>());
	void SubmitPlayCardOnHandCard(
		const FGuid& CardId,
		const FGuid& TargetCardId,
		const TOptional<FVector2D>& PresentationTargetWidgetPosition = TOptional<FVector2D>());
	void HideCardDetailPanel();
	void HideFirstPersonCardDetailPanelForSource(const FGuid& CardInstanceId);
	UWacomCardDetailPanel* EnsureFirstPersonCardDetailPanel();
	bool ShowFirstPersonCardDetailAtSlot(
		const FWacomCardDetailViewData& DetailData,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void PositionFirstPersonCardDetailPanelBesideSlot(const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HideFirstPersonCardDetailPanel();
	void TickCardDetailMotion(float DeltaTime);
	void ForceHideCardDetailHost(EWacomBattleHUDCardDetailHost Host);
	bool ComputeFirstPersonCardDetailTarget(const FWacomFirstPersonCardLayerSlotView& SlotView, FVector2D& OutPosition);
	FVector2D ComputeCardDetailPanelPositionBesideStable(
		const FVector2D& AnchorPosition,
		const FVector2D& AnchorSize,
		const FVector2D& LayerSize,
		const FVector2D& PanelSize,
		float DetailPadding);
	FVector2D GetFirstPersonCardDetailViewportSize() const;
	void SetFirstPersonCardDetailSource(const FGuid& CardInstanceId);
	void ClearFirstPersonCardDetailSource();
	bool IsCurrentFirstPersonCardDetailSource(const FGuid& CardInstanceId) const;
	void UpdateFirstPersonCardDetailSlot(const FWacomFirstPersonCardLayerSlotView& SlotView);
	FVector2D GetLastFirstPersonCardDetailPanelPosition() const;

	void AppendBattleCombatLogBlock(const FWacomBattleCombatLogBlockView& Block);
	void NotifyBattlePresentationProgress(const FWacomBattlePresentationProgress& Progress);
	void StoreFirstPersonCardTransitionEvents(const TArray<FBattleEvent>& Events);
	void QueueDrawPileFeedbackBatch(const FWacomBattleDrawPileFeedbackBatch& Batch);
	void PrepareDrawPileFeedbackForPresentationFrame(
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints);
	void CompleteActiveDrawPileFeedbackBatch();
	void ResetDrawPileFeedback(int32 AuthoritativeDrawPileCount = INDEX_NONE);
	void ClearPendingFirstPersonCardTransitionEvents();
	void RecordFirstPersonPlayCommit(
		const FGuid& CardInstanceId,
		const FBattlePartSlotIdentity& TargetPartKey,
		const TOptional<FVector2D>& TargetWidgetPosition = TOptional<FVector2D>());
	void RecordFirstPersonHandTargetImpact(const FGuid& TargetCardInstanceId);
	TArray<FWacomFirstPersonCardLayerTransitionHint> BuildFirstPersonCardTransitionHints(
		const FBattleSnapshot& PreviousSnapshot,
		const FBattleSnapshot& NextSnapshot) const;
	TArray<FWacomFirstPersonCardLayerFeedbackHint> BuildFirstPersonCardFeedbackHints(
		const FBattleSnapshot& NextSnapshot) const;

	int32 AppendBattlePresentationStackEntry(
		const FWacomBattleCombatLogCommandContext& CommandContext,
		const FBattleSnapshot& PreCommandSnapshot);
	void BeginBattlePresentationStackEntryExit(int32 EntryId);
	void FinishBattlePresentationStackEntryExit(int32 EntryId);
	void ClearBattlePresentationStack();
	bool HasBattlePresentationStackEntries() const;
	void EnqueueBattlePresentationEvents(const TArray<FBattleEvent>& Events, int32 PresentationStackEntryId = INDEX_NONE);
	void ClearBattlePresentationQueue();
	bool IsBattlePresentationQueueBusy() const;
	void QueuePendingTurnBoundaryCommand(EWacomBattleHUDTurnBoundaryCommand Command);
	void ClearPendingTurnBoundaryCommand();
	void TryExecutePendingTurnBoundaryCommand();
	FWacomBattlePresentationTargetRegistry& GetBattlePresentationTargetRegistry();
	void ClearBattlePresentationTargetRegistry();
	void RegisterBattlePresentationTarget(
		const FBattlePartSlotIdentity& TargetPartKey,
		UObject* Owner,
		TFunction<void(const FWacomBattlePresentationTargetCue&)> Handler);
	void UnregisterBattlePresentationTarget(const FBattlePartSlotIdentity& TargetPartKey);
	void UnregisterBattlePresentationTargetsForOwner(const UObject* Owner);
	bool IsBattlePresentationTargetRegisteredForOwner(const UObject* Owner) const;
	void PlayBattlePresentationCue(const FWacomBattlePresentationTargetCue& Cue);
	void PushPendingKnockdownChoiceDialog();
	void AdvanceBattlePresentationQueueOnce();

	void SetBattleSceneEnemyHosts(const TArray<AWacomBattleEnemyActor*>& InHosts);
	bool IsBattleSceneEnemyHostInCurrentRegistry(const AWacomBattleEnemyActor* Host) const;
	bool IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(const FWacomInteractionTargetHandle& TargetHandle) const;
	void RebuildBattleSceneEnemyPartWorldTargetRegistry();
	bool IsBattleSceneEnemyPartInCurrentRegistry(const UWacomBattleEnemyPartComponent* Part) const;
	void SyncBattleEnemyPartWorldTargets(const FBattleSnapshot& Snapshot);
	void ClearBattleEnemyPartWorldTargets();
	bool CanUpdateBattleSceneEnemyPartHoverProbe() const;
	void ApplyActionPreviewPresentation(
		const FWacomBattleActionPreviewPresentation& Presentation);
	void ClearActionPreview();
	bool CanOpenEnemyInspection() const;
	bool TryCloseEnemyInspection();
	void TickBattleSceneEnemyPartHoverProbe(float DeltaTime);
	void UpdateBattleSceneEnemyPartHoverProbe();
	void ClearBattleSceneEnemyPartHoverProbe(FName Reason);

	UWacomFirstPersonCardAnchorComponent* ResolveFirstPersonCardAnchor() const;
	UWacomFirstPersonCardAnchorComponent* ResolveActiveFirstPersonCardAnchor() const;
	void SyncFirstPersonBattleHandLayer(const FBattleSnapshot& Snapshot);
	void SyncFirstPersonBattleHandLayer(
		const FBattleSnapshot& Snapshot,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints);
	void SyncFirstPersonBattleHandLayer(
		const FBattleSnapshot& Snapshot,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints,
		const TArray<FWacomFirstPersonCardLayerFeedbackHint>& FeedbackHints);
	void RefreshFromPresentationPhase(
		const FBattleSnapshot& Snapshot,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints,
		const TArray<FWacomFirstPersonCardLayerFeedbackHint>& FeedbackHints =
			TArray<FWacomFirstPersonCardLayerFeedbackHint>());
	void RefreshCombatPresentationFrame(const FBattleSnapshot& Snapshot);
	void ClearFirstPersonBattleHandLayer();
	bool ShouldUseFirstPersonBattleHandLayer() const;
	bool ShouldEnableFirstPersonBattleHandInteraction() const;
	void BindFirstPersonBattleHandLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor);
	void UnbindFirstPersonBattleHandLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor);
	void HandleFirstPersonCardLayerCardHovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleFirstPersonCardLayerCardUnhovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleFirstPersonCardLayerHoveredCardLayoutUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleFirstPersonCardLayerCardTargetHovered(const FWacomInteractionTargetHandle& CardTargetHandle, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleFirstPersonCardLayerCardTargetUnhovered(const FWacomInteractionTargetHandle& CardTargetHandle, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleFirstPersonCardLayerHoveredCardTargetUpdated(const FWacomInteractionTargetHandle& CardTargetHandle, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleFirstPersonCardLayerPointerMoved(const FWacomFirstPersonCardPointerView& PointerView);
	void HandleFirstPersonCardLayerPileTransferProgress(const FWacomFirstPersonCardPileTransferProgressView& Progress);
	void HandleFirstPersonCardLayerEnterTransitionStarted(
		const FWacomFirstPersonCardEnterTransitionStartedView& View);
	void HandleFirstPersonCardLayerPointerLeft();
	void HandleFirstPersonCardLayerDragStarted(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleFirstPersonCardLayerDragUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleFirstPersonCardLayerDragReleased(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleFirstPersonCardLayerDragCancelled(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void UpdateFirstPersonCardDragTargetFeedback(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void ClearFirstPersonCardDragTargetFeedback();
	bool IsFirstPersonCardDragActiveForBattleSceneHover() const;
	bool TryGetActiveFirstPersonTargetSelectionCardId(FGuid& OutCardInstanceId) const;
	FWacomBattleCardDropResolveResult ResolveFirstPersonCardDropIntent(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView) const;
	TArray<FWacomFirstPersonCardTargetAffordance> BuildFirstPersonCardTargetAffordances(
		const FGuid& SourceCardId,
		const FBattleSnapshot& Snapshot,
		const UBattleSession& BattleSession) const;
	UWacomBattleEnemyPartComponent* ResolveBattleEnemyPartComponent(
		const FWacomInteractionTargetHandle& TargetHandle) const;
	bool ProbeFirstPersonCardDragTarget(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		FWacomInteractionTargetHandle& OutTargetHandle,
		bool& bOutValidTarget) const;
	bool ShouldShowFirstPersonDragInspectDetail(const FWacomFirstPersonCardDragView& DragView) const;

	FWacomBattleHUDCardDetailController& GetCardDetailController();
	const FWacomBattleHUDCardDetailController& GetCardDetailController() const;
	FWacomBattleHUDCombatLogController& GetCombatLogController();
	const FWacomBattleHUDCombatLogController& GetCombatLogController() const;
	FWacomBattleHUDFirstPersonHandBridge& GetFirstPersonHandBridge();
	const FWacomBattleHUDFirstPersonHandBridge& GetFirstPersonHandBridge() const;
	FWacomBattleHUDPresentationCoordinator& GetPresentationCoordinator();
	const FWacomBattleHUDPresentationCoordinator& GetPresentationCoordinator() const;
	FWacomBattleHUDSceneEnemyTargetCoordinator& GetSceneEnemyTargetCoordinator();
	const FWacomBattleHUDSceneEnemyTargetCoordinator& GetSceneEnemyTargetCoordinator() const;
	FWacomBattleHUDEnemyInspectionCoordinator& GetEnemyInspectionCoordinator();
	const FWacomBattleHUDEnemyInspectionCoordinator& GetEnemyInspectionCoordinator() const;
	FWacomBattleHUDCommandController& GetCommandController();
	FWacomBattleDrawPileFeedbackController& GetDrawPileFeedbackController();
	FWacomBattleHUDResultApplicator& GetResultApplicator();
	const FWacomBattleHUDResultApplicator& GetResultApplicator() const;
	FWacomBattleHUDCommandBarPresenter& GetCommandBarPresenter();
	FWacomBattleHUDTargetingController& GetTargetingController();
	FWacomBattleHUDSnapshotPresenter& GetSnapshotPresenter();
	FWacomBattleSecondaryPanelCoordinator& GetSecondaryPanelCoordinator();

#if WITH_AUTOMATION_TESTS
	void PlayBattlePresentationCueForTest(EBattleEventType SourceEventType, const FBattlePartSlotIdentity& TargetPartKey, int32 Amount);
	void PlayTargetConfirmedCueForTest(const FBattlePartSlotIdentity& TargetPartKey);
	bool EnqueueEndTurnPresentationPlanForTest(
		const FBattlePresentationJournal& Journal,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PostCommandSnapshot);
	bool EnqueueCommandPresentationPlanForTest(
		const FBattlePresentationJournal& Journal,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot);
	FWacomBattleHUDAutomationTestView GetAutomationTestViewForTest() const;
	TArray<FWacomFirstPersonCardLayerTransitionHint> BuildFirstPersonCardTransitionHintsForRefreshForTest(
		const FBattleSnapshot& NextSnapshot) const;
	TArray<FWacomFirstPersonCardLayerFeedbackHint> BuildFirstPersonCardFeedbackHintsForTest(
		const FBattleSnapshot& NextSnapshot) const;
	void SetFirstPersonCardTransitionSnapshotForTest(const FBattleSnapshot& Snapshot);
	void PrimeDiscardPileReceiveFeedbackForTest(
		int32 EventSequence,
		int32 TotalCount,
		int32 InitialDiscardCount);
	void PrimeReshufflePileFeedbackForTest(
		int32 EventSequence,
		int32 TotalCount,
		int32 InitialDrawCount,
		int32 FinalDrawCount,
		int32 InitialDiscardCount,
		int32 FinalDiscardCount,
		int32 PlayedCount);
#endif

private:
	FWacomBattleHUDRuntimeHost RuntimeHost;
	TUniquePtr<FWacomBattleHUDSnapshotPresenter> SnapshotPresenter;
	TUniquePtr<FWacomBattleHUDCommandController> CommandController;
	TUniquePtr<FWacomBattleDrawPileFeedbackController> DrawPileFeedbackController;
	TUniquePtr<FWacomBattleHUDResultApplicator> ResultApplicator;
	TUniquePtr<FWacomBattleHUDCommandBarPresenter> CommandBarPresenter;
	TUniquePtr<FWacomBattleHUDTargetingController> TargetingController;
	TSharedPtr<FWacomBattlePresentationTargetRegistry> BattlePresentationTargetRegistry;
	TSharedPtr<FWacomBattleHUDCardDetailController> CardDetailController;
	TSharedPtr<FWacomBattleHUDCombatLogController> CombatLogController;
	TSharedPtr<FWacomBattleHUDFirstPersonHandBridge> FirstPersonHandBridge;
	TSharedPtr<FWacomBattleHUDPresentationCoordinator> PresentationCoordinator;
	TSharedPtr<FWacomBattleHUDSceneEnemyTargetCoordinator> SceneEnemyTargetCoordinator;
	TSharedPtr<FWacomBattleHUDEnemyInspectionCoordinator> EnemyInspectionCoordinator;
	TSharedPtr<FWacomBattleSecondaryPanelCoordinator> SecondaryPanelCoordinator;

	EBattleUIState UIState = EBattleUIState::Idle;
	FGuid PendingTargetingCardId;
	bool bHasBroadcastBattleEnd = false;
	bool bBattleInputReady = true;
	bool bSecondaryPanelOpen = false;
	bool bFirstPersonBattleHandSuppressedForEntry = false;
	bool bEnemyActionPreviewActive = false;
	FBattleSnapshot LastBattleSnapshot;
	bool bHasLastBattleSnapshot = false;
};
