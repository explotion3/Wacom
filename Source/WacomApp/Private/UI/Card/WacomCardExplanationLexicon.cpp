// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardExplanationLexicon.h"

#include "Tags/WacomGameplayTags.h"

#define LOCTEXT_NAMESPACE "WacomCardExplanationLexicon"

namespace WacomCardExplanationLexiconKeys
{
	const FName CardUnknownName(TEXT("Card.UnknownName"));
	const FName SectionDescriptionTitle(TEXT("Section.DescriptionTitle"));
	const FName SectionPassiveTitle(TEXT("Section.PassiveTitle"));
	const FName DetailSkipPrefix(TEXT("Detail.SkipPrefix"));
	const FName NoteParenthesized(TEXT("Note.Parenthesized"));
	const FName ConditionUnknownHandZone(TEXT("Condition.UnknownHandZone"));
	const FName ConditionUnknownStatus(TEXT("Condition.UnknownStatus"));
	const FName ConditionSelfInZone(TEXT("Condition.SelfInZone"));
	const FName ConditionSelfNotInZone(TEXT("Condition.SelfNotInZone"));
	const FName ConditionTargetHasStatus(TEXT("Condition.TargetHasStatus"));
	const FName ConditionTargetHasNoStatus(TEXT("Condition.TargetHasNoStatus"));
	const FName ConditionFallback(TEXT("Condition.Fallback"));
	const FName ConditionFallbackNegated(TEXT("Condition.FallbackNegated"));
	const FName ModifierAddPositive(TEXT("Modifier.AddPositive"));
	const FName ModifierAddNegative(TEXT("Modifier.AddNegative"));
	const FName ModifierMultiply(TEXT("Modifier.Multiply"));
	const FName ModifierUnknown(TEXT("Modifier.Unknown"));
	const FName ModifierConditional(TEXT("Modifier.Conditional"));
}

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

	FWacomCardExplanationTagDisplayEntry MakeTagDisplayEntry(
		const FGameplayTag& KeyTag,
		const FText& DisplayName)
	{
		FWacomCardExplanationTagDisplayEntry Entry;
		Entry.KeyTag = KeyTag;
		Entry.DisplayName = DisplayName;
		return Entry;
	}

	FWacomCardExplanationNamedTextEntry MakeNamedTextEntry(
		const FName& Key,
		const FText& Text)
	{
		FWacomCardExplanationNamedTextEntry Entry;
		Entry.Key = Key;
		Entry.Text = Text;
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
		MakeTemplateEntry(WacomTags::Effect_Damage, LOCTEXT("DefaultDamage", "{icon:EffectIcon} 造成 {value:Magnitude} 点伤害。")),
		MakeTemplateEntry(WacomTags::Effect_Heal, LOCTEXT("DefaultHeal", "{icon:EffectIcon} 恢复 {value:Magnitude} 点生命。")),
		MakeTemplateEntry(WacomTags::Status_Shield, LOCTEXT("DefaultShield", "{icon:EffectIcon} 获得 {value:Magnitude} 点护盾。")),
		MakeTemplateEntry(WacomTags::Effect_Draw, LOCTEXT("DefaultDraw", "抽 {value:Magnitude} 张牌。")),
		MakeTemplateEntry(WacomTags::Effect_ApplyStatus_Poison, LOCTEXT("DefaultPoison", "施加 {value:Magnitude} 层 {status:EffectStatus}。")),
		MakeTemplateEntry(WacomTags::Effect_ApplyStatus_Slow, LOCTEXT("DefaultSlow", "施加 {value:Magnitude} 层 {status:EffectStatus}。")),
		MakeTemplateEntry(WacomTags::Effect_ApplyStatus_Freeze, LOCTEXT("DefaultFreeze", "施加 {value:Magnitude} 层 {status:EffectStatus}。")),
		MakeTemplateEntry(WacomTags::Effect_ApplyStatus_Twilight, LOCTEXT("DefaultTwilight", "施加 {value:Magnitude} 层 {status:EffectStatus}。")),
		MakeTemplateEntry(WacomTags::Effect_Card_AddCost, LOCTEXT("DefaultAddCost", "目标手牌费用增加 {value:Magnitude}。")),
		MakeTemplateEntry(WacomTags::Effect_Card_ReduceCost, LOCTEXT("DefaultReduceCost", "目标手牌费用降低 {value:Magnitude}。")),
		MakeTemplateEntry(WacomTags::Effect_Card_DiscardSelected, LOCTEXT("DefaultDiscardSelected", "弃置目标手牌。")),
		MakeTemplateEntry(WacomTags::Effect_Card_ExhaustSelected, LOCTEXT("DefaultExhaustSelected", "消耗目标手牌。")),
		MakeTemplateEntry(WacomTags::Effect_Discard, LOCTEXT("DefaultDiscard", "随机弃置 {value:Magnitude} 张手牌。")),
		MakeTemplateEntry(WacomTags::Effect_ExhaustSelf, LOCTEXT("DefaultExhaustSelf", "此牌打出后进入消耗牌堆。")),
		MakeTemplateEntry(WacomTags::Effect_GainKeyword, LOCTEXT("DefaultGainKeyword", "赋予目标手牌关键词。")),
		MakeTemplateEntry(WacomTags::Effect_RemoveStatus, LOCTEXT("DefaultRemoveStatus", "移除目标 {value:Magnitude} 层状态。")),
		MakeTemplateEntry(WacomTags::Effect_ModifyInitiative, LOCTEXT("DefaultModifyInitiative", "目标部位先机变化 {value:Magnitude}。")),
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

	PassiveOutcomeTemplates = {
		MakeTemplateEntry(WacomTags::Passive_Trigger_OnCompanionCount, LOCTEXT("DefaultPassiveOutcomeCompanionCount", "使此牌回到手中。"))
	};

	MagnitudeSourceTemplates = {
		MakeTemplateEntry(WacomTags::Magnitude_Source_RuntimeCost, LOCTEXT("DefaultMagnitudeSourceRuntimeCost", "相当于当前费用")),
		MakeTemplateEntry(WacomTags::Magnitude_Source_TargetStatusStacks, LOCTEXT("DefaultMagnitudeSourceTargetStatusStacks", "相当于目标{Status}层数")),
		MakeTemplateEntry(WacomTags::Magnitude_Source_HandCount, LOCTEXT("DefaultMagnitudeSourceHandCount", "相当于当前手牌数量"))
	};

	TagDisplayNames = {
		MakeTagDisplayEntry(WacomTags::HandZone_Left, LOCTEXT("DefaultHandZoneLeft", "左手区")),
		MakeTagDisplayEntry(WacomTags::HandZone_Both, LOCTEXT("DefaultHandZoneBoth", "双手区")),
		MakeTagDisplayEntry(WacomTags::HandZone_Right, LOCTEXT("DefaultHandZoneRight", "右手区")),
		MakeTagDisplayEntry(WacomTags::Status_Poison, LOCTEXT("DefaultStatusPoison", "中毒")),
		MakeTagDisplayEntry(WacomTags::Status_Slow, LOCTEXT("DefaultStatusSlow", "减速")),
		MakeTagDisplayEntry(WacomTags::Status_Freeze, LOCTEXT("DefaultStatusFreeze", "冻结")),
		MakeTagDisplayEntry(WacomTags::Status_Twilight, LOCTEXT("DefaultStatusTwilight", "暮气")),
		MakeTagDisplayEntry(WacomTags::Status_Stunned, LOCTEXT("DefaultStatusStunned", "眩晕")),
		MakeTagDisplayEntry(WacomTags::Status_Shield, LOCTEXT("DefaultStatusShield", "护盾"))
	};

	NamedTexts = {
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::CardUnknownName, LOCTEXT("DefaultUnknownCardName", "未知卡牌")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::SectionDescriptionTitle, LOCTEXT("DefaultDescriptionTitle", "描述")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::SectionPassiveTitle, LOCTEXT("DefaultPassiveTitle", "被动")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::DetailSkipPrefix, LOCTEXT("DefaultSkipPrefix", "不会生效：")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::NoteParenthesized, LOCTEXT("DefaultParenthesizedNote", "（{0}）")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::ConditionUnknownHandZone, LOCTEXT("DefaultUnknownHandZone", "指定区域")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::ConditionUnknownStatus, LOCTEXT("DefaultUnknownStatus", "指定状态")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::ConditionSelfInZone, LOCTEXT("DefaultSelfInZone", "仅当本卡在{0}时")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::ConditionSelfNotInZone, LOCTEXT("DefaultSelfNotInZone", "仅当本卡不在{0}时")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::ConditionTargetHasStatus, LOCTEXT("DefaultTargetHasStatus", "仅当目标有{0}时")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::ConditionTargetHasNoStatus, LOCTEXT("DefaultTargetHasNoStatus", "仅当目标没有{0}时")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::ConditionFallback, LOCTEXT("DefaultFallbackCondition", "仅当满足{0}时")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::ConditionFallbackNegated, LOCTEXT("DefaultFallbackNegatedCondition", "仅当不满足{0}时")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::ModifierAddPositive, LOCTEXT("DefaultModifierAddPositive", "数值 +{0}")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::ModifierAddNegative, LOCTEXT("DefaultModifierAddNegative", "数值 {0}")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::ModifierMultiply, LOCTEXT("DefaultModifierMultiply", "数值 ×{0}")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::ModifierUnknown, LOCTEXT("DefaultModifierUnknown", "数值修正 {0}")),
		MakeNamedTextEntry(WacomCardExplanationLexiconKeys::ModifierConditional, LOCTEXT("DefaultConditionalModifier", "{0}，{1}"))
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

bool UWacomCardExplanationLexicon::FindPassiveOutcomeTemplate(
	FGameplayTag TriggerTag,
	FWacomCardExplanationTemplateEntry& OutEntry) const
{
	return FindBestTemplate(PassiveOutcomeTemplates, TriggerTag, OutEntry);
}

bool UWacomCardExplanationLexicon::FindMagnitudeSourceTemplate(
	FGameplayTag MagnitudeSourceTag,
	FWacomCardExplanationTemplateEntry& OutEntry) const
{
	return FindBestTemplate(MagnitudeSourceTemplates, MagnitudeSourceTag, OutEntry);
}

bool UWacomCardExplanationLexicon::FindTagDisplayName(
	FGameplayTag Tag,
	FText& OutDisplayName) const
{
	return FindBestTagDisplayName(TagDisplayNames, Tag, OutDisplayName);
}

bool UWacomCardExplanationLexicon::FindNamedText(
	FName Key,
	FText& OutText) const
{
	if (Key.IsNone())
	{
		return false;
	}

	for (const FWacomCardExplanationNamedTextEntry& Entry : NamedTexts)
	{
		if (Entry.Key == Key && !Entry.Text.IsEmpty())
		{
			OutText = Entry.Text;
			return true;
		}
	}

	return false;
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

bool UWacomCardExplanationLexicon::FindBestTagDisplayName(
	const TArray<FWacomCardExplanationTagDisplayEntry>& Entries,
	FGameplayTag QueryTag,
	FText& OutDisplayName)
{
	if (!QueryTag.IsValid())
	{
		return false;
	}

	const FWacomCardExplanationTagDisplayEntry* BestEntry = nullptr;
	int32 BestDepth = INDEX_NONE;
	for (const FWacomCardExplanationTagDisplayEntry& Entry : Entries)
	{
		if (!Entry.KeyTag.IsValid() || Entry.DisplayName.IsEmpty())
		{
			continue;
		}

		if (QueryTag.MatchesTagExact(Entry.KeyTag))
		{
			OutDisplayName = Entry.DisplayName;
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

	OutDisplayName = BestEntry->DisplayName;
	return true;
}

#undef LOCTEXT_NAMESPACE
