// Copyright Wacom. All Rights Reserved.

#include "UI/WacomRunMapScreenFlowTestAccess.h"

#if WITH_DEV_AUTOMATION_TESTS

bool FWacomRunMapScreenFlowTestAccess::Attach(
	FWacomRunMapScreenFlowAutomationTestView& View,
	URunSession& Session,
	UWacomRunMapScreen& Screen,
	const bool bPreferRecommended,
	const int32 RequestGeneration)
{
	return View.AttachScreen(
		Session, Screen, bPreferRecommended, RequestGeneration);
}

#endif
