// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDSceneEnemyTargetCoordinator.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemySceneRuntimeComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "Resolution/BattleCardActionPreview.h"
#include "Resolution/BattleCardTargetPreview.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleEnemyActionPlaybackTypes.h"
#include "UI/Battle/WacomBattleEnemyPanelViewData.h"
#include "UI/Battle/WacomBattleHUDFirstPersonHandBridge.h"
#include "UI/Battle/WacomBattleHUDEnemyInspectionCoordinator.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"

namespace
{
	const FHandCardSnapshot* FindHandCard(
		const FBattleSnapshot& Snapshot, const FGuid& InstanceId)
	{
		return Snapshot.Hand.Cards.FindByPredicate(
			[&InstanceId](const FHandCardSnapshot& Card)
			{
				return Card.InstanceId == InstanceId;
			});
	}

	FName LexRejectReason(EWacomBattleTargetRejectReason Reason)
	{
		switch (Reason)
		{
		case EWacomBattleTargetRejectReason::None: return TEXT("None");
		case EWacomBattleTargetRejectReason::InvalidTarget: return TEXT("InvalidTarget");
		case EWacomBattleTargetRejectReason::NotEnoughInitiative: return TEXT("NotEnoughInitiative");
		case EWacomBattleTargetRejectReason::TargetIdentityMismatch: return TEXT("TargetIdentityMismatch");
		default: return TEXT("Rejected");
		}
	}

	FWacomBattleEnemyPartDragPredictionDebugInput BuildPredictionInput(
		const FGuid& SourceId,
		const FHandCardSnapshot& Source,
		const FBattleCardTargetPreview& Preview)
	{
		FWacomBattleEnemyPartDragPredictionDebugInput Result;
		Result.bHasSourceCard = true;
		Result.SourceCardInstanceId = SourceId;
		Result.SourceCardRuntimeCost = Preview.bHasPreview
			? Preview.SourceCardRuntimeCost : Source.RuntimeCost;
		Result.bSourceCardSwift = Preview.bHasPreview
			? Preview.bSourceCardSwift : Source.bIsSwift;
		Result.bPreviewCanSubmit = Preview.Validation.bCanTarget;
		Result.PreviewRejectReason = LexRejectReason(Preview.Validation.RejectReason);
		return Result;
	}

	FWacomBattleEnemyPanelViewData BuildPanelView(
		const FBattleSnapshot& Snapshot, const FEnemySnapshot& Enemy)
	{
		FWacomBattleEnemyPanelViewData View;
		View.EncounterId = Snapshot.EncounterId;
		View.EnemySlotId = Enemy.EnemySlotId;
		View.UnitKey = Enemy.UnitKey;
		View.EnemyDefinition = Enemy.Definition;
		View.EnemyDisplayName = Enemy.Definition
			? Enemy.Definition->DisplayName : FText::FromName(Enemy.EnemySlotId);
		View.EnemyInitiativeSum = Enemy.InitiativeSum;
		View.bAllPartsDestroyed = Enemy.bAllPartsDestroyed;
		for (const FEnemyPartSnapshot& Part : Enemy.Parts)
		{
			FWacomBattleEnemyPartEntryViewData& Item = View.Parts.AddDefaulted_GetRef();
			Item.PartInstanceId = Part.InstanceId;
			Item.Identity = Part.Identity;
			Item.EnemySlotId = Part.EnemySlotId;
			Item.PartSlotId = Part.PartSlotId;
			Item.PartDisplayName = Part.Definition
				? Part.Definition->DisplayName : FText::FromName(Part.PartSlotId);
			Item.CurrentHp = Part.CurrentHp;
			Item.MaxHp = Part.MaxHp;
			Item.Shield = Part.Shield;
			Item.CurrentInitiative = Part.CurrentInitiative;
			Item.CurrentIntentId = Part.CurrentIntentId;
			Item.CurrentIntentDisplayName = Part.CurrentIntent.DisplayName;
			Item.CurrentIntentInitiative = Part.CurrentIntent.Initiative;
			Item.CurrentIntentResistanceValue = Part.CurrentIntent.ResistanceValue;
			Item.RuntimeStatuses = Part.Statuses;
			Item.RuntimeStatusStacks = Part.StatusStacks;
			Item.bDestroyed = Part.bDestroyed;
		}
		return View;
	}

	bool PreviewMatchesPart(
		const FWacomBattleEnemyPartEntryViewData& Preview,
		const UWacomBattleEnemyPartComponent& Part)
	{
		const FWacomInteractionTargetHandle Handle = Part.BuildWorldTargetHandle();
		return Preview.Identity.IsValidSlot()
			&& Preview.Identity.EncounterId == Handle.EncounterId
			&& Preview.Identity.GetEffectiveEnemySlotId() == Handle.EnemySlotId
			&& Preview.Identity.GetEffectivePartSlotId() == Handle.PartSlotId;
	}

	struct FSceneEnemyPartSyncFacts
	{
		const FEnemyPartSnapshot* SnapshotPart = nullptr;
		bool bTargetable = false;
		FName DisabledReason = NAME_None;
	};

