// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UWacomCardExplanationLexicon;

namespace WacomCardExplanationText
{
	FString GetDisplayTagLeafName(const FGameplayTag& Tag);
	FText GetDisplayTagName(
		const FGameplayTag& Tag,
		const UWacomCardExplanationLexicon* Lexicon);
	FText GetDisplayHandZoneName(
		const FGameplayTag& HandZoneTag,
		const UWacomCardExplanationLexicon* Lexicon);
	FText GetDisplayStatusName(
		const FGameplayTag& StatusTag,
		const UWacomCardExplanationLexicon* Lexicon);
	FText ResolveNamedText(
		const UWacomCardExplanationLexicon* Lexicon,
		FName Key,
		const FText& Fallback);
	FText FormatNamedText(
		const UWacomCardExplanationLexicon* Lexicon,
		FName Key,
		const FText& Fallback,
		const FFormatOrderedArguments& Arguments);
}
