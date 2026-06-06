// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerController.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/ActionPanel.h"
#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/Battle/BattlePresentationStackWidget.h"
#include "UI/Battle/WacomKnockdownChoiceDialog.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Common/PileCountView.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "BattleWidgetSpecReceiver.generated.h"

struct FWacomBattleSceneTargetClickTestAccess;

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
class UWacomActionPanelTestProbe : public UActionPanel
{
	GENERATED_BODY()

public:
	bool IsWaitButtonEnabledForTest() const
	{
		return WaitButton ? WaitButton->GetIsEnabled() : false;
	}

	bool IsEndTurnButtonEnabledForTest() const
	{
		return EndTurnButton ? EndTurnButton->GetIsEnabled() : false;
	}

	FText GetWaitValueTextForTest() const
	{
		return WaitValueText ? WaitValueText->GetText() : FText::GetEmpty();
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

	void SetBattleSceneEnemyHostForTest(AWacomBattleEnemyActor* InHost)
	{
		SetBattleSceneEnemyHost(InHost);
	}

	void SetBattleSceneEnemyHostsForTest(const TArray<AWacomBattleEnemyActor*>& InHosts)
	{
		SetBattleSceneEnemyHosts(InHosts);
	}

	FWacomBattleHUDAutomationTestView AutomationViewForTest() const
	{
		return GetAutomationTestViewForTest();
	}

	int32 GetBattleSceneEnemyPartWorldTargetBridgeCountForTest() const
	{
		return AutomationViewForTest().SceneEnemyPartWorldTargetBridgeCount;
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

	void RecordFirstPersonPlayCommitForTest(const FGuid& CardInstanceId, const FGuid& TargetPartInstanceId = FGuid())
	{
		RecordFirstPersonPlayCommit(CardInstanceId, TargetPartInstanceId);
	}

	TArray<FWacomFirstPersonCardLayerTransitionHint> BuildFirstPersonCardTransitionHintsForTest(
		const FBattleSnapshot& PreviousSnapshot,
		const FBattleSnapshot& NextSnapshot) const
	{
		return BuildFirstPersonCardTransitionHints(PreviousSnapshot, NextSnapshot);
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
		PendingTargetingCardId = PendingCardId;
		SetUIState(EBattleUIState::TargetSelect);
	}

	void ClearTargetSelectionStateForTest()
	{
		PendingTargetingCardId.Invalidate();
		SetUIState(EBattleUIState::Idle);
	}

	void SetUIStateForTest(EBattleUIState NewState)
	{
		SetUIState(NewState);
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

	void HandleFirstPersonCardPointerMovedForTest(
		const FWacomFirstPersonCardPointerView& PointerView)
	{
		HandleFirstPersonCardLayerPointerMoved(PointerView);
	}

	void HandleFirstPersonCardPointerLeftForTest()
	{
		HandleFirstPersonCardLayerPointerLeft();
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
		return UIState;
	}

	bool HasLastBattleSnapshotForTest() const
	{
		return bHasLastBattleSnapshot;
	}

	int32 GetLastBattleSnapshotHandCountForTest() const
	{
		return LastBattleSnapshot.Hand.Cards.Num();
	}

	bool HasLastBattleHandCardForTest(const FGuid& CardInstanceId) const
	{
		return FindLastBattleHandCardSnapshot(CardInstanceId) != nullptr;
	}

	void SetCombatLogFeedForTest(UBattleCombatLogFeedWidget* InFeed)
	{
		CombatLogFeed = InFeed;
		if (InFeed)
		{
			ChildBattleWidgets.AddUnique(InFeed);
		}
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
		QueuePendingTurnBoundaryCommand(ETurnBoundaryCommand::Wait);
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

	void AppendBattleCombatLogBlockForTest(const FWacomBattleCombatLogBlockView& Block)
	{
		AppendBattleCombatLogBlock(Block);
	}

	void SetActionPanelForTest(UActionPanel* InActionPanel)
	{
		ActionPanel = InActionPanel;
		if (InActionPanel)
		{
			ChildBattleWidgets.AddUnique(InActionPanel);
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

	void PlayBattlePresentationCueForTest(
		EBattleEventType SourceEventType,
		const FGuid& TargetPartInstanceId,
		int32 Amount)
	{
		UBattleHUD::PlayBattlePresentationCueForTest(SourceEventType, TargetPartInstanceId, Amount);
	}

	void PlayTargetConfirmedCueForTest(const FGuid& TargetPartInstanceId)
	{
		UBattleHUD::PlayTargetConfirmedCueForTest(TargetPartInstanceId);
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

UCLASS()
class UWacomBattleKnockdownChoiceDialogTest : public UWacomKnockdownChoiceDialog
{
	GENERATED_BODY()

public:
	bool IsAidButtonEnabledForTest() const
	{
		return AidButton ? AidButton->GetIsEnabled() : false;
	}

	bool IsWithdrawButtonEnabledForTest() const
	{
		return WithdrawButton ? WithdrawButton->GetIsEnabled() : false;
	}

	bool IsDestroyButtonEnabledForTest() const
	{
		return DestroyButton ? DestroyButton->GetIsEnabled() : false;
	}

	FString GetPartNameTextForTest() const
	{
		return PartNameText ? PartNameText->GetText().ToString() : FString();
	}

	FReply PressEscapeForTest()
	{
		const FKeyEvent KeyEvent(EKeys::Escape, FModifierKeysState(), 0, false, 0, 0);
		return NativeOnKeyDown(FGeometry(), KeyEvent);
	}

	FReply PressGamepadBackForTest()
	{
		const FKeyEvent KeyEvent(EKeys::Gamepad_FaceButton_Right, FModifierKeysState(), 0, false, 0, 0);
		return NativeOnKeyDown(FGeometry(), KeyEvent);
	}
};
