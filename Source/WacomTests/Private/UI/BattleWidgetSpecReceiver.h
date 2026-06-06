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
#include "UI/Battle/CardWidget.h"
#include "UI/Battle/EnemyInfoBar.h"
#include "UI/Battle/EnemyPartWidget.h"
#include "UI/Battle/HandPanel.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/Battle/WacomKnockdownChoiceDialog.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Common/PileCountView.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/HorizontalBoxSlot.h"
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
class UWacomBattleCardWidgetClickReceiver : public UObject
{
	GENERATED_BODY()

public:
	int32 ClickCount = 0;
	FGuid LastClickedId;

	UFUNCTION()
	void HandleClicked(FGuid CardInstanceId)
	{
		++ClickCount;
		LastClickedId = CardInstanceId;
	}

	int32 HoverCount = 0;
	int32 UnhoverCount = 0;
	TObjectPtr<UCardWidget> LastHoveredWidget = nullptr;
	TObjectPtr<UCardWidget> LastUnhoveredWidget = nullptr;

	void HandleHovered(UCardWidget* SourceWidget)
	{
		++HoverCount;
		LastHoveredWidget = SourceWidget;
	}

	void HandleUnhovered(UCardWidget* SourceWidget)
	{
		++UnhoverCount;
		LastUnhoveredWidget = SourceWidget;
	}
};

UCLASS()
class UWacomBattleCardWidgetTestProbe : public UCardWidget
{
	GENERATED_BODY()

public:
	bool IsRootButtonEnabledForTest() const
	{
		return IsRootButtonInteractable();
	}

	bool RequestClickForTest()
	{
		return TryClickRootButton();
	}

	void RequestHoverForTest()
	{
		RequestHover();
	}

	void RequestUnhoverForTest()
	{
		RequestUnhover();
	}

	bool IsHoveredForTest() const
	{
		return IsHoverActive();
	}

	FWidgetTransform GetRenderTransformForTest() const
	{
		return GetRenderTransform();
	}

	FVector2D GetRenderTransformPivotForTest() const
	{
		return GetRenderTransformPivot();
	}

	FWidgetTransform GetHoverVisualRenderTransformForTest() const
	{
		const UWidget* Target = GetHoverTransformTarget();
		return Target ? Target->GetRenderTransform() : FWidgetTransform();
	}

	FVector2D GetHoverVisualRenderTransformPivotForTest() const
	{
		const UWidget* Target = GetHoverTransformTarget();
		return Target ? Target->GetRenderTransformPivot() : FVector2D(0.5f, 0.5f);
	}
};

UCLASS()
class UWacomBattleCardWidgetNoCardViewTest : public UWacomBattleCardWidgetTestProbe
{
	GENERATED_BODY()

public:
	void DisableCardViewForTest()
	{
		CardView = nullptr;
	}

	FString GetFallbackZoneText() const
	{
		return ZoneText ? ZoneText->GetText().ToString() : FString();
	}
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
class UWacomBattleCardWidgetHoverVisualRootTest : public UWacomBattleCardWidgetTestProbe
{
	GENERATED_BODY()

public:
	void DisableHoverVisualRootForTest()
	{
		HoverVisualRoot = nullptr;
	}

	bool HasHoverVisualRootForTest() const
	{
		return HoverVisualRoot != nullptr;
	}
};

UCLASS()
class UWacomBattleHandPanelLayoutTest : public UHandPanel
{
	GENERATED_BODY()

public:
	FMargin GetCardSlotPaddingForTest(int32 ChildIndex) const
	{
		if (!UnifiedHandSlot || !UnifiedHandSlot->GetChildAt(ChildIndex))
		{
			return FMargin();
		}

		const UWidget* Child = UnifiedHandSlot->GetChildAt(ChildIndex);
		const UHorizontalBoxSlot* HorizontalSlot = Child ? Cast<UHorizontalBoxSlot>(Child->Slot) : nullptr;
		return HorizontalSlot ? HorizontalSlot->GetPadding() : FMargin();
	}

	EVerticalAlignment GetCardSlotVerticalAlignmentForTest(int32 ChildIndex) const
	{
		if (!UnifiedHandSlot || !UnifiedHandSlot->GetChildAt(ChildIndex))
		{
			return VAlign_Fill;
		}

		const UWidget* Child = UnifiedHandSlot->GetChildAt(ChildIndex);
		const UHorizontalBoxSlot* HorizontalSlot = Child ? Cast<UHorizontalBoxSlot>(Child->Slot) : nullptr;
		return HorizontalSlot ? HorizontalSlot->GetVerticalAlignment() : VAlign_Fill;
	}

	EHorizontalAlignment GetUnifiedSlotHorizontalAlignmentForTest() const
	{
		if (const UHorizontalBoxSlot* HorizontalSlot = UnifiedHandSlot ? Cast<UHorizontalBoxSlot>(UnifiedHandSlot->Slot) : nullptr)
		{
			return HorizontalSlot->GetHorizontalAlignment();
		}
		if (const UBorderSlot* BorderSlot = UnifiedHandSlot ? Cast<UBorderSlot>(UnifiedHandSlot->Slot) : nullptr)
		{
			return BorderSlot->GetHorizontalAlignment();
		}
		return HAlign_Fill;
	}

