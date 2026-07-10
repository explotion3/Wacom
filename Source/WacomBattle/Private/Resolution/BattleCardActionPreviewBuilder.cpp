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

	void FillProjectedValues(
		FBattleCardActionPreview& Preview,
		const FBattleSnapshot& BaselineSnapshot,
		const FBattleSnapshot& ProjectedSnapshot,
		const TSet<FGuid>& WillActPartIds)
	{
		if (HasPlayerChanged(BaselineSnapshot.Player, ProjectedSnapshot.Player))
		{
			Preview.bHasProjectedPlayer = true;
			Preview.ProjectedPlayer = ProjectedSnapshot.Player;
		}

		for (const FEnemySnapshot& Enemy : ProjectedSnapshot.Enemies)
		{
			for (const FEnemyPartSnapshot& ProjectedPart : Enemy.Parts)
			{
				const bool bWillAct = WillActPartIds.Contains(ProjectedPart.InstanceId);
				const FEnemyPartSnapshot* BaselinePart =
					FindPartSnapshot(BaselineSnapshot, ProjectedPart.InstanceId);
				if (!bWillAct && BaselinePart && !HasEnemyPartChanged(*BaselinePart, ProjectedPart))
				{
					continue;
				}

				FBattleCardActionPreviewEnemyPartState PartState;
				PartState.Snapshot = ProjectedPart;
				PartState.bWillAct = bWillAct;
				if (bWillAct && !PartState.Snapshot.bDestroyed)
				{
					PartState.Snapshot.CurrentInitiative = 0;
				}
				Preview.ProjectedEnemyParts.Add(MoveTemp(PartState));
			}
		}
	}

	void CollectWillActPartIds(const TArray<FBattleEvent>& Events, TSet<FGuid>& OutPartIds)
	{
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type == EBattleEventType::EnemyPartActed && Event.ActorInstanceId.IsValid())
			{
				OutPartIds.Add(Event.ActorInstanceId);
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
	TSet<FGuid> WillActPartIds;
	CollectWillActPartIds(ScratchEvents.Consume(), WillActPartIds);
	const FBattleSnapshot ProjectedSnapshot = FBattleSnapshotBuilder::Build(WorkingState);
	FillProjectedValues(Preview, BaselineSnapshot, ProjectedSnapshot, WillActPartIds);
	return Preview;
}
