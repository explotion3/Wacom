// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "WacomCardExplanationLexicon.generated.h"

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation", meta = (TitleProperty = "KeyTag", ToolTip = "按 Effect.* tag 匹配的主动效果说明模板。"))
	TArray<FWacomCardExplanationTemplateEntry> EffectTemplates;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Explanation", meta = (TitleProperty = "KeyTag", ToolTip = "按 Passive.Trigger.* tag 匹配的被动触发说明模板。"))
	TArray<FWacomCardExplanationTemplateEntry> PassiveTriggerTemplates;

private:
	static bool FindBestTemplate(
		const TArray<FWacomCardExplanationTemplateEntry>& Entries,
		FGameplayTag QueryTag,
		FWacomCardExplanationTemplateEntry& OutEntry);
};
