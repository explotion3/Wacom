// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomRunPathBranchSelectionController.h"

#include "Actors/WacomRunPathBranchTargetActor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WacomPlayerController.h"

#define LOCTEXT_NAMESPACE "WacomRunPathBranchSelectionController"

namespace
{
	struct FOrderedBranchTarget
	{
		TWeakObjectPtr<AWacomRunPathBranchTargetActor> Target;
		float SignedAngle = 0.0f;
	};

	FString StableEdgeKey(const AWacomRunPathBranchTargetActor* Target)
	{
		return Target ? Target->EdgeId.ToString() : FString();
	}
}

void FWacomRunPathBranchSelectionController::Initialize(
	AWacomPlayerController& InOwner,
	const TArray<TWeakObjectPtr<AWacomRunPathBranchTargetActor>>& InTargets)
{
	Shutdown();
	Owner = &InOwner;
	AllTargets = InTargets;
	HideAllTargets();
}

void FWacomRunPathBranchSelectionController::Shutdown()
{
	HideAllTargets();
	Owner.Reset();
	AllTargets.Reset();
	OrderedChoiceTargets.Reset();
	RouteChoiceState = FWacomRunRouteChoiceState();
	FocusedIndex = INDEX_NONE;
	bPresentationEnabled = false;
	bPresentationValid = false;
}

void FWacomRunPathBranchSelectionController::ApplyRouteChoiceState(
	const FWacomRunRouteChoiceState& InState)
{
	if (RouteChoiceState == InState)
	{
		ApplyTargetPresentation();
		return;
	}

	RouteChoiceState = InState;
	FocusedIndex = INDEX_NONE;
	RebuildChoiceTargets();
	ApplyTargetPresentation();
}

void FWacomRunPathBranchSelectionController::SetPresentationEnabled(
	const bool bEnabled)
{
	if (bPresentationEnabled == bEnabled)
	{
		return;
	}
	bPresentationEnabled = bEnabled;
	ApplyTargetPresentation();
}

void FWacomRunPathBranchSelectionController::TickPointerHover()
{
	if (!IsChoiceRequired() || !bPresentationValid)
	{
		return;
	}

	AWacomPlayerController* StrongOwner = Owner.Get();
	if (!StrongOwner)
	{
		return;
	}

	FHitResult HitResult;
	if (!StrongOwner->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		return;
	}

	AWacomRunPathBranchTargetActor* HitTarget =
		Cast<AWacomRunPathBranchTargetActor>(HitResult.GetActor());
	if (!HitTarget && HitResult.GetComponent())
	{
		HitTarget = Cast<AWacomRunPathBranchTargetActor>(
			HitResult.GetComponent()->GetOwner());
	}
	const int32 HitIndex = FindTargetIndex(HitTarget);
	if (HitIndex != INDEX_NONE && HitIndex != FocusedIndex)
	{
		FocusedIndex = HitIndex;
		ApplyTargetPresentation();
	}
}

bool FWacomRunPathBranchSelectionController::ShiftFocus(const int32 Direction)
{
	if (!IsChoiceRequired() || !bPresentationValid
		|| OrderedChoiceTargets.IsEmpty() || Direction == 0)
	{
		return false;
	}

	const int32 PreviousIndex = FocusedIndex;
	FocusedIndex = FMath::Clamp(
		FocusedIndex + FMath::Sign(Direction),
		0,
		OrderedChoiceTargets.Num() - 1);
	if (FocusedIndex != PreviousIndex)
	{
		ApplyTargetPresentation();
	}
	return true;
}

bool FWacomRunPathBranchSelectionController::ConfirmFocused()
{
	if (!IsChoiceRequired() || !bPresentationValid
		|| !OrderedChoiceTargets.IsValidIndex(FocusedIndex))
	{
		return false;
	}
	AWacomRunPathBranchTargetActor* Target =
		OrderedChoiceTargets[FocusedIndex].Get();
	return Target && Target->RequestBranch();
}

