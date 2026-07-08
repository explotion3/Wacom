// Copyright Wacom. All Rights Reserved.

#include "WacomCardExplanationTemplateResolver.h"

#include "Cards/CardEffect.h"
#include "Cards/CardPassive.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardExplanationLexicon.h"
#include "WacomCardExplanationText.h"

#define LOCTEXT_NAMESPACE "WacomCardExplanationTemplateResolver"

DEFINE_LOG_CATEGORY_STATIC(LogWacomCardExplanationTemplateResolver, Log, All);

namespace WacomCardExplanationTemplateResolver
{
	namespace
	{
		FText DefaultEffectTemplate(const FCardEffect& Effect)
		{
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Damage))
			{
				return LOCTEXT("TemplateDamage", "{icon:EffectIcon} 造成 {value:Magnitude} 点伤害。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Heal))
			{
				return LOCTEXT("TemplateHeal", "{icon:EffectIcon} 恢复 {value:Magnitude} 点生命。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Status_Shield))
			{
				return LOCTEXT("TemplateShield", "{icon:EffectIcon} 获得 {value:Magnitude} 点护盾。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Draw))
			{
				return LOCTEXT("TemplateDraw", "抽 {value:Magnitude} 张牌。");
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
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Discard))
			{
				return LOCTEXT("TemplateDiscard", "随机弃置 {value:Magnitude} 张手牌。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ExhaustSelf))
			{
				return LOCTEXT("TemplateExhaustSelf", "此牌打出后进入消耗牌堆。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_GainKeyword))
			{
				return LOCTEXT("TemplateGainKeyword", "赋予目标手牌关键词。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_RemoveStatus))
			{
				return LOCTEXT("TemplateRemoveStatus", "移除目标 {value:Magnitude} 层状态。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ModifyInitiative))
			{
				return LOCTEXT("TemplateModifyInitiative", "目标部位先机变化 {value:Magnitude}。");
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
				FText::FromString(WacomCardExplanationText::GetDisplayTagLeafName(Effect.EffectType)));
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
				FText::FromString(WacomCardExplanationText::GetDisplayTagLeafName(Passive.Trigger)));
		}

		bool DefaultPassiveOutcomeTemplate(const FCardPassive& Passive, FText& OutTemplate)
		{
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnCompanionCount))
			{
				OutTemplate = LOCTEXT("TemplatePassiveOutcomeCompanionCount", "使此牌回到手中。");
				return true;
			}
			return false;
		}
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
				LogWacomCardExplanationTemplateResolver,
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
				LogWacomCardExplanationTemplateResolver,
				Verbose,
				TEXT("Missing card passive trigger explanation template for '%s'; using generated fallback."),
				*Passive.Trigger.ToString());
		}
		return DefaultPassiveTriggerTemplate(Passive);
	}

	bool ResolvePassiveOutcomeTemplate(
		const FCardPassive& Passive,
		const UWacomCardExplanationLexicon* Lexicon,
		FText& OutTemplate)
	{
		FWacomCardExplanationTemplateEntry Entry;
		if (Lexicon && Lexicon->FindPassiveOutcomeTemplate(Passive.Trigger, Entry))
		{
			OutTemplate = Entry.Template;
			return true;
		}
		if (DefaultPassiveOutcomeTemplate(Passive, OutTemplate))
		{
			return true;
		}
		if (Lexicon)
		{
			UE_LOG(
				LogWacomCardExplanationTemplateResolver,
				Verbose,
				TEXT("Missing card passive outcome explanation template for '%s'; no outcome block generated."),
				*Passive.Trigger.ToString());
		}
		return false;
	}
}

#undef LOCTEXT_NAMESPACE
