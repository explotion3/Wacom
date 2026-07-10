// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDSceneEnemyTargetCoordinator.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/WacomPlayerController.h"
#include "Resolution/BattleCardActionPreview.h"
#include "Resolution/BattleCardTargetPreview.h"
#include "Resolution/BattleTargetValidationResult.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleCardPresentationHelper.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleHUDFirstPersonHandBridge.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"

namespace
{
	const TCHAR* LexToStringForSceneEnemyTarget(EWacomBattleTargetRejectReason RejectReason)
	{
		switch (RejectReason)
		{
		case EWacomBattleTargetRejectReason::None: return TEXT("None");
		case EWacomBattleTargetRejectReason::InvalidTarget: return TEXT("InvalidTarget");
		case EWacomBattleTargetRejectReason::SourceCardInvalid: return TEXT("SourceCardInvalid");
		case EWacomBattleTargetRejectReason::SourceCardNotInHand: return TEXT("SourceCardNotInHand");
		case EWacomBattleTargetRejectReason::SourceCardMissingDefinition: return TEXT("SourceCardMissingDefinition");
		case EWacomBattleTargetRejectReason::SourceCardFrozen: return TEXT("SourceCardFrozen");
		case EWacomBattleTargetRejectReason::UnsupportedWorldTarget: return TEXT("UnsupportedWorldTarget");
		case EWacomBattleTargetRejectReason::InvalidWorldTarget: return TEXT("InvalidWorldTarget");
		case EWacomBattleTargetRejectReason::UnsupportedCardTarget: return TEXT("UnsupportedCardTarget");
		case EWacomBattleTargetRejectReason::TargetCardInvalid: return TEXT("TargetCardInvalid");
		case EWacomBattleTargetRejectReason::TargetCardNotInHand: return TEXT("TargetCardNotInHand");
		case EWacomBattleTargetRejectReason::SelfTarget: return TEXT("SelfTarget");
		case EWacomBattleTargetRejectReason::UnsupportedNormalHandCardTarget: return TEXT("UnsupportedNormalHandCardTarget");
		case EWacomBattleTargetRejectReason::UnsupportedHandAnchorTarget: return TEXT("UnsupportedHandAnchorTarget");
		case EWacomBattleTargetRejectReason::MissingRequiredTargetKeyword: return TEXT("MissingRequiredTargetKeyword");
		case EWacomBattleTargetRejectReason::BlockedTargetKeyword: return TEXT("BlockedTargetKeyword");
		case EWacomBattleTargetRejectReason::UnsupportedZoneTarget: return TEXT("UnsupportedZoneTarget");
		case EWacomBattleTargetRejectReason::TargetIdentityMismatch: return TEXT("TargetIdentityMismatch");
		case EWacomBattleTargetRejectReason::NotEnoughInitiative: return TEXT("NotEnoughInitiative");
		default: return TEXT("Unknown");
		}
	}

