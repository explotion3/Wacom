// Copyright Wacom. All Rights Reserved.

#include "WacomCardExplanationCompiler.h"

#include "WacomCardExplanationConditionRenderer.h"
#include "WacomCardExplanationTemplateResolver.h"
#include "WacomCardExplanationTemplateRenderer.h"
#include "WacomCardExplanationText.h"

#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardExplanationLexicon.h"

#define LOCTEXT_NAMESPACE "WacomCardExplanationCompiler"

namespace WacomCardExplanationCompiler
{
	namespace
	{
		const FWacomCardPresentationRuntimeContext::FEffectPreview* FindEffectPreview(
			const FWacomCardPresentationRuntimeContext& RuntimeContext,
			int32 EffectIndex)
		{
			for (const FWacomCardPresentationRuntimeContext::FEffectPreview& Preview : RuntimeContext.EffectPreviews)
			{
				if (Preview.EffectIndex == EffectIndex)
				{
					return &Preview;
				}
			}
			return nullptr;
		}

		bool IsRenderableBlock(const FWacomCardDetailBlock& Block)
		{
			return !Block.Runs.IsEmpty();
		}
	}

	FWacomCardDetailBlock BuildEffectBlock(
		const UCardDefinition* Card,
		const FCardEffect& Effect,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		const UWacomCardExplanationLexicon* Lexicon,
		int32 EffectIndex,
		const FString& StableIdPrefix,
		EWacomCardDetailBlockKind BlockKind)
	{
		FWacomCardDetailBlock Block;
		Block.BlockId = FName(*FString::Printf(TEXT("%s.Block"), *StableIdPrefix));
		Block.Kind = BlockKind;

		const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview =
			FindEffectPreview(RuntimeContext, EffectIndex);
		Block.bSkipped = Preview && Preview->bSkip;
		if (Block.bSkipped)
		{
			int32 RunIndex = Block.Runs.Num();
			WacomCardExplanationTemplateRenderer::AppendMutedRun(
				Block,
				WacomCardExplanationText::ResolveNamedText(
					Lexicon,
					WacomCardExplanationLexiconKeys::DetailSkipPrefix,
					LOCTEXT("SkipPrefix", "不会生效：")),
				StableIdPrefix,
				RunIndex);
		}

		WacomCardExplanationTemplateRenderer::CompileTemplate(
			Block,
			WacomCardExplanationTemplateResolver::ResolveEffectTemplate(
				Card,
				Effect,
				Lexicon,
				EffectIndex),
			Card,
			&Effect,
			nullptr,
			EffectIndex,
			RuntimeContext,
			Preview,
			Lexicon,
			StableIdPrefix);
		int32 RunIndex = Block.Runs.Num();
		WacomCardExplanationConditionRenderer::AppendConditionRuns(
			Block,
			Effect.Condition,
			Lexicon,
			StableIdPrefix,
			RunIndex);
		WacomCardExplanationConditionRenderer::AppendMagnitudeModifierRuns(
			Block,
			Effect.MagnitudeModifiers,
			Lexicon,
			StableIdPrefix,
			RunIndex);
		return Block;
	}

	FWacomCardDetailBlock BuildPassiveTemplateBlock(
		const UCardDefinition* Card,
		const FCardPassive& Passive,
		const FText& Template,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		const UWacomCardExplanationLexicon* Lexicon,
		const int32 PassiveIndex)
	{
		const FString StableIdPrefix =
			FString::Printf(TEXT("Passive.%d.Authored"), PassiveIndex);
		FWacomCardDetailBlock Block;
		Block.BlockId = FName(*FString::Printf(TEXT("%s.Block"), *StableIdPrefix));
		Block.Kind = EWacomCardDetailBlockKind::PassiveOutcome;

		WacomCardExplanationTemplateRenderer::CompileTemplate(
			Block,
			Template,
			Card,
			nullptr,
			&Passive,
			INDEX_NONE,
			RuntimeContext,
			nullptr,
			Lexicon,
			StableIdPrefix);
		return Block;
	}

	FWacomCardDetailBlock BuildKeywordTemplateBlock(
		const FGameplayTag Keyword,
		const FText& Template,
		const UWacomCardExplanationLexicon* Lexicon,
		const int32 KeywordIndex)
	{
		const FString StableIdPrefix =
			FString::Printf(TEXT("Keyword.%d.Authored"), KeywordIndex);
		FWacomCardDetailBlock Block;
		Block.BlockId =
			FName(*FString::Printf(TEXT("%s.Block"), *StableIdPrefix));
		Block.Kind = EWacomCardDetailBlockKind::EffectSentence;
		WacomCardExplanationTemplateRenderer::CompileKeywordTemplate(
			Block,
			Template,
			Keyword,
			Lexicon,
			StableIdPrefix);
		return Block;
	}

