// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDFirstPersonHandBridge.h"

#include "Cards/CardDefinition.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "Resolution/BattleTargetValidationResult.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleHUDCommandFlow.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

namespace
{
	const FName FirstPersonBattleHandLayerSourceId(TEXT("BattleHand"));

	bool ContainsHandCardIdForFirstPersonHandBridge(
		const FBattleSnapshot& Snapshot,
		const FGuid& CardInstanceId)
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

	const FHandCardSnapshot* FindHandCardSnapshotForFirstPersonHandBridge(
		const FBattleSnapshot& Snapshot,
		const FGuid& CardInstanceId)
	{
		if (!CardInstanceId.IsValid())
		{
			return nullptr;
		}

		for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
		{
			if (CardSnapshot.InstanceId == CardInstanceId)
			{
				return &CardSnapshot;
			}
		}
		return nullptr;
	}

	const TCHAR* WacomCardDropRejectReasonToString(EWacomBattleCardDropRejectReason RejectReason)
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
		case EWacomBattleTargetRejectReason::TargetIdentityMismatch:
			return EWacomBattleCardDropRejectReason::InvalidWorldTarget;
		case EWacomBattleTargetRejectReason::UnsupportedCardTarget:
		case EWacomBattleTargetRejectReason::TargetCardInvalid:
		case EWacomBattleTargetRejectReason::TargetCardNotInHand:
		case EWacomBattleTargetRejectReason::UnsupportedNormalHandCardTarget:
		case EWacomBattleTargetRejectReason::UnsupportedHandAnchorTarget:
		case EWacomBattleTargetRejectReason::MissingRequiredTargetKeyword:
		case EWacomBattleTargetRejectReason::BlockedTargetKeyword:
			return EWacomBattleCardDropRejectReason::UnsupportedCardTarget;
		case EWacomBattleTargetRejectReason::SelfTarget:
			return EWacomBattleCardDropRejectReason::SelfTarget;
		case EWacomBattleTargetRejectReason::UnsupportedZoneTarget:
			return EWacomBattleCardDropRejectReason::UnsupportedZoneTarget;
		default:
			return EWacomBattleCardDropRejectReason::MissingTarget;
		}
	}
}

FWacomBattleHUDFirstPersonHandBridge::FWacomBattleHUDFirstPersonHandBridge(UBattleHUD& InHUD)
	: HUD(InHUD)
{
}

FWacomBattleHUDFirstPersonHandBridge::~FWacomBattleHUDFirstPersonHandBridge()
{
	ClearLayer();
}

UWacomFirstPersonCardAnchorComponent* FWacomBattleHUDFirstPersonHandBridge::ResolveAnchor() const
{
	const APlayerController* PC = HUD.GetOwningPlayer();
	const AWacomPlayerCharacter* Character = PC ? Cast<AWacomPlayerCharacter>(PC->GetPawn()) : nullptr;
	return Character ? Character->GetFirstPersonCardAnchorComponent() : nullptr;
}

UWacomFirstPersonCardAnchorComponent* FWacomBattleHUDFirstPersonHandBridge::ResolveActiveAnchor() const
{
	if (UWacomFirstPersonCardAnchorComponent* PreviousAnchor = LastAnchor.Get())
	{
		return PreviousAnchor;
	}

	return ResolveAnchor();
}