bool FWacomRunPathBranchSelectionController::TrySelectHitActor(AActor* HitActor)
{
	if (!IsChoiceRequired() || !bPresentationValid)
	{
		return false;
	}

	AWacomRunPathBranchTargetActor* Target =
		Cast<AWacomRunPathBranchTargetActor>(HitActor);
	const int32 TargetIndex = FindTargetIndex(Target);
	if (TargetIndex == INDEX_NONE)
	{
		PulseAvailable();
		return false;
	}
	FocusedIndex = TargetIndex;
	ApplyTargetPresentation();
	return Target->RequestBranch();
}

void FWacomRunPathBranchSelectionController::PulseAvailable()
{
	if (!IsChoiceRequired() || !bPresentationValid)
	{
		return;
	}
	for (const TWeakObjectPtr<AWacomRunPathBranchTargetActor>& Candidate :
		OrderedChoiceTargets)
	{
		if (AWacomRunPathBranchTargetActor* Target = Candidate.Get())
		{
			Target->PlayAttentionPulse();
		}
	}
}

bool FWacomRunPathBranchSelectionController::IsChoiceRequired() const
{
	return bPresentationEnabled
		&& RouteChoiceState.Mode == EWacomRunRouteChoiceMode::ChoiceRequired;
}

FName FWacomRunPathBranchSelectionController::GetFocusedEdgeId() const
{
	if (!OrderedChoiceTargets.IsValidIndex(FocusedIndex))
	{
		return NAME_None;
	}
	const AWacomRunPathBranchTargetActor* Target =
		OrderedChoiceTargets[FocusedIndex].Get();
	return Target ? Target->EdgeId : NAME_None;
}

FText FWacomRunPathBranchSelectionController::BuildInteractionPrompt() const
{
	return IsChoiceRequired()
		? LOCTEXT(
			"RouteChoicePrompt",
			"A / D 或左摇杆：选择道路    E / 手柄 A：确认")
		: FText::GetEmpty();
}

void FWacomRunPathBranchSelectionController::RebuildChoiceTargets()
{
	OrderedChoiceTargets.Reset();
	bPresentationValid = false;

	if (RouteChoiceState.Mode != EWacomRunRouteChoiceMode::ChoiceRequired)
	{
		return;
	}

	TMap<FName, AWacomRunPathBranchTargetActor*> TargetsByEdge;
	TSet<FName> DuplicateEdges;
	for (const TWeakObjectPtr<AWacomRunPathBranchTargetActor>& Candidate : AllTargets)
	{
		AWacomRunPathBranchTargetActor* Target = Candidate.Get();
		if (!Target || Target->EdgeId.IsNone())
		{
			continue;
		}
		if (TargetsByEdge.Contains(Target->EdgeId))
		{
			DuplicateEdges.Add(Target->EdgeId);
			continue;
		}
		TargetsByEdge.Add(Target->EdgeId, Target);
	}

	TArray<FOrderedBranchTarget> Ordered;
	const AWacomPlayerController* StrongOwner = Owner.Get();
	const APawn* Pawn = StrongOwner ? StrongOwner->GetPawn() : nullptr;
	const FVector Origin = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	const FVector Forward = Pawn ? Pawn->GetActorForwardVector() : FVector::ForwardVector;
	const FVector Right = Pawn ? Pawn->GetActorRightVector() : FVector::RightVector;

	for (const FName EdgeId : RouteChoiceState.LegalEdgeIds)
	{
		AWacomRunPathBranchTargetActor* const* Found = TargetsByEdge.Find(EdgeId);
		if (!Found || !*Found || DuplicateEdges.Contains(EdgeId))
		{
			OrderedChoiceTargets.Reset();
			return;
		}

		const FVector Direction = ((*Found)->GetActorLocation() - Origin).GetSafeNormal2D();
		const float SignedAngle = FMath::Atan2(
			FVector::DotProduct(Direction, Right),
			FVector::DotProduct(Direction, Forward));
		Ordered.Add({ *Found, SignedAngle });
	}

	Ordered.Sort([](const FOrderedBranchTarget& A, const FOrderedBranchTarget& B)
	{
		if (!FMath::IsNearlyEqual(A.SignedAngle, B.SignedAngle, KINDA_SMALL_NUMBER))
		{
			return A.SignedAngle < B.SignedAngle;
		}
		return StableEdgeKey(A.Target.Get()) < StableEdgeKey(B.Target.Get());
	});
	for (const FOrderedBranchTarget& Item : Ordered)
	{
		OrderedChoiceTargets.Add(Item.Target);
	}

	bPresentationValid = OrderedChoiceTargets.Num()
		== RouteChoiceState.LegalEdgeIds.Num();
	FocusedIndex = ChooseInitialFocusIndex();
}

