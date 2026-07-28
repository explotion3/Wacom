// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Styling/SlateBrush.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "WacomBattleStatusPresentationCatalog.generated.h"

/** One host-specific, UI-only three-line explanation for a battle status. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleStatusRuleTextSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Status Presentation",
		meta = (MultiLine = true, ToolTip = "Tooltip 第一行：状态的核心效果。支持 Catalog 已注册的命名占位符。"))
	FText CoreEffectText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Status Presentation",
		meta = (MultiLine = true, ToolTip = "Tooltip 第二行：状态的触发时机。支持 Catalog 已注册的命名占位符。"))
	FText TriggerTimingText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Status Presentation",
		meta = (MultiLine = true, ToolTip = "Tooltip 第三行：状态的叠层、消耗或清除规则。支持 Catalog 已注册的命名占位符。"))
	FText StackPolicyText;

	bool IsComplete() const;
};

/** One canonical battle-status presentation entry shared by HUD status surfaces. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleStatusPresentationEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Status Presentation",
		meta = (Categories = "Status", ToolTip = "状态的规范 Status.* GameplayTag。必须有效且不可重复。"))
	FGameplayTag StatusTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Status Presentation",
		meta = (ToolTip = "解析到同一状态表现的精确别名。用于兼容 Effect.ApplyStatus.* 等 UI 查询；别名不可跨条目重复。"))
	TArray<FGameplayTag> LookupAliases;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Status Presentation",
		meta = (ToolTip = "状态在 Tooltip、Combat Log、Combat Activity 和卡牌详情中共用的玩家可读名称。"))
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Status Presentation",
		meta = (ToolTip = "Battle HUD 状态图标与 Combat Activity 状态结果共用的 Brush。推荐 24×24 至 36×36 的像素图标。"))
	FSlateBrush IconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Status Presentation",
		meta = (ToolTip = "状态图标的稳定升序优先级。数值相同时按完整 GameplayTag 排序；推荐相邻正式状态间隔 10。"))
	int32 SortPriority = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Status Presentation",
		meta = (ToolTip = "状态位于玩家身上时使用的准确三行规则说明。"))
	FWacomBattleStatusRuleTextSet PlayerRules;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Status Presentation",
		meta = (ToolTip = "状态位于敌方部位时使用的准确三行规则说明。"))
	FWacomBattleStatusRuleTextSet EnemyPartRules;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Status Presentation",
		meta = (ToolTip = "状态位于战斗内卡牌实例上时使用的准确三行规则说明；只在该状态存在卡牌宿主语义时填写。"))
	FWacomBattleStatusRuleTextSet CardRules;
};

/**
 * Project-level UI-only catalog for battle status presentation.
 *
 * Battle owns status facts and rule constants. This asset only localizes and
 * styles those facts for HUD surfaces.
 */
UCLASS(BlueprintType, Const, meta = (ToolTip = "战斗状态的项目级 UI 表现目录。统一状态名称、HUD 图标、排序和玩家/敌人三行规则；不参与战斗结算。"))
class WACOMAPP_API UWacomBattleStatusPresentationCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UWacomBattleStatusPresentationCatalog();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Status Presentation",
		meta = (TitleProperty = "StatusTag", ToolTip = "状态表现条目。StatusTag 与 LookupAliases 均使用精确匹配。"))
	TArray<FWacomBattleStatusPresentationEntry> Entries;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Status Presentation|Fallback",
		meta = (ToolTip = "未知状态在 Battle HUD 中使用的 fallback Brush。必须引用有效资源并具有正数 ImageSize。"))
	FSlateBrush FallbackIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Status Presentation|Fallback",
		meta = (ToolTip = "未知状态的通用三行说明。未知状态名称仍显示其完整 GameplayTag。"))
	FWacomBattleStatusRuleTextSet UnknownRules;

	const FWacomBattleStatusPresentationEntry* FindEntry(FGameplayTag QueryTag) const;
	FText ResolveDisplayName(FGameplayTag QueryTag) const;
	const FSlateBrush* ResolveIconBrush(FGameplayTag QueryTag) const;
	int32 ResolveSortPriority(FGameplayTag QueryTag) const;

	static bool IsIconBrushRenderable(const FSlateBrush& Brush);
	static bool IsIconBrushAssetConfigured(const FSlateBrush& Brush);

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
