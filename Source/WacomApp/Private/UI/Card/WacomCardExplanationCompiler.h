// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPassive.h"
#include "UI/Card/WacomCardPresentationTypes.h"

class UCardDefinition;
class UWacomCardExplanationLexicon;

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

	void AddCardDetailSection(
		FWacomCardDetailViewData& Data,
		FName SectionId,
		EWacomCardDetailSectionKind Kind,
		const FText& Title,
		TArray<FWacomCardDetailBlock>&& Blocks);
}
