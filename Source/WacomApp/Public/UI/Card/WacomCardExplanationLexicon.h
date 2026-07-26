// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "WacomCardExplanationLexicon.generated.h"

namespace WacomCardExplanationLexiconKeys
{
	WACOMAPP_API extern const FName CardUnknownName;
	WACOMAPP_API extern const FName SectionDescriptionTitle;
	WACOMAPP_API extern const FName SectionPassiveTitle;
	WACOMAPP_API extern const FName DetailSkipPrefix;
	WACOMAPP_API extern const FName NoteParenthesized;
	WACOMAPP_API extern const FName ConditionUnknownHandZone;
	WACOMAPP_API extern const FName ConditionUnknownStatus;
	WACOMAPP_API extern const FName ConditionSelfInZone;
	WACOMAPP_API extern const FName ConditionSelfNotInZone;
	WACOMAPP_API extern const FName ConditionTargetHasStatus;
	WACOMAPP_API extern const FName ConditionTargetHasNoStatus;
	WACOMAPP_API extern const FName ConditionFallback;
	WACOMAPP_API extern const FName ConditionFallbackNegated;
	WACOMAPP_API extern const FName ModifierAddPositive;
	WACOMAPP_API extern const FName ModifierAddNegative;
	WACOMAPP_API extern const FName ModifierMultiply;
	WACOMAPP_API extern const FName ModifierUnknown;
	WACOMAPP_API extern const FName ModifierConditional;
}

namespace WacomCardFaceSemanticIds
{
	WACOMAPP_API extern const FName Backpack;
	WACOMAPP_API extern const FName Container;
}

/**
 * One typed explanation template keyed by an effect or passive trigger tag.
 *
 * Supported v1 slots:
 * - {value:Magnitude}
 * - {value:TriggerThreshold}
 * - {icon:EffectIcon}
 * - {status:EffectStatus}
 * - {keyword:Tag}
 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardExplanationTemplateEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation", meta = (ToolTip = "说明模板匹配的 GameplayTag。效果模板使用 Effect.*，被动触发模板使用 Passive.Trigger.*。"))
	FGameplayTag KeyTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation", meta = (MultiLine = true, ToolTip = "卡牌详情说明模板。支持 typed slot，例如 {value:Magnitude}、{status:EffectStatus}。"))
	FText Template;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardExplanationTagDisplayEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation", meta = (ToolTip = "需要覆盖显示名的 GameplayTag。常用于 Status.*、HandZone.* 等详情文本。"))
	FGameplayTag KeyTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation", meta = (ToolTip = "详情面板中显示给玩家看的名称。"))
	FText DisplayName;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardExplanationNamedTextEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation", meta = (ToolTip = "详情系统内部文案 key，例如 Section.DescriptionTitle、Condition.SelfInZone。"))
	FName Key;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation", meta = (MultiLine = true, ToolTip = "详情系统内部文案或格式模板。支持 FText::Format 的 {0}、{1} 等参数。"))
	FText Text;
};

/** UI-only tooltip and display-name entry for one compact card-face semantic. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardFaceSemanticLexiconEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation",
		meta = (ToolTip = "卡面语义的稳定身份。GameplayTag 关键词默认使用完整 Tag 名；背包与容器使用独立 UI 语义 ID。"))
	FName SemanticId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation",
		meta = (ToolTip = "可选的来源 GameplayTag。只用于展示和制作检索，不参与卡牌规则。"))
	FGameplayTag SourceTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation",
		meta = (ToolTip = "卡面 TypeText 中显示给玩家看的短名称。"))
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation",
		meta = (MultiLine = true, ToolTip = "鼠标悬浮该卡面关键词时显示的规则说明。只描述现有规则，不执行规则。"))
	FText Description;
};

/**
 * Data-driven card explanation templates.
 *
 * The compiler uses exact tag matches first, then the most specific parent tag
 * entry. Missing entries fall back to a readable generated block.
 */
