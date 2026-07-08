// Copyright Wacom. All Rights Reserved.

#include "WacomCardDetailThemeProvider.h"

#include "UI/Card/WacomCardDetailTheme.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"

#include "UObject/SoftObjectPath.h"

namespace WacomCardDetailThemeProvider
{
	namespace
	{
		struct FCachedTheme
		{
			FSoftObjectPath Path;
			TWeakObjectPtr<UWacomCardDetailTheme> Theme;
			bool bHasPath = false;
			bool bLastLoadFailed = false;
		};

		FCachedTheme& GetCache()
		{
			static FCachedTheme Cache;
			return Cache;
		}

		void ClearCache(FCachedTheme& Cache)
		{
			Cache.Path.Reset();
			Cache.Theme.Reset();
			Cache.bHasPath = false;
			Cache.bLastLoadFailed = false;
		}
	}

	const UWacomCardDetailTheme* GetConfiguredTheme()
	{
		const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
		FCachedTheme& Cache = GetCache();
		if (!Settings || Settings->CardDetailTheme.IsNull())
		{
			ClearCache(Cache);
			return nullptr;
		}

		const FSoftObjectPath ConfiguredPath = Settings->CardDetailTheme.ToSoftObjectPath();
		if (Cache.bHasPath && Cache.Path == ConfiguredPath)
		{
			if (const UWacomCardDetailTheme* CachedTheme = Cache.Theme.Get())
			{
				return CachedTheme;
			}
			if (Cache.bLastLoadFailed)
			{
				return nullptr;
			}
		}

		UWacomCardDetailTheme* LoadedTheme = Settings->CardDetailTheme.LoadSynchronous();
		Cache.Path = ConfiguredPath;
		Cache.Theme = LoadedTheme;
		Cache.bHasPath = true;
		Cache.bLastLoadFailed = LoadedTheme == nullptr;
		return LoadedTheme;
	}

#if WITH_AUTOMATION_TESTS
	void ClearCachedThemeForTests()
	{
		ClearCache(GetCache());
	}
#endif
}
