// Copyright Wacom. All Rights Reserved.

#include "WacomBattleFloatingCombatTextStyleProvider.h"

#include "UI/Battle/WacomBattleFloatingCombatTextStyle.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"

#include "UObject/SoftObjectPath.h"

namespace WacomBattleFloatingCombatTextStyleProvider
{
	namespace
	{
		struct FCachedStyle
		{
			FSoftObjectPath Path;
			TWeakObjectPtr<UWacomBattleFloatingCombatTextStyle> Style;
			bool bHasPath = false;
			bool bReportedFallback = false;
		};

		FCachedStyle& GetCache()
		{
			static FCachedStyle Cache;
			return Cache;
		}

		void ClearCache(FCachedStyle& Cache)
		{
			Cache.Path.Reset();
			Cache.Style.Reset();
			Cache.bHasPath = false;
			Cache.bReportedFallback = false;
		}

		const UWacomBattleFloatingCombatTextStyle& GetFallback(
			FCachedStyle& Cache,
			const TCHAR* Reason)
		{
			if (!Cache.bReportedFallback)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[BattleFloatingCombatText] %s; using CDO fallback."),
					Reason);
				Cache.bReportedFallback = true;
			}
			return *GetDefault<UWacomBattleFloatingCombatTextStyle>();
		}
	}

	const UWacomBattleFloatingCombatTextStyle& GetStyle()
	{
		const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
		FCachedStyle& Cache = GetCache();
		if (!Settings || Settings->BattleFloatingCombatTextStyle.IsNull())
		{
			if (Cache.bHasPath)
			{
				ClearCache(Cache);
			}
			return GetFallback(Cache, TEXT("No configured style"));
		}

		const FSoftObjectPath ConfiguredPath =
			Settings->BattleFloatingCombatTextStyle.ToSoftObjectPath();
		if (Cache.bHasPath && Cache.Path == ConfiguredPath)
		{
			if (const UWacomBattleFloatingCombatTextStyle* Cached = Cache.Style.Get())
			{
				return *Cached;
			}
		}
		else
		{
			ClearCache(Cache);
			Cache.Path = ConfiguredPath;
			Cache.bHasPath = true;
		}

		UWacomBattleFloatingCombatTextStyle* Loaded =
			Cast<UWacomBattleFloatingCombatTextStyle>(ConfiguredPath.TryLoad());
		if (!Loaded)
		{
			return GetFallback(Cache, TEXT("Configured style failed to load"));
		}
		Cache.Style = Loaded;
		Cache.bReportedFallback = false;
		return *Loaded;
	}

#if WITH_AUTOMATION_TESTS
	void ClearCachedStyleForTests()
	{
		ClearCache(GetCache());
	}
#endif
}
