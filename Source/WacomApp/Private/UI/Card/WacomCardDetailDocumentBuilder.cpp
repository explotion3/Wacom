// Copyright Wacom. All Rights Reserved.

#include "WacomCardDetailDocumentBuilder.h"

#include "Cards/CardDefinition.h"
#include "UI/Card/WacomCardExplanationLexicon.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"
#include "WacomCardExplanationCompiler.h"

#define LOCTEXT_NAMESPACE "WacomCardDetailDocumentBuilder"

namespace WacomCardDetailDocumentBuilder
{
	namespace
	{
		FText GetCardDisplayName(const UCardDefinition* Card)
		{
			if (!Card)
			{
				return LOCTEXT("UnknownCardName", "未知卡牌");
			}
			return Card->DisplayName.IsEmpty()
				? FText::FromName(Card->CardId)
				: Card->DisplayName;
		}

		UWacomCardExplanationLexicon* LoadExplanationLexicon()
		{
			const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
			return Settings && !Settings->CardExplanationLexicon.IsNull()
				? Settings->CardExplanationLexicon.LoadSynchronous()
				: nullptr;
		}

		FWacomCardPresentationRuntimeContext MakePassiveRuntimeContext(
			const FWacomCardPresentationRuntimeContext& RuntimeContext)
		{
			FWacomCardPresentationRuntimeContext PassiveContext;
			PassiveContext.bHasRuntimeCost = RuntimeContext.bHasRuntimeCost;
			PassiveContext.RuntimeCost = RuntimeContext.RuntimeCost;
			PassiveContext.bHasPlayableState = RuntimeContext.bHasPlayableState;
			PassiveContext.bIsPlayable = RuntimeContext.bIsPlayable;
			return PassiveContext;
		}
	}

	FWacomCardDetailViewData BuildCardDetailViewData(
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext)
	{
		FWacomCardDetailViewData Data;
		Data.Name = GetCardDisplayName(Card);
		if (!Card)
		{
			return Data;
		}

		UWacomCardExplanationLexicon* Lexicon = LoadExplanationLexicon();

		TArray<FWacomCardDetailBlock> DescriptionBlocks;
		for (int32 EffectIndex = 0; EffectIndex < Card->Effects.Num(); ++EffectIndex)
		{
			DescriptionBlocks.Add(WacomCardExplanationCompiler::BuildEffectBlock(
				Card,
				Card->Effects[EffectIndex],
				RuntimeContext,
				Lexicon,
				EffectIndex,
				FString::Printf(TEXT("Effect.%d"), EffectIndex),
				EWacomCardDetailBlockKind::EffectSentence));
		}

		TArray<FWacomCardDetailBlock> PassiveBlocks;
		const FWacomCardPresentationRuntimeContext PassiveRuntimeContext =
			MakePassiveRuntimeContext(RuntimeContext);
		for (int32 PassiveIndex = 0; PassiveIndex < Card->Passives.Num(); ++PassiveIndex)
		{
			const FCardPassive& Passive = Card->Passives[PassiveIndex];
			PassiveBlocks.Add(WacomCardExplanationCompiler::BuildPassiveTriggerBlock(
				Passive,
				Lexicon,
				PassiveIndex));
			for (int32 EffectIndex = 0; EffectIndex < Passive.Effects.Num(); ++EffectIndex)
			{
				PassiveBlocks.Add(WacomCardExplanationCompiler::BuildEffectBlock(
					Card,
					Passive.Effects[EffectIndex],
					PassiveRuntimeContext,
					Lexicon,
					EffectIndex,
					FString::Printf(TEXT("Passive.%d.Effect.%d"), PassiveIndex, EffectIndex),
					EWacomCardDetailBlockKind::PassiveEffect));
			}
		}

		WacomCardExplanationCompiler::AddCardDetailSection(
			Data,
			FName(TEXT("Description")),
			EWacomCardDetailSectionKind::Description,
			LOCTEXT("DescriptionSectionTitle", "描述"),
			MoveTemp(DescriptionBlocks));
		WacomCardExplanationCompiler::AddCardDetailSection(
			Data,
			FName(TEXT("Passive")),
			EWacomCardDetailSectionKind::Passive,
			LOCTEXT("PassivesSectionTitle", "被动"),
			MoveTemp(PassiveBlocks));
		return Data;
	}
}

#undef LOCTEXT_NAMESPACE
