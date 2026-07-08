// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/Card/WacomCardPresentationTypes.h"

class UCardDefinition;
struct FCardEffect;
struct FCardPassive;

namespace WacomCardExplanationTemplateRenderer
{
	FString GetDisplayTagLeafName(const FGameplayTag& Tag);

	void AppendTextRun(
		FWacomCardDetailBlock& Block,
		const FString& Text,
		const FString& StableIdPrefix,
		int32& RunIndex);

	void CompileTemplate(
		FWacomCardDetailBlock& Block,
		const FText& Template,
		const UCardDefinition* Card,
		const FCardEffect* Effect,
		const FCardPassive* Passive,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview,
		const FString& StableIdPrefix);
}
