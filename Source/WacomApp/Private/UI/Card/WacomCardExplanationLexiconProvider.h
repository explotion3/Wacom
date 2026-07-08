// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomCardExplanationLexicon;

namespace WacomCardExplanationLexiconProvider
{
	const UWacomCardExplanationLexicon* GetConfiguredLexicon();

#if WITH_AUTOMATION_TESTS
	void ClearCachedLexiconForTests();
#endif
}
