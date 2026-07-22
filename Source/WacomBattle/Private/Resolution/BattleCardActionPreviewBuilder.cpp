// Copyright Wacom. All Rights Reserved.

#include "Resolution/BattleCardActionPreviewBuilder.h"

#include "Commands/PlayCardEvaluation.h"
#include "Commands/PlayCardResolver.h"
#include "Core/BattleOperationAdapter.h"
#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Resolution/BattleCardTargetPreviewBuilder.h"
#include "Snapshots/BattleSnapshotBuilder.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "Types/WacomResult.h"

namespace
{
	bool AreStatusStacksEqual(
		const TMap<FGameplayTag, int32>& Left,
		const TMap<FGameplayTag, int32>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}

		for (const TPair<FGameplayTag, int32>& Entry : Left)
		{
			const int32* RightValue = Right.Find(Entry.Key);
			if (!RightValue || *RightValue != Entry.Value)
			{
				return false;
			}
		}
		return true;
	}

	bool AreTagContainersEqualExact(
		const FGameplayTagContainer& Left,
		const FGameplayTagContainer& Right)
	{
		TArray<FGameplayTag> LeftTags;
		TArray<FGameplayTag> RightTags;
		Left.GetGameplayTagArray(LeftTags);
		Right.GetGameplayTagArray(RightTags);
		if (LeftTags.Num() != RightTags.Num())
		{
			return false;
		}

		for (const FGameplayTag& Tag : LeftTags)
		{
			if (!Right.HasTagExact(Tag))
			{
				return false;
			}
		}
		return true;
	}

	bool HasPlayerChanged(const FPlayerSnapshot& Baseline, const FPlayerSnapshot& Projected)
	{
		return Baseline.CurrentHp != Projected.CurrentHp
			|| Baseline.MaxHp != Projected.MaxHp
			|| Baseline.Shield != Projected.Shield
			|| !AreTagContainersEqualExact(Baseline.Statuses, Projected.Statuses)
			|| !AreStatusStacksEqual(Baseline.StatusStacks, Projected.StatusStacks);
	}

	bool HasEnemyPartChanged(
		const FEnemyPartSnapshot& Baseline,
		const FEnemyPartSnapshot& Projected)
	{
		return Baseline.CurrentHp != Projected.CurrentHp
			|| Baseline.MaxHp != Projected.MaxHp
			|| Baseline.CurrentInitiative != Projected.CurrentInitiative
			|| Baseline.Shield != Projected.Shield
			|| Baseline.bDestroyed != Projected.bDestroyed
			|| !AreTagContainersEqualExact(Baseline.Statuses, Projected.Statuses)
			|| !AreStatusStacksEqual(Baseline.StatusStacks, Projected.StatusStacks);
	}

	bool HasHandCardChanged(
		const FHandCardSnapshot& Baseline,
		const FHandCardSnapshot& Projected)
	{
		return Baseline.InstanceId != Projected.InstanceId
			|| Baseline.RuntimeCost != Projected.RuntimeCost
			|| Baseline.Zone != Projected.Zone
			|| Baseline.bIsCostLegal != Projected.bIsCostLegal
			|| Baseline.bIsPlayable != Projected.bIsPlayable
			|| Baseline.bIsFrozen != Projected.bIsFrozen
			|| !AreTagContainersEqualExact(Baseline.Statuses, Projected.Statuses)
			|| !AreStatusStacksEqual(Baseline.StatusStacks, Projected.StatusStacks);
	}

	bool HasHandChanged(
		const FHandQueueSnapshot& Baseline,
		const FHandQueueSnapshot& Projected)
	{
		if (Baseline.Cards.Num() != Projected.Cards.Num())
		{
			return true;
		}
		for (int32 Index = 0; Index < Baseline.Cards.Num(); ++Index)
		{
			if (HasHandCardChanged(Baseline.Cards[Index], Projected.Cards[Index]))
			{
				return true;
			}
		}
		return false;
	}

	const FEnemyPartSnapshot* FindPartSnapshot(
		const FBattleSnapshot& Snapshot,
		const FGuid& PartInstanceId)
	{
		for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
		{
			for (const FEnemyPartSnapshot& Part : Enemy.Parts)
			{
				if (Part.InstanceId == PartInstanceId)
				{
					return &Part;
				}
			}
		}
		return nullptr;
	}

	struct FEnemyActionPreviewFacts
	{
		bool bWillAct = false;
		bool bWillSkipActionDueToStun = false;
	};

	struct FActionPreviewEventFacts
	{
		TSet<FGuid> PerfectReleasePartIds;
		TMap<FGuid, FEnemyActionPreviewFacts> EnemyActions;
	};

	void FillProjectedValues(
		FBattleCardActionPreview& Preview,
		const FBattleSnapshot& BaselineSnapshot,
		const FBattleSnapshot& ProjectedSnapshot,
		const FActionPreviewEventFacts& EventFacts)
	{
		if (HasPlayerChanged(BaselineSnapshot.Player, ProjectedSnapshot.Player))
		{
			Preview.bHasProjectedPlayer = true;
			Preview.ProjectedPlayer = ProjectedSnapshot.Player;
		}
		if (HasHandChanged(BaselineSnapshot.Hand, ProjectedSnapshot.Hand))
		{
			Preview.bHasProjectedHand = true;
			Preview.ProjectedHand = ProjectedSnapshot.Hand;
		}

		for (const FEnemySnapshot& Enemy : ProjectedSnapshot.Enemies)
		{
			for (const FEnemyPartSnapshot& ProjectedPart : Enemy.Parts)
			{
				const FEnemyActionPreviewFacts* EnemyAction =
					EventFacts.EnemyActions.Find(ProjectedPart.InstanceId);
				const bool bWillAct = EnemyAction && EnemyAction->bWillAct;
				const bool bWillSkipActionDueToStun =
					EnemyAction && EnemyAction->bWillSkipActionDueToStun;
				const bool bPerfectReleaseCandidate =
					EventFacts.PerfectReleasePartIds.Contains(ProjectedPart.InstanceId);
				const FEnemyPartSnapshot* BaselinePart =
					FindPartSnapshot(BaselineSnapshot, ProjectedPart.InstanceId);
				if (!bWillAct
					&& !bWillSkipActionDueToStun
					&& !bPerfectReleaseCandidate
					&& BaselinePart
					&& !HasEnemyPartChanged(*BaselinePart, ProjectedPart))
				{
					continue;
				}

				FBattleCardActionPreviewEnemyPartState PartState;
				PartState.Snapshot = ProjectedPart;
				PartState.bWillAct = bWillAct;
				PartState.bWillSkipActionDueToStun = bWillSkipActionDueToStun;
				PartState.bPerfectReleaseCandidate = bPerfectReleaseCandidate;
				if ((bWillAct || bWillSkipActionDueToStun) && !PartState.Snapshot.bDestroyed)
				{
					PartState.Snapshot.CurrentInitiative = 0;
				}
				Preview.ProjectedEnemyParts.Add(MoveTemp(PartState));
			}
		}
	}

	void CollectPreviewEventFacts(
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& BaselineSnapshot,
		FBattleCardActionPreview& Preview,
		FActionPreviewEventFacts& OutFacts)
	{
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type == EBattleEventType::InitiativeHit
				&& Event.ActorInstanceId.IsValid())
			{
				OutFacts.PerfectReleasePartIds.Add(Event.ActorInstanceId);
			}
			else if (Event.Type == EBattleEventType::EnemyPartActed
				&& Event.ActorInstanceId.IsValid())
			{
				FEnemyActionPreviewFacts& Action =
					OutFacts.EnemyActions.FindOrAdd(Event.ActorInstanceId);
				Action.bWillAct |= Event.Count > 0;
				Action.bWillSkipActionDueToStun |= Event.Count <= 0;
			}
			else if (Event.Type == EBattleEventType::ResistanceResolved
				&& Event.ActorInstanceId.IsValid())
			{
				FBattleCardResistancePreview& Resistance =
					Preview.ResistancePreviews.AddDefaulted_GetRef();
				Resistance.TargetEnemyPartInstanceId = Event.ActorInstanceId;
				Resistance.TargetEnemyPartKey = Event.ActorEnemyPartKey;
				if (const FEnemyPartSnapshot* Part =
					FindPartSnapshot(BaselineSnapshot, Event.ActorInstanceId))
				{
					Resistance.TargetEnemyPartIdentity = Part->Identity;
				}
				Resistance.PlayerPeakSingleHitDamage = Event.Amount;
				Resistance.EnemyPeakSingleHitDamage = Event.Count;
				Resistance.bWillStun = Event.bSuccess;
			}
		}
	}

	void AppendTransactionFailureDebug(
		FWacomBattleTargetValidationResult& Validation,
		const FWacomStatus& Status)
	{
		Validation.DebugSummary += FString::Printf(
			TEXT(" ActionTransaction{Code=%d Detail=%s}"),
			static_cast<int32>(Status.Code),
			*Status.Detail.ToString());
	}
}

