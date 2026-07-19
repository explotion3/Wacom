// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Styling/SlateBrush.h"
#include "WacomBattleCombatActivityStyle.generated.h"

class UWacomBattleEnemyIntentPresentationStyle;
struct FWacomBattleCombatActivityRowView;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleCombatActivityTagIconEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity",
		meta = (ToolTip = "要匹配的完整 GameplayTag。用于状态、被动与效果结果行图标；重复 Tag 由内容审计处理。"))
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity",
		meta = (ToolTip = "该 Tag 在常驻活动播报器中使用的像素图标 Brush。推荐 24×24 至 36×36。"))
	FSlateBrush IconBrush;
};

/** BattleHUD 常驻三行活动播报器的 UI-only 样式。 */
UCLASS(BlueprintType, Const)
class WACOMAPP_API UWacomBattleCombatActivityStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Icons",
		meta = (ToolTip = "玩家出牌根行动使用的头像 Brush。只属于 Battle UI，不写入角色规则定义。"))
	FSlateBrush PlayerPortraitBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Icons",
		meta = (ToolTip = "敌人根行动复用的 Intent 图标映射资产。IntentId 无映射时回退到本资产的未知图标。"))
	TObjectPtr<UWacomBattleEnemyIntentPresentationStyle> EnemyIntentStyle = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Icons",
		meta = (ToolTip = "状态、被动和效果 GameplayTag 到播报图标的精确映射。"))
	TArray<FWacomBattleCombatActivityTagIconEntry> TagIcons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Icons",
		meta = (ToolTip = "伤害结果行使用的通用图标 Brush。推荐 24×24 至 36×36；Tag 精确映射优先。"))
	FSlateBrush DamageIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Icons",
		meta = (ToolTip = "获得、弃置、消耗、费用或卡牌状态变化使用的通用图标 Brush。推荐 24×24 至 36×36。"))
	FSlateBrush CardFlowIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Icons",
		meta = (ToolTip = "玩家 Wait 根行动使用的图标 Brush。推荐 24×24 至 36×36。"))
	FSlateBrush WaitIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Icons",
		meta = (ToolTip = "系统类活动使用的通用图标 Brush。初始化事件不会进入短时播报。"))
	FSlateBrush SystemIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Icons",
		meta = (ToolTip = "无法解析玩家、Intent、Tag 或事件专用图标时使用的最终回退 Brush。"))
	FSlateBrush FallbackIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Icons",
		meta = (ToolTip = "Footer 当前回合前显示的沙漏图标 Brush。推荐 18×18 至 24×24 的硬像素图标。"))
	FSlateBrush TurnIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Timing",
		meta = (ToolTip = "同时可见的短时活动行上限。正式布局推荐为 3；只影响播报行，不影响完整历史。"))
	int32 MaxVisibleRows = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Timing",
		meta = (ToolTip = "新活动行淡入时间，单位秒。推荐 0.08–0.18。"))
	float EnterSeconds = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Timing",
		meta = (ToolTip = "同一根行动的结果行错峰间隔，单位秒。推荐 0.10–0.24。"))
	float ResultStaggerSeconds = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Timing",
		meta = (ToolTip = "每组最后一行发出后保留的最短可读时间，单位秒。推荐 0.65–1.20。"))
	float MinimumReadableSeconds = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Timing",
		meta = (ToolTip = "新行把旧行向上推动的过渡时间，单位秒。推荐 0.08–0.18。"))
	float ShiftSeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Timing",
		meta = (ToolTip = "队列清空后临时行继续停留的时间，单位秒。Footer 不受影响。"))
	float EmptyHoldSeconds = 0.90f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Timing",
		meta = (ToolTip = "队列清空后临时行淡出的时间，单位秒。Footer 始终保留。"))
	float CollapseSeconds = 0.18f;

	const FSlateBrush* ResolveTagIcon(FGameplayTag Tag) const;
	FSlateBrush ResolveActivityIconBrush(const FWacomBattleCombatActivityRowView& Row) const;
};
