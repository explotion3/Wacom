// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunState.h"

class USizeBox;
class UVerticalBox;
class UWacomRunEventChoiceButton;
class UWacomRunMenuDropTargetWidget;

struct FWacomRunEventChoiceListReconcileContext
{
	UVerticalBox* ChoiceList = nullptr;
	float PaymentChoiceMinDesiredWidth = 0.0f;
	TArray<TObjectPtr<UWacomRunEventChoiceButton>>* ChoiceButtonWidgets = nullptr;
	TArray<TObjectPtr<UWacomRunMenuDropTargetWidget>>* PaymentDropTargets = nullptr;
	TMap<FName, FName>* PaymentZoneToChoiceId = nullptr;
};

struct FWacomRunEventChoiceListReconciler
{
	static void Reconcile(
		const FWacomRunEventChoiceListReconcileContext& Context,
		TConstArrayView<FRunEventChoiceSnapshot> DesiredChoices,
		TFunctionRef<UWacomRunEventChoiceButton*(const FRunEventChoiceSnapshot&)> CreateChoiceButton,
		TFunctionRef<UWacomRunMenuDropTargetWidget*(const FRunEventChoiceSnapshot&)> CreatePaymentDropTarget,
		TFunctionRef<USizeBox*(const FRunEventChoiceSnapshot&)> CreatePaymentSizeBox,
		TFunctionRef<void(UWacomRunEventChoiceButton&, const FRunEventChoiceSnapshot&)> ApplyChoiceButton);
};