void FWacomBattleHUDFirstPersonHandBridge::SyncLayer(
	const FBattleSnapshot& Snapshot,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints)
{
	if (HUD.IsFirstPersonBattleHandSuppressedForEntry())
	{
		SuppressLayerForEntry();
		return;
	}

	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveAnchor();
	const bool bCanShowBattleHand =
		ShouldUseFirstPersonBattleHandLayer()
		&& HUD.GetSession()
		&& Snapshot.Phase != EBattlePhase::BattleEnd
		&& HUD.UIState != EBattleUIState::BattleEnd
		&& Anchor;
	if (!bCanShowBattleHand)
	{
		ClearLayer();
		return;
	}

	bFirstPersonBattleHandLayerRuntimeActive = true;
	TArray<FWacomFirstPersonCardLayerEntry> CardEntries;
	CardEntries.Reserve(Snapshot.Hand.Cards.Num());
	for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
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
			HUD.IsInTargetSelect()
			&& HUD.PendingTargetingCardId.IsValid()
			&& CardSnapshot.InstanceId == HUD.PendingTargetingCardId;
		CardEntries.Add(MoveTemp(Entry));
	}

	Anchor->SetRuntimeCardLayerTransitionHints(FirstPersonBattleHandLayerSourceId, TransitionHints);
	Anchor->SetRuntimeCardLayerEntries(FirstPersonBattleHandLayerSourceId, CardEntries);
	Anchor->SetBattleHandInteractionEnabled(ShouldEnableFirstPersonBattleHandInteraction());
	BindLayerInteractions(Anchor);
	LastAnchor = Anchor;
}

void FWacomBattleHUDFirstPersonHandBridge::ClearLayer()
{
	bFirstPersonBattleHandLayerRuntimeActive = false;

	auto ClearBattleHandLayerOwnership = [this](UWacomFirstPersonCardAnchorComponent* Anchor)
	{
		if (!Anchor)
		{
			return;
		}

		const bool bOwnsBattleHandLayer =
			Anchor->GetRuntimeCardLayerSourceId() == FirstPersonBattleHandLayerSourceId;
		if (bOwnsBattleHandLayer)
		{
			Anchor->SetBattleHandInteractionEnabled(false);
			Anchor->CancelFirstPersonCardDragGesture(true);
		}

		UnbindLayerInteractions(Anchor);
		if (bOwnsBattleHandLayer)
		{
			Anchor->ClearRuntimeCardLayerData(FirstPersonBattleHandLayerSourceId);
		}
	};

	UWacomFirstPersonCardAnchorComponent* PreviousAnchor = LastAnchor.Get();
	UWacomFirstPersonCardAnchorComponent* CurrentAnchor = ResolveAnchor();
	ClearBattleHandLayerOwnership(PreviousAnchor);
	if (CurrentAnchor != PreviousAnchor)
	{
		ClearBattleHandLayerOwnership(CurrentAnchor);
	}
	ClearDragCameraLookOverride();
	ClearDragTargetFeedback(/*bClearFirstPersonCardLayerFeedback*/ false);
	bFirstPersonCardDragActiveForBattleSceneHover = false;
	LastAnchor.Reset();
	HUD.ForceHideCardDetailHost(UBattleHUD::ECardDetailHost::FirstPersonViewport);
	ClearPendingTransitionEvents();
}

void FWacomBattleHUDFirstPersonHandBridge::SuppressLayerForEntry()
{
	ClearLayer();

	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveAnchor();
	if (!Anchor)
	{
		return;
	}

	Anchor->SetBattleHandInteractionEnabled(false);
	Anchor->CancelFirstPersonCardDragGesture(true);
	Anchor->ClearCardLayerVisualState();
	TArray<FWacomFirstPersonCardLayerTransitionHint> EmptyHints;
	TArray<FWacomFirstPersonCardLayerEntry> EmptyEntries;
	Anchor->SetRuntimeCardLayerTransitionHints(FirstPersonBattleHandLayerSourceId, EmptyHints);
	Anchor->SetRuntimeCardLayerEntries(FirstPersonBattleHandLayerSourceId, EmptyEntries);
	Anchor->SetBattleHandInteractionEnabled(false);
	LastAnchor = Anchor;
}

bool FWacomBattleHUDFirstPersonHandBridge::ShouldUseFirstPersonBattleHandLayer() const
{
	return !HUD.IsFirstPersonBattleHandSuppressedForEntry();
}

bool FWacomBattleHUDFirstPersonHandBridge::ShouldEnableFirstPersonBattleHandInteraction() const
{
	return ShouldUseFirstPersonBattleHandLayer() && HUD.IsBattleInputReady();
}