UCLASS(BlueprintType, meta = (ToolTip = "卡牌详情说明模板表。只服务 UI 展示，不参与卡牌规则结算。"))
class WACOMAPP_API UWacomCardExplanationLexicon : public UDataAsset
{
	GENERATED_BODY()

public:
	UWacomCardExplanationLexicon();

	UFUNCTION(BlueprintPure, Category = "Wacom|Card Explanation")
	bool FindEffectTemplate(FGameplayTag EffectType, FWacomCardExplanationTemplateEntry& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Card Explanation")
	bool FindPassiveTriggerTemplate(FGameplayTag TriggerTag, FWacomCardExplanationTemplateEntry& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Card Explanation")
	bool FindPassiveOutcomeTemplate(FGameplayTag TriggerTag, FWacomCardExplanationTemplateEntry& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Card Explanation")
	bool FindMagnitudeSourceTemplate(FGameplayTag MagnitudeSourceTag, FWacomCardExplanationTemplateEntry& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Card Explanation")
	bool FindTagDisplayName(FGameplayTag Tag, FText& OutDisplayName) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Card Explanation")
	bool FindNamedText(FName Key, FText& OutText) const;

	/**
	 * Finds the matching card-face semantic entry in this lexicon only.
	 *
	 * Matching is by SemanticId first, then by exact SourceTag. A matched entry
	 * is returned even when DisplayName or Description is empty, so callers can
	 * merge per field across lexicons. Use the provider seam when the C++
	 * default lexicon should fill fields a configured asset left empty.
	 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Card Explanation")
	bool FindCardFaceSemantic(
		FName SemanticId,
		FGameplayTag SourceTag,
		FWacomCardFaceSemanticLexiconEntry& OutEntry) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation", meta = (TitleProperty = "KeyTag", ToolTip = "按 Effect.* tag 匹配的主动效果说明模板。"))
	TArray<FWacomCardExplanationTemplateEntry> EffectTemplates;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation", meta = (TitleProperty = "KeyTag", ToolTip = "按 Passive.Trigger.* tag 匹配的被动触发说明模板。"))
	TArray<FWacomCardExplanationTemplateEntry> PassiveTriggerTemplates;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation", meta = (TitleProperty = "KeyTag", ToolTip = "按 Passive.Trigger.* tag 匹配的规则专用被动结果说明。只描述 runtime 真实存在但不走 Passive.Effects 的结果。"))
	TArray<FWacomCardExplanationTemplateEntry> PassiveOutcomeTemplates;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation", meta = (TitleProperty = "KeyTag", ToolTip = "按 Magnitude.Source.* tag 匹配的数值来源短语。用于说明 {value:Magnitude} 的当前数值来自哪里，例如当前费用、目标状态层数。"))
	TArray<FWacomCardExplanationTemplateEntry> MagnitudeSourceTemplates;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation", meta = (TitleProperty = "KeyTag", ToolTip = "详情面板内 GameplayTag 的显示名覆盖。"))
	TArray<FWacomCardExplanationTagDisplayEntry> TagDisplayNames;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation", meta = (TitleProperty = "Key", ToolTip = "详情面板内部固定文案和格式模板。"))
	TArray<FWacomCardExplanationNamedTextEntry> NamedTexts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation",
		meta = (TitleProperty = "DisplayName", ToolTip = "卡面 TypeText 的逐词显示名与 Tooltip 说明覆盖。配置资产优先于 C++ 默认词典，且按字段生效：只填 DisplayName 时 Description 仍取 C++ 默认，不会关闭该关键词的 Tooltip。"))
	TArray<FWacomCardFaceSemanticLexiconEntry> CardFaceSemantics;

private:
	static bool FindBestTemplate(
		const TArray<FWacomCardExplanationTemplateEntry>& Entries,
		FGameplayTag QueryTag,
		FWacomCardExplanationTemplateEntry& OutEntry);

	static bool FindBestTagDisplayName(
		const TArray<FWacomCardExplanationTagDisplayEntry>& Entries,
		FGameplayTag QueryTag,
		FText& OutDisplayName);
};
