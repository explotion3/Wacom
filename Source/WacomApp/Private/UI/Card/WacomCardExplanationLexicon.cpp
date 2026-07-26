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

namespace WacomCardFaceSemanticIds
{
	const FName Backpack(TEXT("Card.Face.Backpack"));
	const FName Container(TEXT("Card.Face.Container"));
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

	FWacomCardFaceSemanticLexiconEntry MakeCardFaceSemanticEntry(
		const FName SemanticId,
		const FGameplayTag& SourceTag,
		const FText& DisplayName,
		const FText& Description)
	{
		FWacomCardFaceSemanticLexiconEntry Entry;
		Entry.SemanticId = SemanticId;
		Entry.SourceTag = SourceTag;
		Entry.DisplayName = DisplayName;
		Entry.Description = Description;
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
		MakeTagDisplayEntry(WacomTags::HandZone_Right, LOCTEXT("DefaultHandZoneRight", "右手区"))
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

	CardFaceSemantics = {
		MakeCardFaceSemanticEntry(
			WacomTags::Card_Keyword_Swift.GetTag().GetTagName(),
			WacomTags::Card_Keyword_Swift,
			LOCTEXT("CardFaceSwiftName", "迅捷"),
			LOCTEXT("CardFaceSwiftDescription", "不造成先机命中、不推进敌方先机、不触发完美释放，并跳过本次出牌后的先机归零行动。")),
		MakeCardFaceSemanticEntry(
			WacomTags::Card_Keyword_Retain.GetTag().GetTagName(),
			WacomTags::Card_Keyword_Retain,
			LOCTEXT("CardFaceRetainName", "保留"),
			LOCTEXT("CardFaceRetainDescription", "回合结束时不会被弃置；只保证留在下一回合手牌池，不保证原位置和顺序。")),
		MakeCardFaceSemanticEntry(
			WacomTags::Card_Keyword_Combo.GetTag().GetTagName(),
			WacomTags::Card_Keyword_Combo,
			LOCTEXT("CardFaceComboName", "连击"),
			LOCTEXT("CardFaceComboDescription", "打出后回到手牌，并尽量恢复打出前的相对位置。")),
		MakeCardFaceSemanticEntry(
			WacomTags::Card_Keyword_Companion.GetTag().GetTagName(),
			WacomTags::Card_Keyword_Companion,
			LOCTEXT("CardFaceCompanionName", "伙伴"),
			LOCTEXT("CardFaceCompanionDescription", "打出时计入伙伴次数；只有伙伴卡会让体格中的最大生命加成生效。")),
		MakeCardFaceSemanticEntry(
			WacomTags::Card_Keyword_Weapon.GetTag().GetTagName(),
			WacomTags::Card_Keyword_Weapon,
			LOCTEXT("CardFaceWeaponName", "武器"),
			LOCTEXT("CardFaceWeaponDescription", "武器类卡牌，可被容量效果、目标筛选及其它武器规则识别。")),
		MakeCardFaceSemanticEntry(
			WacomTags::Card_Keyword_Tool.GetTag().GetTagName(),
			WacomTags::Card_Keyword_Tool,
			LOCTEXT("CardFaceToolName", "工具"),
			LOCTEXT("CardFaceToolDescription", "工具类卡牌，用于卡牌分类和规则筛选。")),
		MakeCardFaceSemanticEntry(
			WacomTags::Card_Keyword_Hand.GetTag().GetTagName(),
			WacomTags::Card_Keyword_Hand,
			LOCTEXT("CardFaceHandName", "手"),
			LOCTEXT("CardFaceHandDescription", "左右手固有卡使用的分类标记。")),
		MakeCardFaceSemanticEntry(
			WacomTags::Card_Keyword_Exhaust.GetTag().GetTagName(),
			WacomTags::Card_Keyword_Exhaust,
			LOCTEXT("CardFaceExhaustName", "消耗"),
			LOCTEXT("CardFaceExhaustDescription", "获得此临时关键词后，本次打出会进入消耗牌堆。")),
		MakeCardFaceSemanticEntry(
			WacomTags::Card_Keyword_BagProvider.GetTag().GetTagName(),
			WacomTags::Card_Keyword_BagProvider,
			LOCTEXT("CardFaceBagProviderName", "容器兼容标记"),
			LOCTEXT("CardFaceBagProviderDescription", "历史兼容关键词；当前容量与背包可用性由 Capacity>0 决定，本关键词本身不提供容量。")),
		MakeCardFaceSemanticEntry(
			WacomTags::Card_Keyword_DeleteProvider.GetTag().GetTagName(),
			WacomTags::Card_Keyword_DeleteProvider,
			LOCTEXT("CardFaceDeleteProviderName", "删牌"),
			LOCTEXT("CardFaceDeleteProviderDescription", "持有任意一张即可启用删牌换金币；最后一张提供者只能单独出售。")),
		MakeCardFaceSemanticEntry(
			WacomCardFaceSemanticIds::Backpack,
			FGameplayTag(),
			LOCTEXT("CardFaceBackpackName", "背包"),
			LOCTEXT("CardFaceBackpackDescription", "A 类容器，容量计入通量存放区总容量。")),
		MakeCardFaceSemanticEntry(
			WacomCardFaceSemanticIds::Container,
			FGameplayTag(),
			LOCTEXT("CardFaceContainerName", "容器"),
			LOCTEXT("CardFaceContainerDescription", "B 类容器，开辟专属存放区，容量为卡面容量减一；区内卡牌可获得容量效果。"))
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

bool UWacomCardExplanationLexicon::FindCardFaceSemantic(
	const FName SemanticId,
	const FGameplayTag SourceTag,
	FWacomCardFaceSemanticLexiconEntry& OutEntry) const
{
	if (SemanticId.IsNone() && !SourceTag.IsValid())
	{
		return false;
	}

	for (int32 Index = CardFaceSemantics.Num() - 1; Index >= 0; --Index)
	{
		const FWacomCardFaceSemanticLexiconEntry& Entry =
			CardFaceSemantics[Index];
		if (!SemanticId.IsNone()
			&& Entry.SemanticId == SemanticId)
		{
			OutEntry = Entry;
			return true;
		}
	}

	if (!SourceTag.IsValid())
	{
		return false;
	}
	for (int32 Index = CardFaceSemantics.Num() - 1; Index >= 0; --Index)
	{
		const FWacomCardFaceSemanticLexiconEntry& Entry =
			CardFaceSemantics[Index];
		if (Entry.SourceTag.MatchesTagExact(SourceTag))
		{
			OutEntry = Entry;
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