void FWacomBattleHUDFirstPersonHandBridge::BindLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor)
{
	if (!Anchor)
	{
		return;
	}

	if (UWacomFirstPersonCardAnchorComponent* PreviousAnchor = LastAnchor.Get())
	{
		if (PreviousAnchor != Anchor)
		{
			UnbindLayerInteractions(PreviousAnchor);
		}
	}

	Anchor->OnFirstPersonCardLayerCardHovered.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerCardUnhovered.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerHoveredCardLayoutUpdated.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerDragStarted.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerDragUpdated.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerDragReleased.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerDragCancelled.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerPointerMoved.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerPointerLeft.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerCardHovered.AddUObject(
		&HUD,
		&UBattleHUD::HandleFirstPersonCardLayerCardHovered);
	Anchor->OnFirstPersonCardLayerCardUnhovered.AddUObject(
		&HUD,
		&UBattleHUD::HandleFirstPersonCardLayerCardUnhovered);
	Anchor->OnFirstPersonCardLayerHoveredCardLayoutUpdated.AddUObject(
		&HUD,
		&UBattleHUD::HandleFirstPersonCardLayerHoveredCardLayoutUpdated);
	Anchor->OnFirstPersonCardLayerDragStarted.AddUObject(
		&HUD,
		&UBattleHUD::HandleFirstPersonCardLayerDragStarted);
	Anchor->OnFirstPersonCardLayerDragUpdated.AddUObject(
		&HUD,
		&UBattleHUD::HandleFirstPersonCardLayerDragUpdated);
	Anchor->OnFirstPersonCardLayerDragReleased.AddUObject(
		&HUD,
		&UBattleHUD::HandleFirstPersonCardLayerDragReleased);
	Anchor->OnFirstPersonCardLayerDragCancelled.AddUObject(
		&HUD,
		&UBattleHUD::HandleFirstPersonCardLayerDragCancelled);
	Anchor->OnFirstPersonCardLayerPointerMoved.AddUObject(
		&HUD,
		&UBattleHUD::HandleFirstPersonCardLayerPointerMoved);
	Anchor->OnFirstPersonCardLayerPointerLeft.AddUObject(
		&HUD,
		&UBattleHUD::HandleFirstPersonCardLayerPointerLeft);
	LastAnchor = Anchor;
}

void FWacomBattleHUDFirstPersonHandBridge::UnbindLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor)
{
	if (!Anchor)
	{
		return;
	}

	Anchor->OnFirstPersonCardLayerCardHovered.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerCardUnhovered.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerHoveredCardLayoutUpdated.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerDragStarted.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerDragUpdated.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerDragReleased.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerDragCancelled.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerPointerMoved.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerPointerLeft.RemoveAll(&HUD);
	if (LastAnchor.Get() == Anchor)
	{
		LastAnchor.Reset();
	}
}

void FWacomBattleHUDFirstPersonHandBridge::HandleCardHovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (!ShouldEnableFirstPersonBattleHandInteraction()
		|| HUD.UIState != EBattleUIState::Idle
		|| !CardInstanceId.IsValid()
		|| !SlotView.bProjected)
	{
		HUD.ForceHideCardDetailHost(UBattleHUD::ECardDetailHost::FirstPersonViewport);
		return;
	}

	const FHandCardSnapshot* CardSnapshot = FindLastBattleHandCardSnapshot(CardInstanceId);
	if (!CardSnapshot || !CardSnapshot->Definition)
	{
		HUD.ForceHideCardDetailHost(UBattleHUD::ECardDetailHost::FirstPersonViewport);
		return;
	}

	HUD.SetFirstPersonCardDetailSource(CardInstanceId);
	if (HUD.ShowFirstPersonCardDetailAtSlot(
		UWacomCardPresentationBuilder::BuildCardDetailViewData(CardSnapshot->Definition),
		SlotView))
	{
	}
	else
	{
		HUD.ClearFirstPersonCardDetailSource();
	}
}