	class FSceneEnemySnapshotSyncFrame
	{
	public:
		FSceneEnemySnapshotSyncFrame(
			const FBattleSnapshot& Snapshot,
			const FBattleTargetSelectionView& Selection)
		{
			TMap<FGuid, const FBattleTargetablePartView*> TargetabilityByInstance;
			TargetabilityByInstance.Reserve(Selection.TargetableParts.Num());
			for (const FBattleTargetablePartView& Targetable : Selection.TargetableParts)
			{
				TargetabilityByInstance.Add(Targetable.PartInstanceId, &Targetable);
			}

			EnemiesBySlot.Reserve(Snapshot.Enemies.Num());
			for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
			{
				EnemiesBySlot.Add(Enemy.EnemySlotId, &Enemy);
				for (const FEnemyPartSnapshot& Part : Enemy.Parts)
				{
					const FBattlePartSlotIdentity Identity = FBattlePartSlotIdentity::Make(
						Part.EncounterId,
						Part.EnemySlotId,
						Part.PartSlotId);
					FSceneEnemyPartSyncFacts& Facts = PartsByIdentity.Add(Identity);
					Facts.SnapshotPart = &Part;
					if (const FBattleTargetablePartView* const* Targetable =
						TargetabilityByInstance.Find(Part.InstanceId))
					{
						Facts.bTargetable = (*Targetable)->bTargetable;
						Facts.DisabledReason = (*Targetable)->DisabledReason;
					}
				}
			}
		}

		const FEnemySnapshot* FindEnemy(FName EnemySlotId) const
		{
			const FEnemySnapshot* const* Match = EnemiesBySlot.Find(EnemySlotId);
			return Match ? *Match : nullptr;
		}

		const FSceneEnemyPartSyncFacts* FindPart(
			const FBattlePartSlotIdentity& Identity) const
		{
			return PartsByIdentity.Find(Identity);
		}

	private:
		TMap<FName, const FEnemySnapshot*> EnemiesBySlot;
		TMap<FBattlePartSlotIdentity, FSceneEnemyPartSyncFacts> PartsByIdentity;
	};
}

