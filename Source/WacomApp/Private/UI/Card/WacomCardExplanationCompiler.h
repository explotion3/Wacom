// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPassive.h"
#include "UI/Card/WacomCardPresentationTypes.h"

class UCardDefinition;
class UWacomCardExplanationLexicon;
struct FWacomCardDynamicCostRule;

namespace WacomCardExplanationCompiler
{
	FWacomCardDetailBlock BuildEffectBlock(
		const UCardDefinition* Card,
		const FCardEffect& Effect,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		const UWacomCardExplanationLexicon* Lexicon,
		int32 EffectIndex,
		const FString& StableIdPrefix,
		EWacomCardDetailBlockKind BlockKind);

	FWacomCardDetailBlock BuildPassiveTriggerBlock(
		const FCardPassive& Passive,
		const UWacomCardExplanationLexicon* Lexicon,
		int32 PassiveIndex);

	FWacomCardDetailBlock BuildPassiveTemplateBlock(
		const UCardDefinition* Card,
		const FCardPassive& Passive,
		const FText& Template,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		const UWacomCardExplanationLexicon* Lexicon,
		int32 PassiveIndex);

	FWacomCardDetailBlock BuildKeywordTemplateBlock(
		FGameplayTag Keyword,
		const FText& Template,
		const UWacomCardExplanationLexicon* Lexicon,
		int32 KeywordIndex);

	FWacomCardDetailBlock BuildDynamicCostTemplateBlock(
		const FWacomCardDynamicCostRule& DynamicCostRule,
		const FText& Template,
		const UWacomCardExplanationLexicon* Lexicon);

	FWacomCardDetailBlock BuildPassiveOutcomeBlock(
		const FCardPassive& Passive,
		const UWacomCardExplanationLexicon* Lexicon,
		int32 PassiveIndex);

	FWacomCardDetailBlock BuildPlainTextBlock(
		FName BlockId,
		EWacomCardDetailBlockKind BlockKind,
		const FText& Text);

	bool ShouldRenderPassiveEffects(const FCardPassive& Passive);

	void AddCardDetailSection(
		FWacomCardDetailViewData& Data,
		FName SectionId,
		EWacomCardDetailSectionKind Kind,
		const FText& Title,
		TArray<FWacomCardDetailBlock>&& Blocks);
}