void FWacomBattleHUDFirstPersonHandBridge::HandleCardUnhovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& /*SlotView*/)
{
	if (HUD.IsFirstPersonCardInspectDetailActiveForSource(CardInstanceId))
	{
		return;
	}
	HUD.HideFirstPersonCardDetailPanelForSource(CardInstanceId);
}

void FWacomBattleHUDFirstPersonHandBridge::HandleHoveredCardLayoutUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (!ShouldEnableFirstPersonBattleHandInteraction()
		|| HUD.UIState != EBattleUIState::Idle
		|| !CardInstanceId.IsValid()
		|| !HUD.IsCurrentFirstPersonCardDetailSource(CardInstanceId)
		|| !SlotView.bProjected)
	{
		return;
	}

	HUD.UpdateFirstPersonCardDetailSlot(SlotView);
	HUD.PositionFirstPersonCardDetailPanelBesideSlot(SlotView);
}

void FWacomBattleHUDFirstPersonHandBridge::HandleDragStarted(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (!ShouldEnableFirstPersonBattleHandInteraction() || !CardInstanceId.IsValid())
	{
		return;
	}

	bFirstPersonCardDragActiveForBattleSceneHover = true;
	HUD.ClearBattleSceneEnemyPartHoverProbe(TEXT("FirstPersonDrag"));

	if (ShouldShowDragInspectDetail(DragView))
	{
		const FHandCardSnapshot* CardSnapshot = FindLastBattleHandCardSnapshot(CardInstanceId);
		if (CardSnapshot && CardSnapshot->Definition)
		{
			HUD.SetFirstPersonCardDetailSource(CardInstanceId);
			if (HUD.ShowFirstPersonCardDetailAtSlot(
				UWacomCardPresentationBuilder::BuildCardDetailViewData(CardSnapshot->Definition),
				DragView.SourceSlotView))
			{
			}
		}
	}
	else
	{
		HUD.ForceHideCardDetailHost(UBattleHUD::ECardDetailHost::FirstPersonViewport);
	}

	HandleDragUpdated(CardInstanceId, DragView);
}

void FWacomBattleHUDFirstPersonHandBridge::HandleDragUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (!ShouldEnableFirstPersonBattleHandInteraction() || !CardInstanceId.IsValid())
	{
		return;
	}

	ApplyDragCameraLookOverride(DragView);

	if (ShouldShowDragInspectDetail(DragView)
		&& HUD.IsCurrentFirstPersonCardDetailSource(CardInstanceId)
		&& DragView.SourceSlotView.bProjected)
	{
		HUD.UpdateFirstPersonCardDetailSlot(DragView.SourceSlotView);
		HUD.PositionFirstPersonCardDetailPanelBesideSlot(DragView.SourceSlotView);
		return;
	}
	else if (DragView.GestureState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
		|| DragView.GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit
		|| DragView.GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard)
	{
		HUD.ForceHideCardDetailHost(UBattleHUD::ECardDetailHost::FirstPersonViewport);
	}

	UpdateDragTargetFeedback(CardInstanceId, DragView);
}

void FWacomBattleHUDFirstPersonHandBridge::HandleDragReleased(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (!ShouldEnableFirstPersonBattleHandInteraction() || !CardInstanceId.IsValid())
	{
		return;
	}

	const FWacomFirstPersonCardDragView ReleaseDragView = DragView;
	const FWacomBattleCardDropResolveResult DropResult =
		ResolveDropIntent(CardInstanceId, ReleaseDragView);

	ClearDragCameraLookOverride();
	bFirstPersonCardDragActiveForBattleSceneHover = false;
	HUD.ClearBattleSceneEnemyPartHoverProbe(TEXT("FirstPersonDragReleased"));
	if (UWacomBattleEnemyPartPresentationComponent* PreviewPresentation = CurrentDragPreviewPresentation.Get())
	{
		PreviewPresentation->ClearDragTargetPreviewState();
	}
	CurrentDragPreviewPresentation.Reset();
	HUD.ForceHideCardDetailHost(UBattleHUD::ECardDetailHost::FirstPersonViewport);

	if (!DropResult.bCanSubmit)
	{
		return;
	}

	switch (DropResult.IntentKind)
	{
	case EWacomBattleCardDropIntentKind::PlayCardNoTarget:
		HUD.SubmitPlayCard(CardInstanceId, FGuid());
		return;

	case EWacomBattleCardDropIntentKind::PlayCardWorldTarget:
		FWacomBattleHUDCommandFlow::SubmitPlayCardOnWorldTarget(HUD, CardInstanceId, DropResult.TargetHandle);
		return;

	case EWacomBattleCardDropIntentKind::PlayCardCardTarget:
		HUD.SubmitPlayCardOnHandCard(CardInstanceId, DropResult.TargetHandle.CardInstanceId);
		return;

	default:
		return;
	}
}

