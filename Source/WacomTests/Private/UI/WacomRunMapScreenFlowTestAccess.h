// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Testing/WacomRunMapScreenFlowAutomationTestView.h"

#if WITH_DEV_AUTOMATION_TESTS

/** Tests-private wrapper，避免测试直接依赖 WacomApp Private flow 类型。 */
struct FWacomRunMapScreenFlowTestAccess
{
	static bool Attach(
		FWacomRunMapScreenFlowAutomationTestView& View,
		URunSession& Session,
		UWacomRunMapScreen& Screen,
		bool bPreferRecommended = false,
		int32 RequestGeneration = 0);
};

#endif
