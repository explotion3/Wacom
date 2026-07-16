// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackCardPresentationController.h"

#include "Components/CanvasPanel.h"
#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "UI/Backpack/WacomDeckCardWidget.h"

void FWacomBackpackCardPresentationController::Reconcile(
	TConstArrayView<TWeakObjectPtr<UWacomDeckCardWidget>> Cards,
	FGuid HoveredInstanceId,
	const FWacomBackpackWorkspaceCarryState* Carry,
	const UCanvasPanel* CarryLayer,
	const FGeometry& WorkspaceGeometry,
	FVector2D PointerLocal)
{
	UWacomDeckCardWidget* Desired = nullptr;
	bool bDesiredCarrying = false;
	if (Carry && CarryLayer && Carry->RemainingInstanceIds.IsValidIndex(Carry->CurrentIndex))
	{
		const FGuid CurrentId = Carry->RemainingInstanceIds[Carry->CurrentIndex];
		for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : Cards)
		{
			UWacomDeckCardWidget* Card = WeakCard.Get();
			if (Card && Card->GetParent() == CarryLayer && Card->GetCardInstanceId() == CurrentId)
			{
				Desired = Card;
				bDesiredCarrying = true;
				break;
			}
		}
	}
	if (!Desired && HoveredInstanceId.IsValid())
	{
		for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : Cards)
		{
			UWacomDeckCardWidget* Card = WeakCard.Get();
			if (Card && Card->GetCardInstanceId() == HoveredInstanceId
				&& Card->IsWorkspaceInteractionEnabled()
				&& Card->GetWorkspaceReadOnlyKind() == EWacomBackpackWorkspaceCardReadOnlyKind::None)
			{
				Desired = Card;
				break;
			}
		}
	}

	if (ActiveCard.Get() != Desired)
	{
		if (UWacomDeckCardWidget* Previous = ActiveCard.Get())
		{
			Previous->SetBackpackRealtimePresentation(false, FVector2D::ZeroVector, false);
		}
		ActiveCard = Desired;
	}
	bActiveCardCarrying = bDesiredCarrying;
	ApplyActivePointer(WorkspaceGeometry, PointerLocal);
}

void FWacomBackpackCardPresentationController::UpdatePointer(
	const FGeometry& WorkspaceGeometry,
	FVector2D PointerLocal,
	bool bCarrying)
{
	// Carry presentation is deliberately pointer-independent: the fan follows the
	// shared CarryLayer anchor while the front card keeps a neutral tilt. Rewriting
	// the Retainer material for every high-frequency mouse event only starves Slate.
	if (bCarrying && bActiveCardCarrying)
	{
		return;
	}
	bActiveCardCarrying = bCarrying;
	ApplyActivePointer(WorkspaceGeometry, PointerLocal);
}

void FWacomBackpackCardPresentationController::Reset()
{
	if (UWacomDeckCardWidget* Previous = ActiveCard.Get())
	{
		Previous->SetBackpackRealtimePresentation(false, FVector2D::ZeroVector, false);
	}
	ActiveCard.Reset();
	bActiveCardCarrying = false;
}

void FWacomBackpackCardPresentationController::ApplyActivePointer(
	const FGeometry& WorkspaceGeometry,
	FVector2D PointerLocal)
{
	UWacomDeckCardWidget* Card = ActiveCard.Get();
	if (!Card)
	{
		return;
	}
	FVector2D NormalizedPointer = FVector2D::ZeroVector;
	if (!bActiveCardCarrying)
	{
		const FVector2D AbsolutePointer = WorkspaceGeometry.LocalToAbsolute(PointerLocal);
		const FGeometry& CardGeometry = Card->GetCachedGeometry();
		const FVector2D CardSize = CardGeometry.GetLocalSize();
		if (CardSize.X > 1.0f && CardSize.Y > 1.0f)
		{
			const FVector2D CardLocal = CardGeometry.AbsoluteToLocal(AbsolutePointer);
			NormalizedPointer = FVector2D(
				FMath::Clamp(CardLocal.X / CardSize.X * 2.0f - 1.0f, -1.0f, 1.0f),
				FMath::Clamp(CardLocal.Y / CardSize.Y * 2.0f - 1.0f, -1.0f, 1.0f));
		}
	}
	Card->SetBackpackRealtimePresentation(true, NormalizedPointer, bActiveCardCarrying);
}