void FWacomBattleHUDFirstPersonHandBridge::HandleDragCancelled(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& /*DragView*/)
{
	ClearDragCameraLookOverride();
	bFirstPersonCardDragActiveForBattleSceneHover = false;
	HUD.ClearBattleSceneEnemyPartHoverProbe(TEXT("FirstPersonDragCancelled"));
	ClearDragTargetFeedback();
	HUD.HideFirstPersonCardDetailPanelForSource(CardInstanceId);
	if (UWacomFirstPersonCardAnchorComponent* Anchor = ResolveActiveAnchor())
	{
		Anchor->SetFirstPersonCardDragFeedbackTarget(
			FWacomInteractionTargetHandle(),
			false,
			EWacomFirstPersonCardDragTargetFeedbackState::None);
	}
}

void FWacomBattleHUDFirstPersonHandBridge::HandlePointerMoved(
	const FWacomFirstPersonCardPointerView& PointerView)
{
	if (bFirstPersonCardDragActiveForBattleSceneHover)
	{
		return;
	}

	ApplyPointerCameraLookOverride(PointerView);
}

void FWacomBattleHUDFirstPersonHandBridge::HandlePointerLeft()
{
	if (bFirstPersonCardDragActiveForBattleSceneHover)
	{
		return;
	}

	ClearPointerCameraLookOverride();
}

void FWacomBattleHUDFirstPersonHandBridge::ApplyDragCameraLookOverride(
	const FWacomFirstPersonCardDragView& DragView)
{
	if (!DragView.bHasPointerViewportPosition)
	{
		return;
	}

	const UWacomFirstPersonCardAnchorComponent* Anchor = ResolveActiveAnchor();
	if (!Anchor
		|| !Anchor->bAllowCameraLookDuringCardDrag
		|| Anchor->CardDragCameraLookScale <= 0.0f)
	{
		ClearDragCameraLookOverride();
		return;
	}

	AWacomPlayerCharacter* Character = nullptr;
	if (const APlayerController* PC = HUD.GetOwningPlayer())
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

void FWacomBattleHUDFirstPersonHandBridge::ClearDragCameraLookOverride()
{
	ClearPointerCameraLookOverride();
}

void FWacomBattleHUDFirstPersonHandBridge::ApplyPointerCameraLookOverride(
	const FWacomFirstPersonCardPointerView& PointerView)
{
	if (!PointerView.bHasPointerViewportPosition)
	{
		return;
	}

	const UWacomFirstPersonCardAnchorComponent* Anchor = ResolveActiveAnchor();
	if (!Anchor
		|| !Anchor->bAllowCameraLookDuringCardPointer
		|| Anchor->CardPointerCameraLookScale <= 0.0f)
	{
		ClearPointerCameraLookOverride();
		return;
	}

	AWacomPlayerCharacter* Character = nullptr;
	if (const APlayerController* PC = HUD.GetOwningPlayer())
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
		PointerView.PointerNormalizedViewportPosition,
		Anchor->CardPointerCameraLookScale,
		Anchor->CardPointerCameraLookInterpSpeedOverride);
}

