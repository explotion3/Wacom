// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UWacomCardExplanationLexicon;
struct FWacomCardFaceSemanticLexiconEntry;

namespace WacomCardExplanationLexiconProvider
{
	const UWacomCardExplanationLexicon* GetConfiguredLexicon();

	/**
	 * Resolves one card-face semantic entry across the configured asset and the
	 * C++ default lexicon.
	 *
	 * The configured asset wins per field, not per entry: fields it leaves empty
	 * fall back to the default entry, so authoring only DisplayName keeps the
	 * default Description instead of silently dropping the tooltip body.
	 */
	WACOMAPP_API bool FindCardFaceSemantic(
		FName SemanticId,
		FGameplayTag SourceTag,
		FWacomCardFaceSemanticLexiconEntry& OutEntry);

#if WITH_AUTOMATION_TESTS
	WACOMAPP_API void ClearCachedLexiconForTests();
#endif
}
