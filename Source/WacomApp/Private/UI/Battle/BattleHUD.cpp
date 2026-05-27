// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattleHUD.h"

#define LOCTEXT_NAMESPACE "WacomBattleHUD"
#include "UI/Battle/ActionPanel.h"
#include "Actors/WacomBattle3DHandPresenter.h"
#include "Actors/WacomBattleCardVisualActor.h"
#include "Components/WacomBattlePresentationTargetComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
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
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "Input/WacomInputContextCoordinatorSubsystem.h"
#include "InputCoreTypes.h"
#include "UObject/UObjectIterator.h"

#include "Input/UIActionBindingHandle.h"
#include "Input/CommonUIInputSettings.h"

#include "Events/BattleEvent.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Types/WacomEnums.h"

namespace
{
	const TCHAR* CardDetailPanelPath = TEXT("/Game/Wacom/UI/Card/WBP_CardDetailPanel.WBP_CardDetailPanel_C");
	const FName FirstPersonBattleHandLayerSourceId(TEXT("BattleHand"));
	const FVector2D FirstPersonCardDetailAnchorBaseSize(260.0f, 380.0f);
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
}

void UBattleHUD::NativeDestruct()
{
	ClearBattlePresentationQueue();
	ClearFirstPersonBattleHandLayer();
	ClearSceneEnemyTargetSelectionAffordances();
	UnregisterSceneEnemyPresentationTargets(false);
	DestroyBattle3DHandPresenter();
	ClearBattlePresentationTargetRegistry();
	ReleaseAllPlayerControllerInteractionEvents();
	if (HandPanel)
	{
		HandPanel->OnCardHoveredNative.RemoveAll(this);
		HandPanel->OnCardUnhoveredNative.RemoveAll(this);
	}
	HideCardDetailPanel();
	CardDetailPanel = nullptr;
	bHasLastBattleSnapshot = false;
	LastBattleSnapshot = FBattleSnapshot();
	Super::NativeDestruct();
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
	LastBattleSnapshot = Snap;
	bHasLastBattleSnapshot = true;

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

	SyncFirstPersonBattleHandLayer(Snap);

	// 战斗结束 → 切到 BattleEnd 状态，并广播一次
	if (Snap.Phase == EBattlePhase::BattleEnd)
	{
		bHasLastBattleSnapshot = false;
		SetUIState(EBattleUIState::BattleEnd);

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

	SyncSceneEnemyPresentationTargets(Snap);
}

void UBattleHUD::NativeOnSessionChanged(UBattleSession* OldSession, UBattleSession* NewSession)
{
	Super::NativeOnSessionChanged(OldSession, NewSession);
	if (OldSession != NewSession)
	{
		ClearBattlePresentationQueue();
		ClearFirstPersonBattleHandLayer();
		ClearSceneEnemyTargetSelectionAffordances();
		UnregisterSceneEnemyPresentationTargets(false);
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
	SyncSceneEnemyPresentationTargets(Snap);
	SyncSceneEnemyTargetSelectionAffordances();
}

UWacomFirstPersonCardAnchorComponent* UBattleHUD::ResolveFirstPersonCardAnchor() const
{
	const APlayerController* PC = GetOwningPlayer();
	const AWacomPlayerCharacter* Character = PC ? Cast<AWacomPlayerCharacter>(PC->GetPawn()) : nullptr;
	return Character ? Character->GetFirstPersonCardAnchorComponent() : nullptr;
}

void UBattleHUD::SyncFirstPersonBattleHandLayer(const FBattleSnapshot& Snap)
{
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchor();
	const bool bCanShowBattleHand =
		bEnableFirstPersonBattleHandLayerPrototype
		&& GetSession()
		&& Snap.Phase != EBattlePhase::BattleEnd
		&& Anchor;
	if (!bCanShowBattleHand)
	{
		ClearFirstPersonBattleHandLayer();
		return;
	}

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
		Entry.bIsPendingTargeting =
			IsInTargetSelect()
			&& PendingTargetingCardId.IsValid()
			&& CardSnapshot.InstanceId == PendingTargetingCardId;
		CardEntries.Add(MoveTemp(Entry));
	}

	Anchor->SetRuntimeCardLayerEntries(FirstPersonBattleHandLayerSourceId, CardEntries);
	Anchor->SetBattleHandInteractionPrototypeEnabled(
		bEnableFirstPersonBattleHandLayerPrototype
		&& bEnableFirstPersonBattleHandInteractionPrototype);
	BindFirstPersonBattleHandLayerInteractions(Anchor);
	LastFirstPersonBattleHandAnchor = Anchor;
}

void UBattleHUD::ClearFirstPersonBattleHandLayer()
{
	if (UWacomFirstPersonCardAnchorComponent* LastAnchor = LastFirstPersonBattleHandAnchor.Get())
	{
		LastAnchor->SetBattleHandInteractionPrototypeEnabled(false);
		UnbindFirstPersonBattleHandLayerInteractions(LastAnchor);
		LastAnchor->ClearRuntimeCardLayerData(FirstPersonBattleHandLayerSourceId);
	}
	if (UWacomFirstPersonCardAnchorComponent* CurrentAnchor = ResolveFirstPersonCardAnchor())
	{
		CurrentAnchor->SetBattleHandInteractionPrototypeEnabled(false);
		UnbindFirstPersonBattleHandLayerInteractions(CurrentAnchor);
		CurrentAnchor->ClearRuntimeCardLayerData(FirstPersonBattleHandLayerSourceId);
	}
	LastFirstPersonBattleHandAnchor.Reset();
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
	Anchor->OnFirstPersonCardLayerCardClicked.AddUObject(this, &UBattleHUD::HandleFirstPersonCardLayerCardClicked);
	Anchor->OnFirstPersonCardLayerCardHovered.AddUObject(this, &UBattleHUD::HandleFirstPersonCardLayerCardHovered);
	Anchor->OnFirstPersonCardLayerCardUnhovered.AddUObject(this, &UBattleHUD::HandleFirstPersonCardLayerCardUnhovered);
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
}

void UBattleHUD::HandleFirstPersonCardLayerCardClicked(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& /*SlotView*/)
{
	if (!bEnableFirstPersonBattleHandLayerPrototype || !bEnableFirstPersonBattleHandInteractionPrototype)
	{
		return;
	}
	OnCardClickedByUser(CardInstanceId);
}

void UBattleHUD::HandleFirstPersonCardLayerCardHovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (!bEnableFirstPersonBattleHandLayerPrototype
		|| !bEnableFirstPersonBattleHandInteractionPrototype
		|| UIState != EBattleUIState::Idle
		|| !CardInstanceId.IsValid()
		|| !SlotView.bProjected)
	{
		HideCardDetailPanel();
		return;
	}

	const FHandCardSnapshot* CardSnapshot = FindLastBattleHandCardSnapshot(CardInstanceId);
	if (!CardSnapshot || !CardSnapshot->Definition)
	{
		HideCardDetailPanel();
		return;
	}

	const FVector2D AnchorSize = FirstPersonCardDetailAnchorBaseSize * FMath::Max(0.01f, SlotView.RenderScale);
	const FVector2D AnchorPosition = SlotView.ScreenPosition - AnchorSize * 0.5f;
	if (ShowCardDetailAtAnchor(
		UWacomCardPresentationBuilder::BuildCardDetailViewData(CardSnapshot->Definition),
		AnchorPosition,
		AnchorSize))
	{
		CurrentCardDetailSource.Reset();
		CurrentFirstPersonCardDetailSourceId = CardInstanceId;
	}
}

