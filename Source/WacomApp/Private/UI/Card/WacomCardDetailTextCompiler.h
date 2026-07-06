// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPassive.h"
#include "UI/Card/WacomCardPresentationTypes.h"

class UCardDefinition;

namespace WacomCardDetailTextCompiler
{
	TArray<FWacomCardDetailTokenLine> BuildAuthoredTextTokenLines(
		const UCardDefinition* Card,
		const TArray<FCardEffect>& Effects,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		const FText& Text,
		EWacomCardDetailTokenLineKind LineKind,
		const FString& StableIdPrefix);

	TArray<FWacomCardDetailTokenLine> BuildEffectTokenLines(
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext);

	void BuildPassiveTokenLines(
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		TArray<FWacomCardDetailTokenLine>& OutLines);

	TArray<FWacomCardDetailTokenLine> BuildPlainTextTokenLines(
		const TArray<FText>& Lines,
		EWacomCardDetailTokenLineKind Kind,
		const FString& StableIdPrefix);

	void AddCardDetailSection(
		FWacomCardDetailViewData& Data,
		FName SectionId,
		EWacomCardDetailSectionKind Kind,
		const FText& Title,
		TArray<FWacomCardDetailTokenLine>&& TokenLines);
}