void FWacomBattleHUDFirstPersonHandBridge::ClearPointerCameraLookOverride()
{
	const APlayerController* PC = HUD.GetOwningPlayer();
	AWacomPlayerCharacter* Character = PC ? Cast<AWacomPlayerCharacter>(PC->GetPawn()) : nullptr;
	if (UWacomBattleCameraLookComponent* BattleCamera = Character ? Character->GetBattleCameraLookComponent() : nullptr)
	{
		BattleCamera->ClearCursorLookOverride();
	}
}

void FWacomBattleHUDFirstPersonHandBridge::UpdateDragTargetFeedback(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	const FWacomBattleCardDropResolveResult DropResult =
		ResolveDropIntent(CardInstanceId, DragView);
	TArray<FWacomFirstPersonCardTargetAffordance> CardTargetAffordances;
	if (DragView.GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard)
	{
		if (const UBattleSession* CurrentSession = HUD.GetSession())
		{
			const FBattleSnapshot CurrentSnapshot = CurrentSession->BuildSnapshot();
			CardTargetAffordances = BuildCardTargetAffordances(
				CardInstanceId,
				CurrentSnapshot,
				*CurrentSession);
		}
	}

	EWacomFirstPersonCardDragTargetFeedbackState FeedbackState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	TOptional<FVector2D> FeedbackTargetPosition;
	UWacomBattleEnemyPartPresentationComponent* PreviewPresentation = nullptr;

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
		PreviewPresentation = HUD.ResolveBattleEnemyPartWorldTargetPresentation(DropResult.TargetHandle);
	}

	if (CurrentDragPreviewPresentation.Get() != PreviewPresentation)
	{
		if (UWacomBattleEnemyPartPresentationComponent* PreviousPresentation =
			CurrentDragPreviewPresentation.Get())
		{
			PreviousPresentation->ClearDragTargetPreviewState();
		}
		CurrentDragPreviewPresentation = PreviewPresentation;
	}
	if (PreviewPresentation)
	{
		FWacomBattleEnemyPartDragPredictionDebugInput PredictionDebugInput;
		PredictionDebugInput.SourceCardInstanceId = CardInstanceId;
		PredictionDebugInput.bPreviewCanSubmit = DropResult.bCanSubmit;
		PredictionDebugInput.PreviewRejectReason = FName(WacomCardDropRejectReasonToString(DropResult.RejectReason));
		if (const UBattleSession* CurrentSession = HUD.GetSession())
		{
			const FBattleSnapshot CurrentSnapshot = CurrentSession->BuildSnapshot();
			if (const FHandCardSnapshot* SourceSnapshot =
				FindHandCardSnapshotForFirstPersonHandBridge(CurrentSnapshot, CardInstanceId))
			{
				PredictionDebugInput.bHasSourceCard = true;
				PredictionDebugInput.SourceCardRuntimeCost = SourceSnapshot->RuntimeCost;
				PredictionDebugInput.bSourceCardSwift = SourceSnapshot->bIsSwift;
			}
		}
		PreviewPresentation->SetDragTargetPreviewState(FeedbackState, PredictionDebugInput);
	}

	if (UWacomFirstPersonCardAnchorComponent* Anchor = ResolveActiveAnchor())
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

void FWacomBattleHUDFirstPersonHandBridge::ClearDragTargetFeedback(bool bClearFirstPersonCardLayerFeedback)
{
	if (UWacomBattleEnemyPartPresentationComponent* PreviewPresentation = CurrentDragPreviewPresentation.Get())
	{
		PreviewPresentation->ClearDragTargetPreviewState();
	}
	CurrentDragPreviewPresentation.Reset();
	if (!bClearFirstPersonCardLayerFeedback)
	{
		return;
	}

	if (UWacomFirstPersonCardAnchorComponent* Anchor = ResolveActiveAnchor())
	{
		Anchor->SetFirstPersonCardDragFeedbackTarget(
			FWacomInteractionTargetHandle(),
			false,
			EWacomFirstPersonCardDragTargetFeedbackState::None);
	}
}

