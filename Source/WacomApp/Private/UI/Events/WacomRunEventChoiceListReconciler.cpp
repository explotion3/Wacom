// Copyright Wacom. All Rights Reserved.

#include "UI/Events/WacomRunEventChoiceListReconciler.h"

#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Events/WacomRunEventChoiceButton.h"
#include "UI/Run/WacomRunMenuDropTargetWidget.h"

namespace
{
void PlaceRunEventChoiceWidget(UVerticalBox& Panel, UWidget& Widget, int32 DesiredIndex)
{
	if (Widget.GetParent() == &Panel)
	{
		Panel.ShiftChild(DesiredIndex, &Widget);
	}
	else
	{
		Widget.RemoveFromParent();
		if (UVerticalBoxSlot* ChoiceSlot = Panel.AddChildToVerticalBox(&Widget))
		{
			ChoiceSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
			Panel.ShiftChild(DesiredIndex, &Widget);
			return;
		}
	}

	if (UVerticalBoxSlot* ChoiceSlot = Cast<UVerticalBoxSlot>(Widget.Slot))
	{
		ChoiceSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}
}

TMap<FName, UWacomRunEventChoiceButton*> BuildExistingChoiceButtonsByChoiceId(
	const TArray<TObjectPtr<UWacomRunEventChoiceButton>>& ChoiceButtonWidgets)
{
	TMap<FName, UWacomRunEventChoiceButton*> ExistingByChoiceId;
	for (UWacomRunEventChoiceButton* ChoiceButton : ChoiceButtonWidgets)
	{
		if (!ChoiceButton)
		{
			continue;
		}

		const FName ChoiceId = ChoiceButton->GetChoiceSnapshot().ChoiceId;
		if (!ChoiceId.IsNone())
		{
			ExistingByChoiceId.Add(ChoiceId, ChoiceButton);
		}
	}
	return ExistingByChoiceId;
}

TMap<FName, UWacomRunMenuDropTargetWidget*> BuildExistingDropTargetsByChoiceId(
	const TArray<TObjectPtr<UWacomRunMenuDropTargetWidget>>& PaymentDropTargets,
	const FWacomRunEventPresentationStateView& PresentationState)
{
	TMap<FName, UWacomRunMenuDropTargetWidget*> ExistingByChoiceId;
	for (UWacomRunMenuDropTargetWidget* DropTarget : PaymentDropTargets)
	{
		if (!DropTarget)
		{
			continue;
		}

		FName ChoiceId;
		if (PresentationState.FindChoiceIdForPaymentZone(DropTarget->ZoneId, ChoiceId))
		{
			ExistingByChoiceId.Add(ChoiceId, DropTarget);
		}
	}
	return ExistingByChoiceId;
}
}

void FWacomRunEventChoiceListReconciler::Reconcile(
	const FWacomRunEventChoiceListReconcileContext& Context,
	TConstArrayView<FRunEventChoiceSnapshot> DesiredChoices,
	TFunctionRef<UWacomRunEventChoiceButton*(const FRunEventChoiceSnapshot&)> CreateChoiceButton,
	TFunctionRef<UWacomRunMenuDropTargetWidget*(const FRunEventChoiceSnapshot&)> CreatePaymentDropTarget,
	TFunctionRef<USizeBox*(const FRunEventChoiceSnapshot&)> CreatePaymentSizeBox,
	TFunctionRef<void(UWacomRunEventChoiceButton&, const FRunEventChoiceSnapshot&)> ApplyChoiceButton)
{
	if (!Context.ChoiceList
		|| !Context.ChoiceButtonWidgets
		|| !Context.PaymentDropTargets
		|| !Context.PresentationState.IsValid())
	{
		if (Context.ChoiceButtonWidgets)
		{
			Context.ChoiceButtonWidgets->Reset();
		}
		if (Context.PaymentDropTargets)
		{
			Context.PaymentDropTargets->Reset();
		}
		Context.PresentationState.ResetPaymentZoneMappings();
		return;
	}

	TMap<FName, UWacomRunEventChoiceButton*> ExistingChoiceButtons =
		BuildExistingChoiceButtonsByChoiceId(*Context.ChoiceButtonWidgets);
	TMap<FName, UWacomRunMenuDropTargetWidget*> ExistingDropTargets =
		BuildExistingDropTargetsByChoiceId(
			*Context.PaymentDropTargets,
			Context.PresentationState.AsView());

	Context.ChoiceButtonWidgets->Reset();
	Context.ChoiceButtonWidgets->Reserve(DesiredChoices.Num());
	Context.PaymentDropTargets->Reset();
	Context.PresentationState.ResetPaymentZoneMappings();

	TSet<UWacomRunEventChoiceButton*> UsedChoiceButtons;
	TSet<UWacomRunMenuDropTargetWidget*> UsedDropTargets;
	for (int32 DesiredIndex = 0; DesiredIndex < DesiredChoices.Num(); ++DesiredIndex)
	{
		const FRunEventChoiceSnapshot& Choice = DesiredChoices[DesiredIndex];
		UWacomRunEventChoiceButton* ChoiceButton = ExistingChoiceButtons.FindRef(Choice.ChoiceId);
		if (!ChoiceButton)
		{
			ChoiceButton = CreateChoiceButton(Choice);
		}
		if (!ChoiceButton)
		{
			continue;
		}

		ApplyChoiceButton(*ChoiceButton, Choice);
		Context.ChoiceButtonWidgets->Add(ChoiceButton);
		UsedChoiceButtons.Add(ChoiceButton);

		UWidget* WidgetToPlace = ChoiceButton;
		if (Choice.bRequiresOwnedCardPayment && !Choice.PaymentZoneId.IsNone())
		{
			UWacomRunMenuDropTargetWidget* DropTarget = ExistingDropTargets.FindRef(Choice.ChoiceId);
			if (!DropTarget)
			{
				DropTarget = CreatePaymentDropTarget(Choice);
			}
			if (DropTarget)
			{
				DropTarget->ZoneId = Choice.PaymentZoneId;
				DropTarget->StableTargetId = Choice.PaymentZoneId;

				USizeBox* ChoiceSize = CreatePaymentSizeBox(Choice);
				if (ChoiceSize)
				{
					ChoiceSize->SetMinDesiredWidth(Context.PaymentChoiceMinDesiredWidth);
					ChoiceSize->SetContent(ChoiceButton);
					DropTarget->SetDropContent(ChoiceSize);
				}

				WidgetToPlace = DropTarget;
				Context.PresentationState.AddPaymentZoneMapping(Choice.PaymentZoneId, Choice.ChoiceId);
				Context.PaymentDropTargets->Add(DropTarget);
				UsedDropTargets.Add(DropTarget);
			}
		}

		PlaceRunEventChoiceWidget(*Context.ChoiceList, *WidgetToPlace, DesiredIndex);
	}

	for (const TPair<FName, UWacomRunMenuDropTargetWidget*>& ExistingPair : ExistingDropTargets)
	{
		if (ExistingPair.Value && !UsedDropTargets.Contains(ExistingPair.Value))
		{
			ExistingPair.Value->RemoveFromParent();
		}
	}

	for (const TPair<FName, UWacomRunEventChoiceButton*>& ExistingPair : ExistingChoiceButtons)
	{
		if (ExistingPair.Value && !UsedChoiceButtons.Contains(ExistingPair.Value))
		{
			ExistingPair.Value->RemoveFromParent();
		}
	}
}
