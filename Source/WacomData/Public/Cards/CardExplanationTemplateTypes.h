// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CardExplanationTemplateTypes.generated.h"

/**
 * 一条由 CardDefinition 自己拥有的详情说明模板。
 *
 * 模板只控制玩家可见句式；规则数值仍从当前强化等级解析出的 Effect / Passive
 * 读取。WacomApp 会把 typed slot 编译为 Value、Icon、Status 等结构化 Run。
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomCardExplanationLineTemplate
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Presentation|Explanation",
		meta = (MultiLine = true, ToolTip = "当前规则条目的专属详情句式。留空时继续使用被动 DisplayText、全局 Lexicon 或 C++ 回退。支持的 typed slot 以 Docs/WacomDataAuthoring.md 为准。"))
	FText Template;

	/**
	 * 仅隐藏该条规则的详情投影，不影响正式规则结算。
	 * 用于一个玩家可见句式由多个内部 Effect 协作完成的情况。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Presentation|Explanation",
		meta = (ToolTip = "是否隐藏该规则条目的详情投影。仅影响详情 UI，不影响战斗结算；适合多个内部 Effect 合成一条玩家文案的情况。"))
	bool bSuppressInDetails = false;
};

/** 一条由卡牌关键词驱动的详情说明。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomCardKeywordExplanationTemplate
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Presentation|Explanation",
		meta = (Categories = "Card.Keyword", ToolTip = "本说明对应的卡牌关键词。必须同时存在于当前 CardDefinition.Keywords。"))
	FGameplayTag Keyword;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Presentation|Explanation",
		meta = (MultiLine = true, ToolTip = "关键词在详情描述区中的专属句式。使用 {keyword:Keyword} 保留类型化关键词身份与后续 Tooltip 扩展能力。"))
	FText Template;
};

/**
 * 单张卡牌的专属详情模板集合。
 *
 * 数组索引与当前 Tier Profile 中的 Effects / Passives 严格对应。四个强化等级
 * 共用本集合，因此模板不会随着 Tier 重复制作，只有结构化数值随 Tier 解析。
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomCardExplanationTemplateSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Presentation|Explanation",
		meta = (TitleProperty = "Template", ToolTip = "主动效果专属模板。数组非空时数量必须与当前卡牌 Effects 完全一致；空元素单独回退到全局效果模板。"))
	TArray<FWacomCardExplanationLineTemplate> EffectTemplates;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Presentation|Explanation",
		meta = (TitleProperty = "Template", ToolTip = "被动专属模板。数组非空时数量必须与当前卡牌 Passives 完全一致；非空元素是该被动的完整玩家文案。"))
	TArray<FWacomCardExplanationLineTemplate> PassiveTemplates;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Presentation|Explanation",
		meta = (TitleProperty = "Keyword", ToolTip = "需要进入详情描述区的关键词句式，按数组顺序显示。只影响详情 UI，不改变关键词规则。"))
	TArray<FWacomCardKeywordExplanationTemplate> KeywordTemplates;

	/**
	 * 当前 Tier DynamicCostRule 的完整玩家文案。
	 * 它显示在“被动”分区，但不伪造 FCardPassive 或重复实现费用规则。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Presentation|Explanation",
		meta = (MultiLine = true, ToolTip = "当前卡牌动态费用规则的完整详情文案，显示在“被动”分区。支持 {status:CountedStatus}、{value:ReductionPerMatchingCard}、{value:MinimumCost}。"))
	FText DynamicCostTemplate;

	const FText* FindEffectTemplate(const int32 EffectIndex) const
	{
		if (!EffectTemplates.IsValidIndex(EffectIndex)
			|| EffectTemplates[EffectIndex].bSuppressInDetails
			|| EffectTemplates[EffectIndex].Template.IsEmpty())
		{
			return nullptr;
		}
		return &EffectTemplates[EffectIndex].Template;
	}

	const FText* FindPassiveTemplate(const int32 PassiveIndex) const
	{
		if (!PassiveTemplates.IsValidIndex(PassiveIndex)
			|| PassiveTemplates[PassiveIndex].bSuppressInDetails
			|| PassiveTemplates[PassiveIndex].Template.IsEmpty())
		{
			return nullptr;
		}
		return &PassiveTemplates[PassiveIndex].Template;
	}

	bool ShouldSuppressEffect(const int32 EffectIndex) const
	{
		return EffectTemplates.IsValidIndex(EffectIndex)
			&& EffectTemplates[EffectIndex].bSuppressInDetails;
	}

	bool ShouldSuppressPassive(const int32 PassiveIndex) const
	{
		return PassiveTemplates.IsValidIndex(PassiveIndex)
			&& PassiveTemplates[PassiveIndex].bSuppressInDetails;
	}
};