FWacomBattleCardDropResolveResult FWacomBattleHUDFirstPersonHandBridge::ResolveDropIntent(
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

	const UBattleSession* CurrentSession = HUD.GetSession();
	if (!CurrentSession)
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::MissingSession;
		return Result;
	}

	if (!HUD.CanSubmitPlayerActionCommand())
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::UIBlocked;
		return Result;
	}

	const FBattleSnapshot CurrentSnapshot = CurrentSession->BuildSnapshot();
	const FHandCardSnapshot* CardSnapshot =
		FindHandCardSnapshotForFirstPersonHandBridge(CurrentSnapshot, CardInstanceId);
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
		bHasTarget = ProbeDragTarget(
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
		if (!HUD.ResolveBattleEnemyPartWorldTargetBridge(CandidateTarget))
		{
			Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
			Result.RejectReason = EWacomBattleCardDropRejectReason::InvalidWorldTarget;
			return Result;
		}

		const FWacomBattleTargetValidationResult Validation =
			CurrentSession->ValidateTargetWithCard(CardInstanceId, CandidateTarget);
		Result.TargetValidationRejectReason = Validation.RejectReason;
		Result.TargetValidationDebugSummary = Validation.DebugSummary;
		if (Validation.bCanTarget && Validation.ResolvedPartKey.IsValidKey())
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

TArray<FWacomFirstPersonCardTargetAffordance>
FWacomBattleHUDFirstPersonHandBridge::BuildCardTargetAffordances(
	const FGuid& SourceCardId,
	const FBattleSnapshot& Snapshot,
	const UBattleSession& BattleSession) const
{
	TArray<FWacomFirstPersonCardTargetAffordance> Affordances;
	if (!SourceCardId.IsValid())
	{
		return Affordances;
	}

	const FHandCardSnapshot* SourceSnapshot =
		FindHandCardSnapshotForFirstPersonHandBridge(Snapshot, SourceCardId);
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
			FWacomInteractionTargetHandle::ForCardTarget(TargetCard.InstanceId, &HUD);
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

bool FWacomBattleHUDFirstPersonHandBridge::ProbeDragTarget(
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

	const AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(HUD.GetOwningPlayer());
	const bool bProbed = DragView.bHasPointerViewportPosition
		? WacomPC && WacomPC->TryProbeBattleSceneInteractionTargetAtWidgetPosition(
			DragView.PointerViewportPosition,
			OutTargetHandle)
		: WacomPC && WacomPC->TryProbeBattleSceneInteractionTarget(OutTargetHandle);
	if (!bProbed)
	{
		return false;
	}

	const UBattleSession* CurrentSession = HUD.GetSession();
	bOutValidTarget = CurrentSession && CurrentSession->ValidateTargetWithCard(CardInstanceId, OutTargetHandle).bCanTarget;
	return OutTargetHandle.IsValid();
}

bool FWacomBattleHUDFirstPersonHandBridge::ShouldShowDragInspectDetail(
	const FWacomFirstPersonCardDragView& DragView) const
{
	const UWacomFirstPersonCardAnchorComponent* Anchor = LastAnchor.Get();
	if (!Anchor)
	{
		Anchor = ResolveAnchor();
	}
	if (!Anchor || !Anchor->bShowDetailDuringCardInspect)
	{
		return false;
	}

	return DragView.GestureState == EWacomFirstPersonCardGestureState::Inspecting;
}

void FWacomBattleHUDFirstPersonHandBridge::StoreTransitionEvents(const TArray<FBattleEvent>& Events)
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
			PendingTransitionEvents.Add(Event);
			break;
		default:
			break;
		}
	}
}

void FWacomBattleHUDFirstPersonHandBridge::ClearPendingTransitionEvents()
{
	PendingTransitionEvents.Reset();
	PendingPlayCommitHints.Reset();
}

void FWacomBattleHUDFirstPersonHandBridge::RecordPlayCommit(
	const FGuid& CardInstanceId,
	const FBattlePartSlotIdentity& TargetPartKey)
{
	if (!CardInstanceId.IsValid())
	{
		return;
	}

	FPlayCommitHint CommitHint;
	CommitHint.CardInstanceId = CardInstanceId;
	if (TargetPartKey.IsValidSlot())
	{
		FWacomBattlePresentationTargetCue Cue;
		Cue.CueKind = EWacomBattlePresentationTargetCueKind::TargetConfirmed;
		Cue.TargetPartKey = TargetPartKey;
		Cue.Duration = 0.10f;
		HUD.PlayBattlePresentationCue(Cue);
	}

	PendingPlayCommitHints.Add(CommitHint);
}

