// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceNavigationController.h"

bool FWacomBackpackWorkspaceNavigationTarget::HasSameIdentity(
	const FWacomBackpackWorkspaceNavigationTarget& Other) const
{
	if (Kind != Other.Kind)
	{
		return false;
	}
	if (Kind == EWacomBackpackWorkspaceNavigationTargetKind::Card)
	{
		return InstanceId.IsValid() && InstanceId == Other.InstanceId;
	}
	if (Kind == EWacomBackpackWorkspaceNavigationTargetKind::Pile)
	{
		return Zone == Other.Zone;
	}
	return Kind != EWacomBackpackWorkspaceNavigationTargetKind::None;
}

void FWacomBackpackWorkspaceNavigationController::ReconcileTargets(
	TConstArrayView<FWacomBackpackWorkspaceNavigationTarget> InTargets)
{
	TOptional<FWacomBackpackWorkspaceNavigationTarget> Previous;
	if (const FWacomBackpackWorkspaceNavigationTarget* Focused = GetFocusedTarget())
	{
		Previous = *Focused;
	}
	Targets = TArray<FWacomBackpackWorkspaceNavigationTarget>(InTargets);
	FocusedIndex = INDEX_NONE;
	if (Previous.IsSet())
	{
		FocusedIndex = Targets.IndexOfByPredicate(
			[&Previous](const FWacomBackpackWorkspaceNavigationTarget& Candidate)
			{
				return Candidate.HasSameIdentity(Previous.GetValue());
			});
	}
	if (FocusedIndex == INDEX_NONE && Previous.IsSet() && !Targets.IsEmpty())
	{
		float BestDistanceSquared = TNumericLimits<float>::Max();
		for (int32 Index = 0; Index < Targets.Num(); ++Index)
		{
			const float DistanceSquared = FVector2D::DistSquared(
				Targets[Index].Center, Previous.GetValue().Center);
			if (DistanceSquared < BestDistanceSquared
				|| (FMath::IsNearlyEqual(DistanceSquared, BestDistanceSquared)
					&& Targets[Index].LayerRank > Targets[FocusedIndex].LayerRank))
			{
				FocusedIndex = Index;
				BestDistanceSquared = DistanceSquared;
			}
		}
	}
	if (FocusedIndex == INDEX_NONE && !Targets.IsEmpty())
	{
		FocusedIndex = 0;
	}
}

bool FWacomBackpackWorkspaceNavigationController::Move(EUINavigation Direction)
{
	bSemanticFocusActive = true;
	if (Targets.IsEmpty())
	{
		FocusedIndex = INDEX_NONE;
		return false;
	}
	if (!Targets.IsValidIndex(FocusedIndex))
	{
		FocusedIndex = 0;
		return true;
	}

	FVector2D Axis = FVector2D::ZeroVector;
	switch (Direction)
	{
	case EUINavigation::Left: Axis = FVector2D(-1.0f, 0.0f); break;
	case EUINavigation::Right: Axis = FVector2D(1.0f, 0.0f); break;
	case EUINavigation::Up: Axis = FVector2D(0.0f, -1.0f); break;
	case EUINavigation::Down: Axis = FVector2D(0.0f, 1.0f); break;
	default: return false;
	}

	const FVector2D Origin = Targets[FocusedIndex].Center;
	int32 BestIndex = INDEX_NONE;
	float BestScore = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Targets.Num(); ++Index)
	{
		if (Index == FocusedIndex)
		{
			continue;
		}
		const FVector2D Delta = Targets[Index].Center - Origin;
		const float Forward = FVector2D::DotProduct(Delta, Axis);
		if (Forward <= UE_KINDA_SMALL_NUMBER)
		{
			continue;
		}
		const float Perpendicular = FMath::Abs(Delta.X * Axis.Y - Delta.Y * Axis.X);
		const float Score = Forward + Perpendicular * 2.5f;
		if (BestIndex == INDEX_NONE || Score < BestScore
			|| (FMath::IsNearlyEqual(Score, BestScore)
				&& Targets[Index].LayerRank > Targets[BestIndex].LayerRank))
		{
			BestScore = Score;
			BestIndex = Index;
		}
	}
	if (BestIndex == INDEX_NONE)
	{
		return false;
	}
	FocusedIndex = BestIndex;
	return true;
}

void FWacomBackpackWorkspaceNavigationController::Clear()
{
	Targets.Reset();
	FocusedIndex = INDEX_NONE;
	bSemanticFocusActive = false;
}

const FWacomBackpackWorkspaceNavigationTarget*
FWacomBackpackWorkspaceNavigationController::GetFocusedTarget() const
{
	return Targets.IsValidIndex(FocusedIndex) ? &Targets[FocusedIndex] : nullptr;
}

bool FWacomBackpackWorkspaceNavigationController::IsCardFocused(FGuid InstanceId) const
{
	const FWacomBackpackWorkspaceNavigationTarget* Target = GetFocusedTarget();
	return Target
		&& Target->Kind == EWacomBackpackWorkspaceNavigationTargetKind::Card
		&& Target->InstanceId == InstanceId;
}
