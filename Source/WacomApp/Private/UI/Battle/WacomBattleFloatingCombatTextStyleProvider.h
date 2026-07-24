// Copyright Wacom. All Rights Reserved.

#pragma once

class UWacomBattleFloatingCombatTextStyle;

namespace WacomBattleFloatingCombatTextStyleProvider
{
	WACOMAPP_API const UWacomBattleFloatingCombatTextStyle& GetStyle();

#if WITH_AUTOMATION_TESTS
	WACOMAPP_API void ClearCachedStyleForTests();
#endif
}