void FWacomRunPathBranchSelectionController::ApplyTargetPresentation()
{
	HideAllTargets();
	if (!IsChoiceRequired() || !bPresentationValid)
	{
		return;
	}
	for (int32 Index = 0; Index < OrderedChoiceTargets.Num(); ++Index)
	{
		if (AWacomRunPathBranchTargetActor* Target =
			OrderedChoiceTargets[Index].Get())
		{
			Target->SetPresentationState(
				Index == FocusedIndex
					? EWacomRunPathBranchPresentationState::Focused
					: EWacomRunPathBranchPresentationState::Available);
		}
	}
}

void FWacomRunPathBranchSelectionController::HideAllTargets()
{
	for (const TWeakObjectPtr<AWacomRunPathBranchTargetActor>& Candidate : AllTargets)
	{
		if (AWacomRunPathBranchTargetActor* Target = Candidate.Get())
		{
			Target->SetPresentationState(
				EWacomRunPathBranchPresentationState::Hidden);
		}
	}
}

int32 FWacomRunPathBranchSelectionController::FindTargetIndex(
	const AWacomRunPathBranchTargetActor* Target) const
{
	return OrderedChoiceTargets.IndexOfByPredicate(
		[Target](const TWeakObjectPtr<AWacomRunPathBranchTargetActor>& Candidate)
		{
			return Candidate.Get() == Target;
		});
}

int32 FWacomRunPathBranchSelectionController::ChooseInitialFocusIndex() const
{
	if (OrderedChoiceTargets.IsEmpty())
	{
		return INDEX_NONE;
	}

	const AWacomPlayerController* StrongOwner = Owner.Get();
	const APawn* Pawn = StrongOwner ? StrongOwner->GetPawn() : nullptr;
	const FVector Origin = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	const FVector Forward = Pawn ? Pawn->GetActorForwardVector() : FVector::ForwardVector;

	int32 BestIndex = 0;
	float BestForwardScore = -BIG_NUMBER;
	FString BestEdgeKey;
	for (int32 Index = 0; Index < OrderedChoiceTargets.Num(); ++Index)
	{
		const AWacomRunPathBranchTargetActor* Target =
			OrderedChoiceTargets[Index].Get();
		const FVector Direction = Target
			? (Target->GetActorLocation() - Origin).GetSafeNormal2D()
			: FVector::ZeroVector;
		const float ForwardScore = FVector::DotProduct(Direction, Forward);
		const FString EdgeKey = StableEdgeKey(Target);
		if (ForwardScore > BestForwardScore + KINDA_SMALL_NUMBER
			|| (FMath::IsNearlyEqual(ForwardScore, BestForwardScore)
				&& (BestEdgeKey.IsEmpty() || EdgeKey < BestEdgeKey)))
		{
			BestIndex = Index;
			BestForwardScore = ForwardScore;
			BestEdgeKey = EdgeKey;
		}
	}
	return BestIndex;
}

#undef LOCTEXT_NAMESPACE