FWacomBattleHUDSceneEnemyTargetCoordinator::FWacomBattleHUDSceneEnemyTargetCoordinator(
	FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
{
}

FWacomBattleHUDSceneEnemyTargetCoordinator::~FWacomBattleHUDSceneEnemyTargetCoordinator()
{
	for (const FHostEntry& Entry : SceneEnemyHosts)
	{
		if (AWacomBattleEnemyActor* Host = Entry.Host.Get())
		{
			UnbindHostInspectionDelegate(*Host);
		}
	}
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::HasSameSceneEnemyHosts(
	const TArray<AWacomBattleEnemyActor*>& InHosts) const
{
	TArray<AWacomBattleEnemyActor*> Valid;
	for (AWacomBattleEnemyActor* Host : InHosts)
	{
		if (IsValid(Host) && !Host->IsActorBeingDestroyed())
		{
			Valid.AddUnique(Host);
		}
	}
	if (Valid.Num() != SceneEnemyHosts.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Valid.Num(); ++Index)
	{
		if (SceneEnemyHosts[Index].Host.Get() != Valid[Index]
			|| SceneEnemyHosts[Index].ObservedEnemySlotId != Valid[Index]->GetEffectiveEnemySlotId())
		{
			return false;
		}
	}
	return true;
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::SetSceneEnemyHosts(
	const TArray<AWacomBattleEnemyActor*>& InHosts)
{
	if (HasSameSceneEnemyHosts(InHosts))
	{
		if (!IsRegistryTopologyCurrent())
		{
			RebuildRegistry();
		}
		if (Runtime.HasLastBattleSnapshot())
		{
			SyncWorldTargets(Runtime.GetLastBattleSnapshot());
		}
		else if (UBattleSession* Session = Runtime.GetSession())
		{
			SyncWorldTargets(Session->BuildSnapshot());
		}
		return;
	}

	ClearActiveWorldTargets(TEXT("SceneEnemyHostsChanged"));
	ClearRetiringHosts(true);
	for (AWacomBattleEnemyActor* Host : InHosts)
	{
		if (!IsValid(Host) || Host->IsActorBeingDestroyed()
			|| SceneEnemyHosts.ContainsByPredicate(
				[Host](const FHostEntry& Entry) { return Entry.Host.Get() == Host; }))
		{
			continue;
		}
		Host->ResetRuntimeScenePresentationForBattle();
		FHostEntry& Entry = SceneEnemyHosts.AddDefaulted_GetRef();
		Entry.Host = Host;
		Entry.ObservedEnemySlotId = Host->GetEffectiveEnemySlotId();
		BindHostInspectionDelegate(*Host);
	}
	if (SceneEnemyHosts.IsEmpty())
	{
		Runtime.GetEnemyInspectionCoordinator().CloseInspection(true);
		return;
	}
	RebuildRegistry();
	if (Runtime.HasLastBattleSnapshot())
	{
		SyncWorldTargets(Runtime.GetLastBattleSnapshot());
	}
	else if (UBattleSession* Session = Runtime.GetSession())
	{
		SyncWorldTargets(Session->BuildSnapshot());
	}
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::HasSceneEnemyHost() const
{
	return SceneEnemyHosts.ContainsByPredicate(
		[](const FHostEntry& Entry) { return Entry.Host.IsValid(); });
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::IsSceneEnemyHostInCurrentRegistry(
	const AWacomBattleEnemyActor* Host) const
{
	return IsValid(Host) && SceneEnemyHosts.ContainsByPredicate(
		[Host](const FHostEntry& Entry) { return Entry.Host.Get() == Host; });
}

UWacomBattleEnemyPartComponent*
FWacomBattleHUDSceneEnemyTargetCoordinator::ResolvePartComponent(
	const FWacomInteractionTargetHandle& Handle) const
{
	if (!Handle.HasBattlePartSlotIdentity())
	{
		return nullptr;
	}
	for (FPartEntry* Entry : RegisteredParts)
	{
		UWacomBattleEnemyPartComponent* Part = Entry ? Entry->Part.Get() : nullptr;
		if (Part && Entry->ObservedIdentity.EncounterId == Handle.EncounterId
			&& Entry->ObservedIdentity.GetEffectiveEnemySlotId() == Handle.EnemySlotId
			&& Entry->ObservedIdentity.GetEffectivePartSlotId() == Handle.PartSlotId)
		{
			return Part;
		}
	}
	return nullptr;
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::IsWorldTargetInCurrentRegistry(
	const FWacomInteractionTargetHandle& Handle) const
{
	return ResolvePartComponent(Handle) != nullptr;
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::IsPartInCurrentRegistry(
	const UWacomBattleEnemyPartComponent* Part) const
{
	return IsValid(Part) && RegisteredParts.ContainsByPredicate(
		[Part](const FPartEntry* Entry) { return Entry && Entry->Part.Get() == Part; });
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::RebuildRegisteredPartPointers()
{
	RegisteredParts.Reset();
	for (FHostEntry& Host : SceneEnemyHosts)
	{
		for (FPartEntry& Part : Host.Parts)
		{
			RegisteredParts.Add(&Part);
		}
	}
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::RebuildRegistry()
{
	ClearRegistryEntries(TEXT("RegistryRebuilt"));
	FName EncounterId = NAME_None;
	if (Runtime.HasLastBattleSnapshot())
	{
		EncounterId = Runtime.GetLastBattleSnapshot().EncounterId;
	}
	else if (UBattleSession* Session = Runtime.GetSession())
	{
		EncounterId = Session->BuildSnapshot().EncounterId;
	}
	SceneEnemyHosts.RemoveAll([](const FHostEntry& Entry)
	{
		const AWacomBattleEnemyActor* Host = Entry.Host.Get();
		return !IsValid(Host) || Host->IsActorBeingDestroyed();
	});
	for (FHostEntry& HostEntry : SceneEnemyHosts)
	{
		AWacomBattleEnemyActor* Host = HostEntry.Host.Get();
		UWacomBattleEnemySceneRuntimeComponent* Scene =
			Host ? Host->GetEnemySceneRuntimeComponent() : nullptr;
		if (!Scene)
		{
			continue;
		}
		Scene->InitializeRuntimeSceneBinding(
			EncounterId, Host->GetEffectiveEnemySlotId());
		if (HostEntry.ObservedEnemySlotId != Host->GetEffectiveEnemySlotId())
		{
			HostEntry.ObservedPanelSnapshotVersion = INDEX_NONE;
		}
		HostEntry.ObservedEnemySlotId = Host->GetEffectiveEnemySlotId();
		HostEntry.ObservedTopologyRevision = Host->GetEnemySceneComponentTopologyRevision();
		HostEntry.Parts.Reset();
		for (UWacomBattleEnemyPartComponent* Part : Host->GetBattleEnemyPartComponents())
		{
			if (!IsValid(Part) || !Part->IsRegistered())
			{
				continue;
			}
			FPartEntry& Entry = HostEntry.Parts.AddDefaulted_GetRef();
			Entry.Part = Part;
			Entry.ObservedIdentity = FBattlePartSlotIdentity::Make(
				EncounterId, HostEntry.ObservedEnemySlotId, Part->PartSlotId);
			Scene->SetPartRegisteredWithHUD(*Part, true);
		}
	}
	RebuildRegisteredPartPointers();
	++RegistryRevision;
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::IsRegistryTopologyCurrent() const
{
	for (const FHostEntry& Entry : SceneEnemyHosts)
	{
		const AWacomBattleEnemyActor* Host = Entry.Host.Get();
		if (!IsValid(Host) || Host->IsActorBeingDestroyed()
			|| Entry.ObservedEnemySlotId != Host->GetEffectiveEnemySlotId()
			|| Entry.ObservedTopologyRevision != Host->GetEnemySceneComponentTopologyRevision())
		{
			return false;
		}
		for (const FPartEntry& PartEntry : Entry.Parts)
		{
			if (!PartEntry.Part.IsValid() || !PartEntry.Part->IsRegistered())
			{
				return false;
			}
		}
	}
	return true;
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ClearPresentationTargetRegistration(
	FPartEntry& Entry)
{
	if (Entry.bPresentationTargetRegistered)
	{
		Runtime.UnregisterBattlePresentationTarget(Entry.ObservedIdentity);
		Entry.bPresentationTargetRegistered = false;
	}
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::EnsurePresentationTargetRegistration(
	FPartEntry& Entry)
{
	UWacomBattleEnemyPartComponent* Part = Entry.Part.Get();
	if (!Part)
	{
		return;
	}
	if (Entry.bPresentationTargetRegistered
		&& Runtime.IsBattlePresentationTargetRegisteredForOwner(Part))
	{
		return;
	}
	ClearPresentationTargetRegistration(Entry);
	TWeakObjectPtr<UWacomBattleEnemyPartComponent> WeakPart = Part;
	Runtime.RegisterBattlePresentationTarget(
		Entry.ObservedIdentity,
		Part,
		[WeakPart](const FWacomBattlePresentationTargetCue& Cue)
		{
			UWacomBattleEnemyPartComponent* StrongPart = WeakPart.Get();
			AWacomBattleEnemyActor* Host = StrongPart
				? StrongPart->GetOwningEnemyHost() : nullptr;
			if (StrongPart && Host && Host->GetEnemySceneRuntimeComponent())
			{
				Host->GetEnemySceneRuntimeComponent()->PlayPartPresentationCue(
					*StrongPart, Cue);
			}
		});
	Entry.bPresentationTargetRegistered = true;
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ClearRegistryEntries(FName Reason)
{
	for (FPartEntry* Entry : RegisteredParts)
	{
		if (!Entry)
		{
			continue;
		}
		ClearPresentationTargetRegistration(*Entry);
		UWacomBattleEnemyPartComponent* Part = Entry->Part.Get();
		AWacomBattleEnemyActor* Host = Part ? Part->GetOwningEnemyHost() : nullptr;
		UWacomBattleEnemySceneRuntimeComponent* Scene =
			Host ? Host->GetEnemySceneRuntimeComponent() : nullptr;
		if (Part && Scene)
		{
			Scene->ClearPartDragTargetPreviewState(*Part);
			Scene->ClearPartHoverProbeState(*Part, Reason);
			Scene->ClearPartActionPreview(*Part);
			Scene->SetPartTargetable(*Part, false, Reason);
			Scene->SetPartRegisteredWithHUD(*Part, false);
			Scene->ClearPartBattleBinding(*Part);
		}
	}
	RegisteredParts.Reset();
	for (FHostEntry& Host : SceneEnemyHosts)
	{
		Host.Parts.Reset();
	}
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::SyncWorldTargets(
	const FBattleSnapshot& Snapshot)
{
	if (Snapshot.Phase == EBattlePhase::BattleEnd)
	{
		Runtime.GetEnemyInspectionCoordinator().CloseInspection(true);
		RetireWorldTargetsForBattleEnd(Snapshot);
		return;
	}
	if (Snapshot.Phase == EBattlePhase::None || !HasSceneEnemyHost())
	{
		ClearWorldTargets();
		return;
	}
	if (!IsRegistryTopologyCurrent())
	{
		RebuildRegistry();
	}

	const FBattleTargetSelectionView Selection = Runtime.BuildTargetSelectionView(Snapshot);
	const FSceneEnemySnapshotSyncFrame SyncFrame(Snapshot, Selection);
	for (FHostEntry& HostEntry : SceneEnemyHosts)
	{
		AWacomBattleEnemyActor* Host = HostEntry.Host.Get();
		if (!Host)
		{
			continue;
		}
		if (HostEntry.ObservedPanelSnapshotVersion == Snapshot.Version)
		{
			continue;
		}
		const FEnemySnapshot* Enemy = SyncFrame.FindEnemy(HostEntry.ObservedEnemySlotId);
		if (Enemy)
		{
			const FWacomBattleEnemyPanelViewData View = BuildPanelView(Snapshot, *Enemy);
			Host->SetEnemyPanelViewData(View);
			Runtime.GetEnemyInspectionCoordinator().UpdateEnemyView(View);
		}
		else
		{
			Host->ClearEnemyPanelViewData();
		}
		HostEntry.ObservedPanelSnapshotVersion = Snapshot.Version;
	}

	for (FPartEntry* Entry : RegisteredParts)
	{
		UWacomBattleEnemyPartComponent* Part = Entry ? Entry->Part.Get() : nullptr;
		AWacomBattleEnemyActor* Host = Part ? Part->GetOwningEnemyHost() : nullptr;
		UWacomBattleEnemySceneRuntimeComponent* Scene =
			Host ? Host->GetEnemySceneRuntimeComponent() : nullptr;
		if (!Part || !Scene)
		{
			continue;
		}
		const FSceneEnemyPartSyncFacts* Facts = SyncFrame.FindPart(Entry->ObservedIdentity);
		const bool bBound = Scene->ApplyPartSnapshotFacts(
			*Part,
			Facts ? Facts->SnapshotPart : nullptr,
			Facts && Facts->bTargetable,
			Facts ? Facts->DisabledReason : NAME_None);
		if (bBound)
		{
			EnsurePresentationTargetRegistration(*Entry);
		}
		else
		{
			ClearPresentationTargetRegistration(*Entry);
			Scene->ClearPartDragTargetPreviewState(*Part);
			Scene->ClearPartHoverProbeState(*Part, TEXT("BindingCleared"));
			Scene->ClearPartActionPreview(*Part);
		}
	}
	RefreshEnemyPanelInspectionInteraction();
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ClearActiveWorldTargets(FName Reason)
{
	Runtime.GetEnemyInspectionCoordinator().CloseInspection(true);
	Runtime.ClearFirstPersonCardDragTargetFeedback();
	ClearHoverProbe(Reason);
	for (const FHostEntry& Entry : SceneEnemyHosts)
	{
		if (AWacomBattleEnemyActor* Host = Entry.Host.Get())
		{
			UnbindHostInspectionDelegate(*Host);
			Host->SetEnemyPanelInspectionInteractionEnabled(false);
			Host->ClearEnemyPanelViewData();
		}
	}
	ClearRegistryEntries(Reason);
	SceneEnemyHosts.Reset();
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ClearWorldTargets()
{
	TArray<TWeakObjectPtr<AWacomBattleEnemyActor>> Hosts;
	for (const FHostEntry& Entry : SceneEnemyHosts)
	{
		Hosts.AddUnique(Entry.Host);
	}
	ClearActiveWorldTargets(TEXT("WorldTargetsCleared"));
	for (const TWeakObjectPtr<AWacomBattleEnemyActor>& HostPtr : Hosts)
	{
		if (AWacomBattleEnemyActor* Host = HostPtr.Get())
		{
			if (UWacomBattleEnemySceneRuntimeComponent* Scene = Host->GetEnemySceneRuntimeComponent())
			{
				Scene->CancelAllPlayback(true);
			}
		}
	}
	ClearRetiringHosts(true);
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::RetireWorldTargetsForBattleEnd(
	const FBattleSnapshot& Snapshot)
{
	for (const FHostEntry& Active : SceneEnemyHosts)
	{
		AWacomBattleEnemyActor* Host = Active.Host.Get();
		if (!Host)
		{
			continue;
		}
		const FEnemySnapshot* Enemy = Snapshot.Enemies.FindByPredicate(
			[Slot = Active.ObservedEnemySlotId](const FEnemySnapshot& Item)
			{
				return Item.EnemySlotId == Slot;
			});
		FRetiringHostEntry& Retiring = RetiringSceneEnemyHosts.AddDefaulted_GetRef();
		Retiring.Host = Host;
		Retiring.ObservedEnemySlotId = Active.ObservedEnemySlotId;
		Retiring.bAllPartsDestroyed = Enemy && Enemy->bAllPartsDestroyed;
		Retiring.Parts = Active.Parts;
	}
	ClearActiveWorldTargets(TEXT("BattleEnd"));
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::PlaySceneEnemyActionAnimation(
	const FBattlePartSlotIdentity& ActingPartKey,
	FName IntentId,
	FWacomBattleEnemyActionPlaybackCallbacks&& Callbacks)
{
	if (!ActingPartKey.IsValidSlot())
	{
		Callbacks.CompleteImmediately();
		return;
	}
	auto TryPlay = [&ActingPartKey, IntentId, &Callbacks](
		AWacomBattleEnemyActor* Host, const TArray<FPartEntry>& Parts)
	{
		const FPartEntry* Entry = Parts.FindByPredicate(
			[&ActingPartKey](const FPartEntry& Candidate)
			{
				return Candidate.ObservedIdentity.MatchesRuntimeSlot(ActingPartKey);
			});
		UWacomBattleEnemyPartComponent* Part = Entry ? Entry->Part.Get() : nullptr;
		UWacomBattleEnemySceneRuntimeComponent* Scene =
			Host ? Host->GetEnemySceneRuntimeComponent() : nullptr;
		if (!Part || !Scene)
		{
			return false;
		}
		Scene->PlayPartActionAnimation(*Part, IntentId, MoveTemp(Callbacks));
		return true;
	};
	for (const FHostEntry& Entry : SceneEnemyHosts)
	{
		if (TryPlay(Entry.Host.Get(), Entry.Parts)) return;
	}
	for (const FRetiringHostEntry& Entry : RetiringSceneEnemyHosts)
	{
		if (TryPlay(Entry.Host.Get(), Entry.Parts)) return;
	}
	Callbacks.CompleteImmediately();
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::PlayEnemyDestroyedAnimation(
	FName EnemySlotId, TFunction<void()>&& Completion)
{
	auto TryPlay = [&Completion, EnemySlotId](
		AWacomBattleEnemyActor* Host, FName Slot, bool bDestroyed)
	{
		if (Slot != EnemySlotId || !bDestroyed || !Host
			|| !Host->GetEnemySceneRuntimeComponent())
		{
			return false;
		}
		Host->GetEnemySceneRuntimeComponent()->PlayEnemyDestroyedAnimation(
			MoveTemp(Completion));
		return true;
	};
	for (const FHostEntry& Entry : SceneEnemyHosts)
	{
		if (TryPlay(Entry.Host.Get(), Entry.ObservedEnemySlotId,
			IsActiveEnemyAllPartsDestroyed(EnemySlotId))) return;
	}
	for (const FRetiringHostEntry& Entry : RetiringSceneEnemyHosts)
	{
		if (TryPlay(Entry.Host.Get(), Entry.ObservedEnemySlotId,
			Entry.bAllPartsDestroyed)) return;
	}
	if (Completion) Completion();
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::IsActiveEnemyAllPartsDestroyed(
	FName EnemySlotId) const
{
	const UBattleSession* Session = Runtime.GetSession();
	if (!Session) return false;
	const FBattleSnapshot Snapshot = Runtime.HasLastBattleSnapshot()
		? Runtime.GetLastBattleSnapshot()
		: Session->BuildSnapshot();
	const FEnemySnapshot* Enemy = Snapshot.Enemies.FindByPredicate(
		[EnemySlotId](const FEnemySnapshot& Item)
		{
			return Item.EnemySlotId == EnemySlotId;
		});
	return Enemy && Enemy->bAllPartsDestroyed;
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ClearRetiringHosts(
	bool bCancelPendingPlayback)
{
	if (bCancelPendingPlayback)
	{
		for (const FRetiringHostEntry& Entry : RetiringSceneEnemyHosts)
		{
			if (AWacomBattleEnemyActor* Host = Entry.Host.Get())
			{
				if (UWacomBattleEnemySceneRuntimeComponent* Scene = Host->GetEnemySceneRuntimeComponent())
				{
					Scene->CancelAllPlayback(true);
				}
			}
		}
	}
	RetiringSceneEnemyHosts.Reset();
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ApplyActionPreviewToEnemyPanels(
	const TArray<FWacomBattleEnemyPartEntryViewData>& PreviewParts,
	bool bApplyScenePartPreview) const
{
	for (const FHostEntry& Entry : SceneEnemyHosts)
	{
		if (AWacomBattleEnemyActor* Host = Entry.Host.Get())
		{
			Host->SetEnemyPanelActionPreview(PreviewParts);
		}
	}
	if (bApplyScenePartPreview) ApplyActionPreviewToSceneParts(PreviewParts);
	else ClearActionPreviewFromSceneParts();
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ClearActionPreviewFromEnemyPanels() const
{
	for (const FHostEntry& Entry : SceneEnemyHosts)
	{
		if (AWacomBattleEnemyActor* Host = Entry.Host.Get())
		{
			Host->ClearEnemyPanelActionPreview();
		}
	}
	ClearActionPreviewFromSceneParts();
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ApplyActionPreviewToSceneParts(
	const TArray<FWacomBattleEnemyPartEntryViewData>& PreviewParts) const
{
	for (FPartEntry* Entry : RegisteredParts)
	{
		UWacomBattleEnemyPartComponent* Part = Entry ? Entry->Part.Get() : nullptr;
		AWacomBattleEnemyActor* Host = Part ? Part->GetOwningEnemyHost() : nullptr;
		UWacomBattleEnemySceneRuntimeComponent* Scene = Host
			? Host->GetEnemySceneRuntimeComponent() : nullptr;
		if (!Part || !Scene) continue;
		const FWacomBattleEnemyPartEntryViewData* Match = PreviewParts.FindByPredicate(
			[Part](const FWacomBattleEnemyPartEntryViewData& Item)
			{
				return PreviewMatchesPart(Item, *Part);
			});
		if (Match) Scene->SetPartActionPreview(*Part, *Match);
		else Scene->ClearPartActionPreview(*Part);
	}
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ClearActionPreviewFromSceneParts() const
{
	for (FPartEntry* Entry : RegisteredParts)
	{
		UWacomBattleEnemyPartComponent* Part = Entry ? Entry->Part.Get() : nullptr;
		AWacomBattleEnemyActor* Host = Part ? Part->GetOwningEnemyHost() : nullptr;
		if (Part && Host && Host->GetEnemySceneRuntimeComponent())
		{
			Host->GetEnemySceneRuntimeComponent()->ClearPartActionPreview(*Part);
		}
	}
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::RefreshEnemyPanelInspectionInteraction() const
{
	const bool bEnabled = Runtime.CanOpenEnemyInspection();
	for (const FHostEntry& Entry : SceneEnemyHosts)
	{
		if (AWacomBattleEnemyActor* Host = Entry.Host.Get())
		{
			Host->SetEnemyPanelInspectionInteractionEnabled(bEnabled);
		}
	}
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::BindHostInspectionDelegate(
	AWacomBattleEnemyActor& Host)
{
	Host.OnEnemyPanelInspectionRequestedNative.RemoveAll(this);
	Host.OnEnemyPanelInspectionRequestedNative.AddRaw(
		this,
		&FWacomBattleHUDSceneEnemyTargetCoordinator::HandleEnemyPanelInspectionRequested);
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::UnbindHostInspectionDelegate(
	AWacomBattleEnemyActor& Host)
{
	Host.OnEnemyPanelInspectionRequestedNative.RemoveAll(this);
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::HandleEnemyPanelInspectionRequested(
	AWacomBattleEnemyActor* Host,
	const FBattlePartSlotIdentity& PartIdentity)
{
	if (!Runtime.CanOpenEnemyInspection() || !Host || !PartIdentity.IsValidSlot()) return;
	const FHostEntry* Entry = SceneEnemyHosts.FindByPredicate(
		[Host](const FHostEntry& Item) { return Item.Host.Get() == Host; });
	UBattleSession* Session = Runtime.GetSession();
	if (!Entry || !Session) return;
	const FBattleSnapshot Snapshot = Runtime.HasLastBattleSnapshot()
		? Runtime.GetLastBattleSnapshot()
		: Session->BuildSnapshot();
	const FEnemySnapshot* Enemy = Snapshot.Enemies.FindByPredicate(
		[Slot = Entry->ObservedEnemySlotId](const FEnemySnapshot& Item)
		{
			return Item.EnemySlotId == Slot;
		});
	if (Enemy)
	{
		Runtime.GetEnemyInspectionCoordinator().ToggleInspection(
			BuildPanelView(Snapshot, *Enemy), PartIdentity);
	}
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::TickHoverProbe(float DeltaTime)
{
	if (!CanUpdateHoverProbe())
	{
		ClearHoverProbe(TEXT("ProbeGated"),
			!Runtime.IsFirstPersonCardDragActiveForBattleSceneHover());
		HoverProbeElapsedSeconds = 0.0f;
		return;
	}
	HoverProbeElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	if (HoverProbeElapsedSeconds < FMath::Max(
		0.01f, Runtime.Host().GetBattleSceneEnemyPartHoverProbeIntervalSeconds()))
	{
		return;
	}
	HoverProbeElapsedSeconds = 0.0f;
	UpdateHoverProbe();
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ClearHoverProbe(
	FName Reason, bool bClearFirstPersonTargetPreviewLayer)
{
	const bool bHadHoverPresentation = HoveredPart.IsValid()
		|| HoveredEnemyHost.IsValid()
		|| HoveredHandle.IsValid()
		|| bHoverPresentationCacheValid;
	if (!bHadHoverPresentation)
	{
		return;
	}
	if (UWacomBattleEnemyPartComponent* Part = HoveredPart.Get())
	{
		if (AWacomBattleEnemyActor* Host = Part->GetOwningEnemyHost())
		{
			if (UWacomBattleEnemySceneRuntimeComponent* Scene = Host->GetEnemySceneRuntimeComponent())
			{
				Scene->ClearPartHoverProbeState(*Part, Reason);
			}
		}
	}
	if (AWacomBattleEnemyActor* Host = HoveredEnemyHost.Get())
	{
		Host->SetEnemyPanelHoveredPart(NAME_None);
	}
	HoveredPart.Reset();
	HoveredEnemyHost.Reset();
	HoveredHandle = FWacomInteractionTargetHandle();
	ResetHoverPresentationCache();
	if (bClearFirstPersonTargetPreviewLayer)
	{
		Runtime.GetFirstPersonHandBridge().ClearTargetPreviewLayer();
	}
	if (Runtime.GetUIState() == EBattleUIState::TargetSelect
		&& Runtime.GetPendingTargetingCardId().IsValid())
	{
		Runtime.HideFirstPersonCardDetailPanelForSource(Runtime.GetPendingTargetingCardId());
	}
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ResetHoverPresentationCache()
{
	bHoverPresentationCacheValid = false;
	HoverPresentationSnapshotVersion = INDEX_NONE;
	HoverPresentationUIState = EBattleUIState::Idle;
	HoverPresentationPendingCardId.Invalidate();
	HoverPresentationWorldTargetId.Invalidate();
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::CanUpdateHoverProbe() const
{
	if (Runtime.IsFirstPersonCardDragActiveForBattleSceneHover()
		|| Runtime.HasPendingTurnBoundaryCommand()
		|| Runtime.GetUIState() == EBattleUIState::BattleEnd)
	{
		return false;
	}
	return Runtime.GetSession()
		&& Runtime.HasLastBattleSnapshot()
		&& Runtime.GetLastBattleSnapshot().Phase == EBattlePhase::PlayerAction;
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::UpdateHoverProbe()
{
	FWacomInteractionTargetHandle Handle;
	const AWacomPlayerController* PC = Cast<AWacomPlayerController>(Runtime.GetOwningPlayer());
	const bool bHasTarget = PC
		&& PC->TryProbeBattleSceneInteractionTarget(Handle)
		&& Handle.TargetKind == EWacomInteractionTargetKind::World
		&& Handle.TargetTag.MatchesTagExact(WacomTags::Interaction_Target_Battle_EnemyPart)
		&& Handle.HasBattlePartSlotIdentity();
	++HoverProbeTraceCount;
	UWacomBattleEnemyPartComponent* Part = bHasTarget ? ResolvePartComponent(Handle) : nullptr;
	if (!Part)
	{
		ClearHoverProbe(TEXT("NoTarget"));
		return;
	}
	AWacomBattleEnemyActor* Host = Part->GetOwningEnemyHost();
	UWacomBattleEnemySceneRuntimeComponent* Scene = Host
		? Host->GetEnemySceneRuntimeComponent() : nullptr;
	if (!Host || !Scene)
	{
		ClearHoverProbe(TEXT("MissingRuntime"));
		return;
	}
	if (HoveredPart.Get() != Part)
	{
		ClearHoverProbe(TEXT("TargetChanged"));
		HoveredPart = Part;
		HoveredEnemyHost = Host;
	}
	HoveredHandle = Handle;
	const int32 SnapshotVersion = Runtime.HasLastBattleSnapshot()
		? Runtime.GetLastBattleSnapshot().Version
		: INDEX_NONE;
	const EBattleUIState UIState = Runtime.GetUIState();
	const FGuid PendingCardId = Runtime.GetPendingTargetingCardId();
	if (bHoverPresentationCacheValid
		&& HoveredPart.Get() == Part
		&& HoverPresentationSnapshotVersion == SnapshotVersion
		&& HoverPresentationUIState == UIState
		&& HoverPresentationPendingCardId == PendingCardId
		&& HoverPresentationWorldTargetId == Handle.WorldTargetId)
	{
		++HoverPreviewReuseCount;
		return;
	}
	Host->SetEnemyPanelHoveredPart(Part->PartSlotId);

	const FBattleSnapshot* Snapshot = nullptr;
	const FHandCardSnapshot* Source = nullptr;
	FBattleCardActionPreview ActionPreview;
	FBattleCardTargetPreview TargetPreview;
	FWacomBattleEnemyPartDragPredictionDebugInput Prediction;
	FWacomBattleActionPreviewPresentation ActionPresentation;
	FWacomBattleCardTargetPreviewPresentation TargetPresentation;
	const bool bHasContext = TryBuildHoverTargetPreviewContext(
		Handle, Snapshot, Source, ActionPreview, TargetPreview, Prediction);
	++HoverPreviewBuildCount;
	if (bHasContext && TargetPreview.bHasPreview)
	{
		ActionPresentation = WacomBattleCardPresentation::BuildActionPreviewPresentation(
			*Snapshot, ActionPreview);
		TargetPresentation = ActionPresentation.TargetPreviewPresentation;
	}
	Scene->SetPartHoverProbeState(*Part, Handle, TEXT("Hovered"), Prediction);
	ApplyHoverTargetPreview(TargetPresentation, bHasContext);
	if (ActionPresentation.bHasPreview)
	{
		Runtime.ApplyActionPreviewPresentation(ActionPresentation, false);
	}
	bHoverPresentationCacheValid = true;
	HoverPresentationSnapshotVersion = SnapshotVersion;
	HoverPresentationUIState = UIState;
	HoverPresentationPendingCardId = PendingCardId;
	HoverPresentationWorldTargetId = Handle.WorldTargetId;
}

FWacomBattleEnemyPartDragPredictionDebugInput
FWacomBattleHUDSceneEnemyTargetCoordinator::BuildHoverPredictionInput(
	const FWacomInteractionTargetHandle& TargetHandle) const
{
	const FBattleSnapshot* Snapshot = nullptr;
	const FHandCardSnapshot* Source = nullptr;
	FBattleCardActionPreview ActionPreview;
	FBattleCardTargetPreview TargetPreview;
	FWacomBattleEnemyPartDragPredictionDebugInput Prediction;
	TryBuildHoverTargetPreviewContext(
		TargetHandle, Snapshot, Source, ActionPreview, TargetPreview, Prediction);
	return Prediction;
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::TryBuildHoverTargetPreviewContext(
	const FWacomInteractionTargetHandle& TargetHandle,
	const FBattleSnapshot*& OutSnapshot,
	const FHandCardSnapshot*& OutSourceSnapshot,
	FBattleCardActionPreview& OutActionPreview,
	FBattleCardTargetPreview& OutTargetPreview,
	FWacomBattleEnemyPartDragPredictionDebugInput& OutPredictionInput) const
{
	OutSnapshot = nullptr;
	OutSourceSnapshot = nullptr;
	OutActionPreview = FBattleCardActionPreview();
	OutTargetPreview = FBattleCardTargetPreview();
	OutPredictionInput = FWacomBattleEnemyPartDragPredictionDebugInput();
	if (Runtime.GetUIState() != EBattleUIState::TargetSelect
		|| !Runtime.GetPendingTargetingCardId().IsValid()) return false;
	const UBattleSession* Session = Runtime.GetSession();
	if (!Session || !Runtime.HasLastBattleSnapshot()) return false;
	OutSnapshot = &Runtime.GetLastBattleSnapshot();
	OutSourceSnapshot = FindHandCard(*OutSnapshot, Runtime.GetPendingTargetingCardId());
	if (!OutSourceSnapshot) return false;
	OutActionPreview = Session->BuildCardActionPreview(
		Runtime.GetPendingTargetingCardId(), TargetHandle);
	OutTargetPreview = OutActionPreview.TargetPreview;
	OutPredictionInput = BuildPredictionInput(
		Runtime.GetPendingTargetingCardId(), *OutSourceSnapshot, OutTargetPreview);
	return true;
}

bool FWacomBattleHUDSceneEnemyTargetCoordinator::TryFindPendingTargetingCardSlot(
	FWacomFirstPersonCardLayerSlotView& OutSlotView) const
{
	if (!Runtime.GetPendingTargetingCardId().IsValid()) return false;
	const UWacomFirstPersonCardAnchorComponent* Anchor =
		Runtime.ResolveActiveFirstPersonCardAnchor();
	if (!Anchor) return false;
	for (const FWacomFirstPersonCardLayerSlotView& Slot : Anchor->BuildActiveCardLayerSlotViews())
	{
		if (Slot.Entry.CardInstanceId == Runtime.GetPendingTargetingCardId() && Slot.bProjected)
		{
			OutSlotView = Slot;
			return true;
		}
	}
	return false;
}

void FWacomBattleHUDSceneEnemyTargetCoordinator::ApplyHoverTargetPreview(
	const FWacomBattleCardTargetPreviewPresentation& Presentation,
	bool bHasTargetPreviewContext) const
{
	if (!bHasTargetPreviewContext || Runtime.GetUIState() != EBattleUIState::TargetSelect
		|| !Runtime.GetPendingTargetingCardId().IsValid())
	{
		Runtime.GetFirstPersonHandBridge().ClearTargetPreviewLayer();
		return;
	}
	FWacomFirstPersonCardLayerSlotView SourceSlot;
	if (!TryFindPendingTargetingCardSlot(SourceSlot)) return;
	FWacomBattleHUDFirstPersonHandBridge& Bridge = Runtime.GetFirstPersonHandBridge();
	if (!Presentation.bHasPreview)
	{
		Bridge.ClearTargetPreviewLayer();
		Runtime.HideFirstPersonCardDetailPanelForSource(Runtime.GetPendingTargetingCardId());
		return;
	}
	if (Runtime.IsCurrentFirstPersonCardDetailSource(Runtime.GetPendingTargetingCardId())
		&& Bridge.IsSameActiveTargetPreviewState(Presentation))
	{
		Runtime.UpdateFirstPersonCardDetailSlot(SourceSlot);
		Runtime.PositionFirstPersonCardDetailPanelBesideSlot(SourceSlot);
		return;
	}
	Bridge.ApplyTargetPreviewPresentationToLayer(Presentation);
	Bridge.StoreActiveTargetPreviewState(Presentation);
	Runtime.SetFirstPersonCardDetailSource(Runtime.GetPendingTargetingCardId());
	if (Presentation.bHasSourceCardDetailViewData)
	{
		Runtime.ShowFirstPersonCardDetailAtSlot(
			Presentation.SourceCardDetailViewData, SourceSlot);
	}
}