	FWacomCardDetailBlock BuildDynamicCostTemplateBlock(
		const FWacomCardDynamicCostRule& DynamicCostRule,
		const FText& Template,
		const UWacomCardExplanationLexicon* Lexicon)
	{
		const FString StableIdPrefix = TEXT("DynamicCost.Authored");
		FWacomCardDetailBlock Block;
		Block.BlockId =
			FName(*FString::Printf(TEXT("%s.Block"), *StableIdPrefix));
		Block.Kind = EWacomCardDetailBlockKind::PassiveOutcome;
		WacomCardExplanationTemplateRenderer::CompileDynamicCostTemplate(
			Block,
			Template,
			DynamicCostRule,
			Lexicon,
			StableIdPrefix);
		return Block;
	}

	FWacomCardDetailBlock BuildPassiveTriggerBlock(
		const FCardPassive& Passive,
		const UWacomCardExplanationLexicon* Lexicon,
		int32 PassiveIndex)
	{
		const FString StableIdPrefix = FString::Printf(TEXT("Passive.%d.Trigger"), PassiveIndex);
		FWacomCardDetailBlock Block;
		Block.BlockId = FName(*FString::Printf(TEXT("%s.Block"), *StableIdPrefix));
		Block.Kind = EWacomCardDetailBlockKind::PassiveTrigger;

		FWacomCardPresentationRuntimeContext EmptyContext;
		WacomCardExplanationTemplateRenderer::CompileTemplate(
			Block,
			WacomCardExplanationTemplateResolver::ResolvePassiveTriggerTemplate(Passive, Lexicon),
			nullptr,
			nullptr,
			&Passive,
			INDEX_NONE,
			EmptyContext,
			nullptr,
			Lexicon,
			StableIdPrefix);

		int32 RunIndex = Block.Runs.Num();
		WacomCardExplanationConditionRenderer::AppendConditionRuns(
			Block,
			Passive.Condition,
			Lexicon,
			StableIdPrefix,
			RunIndex);
		return Block;
	}

	FWacomCardDetailBlock BuildPassiveOutcomeBlock(
		const FCardPassive& Passive,
		const UWacomCardExplanationLexicon* Lexicon,
		int32 PassiveIndex)
	{
		const FString StableIdPrefix = FString::Printf(TEXT("Passive.%d.Outcome"), PassiveIndex);
		FWacomCardDetailBlock Block;
		Block.BlockId = FName(*FString::Printf(TEXT("%s.Block"), *StableIdPrefix));
		Block.Kind = EWacomCardDetailBlockKind::PassiveOutcome;

		FText Template;
		if (!WacomCardExplanationTemplateResolver::ResolvePassiveOutcomeTemplate(Passive, Lexicon, Template))
		{
			return Block;
		}

		FWacomCardPresentationRuntimeContext EmptyContext;
		WacomCardExplanationTemplateRenderer::CompileTemplate(
			Block,
			Template,
			nullptr,
			nullptr,
			&Passive,
			INDEX_NONE,
			EmptyContext,
			nullptr,
			Lexicon,
			StableIdPrefix);
		return Block;
	}

	FWacomCardDetailBlock BuildPlainTextBlock(
		FName BlockId,
		EWacomCardDetailBlockKind BlockKind,
		const FText& Text)
	{
		FWacomCardDetailBlock Block;
		Block.BlockId = BlockId;
		Block.Kind = BlockKind;
		if (!Text.IsEmpty())
		{
			FWacomCardDetailRun Run;
			Run.StableId = FName(*FString::Printf(TEXT("%s.Run.0.Text"), *BlockId.ToString()));
			Run.Kind = EWacomCardDetailRunKind::Text;
			Run.Text = Text;
			Block.Runs.Add(MoveTemp(Run));
		}
		return Block;
	}

	bool ShouldRenderPassiveEffects(const FCardPassive& Passive)
	{
		return !Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnCompanionCount)
			&& !Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTwilightTriggered);
	}

	void AddCardDetailSection(
		FWacomCardDetailViewData& Data,
		FName SectionId,
		EWacomCardDetailSectionKind Kind,
		const FText& Title,
		TArray<FWacomCardDetailBlock>&& Blocks)
	{
		TArray<FWacomCardDetailBlock> NonEmptyBlocks;
		for (FWacomCardDetailBlock& Block : Blocks)
		{
			if (IsRenderableBlock(Block))
			{
				NonEmptyBlocks.Add(MoveTemp(Block));
			}
		}
		if (NonEmptyBlocks.IsEmpty())
		{
			return;
		}

		FWacomCardDetailSection Section;
		Section.SectionId = SectionId;
		Section.Kind = Kind;
		Section.Title = Title;
		Section.Blocks = MoveTemp(NonEmptyBlocks);
		Data.Sections.Add(MoveTemp(Section));
	}
}

#undef LOCTEXT_NAMESPACE
