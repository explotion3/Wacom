// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomCardPresentationTypes.h"

class UCardDefinition;
class UWacomCardExplanationLexicon;
struct FCardEffect;
struct FCardPassive;
struct FWacomCardDynamicCostRule;

namespace WacomCardExplanationTemplateRenderer
{
	void AppendTextRun(
		FWacomCardDetailBlock& Block,
		const FString& Text,
		const FString& StableIdPrefix,
		int32& RunIndex);

	void AppendMutedRun(
		FWacomCardDetailBlock& Block,
		const FText& Text,
		const FString& StableIdPrefix,
		int32& RunIndex);

	void CompileTemplate(
		FWacomCardDetailBlock& Block,
		const FText& Template,
		const UCardDefinition* Card,
		const FCardEffect* Effect,
		const FCardPassive* Passive,
		int32 EffectIndex,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview,
		const UWacomCardExplanationLexicon* Lexicon,
		const FString& StableIdPrefix);

	void CompileKeywordTemplate(
		FWacomCardDetailBlock& Block,
		const FText& Template,
		FGameplayTag Keyword,
		const UWacomCardExplanationLexicon* Lexicon,
		const FString& StableIdPrefix);

	void CompileDynamicCostTemplate(
		FWacomCardDetailBlock& Block,
		const FText& Template,
		const FWacomCardDynamicCostRule& DynamicCostRule,
		const UWacomCardExplanationLexicon* Lexicon,
		const FString& StableIdPrefix);
}
