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
#include "UI/Battle/WacomBattleCardPresentationHelper.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"
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

	bool IsCardTargetKindUnsupportedForSource(
		const FWacomBattleTargetValidationResult& Validation)
	{
		return Validation.RejectReason == EWacomBattleTargetRejectReason::UnsupportedCardTarget;
	}

}

FWacomBattleHUDFirstPersonHandBridge::FWacomBattleHUDFirstPersonHandBridge(FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
{
}

FWacomBattleHUDFirstPersonHandBridge::~FWacomBattleHUDFirstPersonHandBridge()
{
	ClearLayer();
}

UWacomFirstPersonCardAnchorComponent* FWacomBattleHUDFirstPersonHandBridge::ResolveAnchor() const
{
	const APlayerController* PC = Runtime.GetOwningPlayer();
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
	if (Runtime.IsFirstPersonBattleHandSuppressedForEntry())
	{
		SuppressLayerForEntry();
		return;
	}

	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveAnchor();
	const bool bCanShowBattleHand =
		ShouldUseFirstPersonBattleHandLayer()
		&& Runtime.GetSession()
		&& Snapshot.Phase != EBattlePhase::BattleEnd
		&& Runtime.GetUIState() != EBattleUIState::BattleEnd
		&& Anchor;
	if (!bCanShowBattleHand)
	{
		ClearLayer();
		return;
	}

	bFirstPersonBattleHandLayerRuntimeActive = true;
	FWacomFirstPersonCardLayerSourceLifecycleFrame LifecycleFrame;
	LifecycleFrame.SourceId = FirstPersonBattleHandLayerSourceId;
	LifecycleFrame.bSetTransitionPresentationEnabled = true;
	LifecycleFrame.bTransitionPresentationEnabled = true;
	LifecycleFrame.bSetInteractionEnabled = true;
	LifecycleFrame.bInteractionEnabled = ShouldEnableFirstPersonBattleHandInteraction();
	Anchor->ApplyRuntimeCardLayerSourceLifecycleFrame(LifecycleFrame);
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
			FWacomFirstPersonCardLayerSourceLifecycleFrame LifecycleFrame;
			LifecycleFrame.SourceId = FirstPersonBattleHandLayerSourceId;
			LifecycleFrame.bSetInteractionEnabled = true;
			LifecycleFrame.bInteractionEnabled = false;
			LifecycleFrame.bCancelActiveDrag = true;
			LifecycleFrame.bSetTransitionPresentationEnabled = true;
			LifecycleFrame.bTransitionPresentationEnabled = true;
			LifecycleFrame.ClearMode =
				EWacomFirstPersonCardLayerSourceClearMode::RuntimeData;
			Anchor->ApplyRuntimeCardLayerSourceLifecycleFrame(LifecycleFrame);
		}

		UnbindLayerInteractions(Anchor);
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
	Runtime.ForceHideCardDetailHost(EWacomBattleHUDCardDetailHost::FirstPersonViewport);
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

	FWacomFirstPersonCardLayerPresentationFrame SuppressedFrame;
	SuppressedFrame.SourceId = FirstPersonBattleHandLayerSourceId;
	SuppressedFrame.CommitMode = EWacomFirstPersonCardLayerFrameCommitMode::Suppressed;
	FWacomFirstPersonCardLayerSourceLifecycleFrame LifecycleFrame =
		FWacomFirstPersonCardLayerSourceLifecycleFrame::FromPresentationFrame(
			SuppressedFrame);
	LifecycleFrame.bSetInteractionEnabled = true;
	LifecycleFrame.bInteractionEnabled = false;
	LifecycleFrame.bSetTransitionPresentationEnabled = true;
	LifecycleFrame.bTransitionPresentationEnabled = false;
	LifecycleFrame.bCancelActiveDrag = true;
	LifecycleFrame.ClearMode = EWacomFirstPersonCardLayerSourceClearMode::VisualState;
	Anchor->ApplyRuntimeCardLayerSourceLifecycleFrame(LifecycleFrame);
	LastAnchor = Anchor;
}

bool FWacomBattleHUDFirstPersonHandBridge::ShouldUseFirstPersonBattleHandLayer() const
{
	return !Runtime.IsFirstPersonBattleHandSuppressedForEntry();
}

bool FWacomBattleHUDFirstPersonHandBridge::ShouldEnableFirstPersonBattleHandInteraction() const
{
	return ShouldUseFirstPersonBattleHandLayer() && Runtime.IsBattleInputReady();
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

	Runtime.Host().BindFirstPersonCardLayerInteractions(*Anchor);
	LastAnchor = Anchor;
}

