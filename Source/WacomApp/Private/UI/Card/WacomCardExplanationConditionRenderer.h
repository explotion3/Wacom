// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomCardPresentationTypes.h"

struct FEffectCondition;
struct FMagnitudeModifier;
class UWacomCardExplanationLexicon;

namespace WacomCardExplanationConditionRenderer
{
	void AppendConditionRuns(
		FWacomCardDetailBlock& Block,
		const FEffectCondition& Condition,
		const UWacomCardExplanationLexicon* Lexicon,
		const FString& StableIdPrefix,
		int32& RunIndex);

	void AppendMagnitudeModifierRuns(
		FWacomCardDetailBlock& Block,
		const TArray<FMagnitudeModifier>& Modifiers,
		const UWacomCardExplanationLexicon* Lexicon,
		const FString& StableIdPrefix,
		int32& RunIndex);
}
