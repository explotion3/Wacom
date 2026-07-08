// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomCardExplanationLexicon;
struct FCardEffect;
struct FCardPassive;

namespace WacomCardExplanationTemplateResolver
{
	FText ResolveEffectTemplate(
		const FCardEffect& Effect,
		const UWacomCardExplanationLexicon* Lexicon);

	FText ResolvePassiveTriggerTemplate(
		const FCardPassive& Passive,
		const UWacomCardExplanationLexicon* Lexicon);

	bool ResolvePassiveOutcomeTemplate(
		const FCardPassive& Passive,
		const UWacomCardExplanationLexicon* Lexicon,
		FText& OutTemplate);
}
