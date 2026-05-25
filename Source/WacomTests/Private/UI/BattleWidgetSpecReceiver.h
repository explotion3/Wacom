// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerController.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/BattleEventLogPanel.h"
#include "UI/Battle/CardWidget.h"
#include "UI/Battle/EnemyInfoBar.h"
#include "UI/Battle/EnemyPartWidget.h"
#include "UI/Battle/EventToast.h"
#include "UI/Battle/HandPanel.h"
#include "UI/Battle/WacomKnockdownChoiceDialog.h"
#include "Components/WacomBattlePresentationTargetComponent.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "BattleWidgetSpecReceiver.generated.h"

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
		return OutHUD && OutHUD->bEnableSceneEnemyTargetBindingPrototype;
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

	void Enable3DHandPrototypeForTest()
	{
		bEnable3DHandPrototype = true;
	}

	void EnableSceneEnemyTargetBindingPrototypeForTest()
	{
		bEnableSceneEnemyTargetBindingPrototype = true;
	}

	void DestroyBattle3DHandPresenterForTest()
	{
		DestroyBattle3DHandPresenter();
	}

	bool HasBattle3DHandPresenterForTest() const
	{
		return Battle3DHandPresenter.Get() != nullptr;
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

	void AppendBattleEventLogEntriesForTest(const TArray<FBattleEvent>& Events)
	{
		AppendBattleEventLogEntries(Events);
	}

	void SyncBattleEventLogPanelForTest()
	{
		SyncBattleEventLogPanel();
	}

	void SetEventLogPanelForTest(UBattleEventLogPanel* InPanel)
	{
		EventLogPanel = InPanel;
	}

	void SetEventToastForTest(UEventToast* InEventToast)
	{
		EventToast = InEventToast;
	}

	void SetEnemyInfoBarForTest(UEnemyInfoBar* InEnemyInfoBar)
	{
		EnemyInfoBar = InEnemyInfoBar;
		if (InEnemyInfoBar)
		{
			ChildBattleWidgets.AddUnique(InEnemyInfoBar);
		}
	}

	void EnqueueBattlePresentationEventsForTest(const TArray<FBattleEvent>& Events)
	{
		EnqueueBattlePresentationEvents(Events);
	}

	void ClearBattlePresentationQueueForTest()
	{
		ClearBattlePresentationQueue();
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

	int32 GetBattlePresentationTargetCountForTest() const
	{
		return UBattleHUD::GetBattlePresentationTargetCountForTest();
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
class UWacomBattleEventToastProbe : public UEventToast
{
	GENERATED_BODY()

public:
	void GetActiveToastTextsForTest(TArray<FString>& OutTexts) const
	{
		OutTexts.Reset();
		for (UTextBlock* Text : ActiveTexts)
		{
			OutTexts.Add(Text ? Text->GetText().ToString() : FString());
		}
	}

	int32 GetActiveToastTextCountForTest() const
	{
		return ActiveTexts.Num();
	}
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
		PlayBattlePresentationCue(SourceEventType, Amount);
	}

	bool IsBattlePresentationCueActiveForTest() const
	{
		return bBattlePresentationCueActive;
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
class UWacomBattlePresentationTargetComponentProbe : public UWacomBattlePresentationTargetComponent
{
	GENERATED_BODY()

public:
	void BroadcastClickForTest(UPrimitiveComponent* Primitive, FKey Button = EKeys::LeftMouseButton)
	{
		if (Primitive)
		{
			Primitive->OnClicked.Broadcast(Primitive, Button);
		}
	}

	bool HasBoundClickTargetForTest() const
	{
		return BoundClickTarget.IsValid();
	}

	bool HasAcquiredPlayerControllerClickEventsForTest() const
	{
		return bHasAcquiredPlayerControllerClickEvents;
	}

	bool IsVisualFeedbackActiveForTest() const
	{
		return IsVisualFeedbackActive();
	}

	void RestoreVisualFeedbackForTest()
	{
		RestoreVisualFeedback();
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
