// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Types/WacomEnums.h"
#include "Cards/CardExplanationTemplateTypes.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPhysique.h"
#include "Cards/CardZoneHook.h"
#include "Cards/CardPassive.h"
#include "Cards/CardUpgradeTypes.h"
#include "Cards/WacomCardFaceTypes.h"
#include "CardDefinition.generated.h"

class UTexture2D;

/**
 * 单个 Definition 在指定强化等级下的统一只读 Battle Profile。
 *
 * 本结构只持有 Definition 内部字段的只读指针，生命周期不得超过来源
 * UCardDefinition。它刻意不反射、不复制效果数组，供 Battle、Run 与 App
 * 使用同一解析结果，同时避免高频 Snapshot 复制整份制作数据。
 */
struct WACOMDATA_API FWacomResolvedCardProfile
{
	EWacomCardUpgradeTier UpgradeTier = EWacomCardUpgradeTier::White;
	bool bUsesTierProfile = false;
	const FText* Description = nullptr;
	int32 BaseCost = 0;
	int32 BaseCriticalChancePercent = 0;
	const FWacomCardDynamicCostRule* DynamicCostRule = nullptr;
	const FCardPhysique* Physique = nullptr;
	const TArray<FCardEffect>* Effects = nullptr;
	const TArray<FCardEffect>* PerfectReleaseEffects = nullptr;
	const TArray<FCardZoneHook>* ZoneHooks = nullptr;
	const TArray<FCardPassive>* Passives = nullptr;
	FGameplayTag Rarity;
};

