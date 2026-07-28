// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomCardExplanationLexicon;
class UCardDefinition;
struct FCardEffect;
struct FCardPassive;

namespace WacomCardExplanationTemplateResolver
{
	FText ResolveEffectTemplate(
		const UCardDefinition* Card,
		const FCardEffect& Effect,
		const UWacomCardExplanationLexicon* Lexicon,
		int32 EffectIndex);

	bool ResolveCardPassiveTemplate(
		const UCardDefinition* Card,
		int32 PassiveIndex,
		FText& OutTemplate);

	FText ResolvePassiveTriggerTemplate(
		const FCardPassive& Passive,
		const UWacomCardExplanationLexicon* Lexicon);

	bool ResolvePassiveOutcomeTemplate(
		const FCardPassive& Passive,
		const UWacomCardExplanationLexicon* Lexicon,
		FText& OutTemplate);
}
