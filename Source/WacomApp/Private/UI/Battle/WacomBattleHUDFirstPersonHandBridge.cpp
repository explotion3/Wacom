// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDFirstPersonHandBridge.h"

#include "Cards/CardDefinition.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "Resolution/BattleCardTargetPreview.h"
#include "Resolution/BattleTargetValidationResult.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleCardPresentationHelper.h"
#include "UI/Battle/WacomBattleHUDCommandFlow.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/Card/WacomFirstPersonCardLayerSourceIds.h"

namespace
{
	const FName FirstPersonBattleHandLayerSourceId =
		WacomFirstPersonCardLayerSourceIds::BattleHand();
	constexpr float PendingHandAnchorEnterFrameTimeoutSeconds = 4.0f;

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
	const FBattleSnapshot& Snapshot)
{
	SyncLayerInternal(Snapshot, nullptr, nullptr);
}

void FWacomBattleHUDFirstPersonHandBridge::SyncLayer(
	const FBattleSnapshot& Snapshot,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints)
{
	SyncLayerInternal(Snapshot, &TransitionHints, nullptr);
}

void FWacomBattleHUDFirstPersonHandBridge::SyncLayer(
	const FBattleSnapshot& Snapshot,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints,
	const TArray<FWacomFirstPersonCardLayerFeedbackHint>& FeedbackHints)
{
	SyncLayerInternal(Snapshot, &TransitionHints, &FeedbackHints);
}

void FWacomBattleHUDFirstPersonHandBridge::SyncLayerInternal(
	const FBattleSnapshot& Snapshot,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>* TransitionHints,
	const TArray<FWacomFirstPersonCardLayerFeedbackHint>* FeedbackHints)
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
	Anchor->SetRuntimeCardLayerTransitionPresentationEnabled(
		FirstPersonBattleHandLayerSourceId,
		true);
	Anchor->SetFirstPersonCardLayerInteractionEnabled(ShouldEnableFirstPersonBattleHandInteraction());
	BindLayerInteractions(Anchor);
	LastAnchor = Anchor;
	if (TransitionHints)
	{
		const TArray<FWacomFirstPersonCardLayerFeedbackHint> EmptyFeedbackHints;
		ApplyPresentationFrame(
			*Anchor,
			PresentationController.BuildExplicitFrame(
				Snapshot,
				*TransitionHints,
				FeedbackHints ? *FeedbackHints : EmptyFeedbackHints));
	}
	else
	{
		RecomposeFirstPersonHandLayer(Snapshot);
	}
}

void FWacomBattleHUDFirstPersonHandBridge::ClearLayer(bool bClearPendingTransitionEvents)
{
	bFirstPersonBattleHandLayerRuntimeActive = false;
	bHasActiveTargetPreviewLayer = false;
	ResetActiveTargetPreviewState();

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
			Anchor->SetFirstPersonCardLayerInteractionEnabled(false);
			Anchor->CancelFirstPersonCardDragGesture(true);
			Anchor->SetRuntimeCardLayerTransitionPresentationEnabled(
				FirstPersonBattleHandLayerSourceId,
				true);
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
	bHasActiveDragView = false;
	ActiveDragCardInstanceId.Invalidate();
	ActiveDragView = FWacomFirstPersonCardDragView();
	bHasActiveCardTargetHandle = false;
	ActiveCardTargetHandle = FWacomInteractionTargetHandle();
	LastAnchor.Reset();
	HUD.ForceHideCardDetailHost(UBattleHUD::ECardDetailHost::FirstPersonViewport);
	if (bClearPendingTransitionEvents)
	{
		ClearPendingTransitionEvents();
		PresentationController.Reset();
	}
	PendingHandAnchorEnterFrameElapsedSeconds = 0.0f;
}

