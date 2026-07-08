// Copyright Wacom. All Rights Reserved.

#include "WacomCardExplanationCompiler.h"

#include "WacomCardExplanationConditionRenderer.h"
#include "WacomCardExplanationTemplateResolver.h"
#include "WacomCardExplanationTemplateRenderer.h"

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

		WacomCardExplanationTemplateRenderer::CompileTemplate(
			Block,
			WacomCardExplanationTemplateResolver::ResolveEffectTemplate(Effect, Lexicon),
			Card,
			&Effect,
			nullptr,
			RuntimeContext,
			Preview,
			StableIdPrefix);
		int32 RunIndex = Block.Runs.Num();
		WacomCardExplanationConditionRenderer::AppendConditionRuns(
			Block,
			Effect.Condition,
			StableIdPrefix,
			RunIndex);
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
			EmptyContext,
			nullptr,
			StableIdPrefix);

		int32 RunIndex = Block.Runs.Num();
		WacomCardExplanationConditionRenderer::AppendConditionRuns(
			Block,
			Passive.Condition,
			StableIdPrefix,
			RunIndex);
		return Block;
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
			if (!Block.Runs.IsEmpty())
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