	UWacomBattleCardWidgetTestProbe* GetSpawnedCardProbeForTest(int32 Index) const
	{
		return Cast<UWacomBattleCardWidgetTestProbe>(GetSpawnedCardAt(Index));
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

	void SetBattleHandPresentationModeForTest(EWacomBattleHandPresentationMode NewMode)
	{
		SetBattleHandPresentationMode(NewMode);
	}

	void SetBattleSceneEnemyHostForTest(AWacomBattleEnemyActor* InHost)
	{
		SetBattleSceneEnemyHost(InHost);
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

	void SyncLegacyHandPanelVisibilityForTest()
	{
		SyncLegacyHandPanelVisibility();
	}

	bool HasHandPanelForTest() const
	{
		return HandPanel != nullptr;
	}

	ESlateVisibility GetHandPanelVisibilityForTest() const
	{
		return HandPanel ? HandPanel->GetVisibility() : ESlateVisibility::Collapsed;
	}

	void SetHandPanelVisibilityForTest(ESlateVisibility InVisibility)
	{
		if (HandPanel)
		{
			HandPanel->SetVisibility(InVisibility);
		}
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

	bool ShowCardDetailForTest(UCardWidget* SourceWidget)
	{
		return ShowCardDetailForCardWidget(SourceWidget);
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

	void HandleCardHoveredForTest(UCardWidget* SourceWidget)
	{
		HandleHandCardHovered(SourceWidget);
	}

	void HandleCardUnhoveredForTest(UCardWidget* SourceWidget)
	{
		HandleHandCardUnhovered(SourceWidget);
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

	bool IsLegacyCardDetailPanelVisibleForTest() const
	{
		return CardDetailPanel && CardDetailPanel->GetVisibility() != ESlateVisibility::Collapsed;
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

	bool HasCardDetailLayerForTest() const
	{
		return CardDetailLayer != nullptr;
	}

	bool EnsureCardDetailPanelForTest()
	{
		return EnsureCardDetailPanel() != nullptr;
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

	void SetEnemyInfoBarForTest(UEnemyInfoBar* InEnemyInfoBar)
	{
		EnemyInfoBar = InEnemyInfoBar;
		if (InEnemyInfoBar)
		{
			ChildBattleWidgets.AddUnique(InEnemyInfoBar);
		}
		SyncEnemyInfoBarFallbackVisibility();
	}

	ESlateVisibility GetEnemyInfoBarVisibilityForTest() const
	{
		return EnemyInfoBar ? EnemyInfoBar->GetVisibility() : ESlateVisibility::Collapsed;
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
class UWacomBattleEnemyInfoBarTest : public UEnemyInfoBar
{
	GENERATED_BODY()

public:
	int32 GetSpawnedPartCountForTest() const
	{
		return SpawnedParts.Num();
	}

	bool IsSpawnedPartTargetableForTest(int32 Index) const
	{
		return SpawnedParts.IsValidIndex(Index) && SpawnedParts[Index]
			? SpawnedParts[Index]->IsTargetable()
			: false;
	}

	UEnemyPartWidget* GetSpawnedPartForTest(int32 Index) const
	{
		return SpawnedParts.IsValidIndex(Index) ? SpawnedParts[Index] : nullptr;
	}
};

UCLASS()
class UWacomBattleEnemyPartWidgetPresentationProbe : public UEnemyPartWidget
{
	GENERATED_BODY()

public:
	void PlayCueForTest(EBattleEventType SourceEventType, int32 Amount)
	{
		FWacomBattlePresentationTargetCue Cue;
		Cue.SourceEventType = SourceEventType;
		Cue.Amount = Amount;
		PlayBattlePresentationCue(Cue);
	}

	void PlayTargetConfirmedCueForTest()
	{
		FWacomBattlePresentationTargetCue Cue;
		Cue.CueKind = EWacomBattlePresentationTargetCueKind::TargetConfirmed;
		PlayBattlePresentationCue(Cue);
	}

	bool IsBattlePresentationCueActiveForTest() const
	{
		return bBattlePresentationCueActive;
	}

	EWacomBattlePresentationTargetCueKind GetLastBattlePresentationCueKindForTest() const
	{
		return LastBattlePresentationCueKind;
	}

	EBattleEventType GetLastBattlePresentationCueTypeForTest() const
	{
		return LastBattlePresentationCueType;
	}

	int32 GetLastBattlePresentationCueAmountForTest() const
	{
		return LastBattlePresentationCueAmount;
	}

	int32 GetBattlePresentationCuePlayCountForTest() const
	{
		return BattlePresentationCuePlayCount;
	}

	void ClearBattlePresentationCueForTest()
	{
		ClearBattlePresentationCue();
	}
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
