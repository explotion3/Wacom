// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardExplanationLexicon.h"

#include "Tags/WacomGameplayTags.h"

#define LOCTEXT_NAMESPACE "WacomCardExplanationLexicon"

namespace
{
	FWacomCardExplanationTemplateEntry MakeTemplateEntry(
		const FGameplayTag& KeyTag,
		const FText& Template)
	{
		FWacomCardExplanationTemplateEntry Entry;
		Entry.KeyTag = KeyTag;
		Entry.Template = Template;
		return Entry;
	}

	int32 TagDepth(const FGameplayTag& Tag)
	{
		FString TagText = Tag.ToString();
		if (TagText.IsEmpty())
		{
			return 0;
		}

		int32 Depth = 1;
		for (const TCHAR Character : TagText)
		{
			if (Character == TEXT('.'))
			{
				++Depth;
			}
		}
		return Depth;
	}
}

UWacomCardExplanationLexicon::UWacomCardExplanationLexicon()
{
	EffectTemplates = {
		MakeTemplateEntry(WacomTags::Effect_Damage, LOCTEXT("DefaultDamage", "造成 {value:Magnitude} 点伤害。")),
		MakeTemplateEntry(WacomTags::Effect_Heal, LOCTEXT("DefaultHeal", "恢复 {value:Magnitude} 点生命。")),
		MakeTemplateEntry(WacomTags::Status_Shield, LOCTEXT("DefaultShield", "获得 {value:Magnitude} 点护盾。")),
		MakeTemplateEntry(WacomTags::Effect_ApplyStatus_Poison, LOCTEXT("DefaultPoison", "施加 {value:Magnitude} 层 {status:EffectStatus}。")),
		MakeTemplateEntry(WacomTags::Effect_ApplyStatus_Slow, LOCTEXT("DefaultSlow", "施加 {value:Magnitude} 层 {status:EffectStatus}。")),
		MakeTemplateEntry(WacomTags::Effect_ApplyStatus_Freeze, LOCTEXT("DefaultFreeze", "施加 {value:Magnitude} 层 {status:EffectStatus}。")),
		MakeTemplateEntry(WacomTags::Effect_ApplyStatus_Twilight, LOCTEXT("DefaultTwilight", "施加 {value:Magnitude} 层 {status:EffectStatus}。")),
		MakeTemplateEntry(WacomTags::Effect_Card_AddCost, LOCTEXT("DefaultAddCost", "目标手牌费用增加 {value:Magnitude}。")),
		MakeTemplateEntry(WacomTags::Effect_Card_ReduceCost, LOCTEXT("DefaultReduceCost", "目标手牌费用降低 {value:Magnitude}。")),
		MakeTemplateEntry(WacomTags::Effect_Card_DiscardSelected, LOCTEXT("DefaultDiscardSelected", "弃置目标手牌。")),
		MakeTemplateEntry(WacomTags::Effect_Card_ExhaustSelected, LOCTEXT("DefaultExhaustSelected", "消耗目标手牌。")),
		MakeTemplateEntry(WacomTags::Effect_Shuffle_Random, LOCTEXT("DefaultShuffleRandom", "随机腾挪 1 张手牌。")),
		MakeTemplateEntry(WacomTags::Effect_Shuffle_FromBothToOther, LOCTEXT("DefaultShuffleFromBothToOther", "将双手区随机 1 张卡牌腾挪至其他区域。")),
		MakeTemplateEntry(WacomTags::Effect_Shuffle_ToRandomZone, LOCTEXT("DefaultShuffleToRandomZone", "该牌腾挪至随机区域。"))
	};

	PassiveTriggerTemplates = {
		MakeTemplateEntry(WacomTags::Passive_Trigger_AfterPlayed, LOCTEXT("DefaultPassiveAfterPlayed", "打出后：")),
		MakeTemplateEntry(WacomTags::Passive_Trigger_OnCompanionCount, LOCTEXT("DefaultPassiveCompanionCount", "每打出 {value:TriggerThreshold} 张伙伴：")),
		MakeTemplateEntry(WacomTags::Passive_Trigger_OnTwilightTriggered, LOCTEXT("DefaultPassiveTwilight", "暮气触发时：")),
		MakeTemplateEntry(WacomTags::Passive_Trigger_OnTurnStart, LOCTEXT("DefaultPassiveTurnStart", "回合开始：")),
		MakeTemplateEntry(WacomTags::Passive_Trigger_OnTurnEnd, LOCTEXT("DefaultPassiveTurnEnd", "回合结束：")),
		MakeTemplateEntry(WacomTags::Passive_Trigger_OnDraw, LOCTEXT("DefaultPassiveDraw", "抽到时：")),
		MakeTemplateEntry(WacomTags::Passive_Trigger_OnDiscard, LOCTEXT("DefaultPassiveDiscard", "弃掉时："))
	};
}

bool UWacomCardExplanationLexicon::FindEffectTemplate(
	FGameplayTag EffectType,
	FWacomCardExplanationTemplateEntry& OutEntry) const
{
	return FindBestTemplate(EffectTemplates, EffectType, OutEntry);
}

bool UWacomCardExplanationLexicon::FindPassiveTriggerTemplate(
	FGameplayTag TriggerTag,
	FWacomCardExplanationTemplateEntry& OutEntry) const
{
	return FindBestTemplate(PassiveTriggerTemplates, TriggerTag, OutEntry);
}

bool UWacomCardExplanationLexicon::FindBestTemplate(
	const TArray<FWacomCardExplanationTemplateEntry>& Entries,
	FGameplayTag QueryTag,
	FWacomCardExplanationTemplateEntry& OutEntry)
{
	if (!QueryTag.IsValid())
	{
		return false;
	}

	const FWacomCardExplanationTemplateEntry* BestEntry = nullptr;
	int32 BestDepth = INDEX_NONE;
	for (const FWacomCardExplanationTemplateEntry& Entry : Entries)
	{
		if (!Entry.KeyTag.IsValid() || Entry.Template.IsEmpty())
		{
			continue;
		}

		if (QueryTag.MatchesTagExact(Entry.KeyTag))
		{
			OutEntry = Entry;
			return true;
		}

		if (QueryTag.MatchesTag(Entry.KeyTag))
		{
			const int32 CandidateDepth = TagDepth(Entry.KeyTag);
			if (!BestEntry || CandidateDepth > BestDepth)
			{
				BestEntry = &Entry;
				BestDepth = CandidateDepth;
			}
		}
	}

	if (!BestEntry)
	{
		return false;
	}

	OutEntry = *BestEntry;
	return true;
}

#undef LOCTEXT_NAMESPACE
