// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomCardDetailTheme;

namespace WacomCardDetailThemeProvider
{
	const UWacomCardDetailTheme* GetConfiguredTheme();

#if WITH_AUTOMATION_TESTS
	void ClearCachedThemeForTests();
#endif
}
