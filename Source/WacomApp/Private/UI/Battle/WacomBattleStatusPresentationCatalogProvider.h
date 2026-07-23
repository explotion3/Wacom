// Copyright Wacom. All Rights Reserved.

#pragma once

class UWacomBattleStatusPresentationCatalog;

namespace WacomBattleStatusPresentationCatalogProvider
{
	WACOMAPP_API const UWacomBattleStatusPresentationCatalog& GetCatalog();

#if WITH_AUTOMATION_TESTS
	WACOMAPP_API void ClearCachedCatalogForTests();
#endif
}
