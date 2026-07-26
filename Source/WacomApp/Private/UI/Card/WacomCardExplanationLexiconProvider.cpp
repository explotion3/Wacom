// Copyright Wacom. All Rights Reserved.

#include "WacomCardExplanationLexiconProvider.h"

#include "UI/Card/WacomCardExplanationLexicon.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"

#include "UObject/SoftObjectPath.h"

namespace WacomCardExplanationLexiconProvider
{
	namespace
	{
		struct FCachedLexicon
		{
			FSoftObjectPath Path;
			TWeakObjectPtr<UWacomCardExplanationLexicon> Lexicon;
			bool bHasPath = false;
			bool bLastLoadFailed = false;
		};

		FCachedLexicon& GetCache()
		{
			static FCachedLexicon Cache;
			return Cache;
		}

		void ClearCache(FCachedLexicon& Cache)
		{
			Cache.Path.Reset();
			Cache.Lexicon.Reset();
			Cache.bHasPath = false;
			Cache.bLastLoadFailed = false;
		}
	}

	const UWacomCardExplanationLexicon* GetConfiguredLexicon()
	{
		const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
		FCachedLexicon& Cache = GetCache();
		if (!Settings || Settings->CardExplanationLexicon.IsNull())
		{
			ClearCache(Cache);
			return nullptr;
		}

		const FSoftObjectPath ConfiguredPath = Settings->CardExplanationLexicon.ToSoftObjectPath();
		if (Cache.bHasPath && Cache.Path == ConfiguredPath)
		{
			if (const UWacomCardExplanationLexicon* CachedLexicon = Cache.Lexicon.Get())
			{
				return CachedLexicon;
			}
			if (Cache.bLastLoadFailed)
			{
				return nullptr;
			}
		}

		UWacomCardExplanationLexicon* LoadedLexicon =
			Settings->CardExplanationLexicon.LoadSynchronous();
		Cache.Path = ConfiguredPath;
		Cache.Lexicon = LoadedLexicon;
		Cache.bHasPath = true;
		Cache.bLastLoadFailed = LoadedLexicon == nullptr;
		return LoadedLexicon;
	}

	bool FindCardFaceSemantic(
		const FName SemanticId,
		const FGameplayTag SourceTag,
		FWacomCardFaceSemanticLexiconEntry& OutEntry)
	{
		FWacomCardFaceSemanticLexiconEntry ConfiguredEntry;
		const UWacomCardExplanationLexicon* Configured = GetConfiguredLexicon();
		const bool bHasConfigured =
			Configured
			&& Configured->FindCardFaceSemantic(
				SemanticId,
				SourceTag,
				ConfiguredEntry);

		FWacomCardFaceSemanticLexiconEntry DefaultEntry;
		const UWacomCardExplanationLexicon* Defaults =
			GetDefault<UWacomCardExplanationLexicon>();
		const bool bHasDefault =
			Defaults
			&& Defaults->FindCardFaceSemantic(
				SemanticId,
				SourceTag,
				DefaultEntry);

		if (!bHasConfigured && !bHasDefault)
		{
			return false;
		}
		if (!bHasConfigured)
		{
			OutEntry = DefaultEntry;
			return true;
		}

		// Configured asset wins per field so authoring one field never blanks
		// the other. Empty asset fields fall back to the C++ default entry.
		OutEntry = ConfiguredEntry;
		if (bHasDefault)
		{
			if (OutEntry.DisplayName.IsEmpty())
			{
				OutEntry.DisplayName = DefaultEntry.DisplayName;
			}
			if (OutEntry.Description.IsEmpty())
			{
				OutEntry.Description = DefaultEntry.Description;
			}
			if (!OutEntry.SourceTag.IsValid())
			{
				OutEntry.SourceTag = DefaultEntry.SourceTag;
			}
			if (OutEntry.SemanticId.IsNone())
			{
				OutEntry.SemanticId = DefaultEntry.SemanticId;
			}
		}
		return true;
	}

#if WITH_AUTOMATION_TESTS
	void ClearCachedLexiconForTests()
	{
		ClearCache(GetCache());
	}
#endif
}