	const FHandCardSnapshot* FindHandCardSnapshotForSceneEnemyTarget(
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

	FWacomBattleEnemyPartDragPredictionDebugInput BuildHoverPredictionInputFromPreview(
		const FGuid& SourceCardInstanceId,
		const FHandCardSnapshot& SourceSnapshot,
		const FBattleCardTargetPreview& TargetPreview)
	{
		FWacomBattleEnemyPartDragPredictionDebugInput PredictionInput;
		PredictionInput.bHasSourceCard = true;
		PredictionInput.SourceCardInstanceId = SourceCardInstanceId;
		PredictionInput.SourceCardRuntimeCost = TargetPreview.bHasPreview
			? TargetPreview.SourceCardRuntimeCost
			: SourceSnapshot.RuntimeCost;
		PredictionInput.bSourceCardSwift = TargetPreview.bHasPreview
			? TargetPreview.bSourceCardSwift
			: SourceSnapshot.bIsSwift;
		PredictionInput.bPreviewCanSubmit = TargetPreview.Validation.bCanTarget;
		PredictionInput.PreviewRejectReason =
			FName(LexToStringForSceneEnemyTarget(TargetPreview.Validation.RejectReason));
		return PredictionInput;
	}

	bool DoesPreviewPartMatchBridge(
		const FWacomBattleEnemyPartEntryViewData& PreviewPart,
		const UWacomBattleEnemyPartWorldTargetBridgeComponent& Bridge)
	{
		if (PreviewPart.PartInstanceId.IsValid()
			&& PreviewPart.PartInstanceId == Bridge.GetPartInstanceId())
		{
			return true;
		}

		const FBattlePartSlotIdentity& Identity = PreviewPart.Identity;
		return Identity.IsValidSlot()
			&& Identity.EncounterId == Bridge.GetBoundEncounterId()
			&& Identity.GetEffectiveEnemySlotId() == Bridge.GetBoundEnemySlotId()
			&& Identity.GetEffectivePartSlotId() == Bridge.GetBoundPartSlotId();
	}
}

FWacomBattleHUDSceneEnemyTargetCoordinator::FWacomBattleHUDSceneEnemyTargetCoordinator(FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
{
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::SetSceneEnemyHosts(
	const TArray<AWacomBattleEnemyActor*>& InHosts)
{
	ClearWorldTargets();
	SceneEnemyHosts.Reset();

	for (AWacomBattleEnemyActor* Host : InHosts)
	{
		if (!IsValid(Host) || Host->IsActorBeingDestroyed())
		{
			continue;
		}

		SceneEnemyHosts.AddUnique(Host);
	}

	if (SceneEnemyHosts.Num() == 0)
	{
		return;
	}

	for (const TWeakObjectPtr<AWacomBattleEnemyActor>& WeakHost : SceneEnemyHosts)
	{
		if (AWacomBattleEnemyActor* Host = WeakHost.Get())
		{
			Host->RefreshAttachedPartBadgeLayout();
		}
	}
	RebuildRegistry();
	if (UBattleSession* Session = Runtime.GetSession())
	{
		SyncWorldTargets(Session->BuildSnapshot());
	}
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::HasSceneEnemyHost() const
{
	for (const TWeakObjectPtr<AWacomBattleEnemyActor>& WeakHost : SceneEnemyHosts)
	{
		if (WeakHost.IsValid())
		{
			return true;
		}
	}
	return false;
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::IsSceneEnemyHostInCurrentRegistry(
	const AWacomBattleEnemyActor* Host) const
{
	if (!IsValid(Host))
	{
		return false;
	}

	for (const TWeakObjectPtr<AWacomBattleEnemyActor>& WeakHost : SceneEnemyHosts)
	{
		if (WeakHost.Get() == Host)
		{
			return true;
		}
	}
	return false;
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::IsWorldTargetInCurrentRegistry(
	const FWacomInteractionTargetHandle& TargetHandle) const
{
	return ResolveWorldTargetBridge(TargetHandle) != nullptr;
}

UWacomBattleEnemyPartWorldTargetBridgeComponent*
FWacomBattleHUDSceneEnemyTargetCoordinator::ResolveWorldTargetBridge(
	const FWacomInteractionTargetHandle& TargetHandle) const
{
	if (!TargetHandle.HasBattlePartSlotIdentity())
	{
		return nullptr;
	}

	for (const FSceneEnemyPartWorldTargetEntry& Entry : SceneEnemyPartWorldTargets)
	{
		UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge = Entry.Bridge.Get();
		if (!IsValid(Bridge) || !Bridge->IsBoundToBattlePart())
		{
			continue;
		}

		if (Bridge->GetBoundEncounterId() == TargetHandle.EncounterId
			&& Bridge->GetBoundEnemySlotId() == TargetHandle.EnemySlotId
			&& Bridge->GetBoundPartSlotId() == TargetHandle.PartSlotId)
		{
			return Bridge;
		}
	}

	return nullptr;
}

UWacomBattleEnemyPartPresentationComponent*
FWacomBattleHUDSceneEnemyTargetCoordinator::ResolveWorldTargetPresentation(
	const FWacomInteractionTargetHandle& TargetHandle) const
{
	const UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge = ResolveWorldTargetBridge(TargetHandle);
	if (!Bridge)
	{
		return nullptr;
	}

	for (const FSceneEnemyPartWorldTargetEntry& Entry : SceneEnemyPartWorldTargets)
	{
		if (Entry.Bridge.Get() == Bridge)
		{
			return Entry.Presentation.Get();
		}
	}
	return nullptr;
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::IsBridgeInCurrentRegistry(
	const UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge) const
{
	if (!IsValid(Bridge))
	{
		return false;
	}

	for (const FSceneEnemyPartWorldTargetEntry& Entry : SceneEnemyPartWorldTargets)
	{
		if (Entry.Bridge.Get() == Bridge)
		{
			return true;
		}
	}
	return false;
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::RebuildRegistry()
{
	for (const FSceneEnemyPartWorldTargetEntry& Entry : SceneEnemyPartWorldTargets)
	{
		if (UWacomBattleEnemyPartPresentationComponent* Presentation = Entry.Presentation.Get())
		{
			Runtime.UnregisterBattlePresentationTargetsForOwner(Presentation);
		}
		if (UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge = Entry.Bridge.Get())
		{
			Bridge->SetBattleHUDSceneRegistryState(false);
		}
	}
	SceneEnemyPartWorldTargets.Reset();

	for (const TWeakObjectPtr<AWacomBattleEnemyActor>& WeakHost : SceneEnemyHosts)
	{
		AWacomBattleEnemyActor* Host = WeakHost.Get();
		if (!IsValid(Host) || Host->IsActorBeingDestroyed())
		{
			continue;
		}

		for (AWacomBattleEnemyPartActor* PartActor : Host->GetBattleEnemyPartActors())
		{
			if (!IsValid(PartActor) || PartActor->IsActorBeingDestroyed())
			{
				continue;
			}

			if (UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
				PartActor->GetWorldTargetBridgeComponent())
			{
				if (Bridge->IsRegistered())
				{
					FSceneEnemyPartWorldTargetEntry Entry;
					Entry.Bridge = Bridge;
					Entry.Presentation = PartActor->GetPresentationComponent();
					SceneEnemyPartWorldTargets.Add(Entry);
					Bridge->SetBattleHUDSceneRegistryState(true);
				}
			}
		}
	}
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::SyncWorldTargets(const FBattleSnapshot& Snapshot)
{
	if (Snapshot.Phase == EBattlePhase::None || Snapshot.Phase == EBattlePhase::BattleEnd)
	{
		ClearWorldTargets();
		return;
	}

	if (!HasSceneEnemyHost())
	{
		ClearWorldTargets();
		return;
	}

	for (const TWeakObjectPtr<AWacomBattleEnemyActor>& WeakHost : SceneEnemyHosts)
	{
		if (AWacomBattleEnemyActor* Host = WeakHost.Get())
		{
			Host->RefreshAttachedPartBadgeLayout();
		}
	}
	for (const TWeakObjectPtr<AWacomBattleEnemyActor>& WeakHost : SceneEnemyHosts)
	{
		if (AWacomBattleEnemyActor* Host = WeakHost.Get())
		{
			Host->RefreshAttachedPartBadgeLayout();
			const FEnemySnapshot* MatchedEnemy = Snapshot.Enemies.FindByPredicate(
				[Host](const FEnemySnapshot& Enemy)
				{
					return Enemy.EnemySlotId == Host->GetEffectiveEnemySlotId();
				});
			if (MatchedEnemy)
			{
				Host->SetEnemyPanelViewData(
					UWacomBattleEnemyPanelWidget::BuildEnemyPanelViewDataFromSnapshot(Snapshot, *MatchedEnemy));
			}
			else
			{
				Host->ClearEnemyPanelViewData();
			}
		}
	}

	RebuildRegistry();
	const FBattleTargetSelectionView TargetSelectionView = Runtime.BuildTargetSelectionView();
	for (const FSceneEnemyPartWorldTargetEntry& Entry : SceneEnemyPartWorldTargets)
	{
		UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge = Entry.Bridge.Get();
		if (!IsValid(Bridge))
		{
			continue;
		}

		if (AWacomBattleEnemyPartActor* PartActor = Cast<AWacomBattleEnemyPartActor>(Bridge->GetOwner()))
		{
			Bridge->SetBattlePartSlotIdentity(
				Snapshot.EncounterId,
				PartActor->EnemySlotId,
				PartActor->GetEffectivePartSlotId());
		}
		FEnemyPartSnapshot MatchedPart;
		const bool bBound = Bridge->SyncFromBattleSnapshot(Snapshot, &MatchedPart);
		UWacomBattleEnemyPartPresentationComponent* Presentation = Entry.Presentation.Get();
		bool bNewTargetable = false;
		FName NewDisabledReason = NAME_None;
		if (bBound)
		{
			for (const FBattleTargetablePartView& PartView : TargetSelectionView.TargetableParts)
			{
				if (PartView.PartInstanceId == Bridge->GetPartInstanceId())
				{
					bNewTargetable = PartView.bTargetable;
					NewDisabledReason = PartView.DisabledReason;
					break;
				}
			}
		}
		Bridge->SetBattleTargetableState(bNewTargetable, NewDisabledReason);

		if (!Presentation)
		{
			continue;
		}
		Runtime.UnregisterBattlePresentationTargetsForOwner(Presentation);

		if (bBound)
		{
			Presentation->CacheRuntimePartFacts(Bridge->PartId, MatchedPart);
		}
		else
		{
			if (MatchedPart.InstanceId.IsValid())
			{
				Presentation->CacheRuntimePartFacts(Bridge->PartId, MatchedPart);
			}
			else
			{
				Presentation->ClearRuntimePartFacts();
			}
			Presentation->ClearDragTargetPreviewState();
			Presentation->ClearHoverProbeState(TEXT("BindingCleared"));
			Presentation->ClearActionPreviewPartView();
			Presentation->SetTargetableAffordance(false);
			continue;
		}
		Presentation->SetTargetableAffordance(bNewTargetable);

		TWeakObjectPtr<UWacomBattleEnemyPartPresentationComponent> WeakPresentation = Presentation;
		Runtime.RegisterBattlePresentationTarget(
			FBattlePartSlotIdentity(
				Bridge->GetBoundEncounterId(),
				Bridge->GetBoundEnemySlotId(),
				Bridge->GetBoundPartSlotId()),
			Presentation,
			[WeakPresentation](const FWacomBattlePresentationTargetCue& Cue)
			{
				if (UWacomBattleEnemyPartPresentationComponent* StrongPresentation = WeakPresentation.Get())
				{
					StrongPresentation->PlayBattlePresentationCue(Cue);
				}
			});
	}
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ClearWorldTargets()
{
	Runtime.ClearFirstPersonCardDragTargetFeedback();
	ClearHoverProbe(TEXT("WorldTargetsCleared"));

	for (const TWeakObjectPtr<AWacomBattleEnemyActor>& WeakHost : SceneEnemyHosts)
	{
		if (AWacomBattleEnemyActor* Host = WeakHost.Get())
		{
			Host->ClearEnemyPanelViewData();
		}
	}

	for (const FSceneEnemyPartWorldTargetEntry& Entry : SceneEnemyPartWorldTargets)
	{
		if (UWacomBattleEnemyPartPresentationComponent* Presentation = Entry.Presentation.Get())
		{
			Runtime.UnregisterBattlePresentationTargetsForOwner(Presentation);
			Presentation->ClearDragTargetPreviewState();
			Presentation->ClearHoverProbeState(TEXT("WorldTargetsCleared"));
			Presentation->ClearActionPreviewPartView();
			Presentation->SetTargetableAffordance(false);
			Presentation->ClearRuntimePartFacts();
		}

		if (UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge = Entry.Bridge.Get())
		{
			Bridge->ClearBattleBinding();
		}
	}
	SceneEnemyPartWorldTargets.Reset();
	SceneEnemyHosts.Reset();
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ApplyActionPreviewToEnemyPanels(
	const TArray<FWacomBattleEnemyPartEntryViewData>& PreviewParts,
	const bool bApplyScenePartPreview) const
{
	if (PreviewParts.IsEmpty())
	{
		ClearActionPreviewFromSceneParts();
		return;
	}

	for (const TWeakObjectPtr<AWacomBattleEnemyActor>& WeakHost : SceneEnemyHosts)
	{
		if (AWacomBattleEnemyActor* Host = WeakHost.Get())
		{
			Host->SetEnemyPanelActionPreview(PreviewParts);
		}
	}

	if (bApplyScenePartPreview)
	{
		ApplyActionPreviewToSceneParts(PreviewParts);
	}
	else
	{
		ClearActionPreviewFromSceneParts();
	}
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ClearActionPreviewFromEnemyPanels() const
{
	for (const TWeakObjectPtr<AWacomBattleEnemyActor>& WeakHost : SceneEnemyHosts)
	{
		if (AWacomBattleEnemyActor* Host = WeakHost.Get())
		{
			Host->ClearEnemyPanelActionPreview();
		}
	}

	ClearActionPreviewFromSceneParts();
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ApplyActionPreviewToSceneParts(
	const TArray<FWacomBattleEnemyPartEntryViewData>& PreviewParts) const
{
	for (const FSceneEnemyPartWorldTargetEntry& Entry : SceneEnemyPartWorldTargets)
	{
		UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge = Entry.Bridge.Get();
		UWacomBattleEnemyPartPresentationComponent* Presentation = Entry.Presentation.Get();
		if (!IsValid(Bridge) || !Bridge->IsBoundToBattlePart() || !Presentation)
		{
			continue;
		}

		const FWacomBattleEnemyPartEntryViewData* MatchedPreview = PreviewParts.FindByPredicate(
			[Bridge](const FWacomBattleEnemyPartEntryViewData& PreviewPart)
			{
				return DoesPreviewPartMatchBridge(PreviewPart, *Bridge);
			});
		if (MatchedPreview)
		{
			Presentation->SetActionPreviewPartView(*MatchedPreview);
		}
		else
		{
			Presentation->ClearActionPreviewPartView();
		}
	}
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ClearActionPreviewFromSceneParts() const
{
	for (const FSceneEnemyPartWorldTargetEntry& Entry : SceneEnemyPartWorldTargets)
	{
		if (UWacomBattleEnemyPartPresentationComponent* Presentation = Entry.Presentation.Get())
		{
			Presentation->ClearActionPreviewPartView();
		}
	}
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::TickHoverProbe(float DeltaTime)
{
	if (!CanUpdateHoverProbe())
	{
		const bool bPreserveFirstPersonDragPreview =
			Runtime.IsFirstPersonCardDragActiveForBattleSceneHover();
		ClearHoverProbe(
			TEXT("ProbeGated"),
			!bPreserveFirstPersonDragPreview);
		HoverProbeElapsedSeconds = 0.0f;
		return;
	}

	HoverProbeElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	if (HoverProbeElapsedSeconds
		< FMath::Max(0.01f, Runtime.Host().GetBattleSceneEnemyPartHoverProbeIntervalSeconds()))
	{
		return;
	}

	HoverProbeElapsedSeconds = 0.0f;
	UpdateHoverProbe();
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ClearHoverProbe(
	FName Reason,
	bool bClearFirstPersonTargetPreviewLayer)
{
	if (UWacomBattleEnemyPartPresentationComponent* Presentation = HoveredPresentation.Get())
	{
		Presentation->ClearHoverProbeState(Reason);
	}
	if (AWacomBattleEnemyActor* Host = HoveredEnemyHost.Get())
	{
		Host->SetEnemyPanelHoveredVisible(false);
	}

	HoveredPresentation.Reset();
	HoveredEnemyHost.Reset();
	HoveredHandle = FWacomInteractionTargetHandle();
	if (bClearFirstPersonTargetPreviewLayer)
	{
		Runtime.GetFirstPersonHandBridge().ClearTargetPreviewLayer();
	}
	if (Runtime.GetUIState() == EBattleUIState::TargetSelect && Runtime.GetPendingTargetingCardId().IsValid())
	{
		Runtime.HideFirstPersonCardDetailPanelForSource(Runtime.GetPendingTargetingCardId());
	}
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::CanUpdateHoverProbe() const
{
	if (Runtime.IsFirstPersonCardDragActiveForBattleSceneHover()
		|| Runtime.HasPendingTurnBoundaryCommand()
		|| Runtime.GetUIState() == EBattleUIState::BattleEnd)
	{
		return false;
	}

	const UBattleSession* CurrentSession = Runtime.GetSession();
	if (!CurrentSession)
	{
		return false;
	}

	return CurrentSession->BuildSnapshot().Phase == EBattlePhase::PlayerAction;
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::UpdateHoverProbe()
{
	FWacomInteractionTargetHandle TargetHandle;
	const AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(Runtime.GetOwningPlayer());
	const bool bHasTarget =
		WacomPC
		&& WacomPC->TryProbeBattleSceneInteractionTarget(TargetHandle)
		&& TargetHandle.TargetKind == EWacomInteractionTargetKind::World
		&& TargetHandle.TargetTag.MatchesTagExact(WacomTags::Interaction_Target_Battle_EnemyPart)
		&& TargetHandle.HasBattlePartSlotIdentity();

	if (!bHasTarget)
	{
		ClearHoverProbe(TEXT("NoTarget"));
		return;
	}

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		ResolveWorldTargetBridge(TargetHandle);
	if (!Bridge)
	{
		ClearHoverProbe(TEXT("MissingBridge"));
		return;
	}

	UWacomBattleEnemyPartPresentationComponent* Presentation =
		ResolveWorldTargetPresentation(TargetHandle);
	if (!Presentation)
	{
		ClearHoverProbe(TEXT("MissingPresentation"));
		return;
	}

	AWacomBattleEnemyActor* HoveredHost = nullptr;
	if (const AWacomBattleEnemyPartActor* PartActor = Cast<AWacomBattleEnemyPartActor>(Bridge->GetOwner()))
	{
		HoveredHost = Cast<AWacomBattleEnemyActor>(PartActor->GetAttachParentActor());
	}

	if (HoveredPresentation.Get() != Presentation)
	{
		ClearHoverProbe(TEXT("TargetChanged"));
		HoveredPresentation = Presentation;
		HoveredEnemyHost = HoveredHost;
	}

	HoveredHandle = TargetHandle;
	if (HoveredHost)
	{
		HoveredHost->SetEnemyPanelHoveredVisible(true);
	}

	FBattleSnapshot CurrentSnapshot;
	const FHandCardSnapshot* SourceSnapshot = nullptr;
	FBattleCardActionPreview ActionPreview;
	FBattleCardTargetPreview TargetPreview;
	FWacomBattleCardTargetPreviewPresentation TargetPreviewPresentation;
	FWacomBattleActionPreviewPresentation ActionPreviewPresentation;
	FWacomBattleEnemyPartDragPredictionDebugInput PredictionInput;
	const bool bHasTargetPreviewContext =
		TryBuildHoverTargetPreviewContext(
			TargetHandle,
			CurrentSnapshot,
			SourceSnapshot,
			ActionPreview,
			TargetPreview,
			PredictionInput);
	if (bHasTargetPreviewContext && TargetPreview.bHasPreview)
	{
		ActionPreviewPresentation =
			WacomBattleCardPresentation::BuildActionPreviewPresentation(
				CurrentSnapshot,
				ActionPreview);
		TargetPreviewPresentation = ActionPreviewPresentation.TargetPreviewPresentation;
	}
	Presentation->SetHoverProbeState(
		TargetHandle,
		TEXT("Hovered"),
		PredictionInput);
	ApplyHoverTargetPreview(
		TargetPreviewPresentation,
		bHasTargetPreviewContext);
	if (ActionPreviewPresentation.bHasPreview)
	{
		Runtime.ApplyActionPreviewPresentation(
			ActionPreviewPresentation,
			/*bApplyScenePartPreview=*/false);
	}
}

FWacomBattleEnemyPartDragPredictionDebugInput
FWacomBattleHUDSceneEnemyTargetCoordinator::BuildHoverPredictionInput(
	const FWacomInteractionTargetHandle& TargetHandle) const
{
	FBattleSnapshot CurrentSnapshot;
	const FHandCardSnapshot* SourceSnapshot = nullptr;
	FBattleCardActionPreview ActionPreview;
	FBattleCardTargetPreview TargetPreview;
	FWacomBattleEnemyPartDragPredictionDebugInput PredictionInput;
	TryBuildHoverTargetPreviewContext(
		TargetHandle,
		CurrentSnapshot,
		SourceSnapshot,
		ActionPreview,
		TargetPreview,
		PredictionInput);
	return PredictionInput;
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::TryBuildHoverTargetPreviewContext(
	const FWacomInteractionTargetHandle& TargetHandle,
	FBattleSnapshot& OutSnapshot,
	const FHandCardSnapshot*& OutSourceSnapshot,
	FBattleCardActionPreview& OutActionPreview,
	FBattleCardTargetPreview& OutTargetPreview,
	FWacomBattleEnemyPartDragPredictionDebugInput& OutPredictionInput) const
{
	OutSourceSnapshot = nullptr;
	OutActionPreview = FBattleCardActionPreview();
	OutTargetPreview = FBattleCardTargetPreview();
	OutPredictionInput = FWacomBattleEnemyPartDragPredictionDebugInput();
	if (Runtime.GetUIState() != EBattleUIState::TargetSelect
		|| !Runtime.GetPendingTargetingCardId().IsValid())
	{
		return false;
	}

	const UBattleSession* CurrentSession = Runtime.GetSession();
	if (!CurrentSession)
	{
		return false;
	}

	OutSnapshot = CurrentSession->BuildSnapshot();
	OutSourceSnapshot =
		FindHandCardSnapshotForSceneEnemyTarget(OutSnapshot, Runtime.GetPendingTargetingCardId());
	if (!OutSourceSnapshot)
	{
		return false;
	}

	OutActionPreview =
		CurrentSession->BuildCardActionPreview(Runtime.GetPendingTargetingCardId(), TargetHandle);
	OutTargetPreview = OutActionPreview.TargetPreview;
	OutPredictionInput =
		BuildHoverPredictionInputFromPreview(
			Runtime.GetPendingTargetingCardId(),
			*OutSourceSnapshot,
			OutTargetPreview);
	return true;
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::TryFindPendingTargetingCardSlot(
	FWacomFirstPersonCardLayerSlotView& OutSlotView) const
{
	if (!Runtime.GetPendingTargetingCardId().IsValid())
	{
		return false;
	}

	const UWacomFirstPersonCardAnchorComponent* Anchor = Runtime.ResolveActiveFirstPersonCardAnchor();
	if (!Anchor)
	{
		return false;
	}

	for (const FWacomFirstPersonCardLayerSlotView& SlotView : Anchor->BuildActiveCardLayerSlotViews())
	{
		if (SlotView.Entry.CardInstanceId == Runtime.GetPendingTargetingCardId() && SlotView.bProjected)
		{
			OutSlotView = SlotView;
			return true;
		}
	}
	return false;
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ApplyHoverTargetPreview(
	const FWacomBattleCardTargetPreviewPresentation& TargetPreviewPresentation,
	bool bHasTargetPreviewContext) const
{
	if (!bHasTargetPreviewContext
		|| Runtime.GetUIState() != EBattleUIState::TargetSelect
		|| !Runtime.GetPendingTargetingCardId().IsValid())
	{
		Runtime.GetFirstPersonHandBridge().ClearTargetPreviewLayer();
		return;
	}

	FWacomFirstPersonCardLayerSlotView SourceSlotView;
	if (!TryFindPendingTargetingCardSlot(SourceSlotView))
	{
		return;
	}

	FWacomBattleHUDFirstPersonHandBridge& FirstPersonHandBridge = Runtime.GetFirstPersonHandBridge();
	if (!TargetPreviewPresentation.bHasPreview)
	{
		FirstPersonHandBridge.ClearTargetPreviewLayer();
		Runtime.HideFirstPersonCardDetailPanelForSource(Runtime.GetPendingTargetingCardId());
		return;
	}

	const bool bCanReuseActiveTargetPreview =
		Runtime.IsCurrentFirstPersonCardDetailSource(Runtime.GetPendingTargetingCardId())
		&& FirstPersonHandBridge.IsSameActiveTargetPreviewState(TargetPreviewPresentation);
	if (bCanReuseActiveTargetPreview)
	{
		Runtime.UpdateFirstPersonCardDetailSlot(SourceSlotView);
		Runtime.PositionFirstPersonCardDetailPanelBesideSlot(SourceSlotView);
		return;
	}

	FirstPersonHandBridge.ApplyTargetPreviewPresentationToLayer(TargetPreviewPresentation);
	FirstPersonHandBridge.StoreActiveTargetPreviewState(TargetPreviewPresentation);
	Runtime.SetFirstPersonCardDetailSource(Runtime.GetPendingTargetingCardId());
	if (TargetPreviewPresentation.bHasSourceCardDetailViewData)
	{
		Runtime.ShowFirstPersonCardDetailAtSlot(
			TargetPreviewPresentation.SourceCardDetailViewData,
			SourceSlotView);
	}
}
