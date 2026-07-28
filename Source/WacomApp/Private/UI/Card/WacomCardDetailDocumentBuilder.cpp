// Copyright Wacom. All Rights Reserved.

#include "WacomCardDetailDocumentBuilder.h"

#include "Cards/CardDefinition.h"
#include "UI/Card/WacomCardExplanationLexicon.h"
#include "WacomCardExplanationCompiler.h"
#include "WacomCardExplanationLexiconProvider.h"
#include "WacomCardExplanationTemplateResolver.h"
#include "WacomCardExplanationText.h"

#define LOCTEXT_NAMESPACE "WacomCardDetailDocumentBuilder"

namespace WacomCardDetailDocumentBuilder
{
	namespace
	{
		FText GetCardDisplayName(
			const UCardDefinition* Card,
			const UWacomCardExplanationLexicon* Lexicon,
			EWacomCardFaceContext FaceContext)
		{
			if (!Card)
			{
				return WacomCardExplanationText::ResolveNamedText(
					Lexicon,
					WacomCardExplanationLexiconKeys::CardUnknownName,
					LOCTEXT("UnknownCardName", "未知卡牌"));
			}
			if (FaceContext == EWacomCardFaceContext::Run
				&& !Card->RunFace.DisplayNameOverride.IsEmpty())
			{
				return Card->RunFace.DisplayNameOverride;
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
			PassiveContext.bHasUpgradeTier = RuntimeContext.bHasUpgradeTier;
			PassiveContext.UpgradeTier = RuntimeContext.UpgradeTier;
			PassiveContext.bHasPlayableState = RuntimeContext.bHasPlayableState;
			PassiveContext.bIsPlayable = RuntimeContext.bIsPlayable;
			return PassiveContext;
		}
	}

	FWacomCardDetailViewData BuildCardDetailViewData(
		const UCardDefinition* Card,
		EWacomCardFaceContext FaceContext,
		const FWacomCardPresentationRuntimeContext& RuntimeContext)
	{
		FWacomCardDetailViewData Data;
		const UWacomCardExplanationLexicon* Lexicon =
			WacomCardExplanationLexiconProvider::GetConfiguredLexicon();
		Data.Name = GetCardDisplayName(Card, Lexicon, FaceContext);
		if (!Card)
		{
			return Data;
		}

		if (FaceContext == EWacomCardFaceContext::Run)
		{
			if (!Card->RunFace.Description.IsEmpty())
			{
				TArray<FWacomCardDetailBlock> RunDescriptionBlocks;
				RunDescriptionBlocks.Add(WacomCardExplanationCompiler::BuildPlainTextBlock(
					FName(TEXT("Run.Description.Block")),
					EWacomCardDetailBlockKind::Paragraph,
					Card->RunFace.Description));
				WacomCardExplanationCompiler::AddCardDetailSection(
					Data,
					FName(TEXT("Description")),
					EWacomCardDetailSectionKind::Description,
					ResolveSectionTitle(
						Lexicon,
						WacomCardExplanationLexiconKeys::SectionDescriptionTitle,
						LOCTEXT("RunDescriptionSectionTitle", "描述")),
					MoveTemp(RunDescriptionBlocks));
			}
			return Data;
		}

		const EWacomCardUpgradeTier Tier = RuntimeContext.bHasUpgradeTier
			? RuntimeContext.UpgradeTier
			: EWacomCardUpgradeTier::White;
		const FWacomResolvedCardProfile ResolvedProfile =
			Card->ResolveProfile(Tier);
		const TArray<FCardEffect>& Effects = *ResolvedProfile.Effects;
		const TArray<FCardPassive>& Passives = *ResolvedProfile.Passives;
		const FText& Description = Card->ResolveDescription(Tier);

		TArray<FWacomCardDetailBlock> DescriptionBlocks;
		for (int32 KeywordIndex = 0;
			KeywordIndex < Card->ExplanationTemplates.KeywordTemplates.Num();
			++KeywordIndex)
		{
			const FWacomCardKeywordExplanationTemplate& KeywordTemplate =
				Card->ExplanationTemplates.KeywordTemplates[KeywordIndex];
			FWacomCardDetailBlock KeywordBlock =
				WacomCardExplanationCompiler::BuildKeywordTemplateBlock(
					KeywordTemplate.Keyword,
					KeywordTemplate.Template,
					Lexicon,
					KeywordIndex);
			if (IsRenderableBlock(KeywordBlock))
			{
				DescriptionBlocks.Add(MoveTemp(KeywordBlock));
			}
		}
		for (int32 EffectIndex = 0; EffectIndex < Effects.Num(); ++EffectIndex)
		{
			if (Card->ExplanationTemplates.ShouldSuppressEffect(EffectIndex))
			{
				continue;
			}
			DescriptionBlocks.Add(WacomCardExplanationCompiler::BuildEffectBlock(
				Card,
				Effects[EffectIndex],
				RuntimeContext,
				Lexicon,
				EffectIndex,
				FString::Printf(TEXT("Effect.%d"), EffectIndex),
				EWacomCardDetailBlockKind::EffectSentence));
		}

		TArray<FWacomCardDetailBlock> PassiveBlocks;
		const FWacomCardPresentationRuntimeContext PassiveRuntimeContext =
			MakePassiveRuntimeContext(RuntimeContext);
		if (ResolvedProfile.DynamicCostRule
			&& !Card->ExplanationTemplates.DynamicCostTemplate.IsEmpty())
		{
			FWacomCardDetailBlock DynamicCostBlock =
				WacomCardExplanationCompiler::BuildDynamicCostTemplateBlock(
					*ResolvedProfile.DynamicCostRule,
					Card->ExplanationTemplates.DynamicCostTemplate,
					Lexicon);
			if (IsRenderableBlock(DynamicCostBlock))
			{
				PassiveBlocks.Add(MoveTemp(DynamicCostBlock));
			}
		}
		for (int32 PassiveIndex = 0; PassiveIndex < Passives.Num(); ++PassiveIndex)
		{
			if (Card->ExplanationTemplates.ShouldSuppressPassive(PassiveIndex))
			{
				continue;
			}
			const FCardPassive& Passive = Passives[PassiveIndex];
			FText CardPassiveTemplate;
			if (WacomCardExplanationTemplateResolver::ResolveCardPassiveTemplate(
				Card,
				PassiveIndex,
				CardPassiveTemplate))
			{
				PassiveBlocks.Add(
					WacomCardExplanationCompiler::BuildPassiveTemplateBlock(
						Card,
						Passive,
						CardPassiveTemplate,
						PassiveRuntimeContext,
						Lexicon,
						PassiveIndex));
				continue;
			}
			if (!Passive.DisplayText.IsEmpty())
			{
				PassiveBlocks.Add(WacomCardExplanationCompiler::BuildPlainTextBlock(
					FName(*FString::Printf(TEXT("Passive.%d.Authored.Block"), PassiveIndex)),
					EWacomCardDetailBlockKind::PassiveOutcome,
					Passive.DisplayText));
				continue;
			}

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
		if (Data.Sections.IsEmpty() && !Description.IsEmpty())
		{
			TArray<FWacomCardDetailBlock> FallbackBlocks;
			FallbackBlocks.Add(WacomCardExplanationCompiler::BuildPlainTextBlock(
				FName(TEXT("Description.Fallback.Block")),
				EWacomCardDetailBlockKind::Paragraph,
				Description));
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
