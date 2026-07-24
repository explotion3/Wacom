// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceNavigationController.h"

#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "UI/Backpack/WacomBackpackWorkspaceRuntimeHost.h"
#include "UI/Backpack/WacomDeckCardWidget.h"

namespace
{
TArray<FGuid> BuildChangedNavigationInstanceIds(
	const TConstArrayView<FGuid> Before,
	const TConstArrayView<FGuid> After)
{
	TArray<FGuid> Changed;
	for (const FGuid InstanceId : Before)
	{
		if (!After.Contains(InstanceId))
		{
			Changed.Add(InstanceId);
		}
	}
	for (const FGuid InstanceId : After)
	{
		if (!Before.Contains(InstanceId))
		{
			Changed.AddUnique(InstanceId);
		}
	}
	return Changed;
}
}

EWacomBackpackWorkspaceInputReply
FWacomBackpackWorkspaceNavigationController::HandleKeyDown(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	const FKeyEvent& Event)
{
	if (!Host.IsValid())
	{
		return EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	const FKey Key = Event.GetKey();
	if (Key == EKeys::F1)
	{
		Host.BroadcastControlsHelpRequested();
		return EWacomBackpackWorkspaceInputReply::Handled;
	}
	if (Key == EKeys::Enter
		|| Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		return HandlePrimary(Host, false)
			? EWacomBackpackWorkspaceInputReply::Handled
			: EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	if (Key == EKeys::SpaceBar
		|| Key == EKeys::Gamepad_FaceButton_Left)
	{
		return HandleSelection(Host)
			? EWacomBackpackWorkspaceInputReply::Handled
			: EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	if (Key == EKeys::T
		|| Key == EKeys::Gamepad_FaceButton_Top)
	{
		return HandleContextAction(Host)
			? EWacomBackpackWorkspaceInputReply::Handled
			: EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	if (Key == EKeys::Q || Key == EKeys::Gamepad_LeftShoulder)
	{
		return StepCarriedCard(Host, -1)
			? EWacomBackpackWorkspaceInputReply::Handled
			: EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	if (Key == EKeys::E || Key == EKeys::Gamepad_RightShoulder)
	{
		return StepCarriedCard(Host, 1)
			? EWacomBackpackWorkspaceInputReply::Handled
			: EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	if (Key == EKeys::LeftShift)
	{
		Host.SetExpandedPileLensInputLocked(true, false);
		return Host.IsExpandedPileLensInputLocked()
			? EWacomBackpackWorkspaceInputReply::Handled
			: EWacomBackpackWorkspaceInputReply::Unhandled;
	}
	if (Event.IsControlDown() && Key == EKeys::A
		&& SelectAllMovable(Host))
	{
		return EWacomBackpackWorkspaceInputReply::Handled;
	}
	if (Key == EKeys::Escape
		|| Key == EKeys::Gamepad_FaceButton_Right)
	{
		if (Host.HasCancelableInteraction())
		{
			Host.CancelInteraction(true);
			Host.BroadcastInteractionChanged();
			return EWacomBackpackWorkspaceInputReply::Handled;
		}
		if (Host.HasExpandedContent())
		{
			Host.BroadcastCollapseExpandedPileRequested();
			return EWacomBackpackWorkspaceInputReply::Handled;
		}
	}
	return EWacomBackpackWorkspaceInputReply::Unhandled;
}

EWacomBackpackWorkspaceInputReply
FWacomBackpackWorkspaceNavigationController::HandleKeyUp(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	const FKeyEvent& Event)
{
	if (Host.IsValid() && Event.GetKey() == EKeys::LeftShift
		&& Host.IsExpandedPileLensInputLocked())
	{
		Host.SetExpandedPileLensInputLocked(false, true);
		return EWacomBackpackWorkspaceInputReply::Handled;
	}
	return EWacomBackpackWorkspaceInputReply::Unhandled;
}

bool FWacomBackpackWorkspaceNavigationController::HandleNavigation(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	const FNavigationEvent& Event)
{
	if (!Host.IsValid())
	{
		return false;
	}
	Host.ReconcileNavigationTargetsForInput();
	TArray<FGuid> ChangedInstanceIds;
	if (const FWacomBackpackWorkspaceNavigationTarget* Previous =
		GetFocusedTarget())
	{
		if (Previous->Kind
			== EWacomBackpackWorkspaceNavigationTargetKind::Card)
		{
			ChangedInstanceIds.Add(Previous->InstanceId);
		}
	}
	if (!Move(Event.GetNavigationType()))
	{
		return false;
	}
	if (const FWacomBackpackWorkspaceNavigationTarget* Current =
		GetFocusedTarget())
	{
		if (Current->Kind
			== EWacomBackpackWorkspaceNavigationTargetKind::Card)
		{
			ChangedInstanceIds.AddUnique(Current->InstanceId);
		}
	}
	Host.NotifyNavigationMoved(ChangedInstanceIds);
	return true;
}

void FWacomBackpackWorkspaceNavigationController::HandleFocusLost(
	FWacomBackpackWorkspaceRuntimeHost& Host)
{
	if (Host.IsValid())
	{
		Host.SetExpandedPileLensInputLocked(false, false);
	}
}

bool FWacomBackpackWorkspaceNavigationController::HandlePrimary(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	const bool bReleaseAll)
{
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	if (!Model || Host.IsCarryInputSuspended())
	{
		return false;
	}
	Host.ReconcileNavigationTargetsForInput();
	ActivateSemanticFocus();
	const FWacomBackpackWorkspaceNavigationTarget* Target =
		GetFocusedTarget();
	if (!Target)
	{
		return false;
	}
	if (Model->IsCarrying())
	{
		EWacomBackpackWorkspaceReleaseTargetKind TargetKind;
		FWacomBackpackZoneKey TargetZone;
		if (!GetFocusedReleaseTarget(
			true,
			TargetKind,
			TargetZone))
		{
			return false;
		}
		Host.BroadcastRelease(
			bReleaseAll,
			TargetKind,
			TargetZone);
		return true;
	}
	if (Target->Kind
		== EWacomBackpackWorkspaceNavigationTargetKind::Pile)
	{
		Host.BroadcastPileExpansion(Target->Zone);
		return true;
	}
	if (Target->Kind
			!= EWacomBackpackWorkspaceNavigationTargetKind::Card
		|| !Target->bActionable
		|| !Model->BeginCarry(
			Target->InstanceId,
			Target->Center,
			Host.GetCurrentStorageRevision()))
	{
		return false;
	}
	Model->NotifyReleaseGestureStarted();
	Host.NotifyCarryStarted(
		Target->Center,
		Model->GetCarry().RemainingInstanceIds);
	return true;
}

bool FWacomBackpackWorkspaceNavigationController::HandleSelection(
	FWacomBackpackWorkspaceRuntimeHost& Host)
{
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	if (!Model || Model->IsCarrying())
	{
		return false;
	}
	Host.ReconcileNavigationTargetsForInput();
	const FWacomBackpackWorkspaceNavigationTarget* Target =
		GetFocusedTarget();
	if (!Target
		|| Target->Kind
			!= EWacomBackpackWorkspaceNavigationTargetKind::Card
		|| !Target->bActionable)
	{
		return false;
	}
	const FGuid InstanceId = Target->InstanceId;
	Model->ClickCard(InstanceId, true);
	Host.UpdateSelectionVisualFreezeLifetime();
	Host.NotifySelectionChanged(
		MakeArrayView(&InstanceId, 1));
	return true;
}

bool FWacomBackpackWorkspaceNavigationController::HandleContextAction(
	FWacomBackpackWorkspaceRuntimeHost& Host)
{
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	if (!Model)
	{
		return false;
	}
	if (Model->IsCarrying())
	{
		return HandlePrimary(Host, true);
	}
	Host.ReconcileNavigationTargetsForInput();
	const FWacomBackpackWorkspaceNavigationTarget* Target =
		GetFocusedTarget();
	if (!Target
		|| Target->Kind
			!= EWacomBackpackWorkspaceNavigationTargetKind::Card)
	{
		return false;
	}
	UWacomDeckCardWidget* Card =
		Host.FindBoundCard(Target->InstanceId);
	return Card && Card->RequestBattleEnabledToggle();
}

bool FWacomBackpackWorkspaceNavigationController::StepCarriedCard(
	FWacomBackpackWorkspaceRuntimeHost& Host,
	const int32 Direction)
{
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	if (!Model || !Model->IsCarrying()
		|| Host.IsCarryInputSuspended() || Direction == 0)
	{
		return false;
	}
	TArray<FGuid> ChangedInstanceIds;
	const int32 PreviousIndex = Model->GetCarry().CurrentIndex;
	if (Model->GetCarry().RemainingInstanceIds.IsValidIndex(
		PreviousIndex))
	{
		const FGuid PreviousId =
			Model->GetCarry().RemainingInstanceIds[PreviousIndex];
		ChangedInstanceIds.Add(PreviousId);
		Host.RememberPreviousCarryCurrentCard(PreviousId);
	}
	Model->StepCurrentByWheel(Direction < 0 ? 1.0f : -1.0f);
	if (Model->GetCarry().CurrentIndex == PreviousIndex)
	{
		return true;
	}
	if (Model->GetCarry().RemainingInstanceIds.IsValidIndex(
		Model->GetCarry().CurrentIndex))
	{
		ChangedInstanceIds.AddUnique(
			Model->GetCarry().RemainingInstanceIds[
				Model->GetCarry().CurrentIndex]);
	}
	Host.NotifyCarryCurrentChanged(
		ChangedInstanceIds,
		false,
		true);
	return true;
}

bool FWacomBackpackWorkspaceNavigationController::SelectAllMovable(
	FWacomBackpackWorkspaceRuntimeHost& Host)
{
	FWacomBackpackWorkspaceInteractionModel* Model =
		Host.GetInteractionModel();
	if (!Model)
	{
		return false;
	}
	const TArray<FGuid> Previous =
		Model->GetSelection().OrderedSelectedInstanceIds;
	if (Model->GetSelection().bHasSourceZone)
	{
		Host.BeginSelectionVisualFreeze(
			Model->GetSelection().SourceZone);
	}
	Model->SelectAllMovable();
	Host.UpdateSelectionVisualFreezeLifetime();
	Host.NotifySelectionChanged(BuildChangedNavigationInstanceIds(
		Previous,
		Model->GetSelection().OrderedSelectedInstanceIds));
	return true;
}

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

bool FWacomBackpackWorkspaceNavigationController::
	GetFocusedReleaseTarget(
		const bool bIsCarrying,
		EWacomBackpackWorkspaceReleaseTargetKind& OutKind,
		FWacomBackpackZoneKey& OutZone) const
{
	const FWacomBackpackWorkspaceNavigationTarget* Target =
		GetFocusedTarget();
	if (!Target || !bSemanticFocusActive || !bIsCarrying)
	{
		return false;
	}
	OutZone = Target->Zone;
	switch (Target->Kind)
	{
	case EWacomBackpackWorkspaceNavigationTargetKind::Flux:
		OutKind =
			EWacomBackpackWorkspaceReleaseTargetKind::Flux;
		return true;
	case EWacomBackpackWorkspaceNavigationTargetKind::Pile:
		OutKind =
			EWacomBackpackWorkspaceReleaseTargetKind::Pile;
		return OutZone.IsValid();
	case EWacomBackpackWorkspaceNavigationTargetKind::Delete:
		OutKind =
			EWacomBackpackWorkspaceReleaseTargetKind::Delete;
		return true;
	default:
		return false;
	}
}