FBattleCardActionPreview FBattleCardActionPreviewBuilder::Build(
	const FBattleState& State,
	const FGuid& CardInstanceId,
	const FWacomInteractionTargetHandle& Target)
{
	FBattleCardActionPreview Preview;
	const FPlayCardPreviewCandidate Candidate =
		FPlayCardEvaluator::EvaluatePreviewCandidate(State, CardInstanceId, Target);
	Preview.TargetPreview = FBattleCardTargetPreviewBuilder::Build(State, Candidate);
	if (!Preview.TargetPreview.bHasPreview || !Preview.TargetPreview.Validation.bCanTarget)
	{
		return Preview;
	}

	const FPlayCardCommitResult Evaluation =
		FPlayCardEvaluator::EvaluateCommit(State, Candidate);
	if (!Evaluation.CanCommit())
	{
		const FWacomStatus& Status = Evaluation.GetStatus();
		if (Status.Code == EWacomError::NotEnoughInitiative)
		{
			Preview.TargetPreview.Validation.bCanTarget = false;
			Preview.TargetPreview.Validation.RejectReason =
				EWacomBattleTargetRejectReason::NotEnoughInitiative;
		}
		else if (Status.Detail == TEXT("CardFrozen"))
		{
			Preview.TargetPreview.Validation.bCanTarget = false;
			Preview.TargetPreview.Validation.RejectReason =
				EWacomBattleTargetRejectReason::SourceCardFrozen;
		}
		AppendTransactionFailureDebug(Preview.TargetPreview.Validation, Status);
		return Preview;
	}

	const FBattleSnapshot BaselineSnapshot = FBattleSnapshotBuilder::Build(State);
	FBattleState WorkingState = State;
	FBattleEventBus ScratchEvents;
	FActionPreviewBattleOperationAdapter OperationAdapter;
	const FWacomStatus Status = FPlayCardResolver::ResolvePrepared(
		WorkingState,
		ScratchEvents,
		Evaluation.GetPrepared(),
		OperationAdapter);

	Preview.bHasUnresolvedFacts = OperationAdapter.HasUnresolvedFacts();
	Preview.UnresolvedEffectTypes = OperationAdapter.GetUnresolvedEffectTypes();
	if (!Status.IsOk())
	{
		AppendTransactionFailureDebug(Preview.TargetPreview.Validation, Status);
		return Preview;
	}

	Preview.bHasPreview = true;
	FActionPreviewEventFacts EventFacts;
	CollectPreviewEventFacts(
		ScratchEvents.Consume(),
		BaselineSnapshot,
		Preview,
		EventFacts);
	const FBattleSnapshot ProjectedSnapshot = FBattleSnapshotBuilder::Build(WorkingState);
	FillProjectedValues(Preview, BaselineSnapshot, ProjectedSnapshot, EventFacts);
	return Preview;
}