void FWacomBattleHUDFirstPersonHandBridge::UnbindLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor)
{
	if (!Anchor)
	{
		return;
	}

	Runtime.Host().UnbindFirstPersonCardLayerInteractions(*Anchor);
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
		|| Runtime.GetUIState() != EBattleUIState::Idle
		|| !CardInstanceId.IsValid()
		|| !SlotView.bProjected)
	{
		Runtime.ForceHideCardDetailHost(EWacomBattleHUDCardDetailHost::FirstPersonViewport);
		return;
	}

	const FHandCardSnapshot* CardSnapshot = FindLastBattleHandCardSnapshot(CardInstanceId);
	if (!CardSnapshot || !CardSnapshot->Definition)
	{
		Runtime.ForceHideCardDetailHost(EWacomBattleHUDCardDetailHost::FirstPersonViewport);
		return;
	}

	Runtime.SetFirstPersonCardDetailSource(CardInstanceId);
	if (Runtime.ShowFirstPersonCardDetailAtSlot(
		WacomBattleCardPresentation::BuildCardDetailViewData(*CardSnapshot),
		SlotView))
	{
	}
	else
	{
		Runtime.ClearFirstPersonCardDetailSource();
	}
}

void FWacomBattleHUDFirstPersonHandBridge::HandleCardUnhovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& /*SlotView*/)
{
	if (Runtime.IsFirstPersonCardInspectDetailActiveForSource(CardInstanceId))
	{
		return;
	}
	Runtime.HideFirstPersonCardDetailPanelForSource(CardInstanceId);
}

