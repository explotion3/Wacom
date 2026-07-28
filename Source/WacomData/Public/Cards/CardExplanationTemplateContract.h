// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FCardEffect;
struct FCardPassive;
struct FWacomCardDynamicCostRule;

enum class EWacomCardExplanationTemplateContext : uint8
{
	Effect,
	Passive,
	Keyword,
	DynamicCost
};

enum class EWacomCardExplanationTemplateSlotKind : uint8
{
	EffectMagnitude,
	EffectIcon,
	EffectStatus,
	EffectTag,
	TriggerThreshold,
	PassiveEffectMagnitude,
	PassiveEffectIcon,
	PassiveEffectStatus,
	Keyword,
	DynamicCostStatus,
	DynamicCostReductionPerMatchingCard,
	DynamicCostMinimumCost
};

struct WACOMDATA_API FWacomCardExplanationTemplateSlot
{
	EWacomCardExplanationTemplateSlotKind Kind =
		EWacomCardExplanationTemplateSlotKind::EffectMagnitude;
	int32 PassiveEffectIndex = INDEX_NONE;
	FName SlotName;
};

/**
 * CardDefinition explanation-template grammar shared by runtime rendering and
 * editor validation. This contract parses authoring tokens only; it never
 * formats player-facing text or reads battle runtime state.
 */
namespace WacomCardExplanationTemplateContract
{
	WACOMDATA_API bool TryParseSlot(
		const FString& Slot,
		EWacomCardExplanationTemplateContext Context,
		FWacomCardExplanationTemplateSlot& OutSlot,
		FString* OutError = nullptr);

	WACOMDATA_API void ValidateEffectTemplate(
		const FText& Template,
		const FCardEffect& Effect,
		TArray<FString>& OutErrors);

	WACOMDATA_API void ValidatePassiveTemplate(
		const FText& Template,
		const FCardPassive& Passive,
		TArray<FString>& OutErrors);

	WACOMDATA_API void ValidateKeywordTemplate(
		const FText& Template,
		FGameplayTag Keyword,
		TArray<FString>& OutErrors);

	WACOMDATA_API void ValidateDynamicCostTemplate(
		const FText& Template,
		const FWacomCardDynamicCostRule& DynamicCostRule,
		TArray<FString>& OutErrors);

	WACOMDATA_API FGameplayTag ResolveEffectStatusTag(
		const FCardEffect& Effect);
}
