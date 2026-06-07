// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDSceneEnemyTargetCoordinator.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/WacomPlayerController.h"
#include "Resolution/BattleTargetValidationResult.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/BattleHUD.h"

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
}

FWacomBattleHUDSceneEnemyTargetCoordinator::FWacomBattleHUDSceneEnemyTargetCoordinator(UBattleHUD& InHUD)
	: HUD(InHUD)
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
	if (UBattleSession* Session = HUD.GetSession())
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
	const UWacomInteractionTargetComponent* InteractionTarget =
		Cast<UWacomInteractionTargetComponent>(TargetHandle.SourceObject.Get());
	const AActor* Owner = InteractionTarget ? InteractionTarget->GetOwner() : nullptr;
	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		Owner ? Owner->FindComponentByClass<UWacomBattleEnemyPartWorldTargetBridgeComponent>() : nullptr;
	return IsBridgeInCurrentRegistry(Bridge) ? Bridge : nullptr;
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::IsBridgeInCurrentRegistry(
	const UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge) const
{
	if (!IsValid(Bridge))
	{
		return false;
	}

	for (const TWeakObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent>& RegisteredBridge :
		SceneEnemyPartWorldTargetBridges)
	{
		if (RegisteredBridge.Get() == Bridge)
		{
			return true;
		}
	}
	return false;
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::RebuildRegistry()
{
	SceneEnemyPartWorldTargetBridges.Reset();

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
					SceneEnemyPartWorldTargetBridges.AddUnique(Bridge);
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
	RebuildRegistry();
	const FBattleTargetSelectionView TargetSelectionView = HUD.BuildTargetSelectionView();
	for (const TWeakObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent>& WeakBridge :
		SceneEnemyPartWorldTargetBridges)
	{
		UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge = WeakBridge.Get();
		if (!IsValid(Bridge))
		{
			continue;
		}

		Bridge->SyncFromBattleHUD(HUD, Snapshot, TargetSelectionView);
	}
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ClearWorldTargets()
{
	HUD.ClearFirstPersonCardDragTargetFeedback();
	ClearHoverProbe(TEXT("WorldTargetsCleared"));

	for (const TWeakObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent>& WeakBridge :
		SceneEnemyPartWorldTargetBridges)
	{
		if (UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge = WeakBridge.Get())
		{
			Bridge->ClearBattleBinding();
		}
	}
	SceneEnemyPartWorldTargetBridges.Reset();
	SceneEnemyHosts.Reset();
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::TickHoverProbe(float DeltaTime)
{
	if (!CanUpdateHoverProbe())
	{
		ClearHoverProbe(TEXT("ProbeGated"));
		HoverProbeElapsedSeconds = 0.0f;
		return;
	}

	HoverProbeElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	if (HoverProbeElapsedSeconds
		< FMath::Max(0.01f, HUD.BattleSceneEnemyPartHoverProbeIntervalSeconds))
	{
		return;
	}

	HoverProbeElapsedSeconds = 0.0f;
	UpdateHoverProbe();
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ClearHoverProbe(FName Reason)
{
	if (UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge = HoveredBridge.Get())
	{
		Bridge->ClearHoverProbeState(Reason);
	}
	HoveredBridge.Reset();
	HoveredHandle = FWacomInteractionTargetHandle();
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::CanUpdateHoverProbe() const
{
	if (HUD.IsFirstPersonCardDragActiveForBattleSceneHover()
		|| HUD.HasPendingTurnBoundaryCommand()
		|| HUD.UIState == EBattleUIState::BattleEnd)
	{
		return false;
	}

	const UBattleSession* CurrentSession = HUD.GetSession();
	if (!CurrentSession)
	{
		return false;
	}

	return CurrentSession->BuildSnapshot().Phase == EBattlePhase::PlayerAction;
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::UpdateHoverProbe()
{
	FWacomInteractionTargetHandle TargetHandle;
	const AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(HUD.GetOwningPlayer());
	const bool bHasTarget =
		WacomPC
		&& WacomPC->TryProbeBattleSceneInteractionTarget(TargetHandle)
		&& TargetHandle.TargetKind == EWacomInteractionTargetKind::World
		&& TargetHandle.TargetTag.MatchesTagExact(WacomTags::Interaction_Target_Battle_EnemyPart)
		&& TargetHandle.WorldTargetId.IsValid();

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

	if (HoveredBridge.Get() != Bridge)
	{
		ClearHoverProbe(TEXT("TargetChanged"));
		HoveredBridge = Bridge;
	}

	HoveredHandle = TargetHandle;
	Bridge->SetHoverProbeState(
		TargetHandle,
		TEXT("Hovered"),
		BuildHoverPredictionInput(TargetHandle));
}

FWacomBattleEnemyPartDragPredictionDebugInput
FWacomBattleHUDSceneEnemyTargetCoordinator::BuildHoverPredictionInput(
	const FWacomInteractionTargetHandle& TargetHandle) const
{
	FWacomBattleEnemyPartDragPredictionDebugInput PredictionInput;
	if (HUD.UIState != EBattleUIState::TargetSelect || !HUD.PendingTargetingCardId.IsValid())
	{
		return PredictionInput;
	}

	const UBattleSession* CurrentSession = HUD.GetSession();
	if (!CurrentSession)
	{
		return PredictionInput;
	}

	const FBattleSnapshot CurrentSnapshot = CurrentSession->BuildSnapshot();
	const FHandCardSnapshot* SourceSnapshot =
		FindHandCardSnapshotForSceneEnemyTarget(CurrentSnapshot, HUD.PendingTargetingCardId);
	if (!SourceSnapshot)
	{
		return PredictionInput;
	}

	const FWacomBattleTargetValidationResult Validation =
		CurrentSession->ValidateTargetWithCard(HUD.PendingTargetingCardId, TargetHandle);
	PredictionInput.bHasSourceCard = true;
	PredictionInput.SourceCardInstanceId = HUD.PendingTargetingCardId;
	PredictionInput.SourceCardRuntimeCost = SourceSnapshot->RuntimeCost;
	PredictionInput.bSourceCardSwift = SourceSnapshot->bIsSwift;
	PredictionInput.bPreviewCanSubmit = Validation.bCanTarget;
	PredictionInput.PreviewRejectReason = FName(LexToStringForSceneEnemyTarget(Validation.RejectReason));
	return PredictionInput;
}