void UBattleHUD::HandleFirstPersonCardLayerCardUnhovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& /*SlotView*/)
{
	HideFirstPersonCardDetailPanelForSource(CardInstanceId);
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
	return CardDetailPanel && CardDetailPanel->GetVisibility() != ESlateVisibility::Collapsed;
}

FText UBattleHUD::GetCardDetailPanelNameText() const
{
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
	if (ShowCardDetailAtAnchor(
		UWacomCardPresentationBuilder::BuildCardDetailViewData(SourceWidget->GetCardSnapshot().Definition),
		AnchorPosition,
		AnchorSize))
	{
		CurrentCardDetailSource = SourceWidget;
		CurrentFirstPersonCardDetailSourceId.Invalidate();
		return true;
	}
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
	Panel->SetRenderOpacity(1.0f);
	Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	return true;
}

void UBattleHUD::HideCardDetailPanel()
{
	if (CardDetailPanel)
	{
		CardDetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	CurrentCardDetailSource.Reset();
	CurrentFirstPersonCardDetailSourceId.Invalidate();
}

void UBattleHUD::HideCardDetailPanelForSource(UCardWidget* SourceWidget)
{
	if (!SourceWidget || CurrentCardDetailSource.Get() != SourceWidget)
	{
		return;
	}
	HideCardDetailPanel();
}

void UBattleHUD::HideFirstPersonCardDetailPanelForSource(const FGuid& CardInstanceId)
{
	if (!CardInstanceId.IsValid() || CurrentFirstPersonCardDetailSourceId != CardInstanceId)
	{
		return;
	}
	HideCardDetailPanel();
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
	if (UCanvasPanelSlot* DetailSlot = CardDetailLayer->AddChildToCanvas(CardDetailPanel))
	{
		DetailSlot->SetAutoSize(false);
		DetailSlot->SetSize(CardDetailPanelEstimatedSize);
		DetailSlot->SetZOrder(1);
	}
	return CardDetailPanel;
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
	const FVector2D Position = ComputeCardDetailPanelPositionBeside(
		AnchorPosition,
		AnchorSize,
		LayerSize,
		CardDetailPanelEstimatedSize,
		CardDetailPanelPadding);

	if (UCanvasPanelSlot* DetailSlot = Cast<UCanvasPanelSlot>(CardDetailPanel->Slot))
	{
		DetailSlot->SetPosition(Position);
		DetailSlot->SetSize(CardDetailPanelEstimatedSize);
	}
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

void UBattleHUD::SyncSceneEnemyPresentationTargets(const FBattleSnapshot& Snap)
{
	const bool bCanAttemptAutoBind =
		bEnableSceneEnemyTargetBindingPrototype
		&& GetSession()
		&& Snap.Phase != EBattlePhase::BattleEnd;
	if (!bCanAttemptAutoBind)
	{
		if (bEnableSceneEnemyTargetBindingPrototype)
		{
			if (UWorld* World = GetWorld())
			{
				for (TObjectIterator<UWacomBattlePresentationTargetComponent> It; It; ++It)
				{
					UWacomBattlePresentationTargetComponent* Component = *It;
					if (IsValid(Component) && Component->GetWorld() == World)
					{
						Component->MarkAutoBindResult(TEXT("BattleEndedOrNoSession"));
					}
				}
			}
		}
		UnregisterSceneEnemyPresentationTargets();
		ClearSceneEnemyTargetSelectionAffordances();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UnregisterSceneEnemyPresentationTargets();
		ClearSceneEnemyTargetSelectionAffordances();
		return;
	}

	TMap<FName, FGuid> RuntimePartIdsByStablePartId;
	for (const FEnemyPartSnapshot& Part : Snap.Enemy.Parts)
	{
		if (!Part.InstanceId.IsValid() || !Part.Definition || Part.Definition->PartId.IsNone())
		{
			continue;
		}
		RuntimePartIdsByStablePartId.Add(Part.Definition->PartId, Part.InstanceId);
	}

	for (TObjectIterator<UWacomBattlePresentationTargetComponent> It; It; ++It)
	{
		UWacomBattlePresentationTargetComponent* Component = *It;
		if (!IsValid(Component) || Component->GetWorld() != World)
		{
			continue;
		}
		if (Component->GetPartId().IsNone())
		{
			Component->MarkAutoBindResult(TEXT("MissingPartId"));
			continue;
		}

		if (const FGuid* RuntimePartId = RuntimePartIdsByStablePartId.Find(Component->GetPartId()))
		{
			Component->SetPartInstanceId(*RuntimePartId);
			Component->RegisterWithBattleHUD(this);
			Component->MarkAutoBindResult(TEXT("MatchedPartId"));
		}
		else
		{
			Component->UnregisterFromBattleHUD();
			Component->SetPartInstanceId(FGuid());
			Component->MarkAutoBindResult(TEXT("MissingPartInSnapshot"));
		}
	}

	SyncSceneEnemyTargetSelectionAffordances();
}

void UBattleHUD::SyncSceneEnemyTargetSelectionAffordances()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FBattleTargetSelectionView TargetView = BuildTargetSelectionView();
	TMap<FGuid, FBattleTargetablePartView> TargetablePartsById;
	TargetablePartsById.Reserve(TargetView.TargetableParts.Num());
	for (const FBattleTargetablePartView& PartView : TargetView.TargetableParts)
	{
		if (PartView.PartInstanceId.IsValid())
		{
			TargetablePartsById.Add(PartView.PartInstanceId, PartView);
		}
	}

	for (TObjectIterator<UWacomBattlePresentationTargetComponent> It; It; ++It)
	{
		UWacomBattlePresentationTargetComponent* Component = *It;
		if (!IsValid(Component) || Component->GetWorld() != World)
		{
			continue;
		}
		if (!Component->IsRegisteredWithBattleHUD() || Component->RegisteredHUD.Get() != this)
		{
			continue;
		}

		const FBattleTargetablePartView* PartView = TargetablePartsById.Find(Component->GetPartInstanceId());
		if (!PartView)
		{
			Component->SetTargetSelectionAffordance(false, TEXT("UnknownPart"));
			continue;
		}

		Component->SetTargetSelectionAffordance(PartView->bTargetable, PartView->DisabledReason);
	}
}

void UBattleHUD::ClearSceneEnemyTargetSelectionAffordances(bool bOnlyThisHUD)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TObjectIterator<UWacomBattlePresentationTargetComponent> It; It; ++It)
	{
		UWacomBattlePresentationTargetComponent* Component = *It;
		if (!IsValid(Component) || Component->GetWorld() != World)
		{
			continue;
		}
		if (bOnlyThisHUD && Component->RegisteredHUD.Get() != this)
		{
			continue;
		}
		Component->SetTargetSelectionAffordance(false, TEXT("NotTargetSelecting"));
	}
}

void UBattleHUD::UnregisterSceneEnemyPresentationTargets(bool bOnlyAutoBoundTargets)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TObjectIterator<UWacomBattlePresentationTargetComponent> It; It; ++It)
	{
		UWacomBattlePresentationTargetComponent* Component = *It;
		if (!IsValid(Component) || Component->GetWorld() != World)
		{
			continue;
		}
		if (bOnlyAutoBoundTargets && Component->GetPartId().IsNone())
		{
			continue;
		}
		if (!Component->IsRegisteredWithBattleHUD() || Component->RegisteredHUD.Get() != this)
		{
			continue;
		}
		Component->SetTargetSelectionAffordance(false, TEXT("NotTargetSelecting"));
		Component->UnregisterFromBattleHUD();
	}
}

void UBattleHUD::ConsumeAndLogEvents()
{
	FWacomBattleHUDEventFlow::ConsumeAndLogEvents(*this);
}

void UBattleHUD::AppendBattleEventLogEntries(const TArray<FBattleEvent>& Events)
{
	FWacomBattleHUDEventFlow::AppendBattleEventLogEntries(*this, Events);
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

