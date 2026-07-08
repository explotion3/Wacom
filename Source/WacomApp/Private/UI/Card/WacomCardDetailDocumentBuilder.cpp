// Copyright Wacom. All Rights Reserved.

#include "WacomCardDetailDocumentBuilder.h"

#include "Cards/CardDefinition.h"
#include "UI/Card/WacomCardExplanationLexicon.h"
#include "WacomCardExplanationCompiler.h"
#include "WacomCardExplanationLexiconProvider.h"
#include "WacomCardExplanationText.h"

#define LOCTEXT_NAMESPACE "WacomCardDetailDocumentBuilder"

namespace WacomCardDetailDocumentBuilder
{
	namespace
	{
		FText GetCardDisplayName(
			const UCardDefinition* Card,
			const UWacomCardExplanationLexicon* Lexicon)
		{
			if (!Card)
			{
				return WacomCardExplanationText::ResolveNamedText(
					Lexicon,
					WacomCardExplanationLexiconKeys::CardUnknownName,
					LOCTEXT("UnknownCardName", "未知卡牌"));
			}
			return Card->DisplayName.IsEmpty()
				? FText::FromName(Card->CardId)
				: Card->DisplayName;
		}

		FText ResolveSectionTitle(
			const UWacomCardExplanationLexicon* Lexicon,
			FName Key,
			const FText& Fallback)
		{
			return WacomCardExplanationText::ResolveNamedText(Lexicon, Key, Fallback);
		}

		bool IsRenderableBlock(const FWacomCardDetailBlock& Block)
		{
			return !Block.Runs.IsEmpty();
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
		const UWacomCardExplanationLexicon* Lexicon =
			WacomCardExplanationLexiconProvider::GetConfiguredLexicon();
		Data.Name = GetCardDisplayName(Card, Lexicon);
		if (!Card)
		{
			return Data;
		}

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

			TArray<FWacomCardDetailBlock> PassiveFollowUpBlocks;
			FWacomCardDetailBlock OutcomeBlock =
				WacomCardExplanationCompiler::BuildPassiveOutcomeBlock(Passive, Lexicon, PassiveIndex);
			if (IsRenderableBlock(OutcomeBlock))
			{
				PassiveFollowUpBlocks.Add(MoveTemp(OutcomeBlock));
			}

			if (WacomCardExplanationCompiler::ShouldRenderPassiveEffects(Passive))
			{
				for (int32 EffectIndex = 0; EffectIndex < Passive.Effects.Num(); ++EffectIndex)
				{
					PassiveFollowUpBlocks.Add(WacomCardExplanationCompiler::BuildEffectBlock(
						Card,
						Passive.Effects[EffectIndex],
						PassiveRuntimeContext,
						Lexicon,
						EffectIndex,
						FString::Printf(TEXT("Passive.%d.Effect.%d"), PassiveIndex, EffectIndex),
						EWacomCardDetailBlockKind::PassiveEffect));
				}
			}

			bool bHasPassiveFollowUp = false;
			for (const FWacomCardDetailBlock& Block : PassiveFollowUpBlocks)
			{
				if (IsRenderableBlock(Block))
				{
					bHasPassiveFollowUp = true;
					break;
				}
			}
			if (!bHasPassiveFollowUp)
			{
				continue;
			}

			PassiveBlocks.Add(WacomCardExplanationCompiler::BuildPassiveTriggerBlock(
				Passive,
				Lexicon,
				PassiveIndex));
			for (FWacomCardDetailBlock& Block : PassiveFollowUpBlocks)
			{
				PassiveBlocks.Add(MoveTemp(Block));
			}
		}

		WacomCardExplanationCompiler::AddCardDetailSection(
			Data,
			FName(TEXT("Description")),
			EWacomCardDetailSectionKind::Description,
			ResolveSectionTitle(
				Lexicon,
				WacomCardExplanationLexiconKeys::SectionDescriptionTitle,
				LOCTEXT("DescriptionSectionTitle", "描述")),
			MoveTemp(DescriptionBlocks));
		WacomCardExplanationCompiler::AddCardDetailSection(
			Data,
			FName(TEXT("Passive")),
			EWacomCardDetailSectionKind::Passive,
			ResolveSectionTitle(
				Lexicon,
				WacomCardExplanationLexiconKeys::SectionPassiveTitle,
				LOCTEXT("PassivesSectionTitle", "被动")),
			MoveTemp(PassiveBlocks));
		if (Data.Sections.IsEmpty() && !Card->Description.IsEmpty())
		{
			TArray<FWacomCardDetailBlock> FallbackBlocks;
			FallbackBlocks.Add(WacomCardExplanationCompiler::BuildPlainTextBlock(
				FName(TEXT("Description.Fallback.Block")),
				EWacomCardDetailBlockKind::Paragraph,
				Card->Description));
			WacomCardExplanationCompiler::AddCardDetailSection(
				Data,
				FName(TEXT("Description")),
				EWacomCardDetailSectionKind::Description,
				ResolveSectionTitle(
					Lexicon,
					WacomCardExplanationLexiconKeys::SectionDescriptionTitle,
					LOCTEXT("DescriptionFallbackSectionTitle", "描述")),
				MoveTemp(FallbackBlocks));
		}
		return Data;
	}
}

#undef LOCTEXT_NAMESPACE
