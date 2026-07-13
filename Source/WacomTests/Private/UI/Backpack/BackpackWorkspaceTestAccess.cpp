// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/BackpackWorkspaceTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"

FGuid FWacomBackpackWorkspaceTestAccess::CurrentCarriedInstanceId(
	const FWacomBackpackWorkspaceAutomationTestView& View)
{
	return View.CarriedInstanceIds.IsValidIndex(View.CurrentCarryIndex)
		? View.CarriedInstanceIds[View.CurrentCarryIndex]
		: FGuid();
}

bool FWacomBackpackWorkspaceTestAccess::HasStableCarryIndices(
	const FWacomBackpackWorkspaceAutomationTestView& View)
{
	if (View.CarriedInstanceIds.IsEmpty())
	{
		return View.CurrentCarryIndex == INDEX_NONE && View.DefaultCarryIndex == INDEX_NONE;
	}

	return View.CarriedInstanceIds.IsValidIndex(View.CurrentCarryIndex)
		&& View.CarriedInstanceIds.IsValidIndex(View.DefaultCarryIndex);
}

#endif