/** TargetMode=HandCard 时的手牌目标基础筛选。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomHandCardTargetFilter
{
	GENERATED_BODY()

	/** 是否使用本结构覆盖默认推断规则。关闭时会按当前卡牌效果自动保持旧行为。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Target", meta = (ToolTip = "是否使用本结构覆盖默认推断规则。关闭时会按当前卡牌效果自动保持旧行为。"))
	bool bUseExplicitHandCardTargetFilter = false;

	/** 允许选择普通手牌作为目标。单位：布尔开关，仅影响 TargetMode=HandCard 的主动打牌目标。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Target", meta = (ToolTip = "允许选择普通手牌作为目标。仅影响 TargetMode=HandCard 的主动打牌目标。"))
	bool bAllowNormalHandCards = true;

	/** 允许选择左右手锚点作为目标。单位：布尔开关，仅影响 TargetMode=HandCard 的主动打牌目标。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Target", meta = (ToolTip = "允许选择左右手锚点作为目标。仅影响 TargetMode=HandCard 的主动打牌目标。"))
	bool bAllowHandAnchors = true;

	/** 目标手牌必须全部拥有的关键词。空集合表示不要求。会同时读取卡牌定义关键词和战斗中的临时关键词。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Target", meta = (ToolTip = "目标手牌必须全部拥有的关键词。空集合表示不要求。会同时读取卡牌定义关键词和战斗中的临时关键词。"))
	FGameplayTagContainer RequiredTargetKeywords;

	/** 目标手牌不能拥有的关键词。命中任意一个即不可作为目标。会同时读取卡牌定义关键词和战斗中的临时关键词。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Target", meta = (ToolTip = "目标手牌不能拥有的关键词。命中任意一个即不可作为目标。会同时读取卡牌定义关键词和战斗中的临时关键词。"))
	FGameplayTagContainer BlockedTargetKeywords;
};

/** 卡牌静态定义。字段说明见 Docs/WacomData.md。 */
UCLASS(BlueprintType)
class WACOMDATA_API UCardDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FName CardId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Presentation", meta = (ToolTip = "卡牌主题插画纹理。推荐使用带透明通道的像素卡面 Texture2D；为空时 CardView 沿用 WBP 中 authored CardArt Brush，便于旧卡牌渐进迁移。RarityBorder 仍独立使用 PaperSprite 图集，不由本字段承载。"))
	TObjectPtr<UTexture2D> CardIllustration = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Presentation", meta = (ToolTip = "可选的插画局部深度图。黑色表示更深，白色表示更靠近实体卡框，中灰表示插画 authored 基准深度；推荐与 CardIllustration 同尺寸或同比例，使用 Masks、sRGB=false、Nearest、NoMipmaps。为空时整张插画仍按统一凹入深度显示。"))
	TObjectPtr<UTexture2D> CardIllustrationDepthMap = nullptr;

	/**
	 * 本卡专属的 Battle 详情说明模板。
	 *
	 * 四个强化等级共用同一套句式；运行时 Value / Icon / Status 仍从当前
	 * Tier Profile 的结构化规则生成。空集合保持旧卡牌的 Lexicon 回退行为。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Presentation|Explanation",
		meta = (ToolTip = "本卡专属详情句式。Effects / Passives 按索引覆盖全局模板；留空时保持旧卡牌回退。只影响 UI，不参与战斗规则。"))
	FWacomCardExplanationTemplateSet ExplanationTemplates;

	/** 同一 CardDefinition 的探索表面。卡牌实例与 Definition 身份不会因表面切换而复制。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Run Face",
		meta = (ToolTip = "同一卡牌定义的探索表面静态合同。关闭时旧卡牌仍可只使用 Battle Face；启用后由 Run Context 卡面 Builder 读取。"))
	FWacomRunCardFaceDefinition RunFace;

	// 以下 BaseCost / Effects / Passives / TargetMode 等扁平字段属于 Battle Face v1。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	int32 BaseCost = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FGameplayTag Rarity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FGameplayTagContainer Keywords;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	ECardTargetMode TargetMode = ECardTargetMode::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Target", meta = (ToolTip = "TargetMode=HandCard 时用于筛选可选手牌目标。默认关闭显式配置以兼容旧卡牌。"))
	FWacomHandCardTargetFilter HandCardTargetFilter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FCardPhysique Physique;

	/** 主动效果。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TArray<FCardEffect> Effects;

	/** 完美释放命中时执行的效果。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TArray<FCardEffect> PerfectReleaseEffects;

	/** 区域相关效果或修正。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TArray<FCardZoneHook> ZoneHooks;

	/** 被动触发。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TArray<FCardPassive> Passives;

	/**
	 * 单 Definition 四阶强化数据。空数组表示旧 flat 卡，只解析为 White 且不可强化；
	 * 可强化卡必须严格提供 White/Blue/Yellow/Purple 四项。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Upgrade",
		meta = (ToolTip = "单 Definition 的四阶 Battle Profile。可强化卡必须严格填写白、蓝、黄、紫四项；空数组表示旧卡 White fallback。"))
	TArray<FWacomCardTierProfile> TierProfiles;

	bool UsesTierProfiles() const;
	const FWacomCardTierProfile* FindTierProfile(EWacomCardUpgradeTier Tier) const;
	FWacomResolvedCardProfile ResolveProfile(EWacomCardUpgradeTier Tier) const;
	FText ResolveDescription(EWacomCardUpgradeTier Tier) const;
	int32 ResolveBaseCost(EWacomCardUpgradeTier Tier) const;
	int32 ResolveBaseCriticalChancePercent(EWacomCardUpgradeTier Tier) const;
	const FCardPhysique& ResolvePhysique(EWacomCardUpgradeTier Tier) const;
	const TArray<FCardEffect>& ResolveEffects(EWacomCardUpgradeTier Tier) const;
	const TArray<FCardEffect>& ResolvePerfectReleaseEffects(EWacomCardUpgradeTier Tier) const;
	const TArray<FCardZoneHook>& ResolveZoneHooks(EWacomCardUpgradeTier Tier) const;
	const TArray<FCardPassive>& ResolvePassives(EWacomCardUpgradeTier Tier) const;
	FGameplayTag ResolveRarity(EWacomCardUpgradeTier Tier) const;

	/** 单 Definition 后 CardId 本身就是稳定身份。 */
	bool MatchesCardIdOrUpgradeFamily(FName Candidate) const;

	/** 是否具有已启用的探索表面。不会检查完整 authoring 合法性。 */
	bool HasEnabledRunFace() const;
};
