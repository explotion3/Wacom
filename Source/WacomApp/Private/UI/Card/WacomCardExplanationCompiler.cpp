// Copyright Wacom. All Rights Reserved.

#include "WacomCardExplanationCompiler.h"

#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardExplanationLexicon.h"
#include "WacomCardExplanationTemplateRenderer.h"

#define LOCTEXT_NAMESPACE "WacomCardExplanationCompiler"

DEFINE_LOG_CATEGORY_STATIC(LogWacomCardExplanation, Log, All);

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

		FText DefaultEffectTemplate(const FCardEffect& Effect)
		{
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Damage))
			{
				return LOCTEXT("TemplateDamage", "造成 {value:Magnitude} 点伤害。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Heal))
			{
				return LOCTEXT("TemplateHeal", "恢复 {value:Magnitude} 点生命。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Status_Shield))
			{
				return LOCTEXT("TemplateShield", "获得 {value:Magnitude} 点护盾。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Poison)
				|| Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Slow)
				|| Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Freeze)
				|| Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Twilight))
			{
				return LOCTEXT("TemplateApplyStatus", "施加 {value:Magnitude} 层 {status:EffectStatus}。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_AddCost))
			{
				return LOCTEXT("TemplateAddCost", "目标手牌费用增加 {value:Magnitude}。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_ReduceCost))
			{
				return LOCTEXT("TemplateReduceCost", "目标手牌费用降低 {value:Magnitude}。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_DiscardSelected))
			{
				return LOCTEXT("TemplateDiscardSelected", "弃置目标手牌。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_ExhaustSelected))
			{
				return LOCTEXT("TemplateExhaustSelected", "消耗目标手牌。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Shuffle_Random))
			{
				return LOCTEXT("TemplateShuffleRandom", "随机腾挪 1 张手牌。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Shuffle_FromBothToOther))
			{
				return LOCTEXT("TemplateShuffleFromBothToOther", "将双手区随机 1 张卡牌腾挪至其他区域。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Shuffle_ToRandomZone))
			{
				return LOCTEXT("TemplateShuffleToRandomZone", "该牌腾挪至随机区域。");
			}
			return FText::Format(
				LOCTEXT("TemplateUnknownEffect", "{0}。"),
				FText::FromString(WacomCardExplanationTemplateRenderer::GetDisplayTagLeafName(Effect.EffectType)));
		}

		FText DefaultPassiveTriggerTemplate(const FCardPassive& Passive)
		{
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_AfterPlayed))
			{
				return LOCTEXT("TemplatePassiveAfterPlayed", "打出后：");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnCompanionCount))
			{
				return LOCTEXT("TemplatePassiveCompanionCount", "每打出 {value:TriggerThreshold} 张伙伴：");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTwilightTriggered))
			{
				return LOCTEXT("TemplatePassiveTwilight", "暮气触发时：");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTurnStart))
			{
				return LOCTEXT("TemplatePassiveTurnStart", "回合开始：");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTurnEnd))
			{
				return LOCTEXT("TemplatePassiveTurnEnd", "回合结束：");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnDraw))
			{
				return LOCTEXT("TemplatePassiveDraw", "抽到时：");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnDiscard))
			{
				return LOCTEXT("TemplatePassiveDiscard", "弃掉时：");
			}
			return FText::Format(
				LOCTEXT("TemplatePassiveUnknown", "{0}："),
				FText::FromString(WacomCardExplanationTemplateRenderer::GetDisplayTagLeafName(Passive.Trigger)));
		}

		FText ResolveEffectTemplate(
			const FCardEffect& Effect,
			const UWacomCardExplanationLexicon* Lexicon)
		{
			FWacomCardExplanationTemplateEntry Entry;
			if (Lexicon && Lexicon->FindEffectTemplate(Effect.EffectType, Entry))
			{
				return Entry.Template;
			}
			if (Lexicon)
			{
				UE_LOG(
					LogWacomCardExplanation,
					Verbose,
					TEXT("Missing card effect explanation template for '%s'; using generated fallback."),
					*Effect.EffectType.ToString());
			}
			return DefaultEffectTemplate(Effect);
		}

		FText ResolvePassiveTriggerTemplate(
			const FCardPassive& Passive,
			const UWacomCardExplanationLexicon* Lexicon)
		{
			FWacomCardExplanationTemplateEntry Entry;
			if (Lexicon && Lexicon->FindPassiveTriggerTemplate(Passive.Trigger, Entry))
			{
				return Entry.Template;
			}
			if (Lexicon)
			{
				UE_LOG(
					LogWacomCardExplanation,
					Verbose,
					TEXT("Missing card passive trigger explanation template for '%s'; using generated fallback."),
					*Passive.Trigger.ToString());
			}
			return DefaultPassiveTriggerTemplate(Passive);
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
			ResolveEffectTemplate(Effect, Lexicon),
			Card,
			&Effect,
			nullptr,
			RuntimeContext,
			Preview,
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
			ResolvePassiveTriggerTemplate(Passive, Lexicon),
			nullptr,
			nullptr,
			&Passive,
			EmptyContext,
			nullptr,
			StableIdPrefix);

		if (Passive.Condition.IsSet())
		{
			int32 RunIndex = Block.Runs.Num();
			WacomCardExplanationTemplateRenderer::AppendTextRun(
				Block,
				TEXT("（有条件）"),
				StableIdPrefix,
				RunIndex);
		}
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

#undef LOCTEXT_NAMESPACE