void FWacomBattleHUDFirstPersonHandBridge::SuppressLayerForEntry()
{
	if (UWacomFirstPersonCardAnchorComponent* ActiveAnchor = ResolveActiveAnchor())
	{
		if (!ActiveAnchor->HasRuntimeCardLayerPendingPresentationFrame(FirstPersonBattleHandLayerSourceId))
		{
			PresentationController.DiscardSubmittedTransitionFrame();
		}
	}
	PreservePendingEntryRevealForNextRefresh();
	ClearLayer(/*bClearPendingTransitionEvents*/ false);

	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveAnchor();
	if (!Anchor)
	{
		return;
	}

	Anchor->SetFirstPersonCardLayerInteractionEnabled(false);
	Anchor->SetRuntimeCardLayerTransitionPresentationEnabled(
		FirstPersonBattleHandLayerSourceId,
		false);
	Anchor->CancelFirstPersonCardDragGesture(true);
	Anchor->ClearCardLayerVisualState();
	TArray<FWacomFirstPersonCardLayerTransitionHint> EmptyHints;
	TArray<FWacomFirstPersonCardLayerFeedbackHint> EmptyFeedbackHints;
	TArray<FWacomFirstPersonCardLayerEntry> EmptyEntries;
	Anchor->SetRuntimeCardLayerTransitionHints(FirstPersonBattleHandLayerSourceId, EmptyHints);
	Anchor->SetRuntimeCardLayerFeedbackHints(FirstPersonBattleHandLayerSourceId, EmptyFeedbackHints);
	Anchor->SetRuntimeCardLayerEntries(FirstPersonBattleHandLayerSourceId, EmptyEntries);
	Anchor->SetFirstPersonCardLayerInteractionEnabled(false);
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
	Anchor->OnFirstPersonCardLayerCardTargetHovered.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerCardTargetUnhovered.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerHoveredCardTargetUpdated.RemoveAll(&HUD);
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
	Anchor->OnFirstPersonCardLayerCardTargetHovered.AddUObject(
		&HUD,
		&UBattleHUD::HandleFirstPersonCardLayerCardTargetHovered);
	Anchor->OnFirstPersonCardLayerCardTargetUnhovered.AddUObject(
		&HUD,
		&UBattleHUD::HandleFirstPersonCardLayerCardTargetUnhovered);
	Anchor->OnFirstPersonCardLayerHoveredCardTargetUpdated.AddUObject(
		&HUD,
		&UBattleHUD::HandleFirstPersonCardLayerHoveredCardTargetUpdated);
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
	Anchor->OnFirstPersonCardLayerCardTargetHovered.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerCardTargetUnhovered.RemoveAll(&HUD);
	Anchor->OnFirstPersonCardLayerHoveredCardTargetUpdated.RemoveAll(&HUD);
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
		WacomBattleCardPresentation::BuildCardDetailViewData(*CardSnapshot),
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

void FWacomBattleHUDFirstPersonHandBridge::HandleCardTargetHovered(
	const FWacomInteractionTargetHandle& CardTargetHandle,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (!ShouldEnableFirstPersonBattleHandInteraction()
		|| !CardTargetHandle.IsValid()
		|| CardTargetHandle.TargetKind != EWacomInteractionTargetKind::Card)
	{
		return;
	}

	ActiveCardTargetHandle = CardTargetHandle;
	bHasActiveCardTargetHandle = true;
	if (bHasActiveDragView)
	{
		ApplyActiveCardTargetPreview(CardTargetHandle, SlotView);
	}
}

void FWacomBattleHUDFirstPersonHandBridge::HandleCardTargetUnhovered(
	const FWacomInteractionTargetHandle& CardTargetHandle,
	const FWacomFirstPersonCardLayerSlotView& /*SlotView*/)
{
	if (!bHasActiveCardTargetHandle
		|| ActiveCardTargetHandle.TargetKind != EWacomInteractionTargetKind::Card
		|| ActiveCardTargetHandle.CardInstanceId != CardTargetHandle.CardInstanceId)
	{
		return;
	}

	if (IsActiveDragCardTargetHandle(CardTargetHandle))
	{
		return;
	}

	bHasActiveCardTargetHandle = false;
	ActiveCardTargetHandle = FWacomInteractionTargetHandle();
	if (bHasActiveDragView)
	{
		ActiveDragView.CurrentTarget = FWacomInteractionTargetHandle();
		ActiveDragView.bTargetValid = false;
		ActiveDragView.TargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
		ActiveDragView.bHasFeedbackTargetScreenPosition = false;
		ActiveDragView.FeedbackTargetScreenPosition = FVector2D::ZeroVector;
	}
	if (HUD.bHasLastBattleSnapshot)
	{
		RecomposeFirstPersonHandLayer(HUD.LastBattleSnapshot);
	}
	else
	{
		ClearDragTargetFeedback();
	}
	if (bHasActiveDragView && ActiveDragView.GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard)
	{
		HUD.ForceHideCardDetailHost(UBattleHUD::ECardDetailHost::FirstPersonViewport);
	}
}

void FWacomBattleHUDFirstPersonHandBridge::HandleHoveredCardTargetUpdated(
	const FWacomInteractionTargetHandle& CardTargetHandle,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (!ShouldEnableFirstPersonBattleHandInteraction()
		|| !CardTargetHandle.IsValid()
		|| CardTargetHandle.TargetKind != EWacomInteractionTargetKind::Card)
	{
		return;
	}

	ActiveCardTargetHandle = CardTargetHandle;
	bHasActiveCardTargetHandle = true;
	if (bHasActiveDragView)
	{
		ApplyActiveCardTargetPreview(CardTargetHandle, SlotView);
	}
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
	bHasActiveDragView = true;
	ActiveDragCardInstanceId = CardInstanceId;
	ActiveDragView = DragView;
	HUD.ClearBattleSceneEnemyPartHoverProbe(TEXT("FirstPersonDrag"));

	if (ShouldShowDragInspectDetail(DragView))
	{
		const FHandCardSnapshot* CardSnapshot = FindLastBattleHandCardSnapshot(CardInstanceId);
		if (CardSnapshot && CardSnapshot->Definition)
		{
			HUD.SetFirstPersonCardDetailSource(CardInstanceId);
			if (HUD.ShowFirstPersonCardDetailAtSlot(
				WacomBattleCardPresentation::BuildCardDetailViewData(*CardSnapshot),
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

	if (!bFirstPersonCardDragActiveForBattleSceneHover)
	{
		bFirstPersonCardDragActiveForBattleSceneHover = true;
		HUD.ClearBattleSceneEnemyPartHoverProbe(TEXT("FirstPersonDrag"));
	}

	bHasActiveDragView = true;
	ActiveDragCardInstanceId = CardInstanceId;
	ActiveDragView = DragView;
	if (DragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Card)
	{
		ActiveCardTargetHandle = DragView.CurrentTarget;
		bHasActiveCardTargetHandle = true;
	}
	else
	{
		ActiveCardTargetHandle = FWacomInteractionTargetHandle();
		bHasActiveCardTargetHandle = false;
	}
	ApplyDragCameraLookOverride(DragView);

	if (ShouldShowDragInspectDetail(DragView)
		&& HUD.IsCurrentFirstPersonCardDetailSource(CardInstanceId)
		&& DragView.SourceSlotView.bProjected)
	{
		HUD.UpdateFirstPersonCardDetailSlot(DragView.SourceSlotView);
		HUD.PositionFirstPersonCardDetailPanelBesideSlot(DragView.SourceSlotView);
		ClearTargetPreviewLayer();
		return;
	}
	else if (DragView.GestureState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
		|| DragView.GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit)
	{
		HUD.ForceHideCardDetailHost(UBattleHUD::ECardDetailHost::FirstPersonViewport);
		ClearTargetPreviewLayer();
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
	bHasActiveDragView = false;
	ActiveDragCardInstanceId.Invalidate();
	ActiveDragView = FWacomFirstPersonCardDragView();
	bHasActiveCardTargetHandle = false;
	ActiveCardTargetHandle = FWacomInteractionTargetHandle();
	HUD.ClearBattleSceneEnemyPartHoverProbe(TEXT("FirstPersonDragReleased"));
	if (UWacomBattleEnemyPartPresentationComponent* PreviewPresentation = CurrentDragPreviewPresentation.Get())
	{
		PreviewPresentation->ClearDragTargetPreviewState();
	}
	CurrentDragPreviewPresentation.Reset();
	ClearTargetPreviewLayer();
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
	bHasActiveDragView = false;
	ActiveDragCardInstanceId.Invalidate();
	ActiveDragView = FWacomFirstPersonCardDragView();
	bHasActiveCardTargetHandle = false;
	ActiveCardTargetHandle = FWacomInteractionTargetHandle();
	HUD.ClearBattleSceneEnemyPartHoverProbe(TEXT("FirstPersonDragCancelled"));
	ClearDragTargetFeedback();
	ClearTargetPreviewLayer();
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
	ApplyPointerCameraLookOverride(PointerView);
}

void FWacomBattleHUDFirstPersonHandBridge::HandlePointerLeft()
{
	ClearPointerCameraLookOverride();
}

void FWacomBattleHUDFirstPersonHandBridge::ApplyDragCameraLookOverride(
	const FWacomFirstPersonCardDragView& DragView)
{
	CameraLookBridge.ApplyDragView(
		DragView,
		[this](const FWacomFirstPersonCardDragView& AppliedDragView)
		{
			ApplyDragCameraLookOverrideToBattleCamera(AppliedDragView);
		});
}

void FWacomBattleHUDFirstPersonHandBridge::ClearDragCameraLookOverride()
{
	CameraLookBridge.ClearDragView(
		[this]()
		{
			ClearCameraLookOverrideOnBattleCamera();
		});
}

void FWacomBattleHUDFirstPersonHandBridge::ApplyPointerCameraLookOverride(
	const FWacomFirstPersonCardPointerView& PointerView)
{
	CameraLookBridge.HandlePointerMoved(
		PointerView,
		[this](const FWacomFirstPersonCardPointerView& AppliedPointerView)
		{
			ApplyPointerCameraLookOverrideToBattleCamera(AppliedPointerView);
		});
}

void FWacomBattleHUDFirstPersonHandBridge::ClearPointerCameraLookOverride()
{
	CameraLookBridge.HandlePointerLeft(
		[this]()
		{
			ClearCameraLookOverrideOnBattleCamera();
		});
}

void FWacomBattleHUDFirstPersonHandBridge::ApplyDragCameraLookOverrideToBattleCamera(
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

void FWacomBattleHUDFirstPersonHandBridge::ApplyPointerCameraLookOverrideToBattleCamera(
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

void FWacomBattleHUDFirstPersonHandBridge::ClearCameraLookOverrideOnBattleCamera()
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
	UpdateDragTargetFeedback(CardInstanceId, DragView, DropResult);
}

void FWacomBattleHUDFirstPersonHandBridge::UpdateDragTargetFeedback(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	const FWacomBattleCardDropResolveResult& DropResult,
	bool bForceApplyTargetPreview)
{
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
	FBattleSnapshot CurrentSnapshot;
	FBattleCardTargetPreview TargetPreview;
	FWacomBattleCardTargetPreviewPresentation TargetPreviewPresentation;
	bool bHasTargetPreview = false;

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

	if ((DropResult.IntentKind == EWacomBattleCardDropIntentKind::PlayCardWorldTarget
			|| DropResult.IntentKind == EWacomBattleCardDropIntentKind::PlayCardCardTarget)
		&& DropResult.bCanSubmit)
	{
		if (const UBattleSession* CurrentSession = HUD.GetSession())
		{
			CurrentSnapshot = CurrentSession->BuildSnapshot();
			TargetPreview = CurrentSession->BuildCardTargetPreview(CardInstanceId, DropResult.TargetHandle);
			bHasTargetPreview = TargetPreview.bHasPreview;
			if (bHasTargetPreview)
			{
				TargetPreviewPresentation =
					WacomBattleCardPresentation::BuildTargetPreviewPresentation(
						CurrentSnapshot,
						TargetPreview);
				bHasTargetPreview = TargetPreviewPresentation.bHasPreview;
			}
		}
	}

	if (bHasTargetPreview)
	{
		const bool bCanReuseActiveTargetPreview =
			!bForceApplyTargetPreview
			&& HUD.IsCurrentFirstPersonCardDetailSource(CardInstanceId)
			&& IsSameActiveTargetPreviewState(TargetPreviewPresentation);

		if (bCanReuseActiveTargetPreview)
		{
			if (DragView.SourceSlotView.bProjected)
			{
				HUD.UpdateFirstPersonCardDetailSlot(DragView.SourceSlotView);
				HUD.PositionFirstPersonCardDetailPanelBesideSlot(DragView.SourceSlotView);
			}
		}
		else
		{
			ApplyTargetPreviewPresentationToLayer(TargetPreviewPresentation);
			StoreActiveTargetPreviewState(TargetPreviewPresentation);
			if (TargetPreviewPresentation.bHasSourceCardDetailViewData)
			{
				HUD.SetFirstPersonCardDetailSource(CardInstanceId);
				if (DragView.SourceSlotView.bProjected)
				{
					HUD.ShowFirstPersonCardDetailAtSlot(
						TargetPreviewPresentation.SourceCardDetailViewData,
						DragView.SourceSlotView);
				}
			}
		}
	}
	else
	{
		ClearTargetPreviewLayer();
		if (DragView.GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard)
		{
			HUD.ForceHideCardDetailHost(UBattleHUD::ECardDetailHost::FirstPersonViewport);
		}
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
		if (bHasTargetPreview)
		{
			PredictionDebugInput.bHasSourceCard = true;
			PredictionDebugInput.SourceCardRuntimeCost = TargetPreview.SourceCardRuntimeCost;
			PredictionDebugInput.bSourceCardSwift = TargetPreview.bSourceCardSwift;
		}
		else if (const UBattleSession* CurrentSession = HUD.GetSession())
		{
			const FBattleSnapshot PredictionSnapshot = CurrentSession->BuildSnapshot();
			if (const FHandCardSnapshot* SourceSnapshot =
				FindHandCardSnapshotForFirstPersonHandBridge(PredictionSnapshot, CardInstanceId))
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

bool FWacomBattleHUDFirstPersonHandBridge::ApplyActiveCardTargetPreview(
	const FWacomInteractionTargetHandle& CardTargetHandle,
	const FWacomFirstPersonCardLayerSlotView& TargetSlotView)
{
	if (!ShouldEnableFirstPersonBattleHandInteraction()
		|| !bHasActiveDragView
		|| !ActiveDragCardInstanceId.IsValid()
		|| ActiveDragView.GestureState != EWacomFirstPersonCardGestureState::AimingTargetedCard
		|| !CardTargetHandle.IsValid()
		|| CardTargetHandle.TargetKind != EWacomInteractionTargetKind::Card)
	{
		return false;
	}

	FWacomFirstPersonCardDragView SemanticDragView =
		BuildSemanticActiveCardTargetDragView(CardTargetHandle, TargetSlotView);

	const FWacomBattleCardDropResolveResult DropResult =
		ResolveDropIntent(ActiveDragCardInstanceId, SemanticDragView);
	SemanticDragView.bTargetValid = DropResult.bCanSubmit;
	SemanticDragView.TargetFeedbackState = DropResult.bCanSubmit
		? EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
		: EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget;
	ActiveDragView = SemanticDragView;
	ActiveCardTargetHandle = CardTargetHandle;
	bHasActiveCardTargetHandle = DropResult.TargetHandle.TargetKind == EWacomInteractionTargetKind::Card
		&& DropResult.TargetHandle.CardInstanceId == CardTargetHandle.CardInstanceId;
	UpdateDragTargetFeedback(ActiveDragCardInstanceId, SemanticDragView, DropResult);
	return DropResult.bCanSubmit;
}

void FWacomBattleHUDFirstPersonHandBridge::ClearDragTargetFeedback(bool bClearFirstPersonCardLayerFeedback)
{
	if (UWacomBattleEnemyPartPresentationComponent* PreviewPresentation = CurrentDragPreviewPresentation.Get())
	{
		PreviewPresentation->ClearDragTargetPreviewState();
	}
	CurrentDragPreviewPresentation.Reset();
	ClearTargetPreviewLayer();
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

void FWacomBattleHUDFirstPersonHandBridge::RecomposeFirstPersonHandLayer(const FBattleSnapshot& Snapshot)
{
	if (!bFirstPersonBattleHandLayerRuntimeActive)
	{
		return;
	}

	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveActiveAnchor();
	if (!Anchor)
	{
		return;
	}

	if (bHasActiveDragView
		&& ActiveDragCardInstanceId.IsValid()
		&& ActiveDragView.CardInstanceId == ActiveDragCardInstanceId)
	{
		const FWacomBattleCardDropResolveResult DropResult =
			ResolveDropIntent(ActiveDragCardInstanceId, ActiveDragView);
		UpdateDragTargetFeedback(
			ActiveDragCardInstanceId,
			ActiveDragView,
			DropResult,
			/*bForceApplyTargetPreview*/ true);
		return;
	}

	FWacomFirstPersonCardLayerPresentationFrame Frame =
		PresentationController.BuildFrame(
			Snapshot,
			HUD.IsFirstPersonBattleHandSuppressedForEntry());
	ApplyPresentationFrame(*Anchor, MoveTemp(Frame));
	bHasActiveTargetPreviewLayer = false;
}

void FWacomBattleHUDFirstPersonHandBridge::ApplyPresentationFrame(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	FWacomFirstPersonCardLayerPresentationFrame&& Frame)
{
	Frame.SourceId = FirstPersonBattleHandLayerSourceId;
	ApplyPendingTargetingFlag(Frame.Entries);
	if (Frame.ShouldApplyAsPresentationFrame())
	{
		Anchor.SetRuntimeCardLayerPresentationFrame(Frame);
	}
	else
	{
		Anchor.SetRuntimeCardLayerEntries(
			Frame.SourceId,
			Frame.Entries);
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

bool FWacomBattleHUDFirstPersonHandBridge::IsActiveDragCardTargetHandle(
	const FWacomInteractionTargetHandle& CardTargetHandle) const
{
	return bHasActiveDragView
		&& ActiveDragCardInstanceId.IsValid()
		&& ActiveDragView.CardInstanceId == ActiveDragCardInstanceId
		&& ActiveDragView.GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard
		&& ActiveDragView.CurrentTarget.IsValid()
		&& ActiveDragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Card
		&& CardTargetHandle.IsValid()
		&& CardTargetHandle.TargetKind == EWacomInteractionTargetKind::Card
		&& ActiveDragView.CurrentTarget.CardInstanceId == CardTargetHandle.CardInstanceId;
}

FWacomFirstPersonCardDragView FWacomBattleHUDFirstPersonHandBridge::BuildSemanticActiveCardTargetDragView(
	const FWacomInteractionTargetHandle& CardTargetHandle,
	const FWacomFirstPersonCardLayerSlotView& TargetSlotView) const
{
	FWacomFirstPersonCardDragView SemanticDragView = ActiveDragView;
	SemanticDragView.CurrentTarget = CardTargetHandle;
	SemanticDragView.CurrentTarget.ScreenPosition = TargetSlotView.ScreenPosition;
	SemanticDragView.bTargetValid = true;
	SemanticDragView.TargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget;
	SemanticDragView.bHasFeedbackTargetScreenPosition = TargetSlotView.bProjected;
	SemanticDragView.FeedbackTargetScreenPosition = TargetSlotView.ScreenPosition;
	return SemanticDragView;
}

void FWacomBattleHUDFirstPersonHandBridge::ApplyTargetPreviewPresentationToLayer(
	const FWacomBattleCardTargetPreviewPresentation& TargetPreviewPresentation)
{
	if (!bFirstPersonBattleHandLayerRuntimeActive || !TargetPreviewPresentation.bHasPreview)
	{
		RestoreBaseTargetPreviewLayer();
		return;
	}

	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveActiveAnchor();
	if (!Anchor)
	{
		bHasActiveTargetPreviewLayer = false;
		return;
	}

	TArray<FWacomFirstPersonCardLayerEntry> CardEntries =
		TargetPreviewPresentation.CardLayerEntries;
	ApplyPendingTargetingFlag(CardEntries);
	Anchor->SetRuntimeCardLayerEntries(FirstPersonBattleHandLayerSourceId, CardEntries);
	bHasActiveTargetPreviewLayer = true;
}

bool FWacomBattleHUDFirstPersonHandBridge::IsSameActiveTargetPreviewState(
	const FWacomBattleCardTargetPreviewPresentation& TargetPreviewPresentation) const
{
	if (!bHasActiveTargetPreviewState || !TargetPreviewPresentation.bHasPreview)
	{
		return false;
	}

	return WacomBattleCardPresentation::AreTargetPreviewStateKeysEquivalent(
		ActiveTargetPreviewState,
		TargetPreviewPresentation.StateKey);
}

void FWacomBattleHUDFirstPersonHandBridge::StoreActiveTargetPreviewState(
	const FWacomBattleCardTargetPreviewPresentation& TargetPreviewPresentation)
{
	if (!TargetPreviewPresentation.bHasPreview)
	{
		ResetActiveTargetPreviewState();
		return;
	}

	ActiveTargetPreviewState = TargetPreviewPresentation.StateKey;
	bHasActiveTargetPreviewState = true;
}

void FWacomBattleHUDFirstPersonHandBridge::ResetActiveTargetPreviewState()
{
	ActiveTargetPreviewState = FWacomBattleCardTargetPreviewPresentationStateKey();
	bHasActiveTargetPreviewState = false;
}

void FWacomBattleHUDFirstPersonHandBridge::ClearTargetPreviewLayer()
{
	ResetActiveTargetPreviewState();
	RestoreBaseTargetPreviewLayer();
}

void FWacomBattleHUDFirstPersonHandBridge::RestoreBaseTargetPreviewLayer()
{
	if (!bHasActiveTargetPreviewLayer)
	{
		return;
	}

	bHasActiveTargetPreviewLayer = false;
	if (!bFirstPersonBattleHandLayerRuntimeActive || !HUD.bHasLastBattleSnapshot)
	{
		return;
	}

	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveActiveAnchor();
	if (!Anchor)
	{
		return;
	}

	TArray<FWacomFirstPersonCardLayerEntry> CardEntries =
		WacomBattleCardPresentation::BuildCardLayerEntries(HUD.LastBattleSnapshot);
	ApplyPendingTargetingFlag(CardEntries);
	Anchor->SetRuntimeCardLayerEntries(FirstPersonBattleHandLayerSourceId, CardEntries);
}

void FWacomBattleHUDFirstPersonHandBridge::ApplyPendingTargetingFlag(
	TArray<FWacomFirstPersonCardLayerEntry>& Entries) const
{
	const bool bHasPendingTargetingCard =
		HUD.UIState == EBattleUIState::TargetSelect && HUD.PendingTargetingCardId.IsValid();
	for (FWacomFirstPersonCardLayerEntry& Entry : Entries)
	{
		Entry.bIsPendingTargeting = bHasPendingTargetingCard
			&& Entry.CardInstanceId == HUD.PendingTargetingCardId;
	}
}

void FWacomBattleHUDFirstPersonHandBridge::StoreTransitionEvents(const TArray<FBattleEvent>& Events)
{
	PresentationController.StoreTransitionEvents(Events);
}

bool FWacomBattleHUDFirstPersonHandBridge::HasPendingTransitionPresentation() const
{
	return PresentationController.HasPendingTransitionPresentation();
}

void FWacomBattleHUDFirstPersonHandBridge::ClearPendingTransitionEvents()
{
	PresentationController.ClearPendingTransitionEvents();
	PendingHandAnchorEnterFrameElapsedSeconds = 0.0f;
}

void FWacomBattleHUDFirstPersonHandBridge::PreservePendingEntryRevealForNextRefresh()
{
	PresentationController.PreservePendingEntryRevealForNextRefresh();
}

bool FWacomBattleHUDFirstPersonHandBridge::HasPendingPresentationFrame() const
{
	const UWacomFirstPersonCardAnchorComponent* ActiveAnchor = ResolveActiveAnchor();
	return (ActiveAnchor
			&& ActiveAnchor->HasRuntimeCardLayerPendingPresentationFrame(FirstPersonBattleHandLayerSourceId))
		|| PresentationController.HasPendingHandAnchorEnterFrame();
}

void FWacomBattleHUDFirstPersonHandBridge::TickPendingPresentationFrames(float DeltaTime)
{
	if (!PresentationController.HasPendingHandAnchorEnterFrame())
	{
		PendingHandAnchorEnterFrameElapsedSeconds = 0.0f;
		return;
	}

	UWacomFirstPersonCardAnchorComponent* ActiveAnchor = ResolveActiveAnchor();
	if (!ActiveAnchor
		|| !bFirstPersonBattleHandLayerRuntimeActive
		|| HUD.IsFirstPersonBattleHandSuppressedForEntry())
	{
		return;
	}

	if (ActiveAnchor->HasRuntimeCardLayerPendingPresentationFrame(FirstPersonBattleHandLayerSourceId))
	{
		return;
	}

	if (ActiveAnchor->HasActiveCardLayerPresentationPlayback())
	{
		PendingHandAnchorEnterFrameElapsedSeconds += FMath::Max(0.0f, DeltaTime);
		if (PendingHandAnchorEnterFrameElapsedSeconds < PendingHandAnchorEnterFrameTimeoutSeconds)
		{
			return;
		}
	}

	ApplyPresentationFrame(
		*ActiveAnchor,
		PresentationController.ConsumePendingHandAnchorEnterFrame());
	PendingHandAnchorEnterFrameElapsedSeconds = 0.0f;
	ActiveAnchor->RefreshCardLayerNow(0.0f);
}

void FWacomBattleHUDFirstPersonHandBridge::RecordPlayCommit(
	const FGuid& CardInstanceId,
	const FBattlePartSlotIdentity& TargetPartKey)
{
	if (!CardInstanceId.IsValid())
	{
		return;
	}

	if (TargetPartKey.IsValidSlot())
	{
		FWacomBattlePresentationTargetCue Cue;
		Cue.CueKind = EWacomBattlePresentationTargetCueKind::TargetConfirmed;
		Cue.TargetPartKey = TargetPartKey;
		Cue.Duration = 0.10f;
		HUD.PlayBattlePresentationCue(Cue);
	}

	PresentationController.RecordPlayCommit(CardInstanceId);
}

TArray<FWacomFirstPersonCardLayerTransitionHint> FWacomBattleHUDFirstPersonHandBridge::BuildTransitionHints(
	const FBattleSnapshot& PreviousSnapshot,
	const FBattleSnapshot& NextSnapshot) const
{
	return PresentationController.BuildTransitionHints(PreviousSnapshot, NextSnapshot);
}

TArray<FWacomFirstPersonCardLayerFeedbackHint> FWacomBattleHUDFirstPersonHandBridge::BuildFeedbackHints(
	const FBattleSnapshot& NextSnapshot) const
{
	return PresentationController.BuildFeedbackHints(NextSnapshot);
}

void FWacomBattleHUDFirstPersonHandBridge::ClearTransitionSnapshot()
{
	PresentationController.ClearTransitionSnapshot();
}

bool FWacomBattleHUDFirstPersonHandBridge::CanBuildTransitionHintsFor(
	const FBattleSnapshot& NextSnapshot) const
{
	return PresentationController.BuildTransitionHintsForRefresh(NextSnapshot).Num() > 0;
}

TArray<FWacomFirstPersonCardLayerTransitionHint>
FWacomBattleHUDFirstPersonHandBridge::BuildTransitionHintsForRefresh(
	const FBattleSnapshot& NextSnapshot) const
{
	return PresentationController.BuildTransitionHintsForRefresh(NextSnapshot);
}

void FWacomBattleHUDFirstPersonHandBridge::SetTransitionSnapshot(const FBattleSnapshot& Snapshot)
{
	PresentationController.SetTransitionSnapshot(Snapshot);
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
