// Copyright Wacom. All Rights Reserved.

#include "WacomBattleStatusPresentationCatalogProvider.h"

#include "UI/Battle/WacomBattleStatusPresentationCatalog.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"

#include "UObject/SoftObjectPath.h"

namespace WacomBattleStatusPresentationCatalogProvider
{
	namespace
	{
		struct FCachedCatalog
		{
			FSoftObjectPath Path;
			TWeakObjectPtr<UWacomBattleStatusPresentationCatalog> Catalog;
			bool bHasPath = false;
			bool bLoadFailed = false;
			bool bReportedFallback = false;
		};

		FCachedCatalog& GetCache()
		{
			static FCachedCatalog Cache;
			return Cache;
		}

		void ClearCache(FCachedCatalog& Cache)
		{
			Cache.Path.Reset();
			Cache.Catalog.Reset();
			Cache.bHasPath = false;
			Cache.bLoadFailed = false;
			Cache.bReportedFallback = false;
		}

		const UWacomBattleStatusPresentationCatalog& GetFallback(
			FCachedCatalog& Cache,
			const TCHAR* Reason)
		{
			if (!Cache.bReportedFallback)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[BattleStatusPresentationCatalog] %s; using CDO fallback."),
					Reason);
				Cache.bReportedFallback = true;
			}
			return *GetDefault<UWacomBattleStatusPresentationCatalog>();
		}
	}

	const UWacomBattleStatusPresentationCatalog& GetCatalog()
	{
		const UWacomUIDeveloperSettings* Settings =
			GetDefault<UWacomUIDeveloperSettings>();
		FCachedCatalog& Cache = GetCache();
		if (!Settings || Settings->BattleStatusPresentationCatalog.IsNull())
		{
			if (Cache.bHasPath)
			{
				ClearCache(Cache);
			}
			return GetFallback(Cache, TEXT("No configured catalog"));
		}

		const FSoftObjectPath ConfiguredPath =
			Settings->BattleStatusPresentationCatalog.ToSoftObjectPath();
		if (Cache.bHasPath && Cache.Path == ConfiguredPath)
		{
			if (const UWacomBattleStatusPresentationCatalog* Cached =
				Cache.Catalog.Get())
			{
				return *Cached;
			}
			if (Cache.bLoadFailed)
			{
				return GetFallback(Cache, TEXT("Configured catalog failed to load"));
			}
		}
		else
		{
			ClearCache(Cache);
		}

		UWacomBattleStatusPresentationCatalog* Loaded =
			Settings->BattleStatusPresentationCatalog.LoadSynchronous();
		Cache.Path = ConfiguredPath;
		Cache.Catalog = Loaded;
		Cache.bHasPath = true;
		Cache.bLoadFailed = Loaded == nullptr;
		if (Loaded)
		{
			Cache.bReportedFallback = false;
			return *Loaded;
		}
		return GetFallback(Cache, TEXT("Configured catalog failed to load"));
	}

#if WITH_AUTOMATION_TESTS
	void ClearCachedCatalogForTests()
	{
		ClearCache(GetCache());
	}
#endif
}