TArray<FWacomFirstPersonCardLayerTransitionHint> FWacomBattleHUDFirstPersonHandBridge::BuildTransitionHints(
	const FBattleSnapshot& PreviousSnapshot,
	const FBattleSnapshot& NextSnapshot) const
{
	TArray<FWacomFirstPersonCardLayerTransitionHint> Hints;
	if (PendingTransitionEvents.IsEmpty()
		&& PendingPlayCommitHints.IsEmpty())
	{
		return Hints;
	}

	TSet<FGuid> NewCardIds;
	for (const FHandCardSnapshot& CardSnapshot : NextSnapshot.Hand.Cards)
	{
		if (CardSnapshot.InstanceId.IsValid()
			&& !ContainsHandCardIdForFirstPersonHandBridge(PreviousSnapshot, CardSnapshot.InstanceId))
		{
			NewCardIds.Add(CardSnapshot.InstanceId);
		}
	}

	TSet<FGuid> RemovedCardIds;
	for (const FHandCardSnapshot& CardSnapshot : PreviousSnapshot.Hand.Cards)
	{
		if (CardSnapshot.InstanceId.IsValid()
			&& !ContainsHandCardIdForFirstPersonHandBridge(NextSnapshot, CardSnapshot.InstanceId))
		{
			RemovedCardIds.Add(CardSnapshot.InstanceId);
		}
	}

	auto FindCommitHint = [this](const FGuid& CardInstanceId) -> const FPlayCommitHint*
	{
		return PendingPlayCommitHints.FindByPredicate(
			[&CardInstanceId](const FPlayCommitHint& CommitHint)
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
			if (const FPlayCommitHint* CommitHint = FindCommitHint(CardInstanceId))
			{
				Hint.bPlayCommitFeedback = true;
			}
		}
		Hints.Add(Hint);
	};

	int32 DrawnCardHintBudget = 0;
	for (const FBattleEvent& Event : PendingTransitionEvents)
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

void FWacomBattleHUDFirstPersonHandBridge::ClearTransitionSnapshot()
{
	LastTransitionSnapshot = FBattleSnapshot();
	bHasTransitionSnapshot = false;
}

bool FWacomBattleHUDFirstPersonHandBridge::CanBuildTransitionHintsFor(
	const FBattleSnapshot& NextSnapshot) const
{
	return bHasTransitionSnapshot
		&& LastTransitionSnapshot.Phase != EBattlePhase::BattleEnd
		&& NextSnapshot.Phase != EBattlePhase::BattleEnd;
}

TArray<FWacomFirstPersonCardLayerTransitionHint>
FWacomBattleHUDFirstPersonHandBridge::BuildTransitionHintsForRefresh(
	const FBattleSnapshot& NextSnapshot) const
{
	return CanBuildTransitionHintsFor(NextSnapshot)
		? BuildTransitionHints(LastTransitionSnapshot, NextSnapshot)
		: TArray<FWacomFirstPersonCardLayerTransitionHint>();
}

void FWacomBattleHUDFirstPersonHandBridge::SetTransitionSnapshot(const FBattleSnapshot& Snapshot)
{
	LastTransitionSnapshot = Snapshot;
	bHasTransitionSnapshot = true;
}

const FHandCardSnapshot* FWacomBattleHUDFirstPersonHandBridge::FindLastBattleHandCardSnapshot(
	const FGuid& CardInstanceId) const
{
	if (!HUD.bHasLastBattleSnapshot || !CardInstanceId.IsValid())
	{
		return nullptr;
	}

	return FindHandCardSnapshotForFirstPersonHandBridge(HUD.LastBattleSnapshot, CardInstanceId);
}