void FWacomBattleHUDFirstPersonHandBridge::HandleHoveredCardLayoutUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (!ShouldEnableFirstPersonBattleHandInteraction()
		|| Runtime.GetUIState() != EBattleUIState::Idle
		|| !CardInstanceId.IsValid()
		|| !Runtime.IsCurrentFirstPersonCardDetailSource(CardInstanceId)
		|| !SlotView.bProjected)
	{
		return;
	}

	Runtime.UpdateFirstPersonCardDetailSlot(SlotView);
	Runtime.PositionFirstPersonCardDetailPanelBesideSlot(SlotView);
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
	if (Runtime.HasLastBattleSnapshot())
	{
		RecomposeFirstPersonHandLayer(Runtime.GetLastBattleSnapshot());
	}
	else
	{
		ClearDragTargetFeedback();
	}
	if (bHasActiveDragView && ActiveDragView.GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard)
	{
		Runtime.ForceHideCardDetailHost(EWacomBattleHUDCardDetailHost::FirstPersonViewport);
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
	Runtime.ClearBattleSceneEnemyPartHoverProbe(TEXT("FirstPersonDrag"));

	if (ShouldShowDragInspectDetail(DragView))
	{
		const FHandCardSnapshot* CardSnapshot = FindLastBattleHandCardSnapshot(CardInstanceId);
		if (CardSnapshot && CardSnapshot->Definition)
		{
			Runtime.SetFirstPersonCardDetailSource(CardInstanceId);
			if (Runtime.ShowFirstPersonCardDetailAtSlot(
				WacomBattleCardPresentation::BuildCardDetailViewData(*CardSnapshot),
				DragView.SourceSlotView))
			{
			}
		}
	}
	else
	{
		Runtime.ForceHideCardDetailHost(EWacomBattleHUDCardDetailHost::FirstPersonViewport);
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
	Runtime.ClearBattleSceneEnemyPartHoverProbe(TEXT("FirstPersonDrag"));
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
		&& Runtime.IsCurrentFirstPersonCardDetailSource(CardInstanceId)
		&& DragView.SourceSlotView.bProjected)
	{
		Runtime.UpdateFirstPersonCardDetailSlot(DragView.SourceSlotView);
		Runtime.PositionFirstPersonCardDetailPanelBesideSlot(DragView.SourceSlotView);
		ClearTargetPreviewLayer();
		return;
	}
	else if (DragView.GestureState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
		|| DragView.GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit)
	{
		Runtime.ForceHideCardDetailHost(EWacomBattleHUDCardDetailHost::FirstPersonViewport);
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
	Runtime.ClearBattleSceneEnemyPartHoverProbe(TEXT("FirstPersonDragReleased"));
	if (UWacomBattleEnemyPartPresentationComponent* PreviewPresentation = CurrentDragPreviewPresentation.Get())
	{
		PreviewPresentation->ClearDragTargetPreviewState();
	}
	CurrentDragPreviewPresentation.Reset();
	ClearTargetPreviewLayer();
	Runtime.ForceHideCardDetailHost(EWacomBattleHUDCardDetailHost::FirstPersonViewport);

	if (!DropResult.bCanSubmit)
	{
		return;
	}

	switch (DropResult.IntentKind)
	{
	case EWacomBattleCardDropIntentKind::PlayCardNoTarget:
		Runtime.SubmitPlayCard(CardInstanceId, FGuid());
		return;

	case EWacomBattleCardDropIntentKind::PlayCardWorldTarget:
		Runtime.SubmitPlayCardOnWorldTarget(CardInstanceId, DropResult.TargetHandle);
		return;

	case EWacomBattleCardDropIntentKind::PlayCardCardTarget:
		Runtime.SubmitPlayCardOnHandCard(CardInstanceId, DropResult.TargetHandle.CardInstanceId);
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
	Runtime.ClearBattleSceneEnemyPartHoverProbe(TEXT("FirstPersonDragCancelled"));
	ClearDragTargetFeedback();
	ClearTargetPreviewLayer();
	Runtime.HideFirstPersonCardDetailPanelForSource(CardInstanceId);
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
	if (const APlayerController* PC = Runtime.GetOwningPlayer())
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
	if (const APlayerController* PC = Runtime.GetOwningPlayer())
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
	const APlayerController* PC = Runtime.GetOwningPlayer();
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
		if (const UBattleSession* CurrentSession = Runtime.GetSession())
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
		PreviewPresentation = Runtime.ResolveBattleEnemyPartWorldTargetPresentation(DropResult.TargetHandle);
	}

	if ((DropResult.IntentKind == EWacomBattleCardDropIntentKind::PlayCardWorldTarget
			|| DropResult.IntentKind == EWacomBattleCardDropIntentKind::PlayCardCardTarget)
		&& DropResult.bCanSubmit)
	{
		if (const UBattleSession* CurrentSession = Runtime.GetSession())
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
			&& Runtime.IsCurrentFirstPersonCardDetailSource(CardInstanceId)
			&& IsSameActiveTargetPreviewState(TargetPreviewPresentation);

		if (bCanReuseActiveTargetPreview)
		{
			if (DragView.SourceSlotView.bProjected)
			{
				Runtime.UpdateFirstPersonCardDetailSlot(DragView.SourceSlotView);
				Runtime.PositionFirstPersonCardDetailPanelBesideSlot(DragView.SourceSlotView);
			}
		}
		else
		{
			ApplyTargetPreviewPresentationToLayer(TargetPreviewPresentation);
			StoreActiveTargetPreviewState(TargetPreviewPresentation);
			if (TargetPreviewPresentation.bHasSourceCardDetailViewData)
			{
				Runtime.SetFirstPersonCardDetailSource(CardInstanceId);
				if (DragView.SourceSlotView.bProjected)
				{
					Runtime.ShowFirstPersonCardDetailAtSlot(
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
			Runtime.ForceHideCardDetailHost(EWacomBattleHUDCardDetailHost::FirstPersonViewport);
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
		else if (const UBattleSession* CurrentSession = Runtime.GetSession())
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
			Runtime.IsFirstPersonBattleHandSuppressedForEntry());
	ApplyPresentationFrame(*Anchor, MoveTemp(Frame));
	bHasActiveTargetPreviewLayer = false;
}

void FWacomBattleHUDFirstPersonHandBridge::ApplyPresentationFrame(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	FWacomFirstPersonCardLayerPresentationFrame&& Frame)
{
	Frame.SourceId = FirstPersonBattleHandLayerSourceId;
	ApplyPendingTargetingFlag(Frame.Entries);
	if (Frame.CommitMode == EWacomFirstPersonCardLayerFrameCommitMode::StateRefresh
		&& Frame.ShouldApplyAsPresentationFrame())
	{
		Frame.CommitMode = EWacomFirstPersonCardLayerFrameCommitMode::PresentationFrame;
	}
	Anchor.ApplyRuntimeCardLayerSourceLifecycleFrame(
		FWacomFirstPersonCardLayerSourceLifecycleFrame::FromPresentationFrame(Frame));
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

	const UBattleSession* CurrentSession = Runtime.GetSession();
	if (!CurrentSession)
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::MissingSession;
		return Result;
	}

	if (!Runtime.CanSubmitPlayerActionCommand())
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
		if (!Runtime.ResolveBattleEnemyPartWorldTargetBridge(CandidateTarget))
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

		if (!IsCardTargetKindUnsupportedForSource(Validation))
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
	if (!SourceSnapshot || !SourceSnapshot->Definition)
	{
		return Affordances;
	}

	Affordances.Reserve(FMath::Max(0, Snapshot.Hand.Cards.Num() - 1));
	bool bSourceSupportsCardTargets = false;
	for (const FHandCardSnapshot& TargetCard : Snapshot.Hand.Cards)
	{
		if (!TargetCard.InstanceId.IsValid() || TargetCard.InstanceId == SourceCardId)
		{
			continue;
		}

		FWacomInteractionTargetHandle TargetHandle =
			FWacomInteractionTargetHandle::ForCardTarget(TargetCard.InstanceId, Runtime.Host().AsObject());
		const FWacomBattleTargetValidationResult Validation =
			BattleSession.ValidateTargetWithCard(SourceCardId, TargetHandle);
		bSourceSupportsCardTargets |=
			Validation.bCanTarget || !IsCardTargetKindUnsupportedForSource(Validation);

		FWacomFirstPersonCardTargetAffordance Affordance;
		Affordance.CardInstanceId = TargetCard.InstanceId;
		Affordance.bCanSubmit = Validation.bCanTarget;
		Affordance.FeedbackState = Validation.bCanTarget
			? EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
			: EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget;
		Affordance.DebugSummary = Validation.DebugSummary;
		Affordances.Add(MoveTemp(Affordance));
	}
	if (!bSourceSupportsCardTargets)
	{
		Affordances.Reset();
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

	const AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(Runtime.GetOwningPlayer());
	const bool bProbed = DragView.bHasPointerViewportPosition
		? WacomPC && WacomPC->TryProbeBattleSceneInteractionTargetAtWidgetPosition(
			DragView.PointerViewportPosition,
			OutTargetHandle)
		: WacomPC && WacomPC->TryProbeBattleSceneInteractionTarget(OutTargetHandle);
	if (!bProbed)
	{
		return false;
	}

	const UBattleSession* CurrentSession = Runtime.GetSession();
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
	FWacomFirstPersonCardLayerPresentationFrame PreviewFrame;
	PreviewFrame.SourceId = FirstPersonBattleHandLayerSourceId;
	PreviewFrame.Entries = MoveTemp(CardEntries);
	PreviewFrame.CommitMode = EWacomFirstPersonCardLayerFrameCommitMode::PreviewOverlay;
	Anchor->ApplyRuntimeCardLayerSourceLifecycleFrame(
		FWacomFirstPersonCardLayerSourceLifecycleFrame::FromPresentationFrame(
			PreviewFrame));
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
	if (!bFirstPersonBattleHandLayerRuntimeActive || !Runtime.HasLastBattleSnapshot())
	{
		return;
	}

	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveActiveAnchor();
	if (!Anchor)
	{
		return;
	}

	TArray<FWacomFirstPersonCardLayerEntry> CardEntries =
		WacomBattleCardPresentation::BuildCardLayerEntries(Runtime.GetLastBattleSnapshot());
	ApplyPendingTargetingFlag(CardEntries);
	FWacomFirstPersonCardLayerPresentationFrame RestoreFrame;
	RestoreFrame.SourceId = FirstPersonBattleHandLayerSourceId;
	RestoreFrame.Entries = MoveTemp(CardEntries);
	RestoreFrame.CommitMode = EWacomFirstPersonCardLayerFrameCommitMode::StateRefresh;
	Anchor->ApplyRuntimeCardLayerSourceLifecycleFrame(
		FWacomFirstPersonCardLayerSourceLifecycleFrame::FromPresentationFrame(
			RestoreFrame));
}

void FWacomBattleHUDFirstPersonHandBridge::ApplyPendingTargetingFlag(
	TArray<FWacomFirstPersonCardLayerEntry>& Entries) const
{
	const bool bHasPendingTargetingCard =
		Runtime.GetUIState() == EBattleUIState::TargetSelect && Runtime.GetPendingTargetingCardId().IsValid();
	for (FWacomFirstPersonCardLayerEntry& Entry : Entries)
	{
		Entry.bIsPendingTargeting = bHasPendingTargetingCard
			&& Entry.CardInstanceId == Runtime.GetPendingTargetingCardId();
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
		|| Runtime.IsFirstPersonBattleHandSuppressedForEntry())
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
		Runtime.PlayBattlePresentationCue(Cue);
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
	if (!Runtime.HasLastBattleSnapshot() || !CardInstanceId.IsValid())
	{
		return nullptr;
	}

	return FindHandCardSnapshotForFirstPersonHandBridge(Runtime.GetLastBattleSnapshot(), CardInstanceId);
}
