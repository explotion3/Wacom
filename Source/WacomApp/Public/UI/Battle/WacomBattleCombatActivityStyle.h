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

/** BattleHUD 常驻流式活动播报器的 UI-only 样式。 */
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
		meta = (ToolTip = "无法映射到专用图标的系统类活动所使用的通用 Brush。Battle 初始化的回合开始活动使用下方独立的沙漏 Brush。"))
	FSlateBrush SystemIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Icons",
		meta = (ToolTip = "无法解析玩家、Intent、Tag 或事件专用图标时使用的最终回退 Brush。"))
	FSlateBrush FallbackIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Icons",
		meta = (ToolTip = "Footer 当前回合及 Battle 初始化‘第 1 回合开始’根行动共用的沙漏图标 Brush。推荐 18×18 至 24×24 的硬像素图标。"))
	FSlateBrush TurnIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Timing",
		meta = (ToolTip = "新活动行淡入时间，单位秒。推荐 0.08–0.18。"))
	float EnterSeconds = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Timing",
		meta = (ToolTip = "同一根行动的结果行错峰间隔，单位秒。推荐 0.10–0.24。"))
	float ResultStaggerSeconds = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Timing",
		meta = (ToolTip = "大量结果积压时使用的最小错峰间隔，单位秒。推荐 0.06–0.12；不影响事件顺序。"))
	float MinimumResultStaggerSeconds = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Timing",
		meta = (ToolTip = "剩余结果数超过该值后开始压缩错峰。推荐 4–8；只影响播报节奏。"))
	int32 BurstStaggerThreshold = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Timing",
		meta = (ToolTip = "剩余结果数达到该值后使用最小错峰。推荐 10–16，必须高于开始压缩数量。"))
	int32 BurstStaggerFullCompressionCount = 12;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Timing",
		meta = (ToolTip = "每组最后一行发出后保留的最短可读时间，单位秒。推荐 0.65–1.20。"))
	float MinimumReadableSeconds = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Retirement",
		meta = (ToolTip = "结果行自进入活动视口起、允许开始退场前的最短可见时间，单位秒。默认 0.35，推荐 0.25–0.50；用于限制大量结果造成的快速消失，不影响完整日志。"))
	float MinimumResultVisibleSeconds = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Timing",
		meta = (ToolTip = "新行把旧行向上推动的过渡时间，单位秒。推荐 0.08–0.18。"))
	float ShiftSeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Retirement",
		meta = (ToolTip = "位于活动区域下方时，单行开始淡出前的停留时间，单位秒。推荐 0.65–1.10。"))
	float BottomRowHoldSeconds = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Retirement",
		meta = (ToolTip = "位于活动区域下方时，单行淡出时间，单位秒。推荐 0.18–0.32。"))
	float BottomRowFadeSeconds = 0.24f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Retirement",
		meta = (ToolTip = "靠近活动区域顶部时，单行开始淡出前的最短停留时间，单位秒。推荐 0.12–0.28。"))
	float TopRowHoldSeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Retirement",
		meta = (ToolTip = "靠近活动区域顶部时，单行完成淡出的最短时间，单位秒。推荐 0.08–0.18。"))
	float TopRowFadeSeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Retirement",
		meta = (ToolTip = "新根行动到达时，上一枚常驻行动图标淡出的时间，单位秒。推荐 0.08–0.14；只使用透明度，不改变布局。"))
	float RootIconReplacementFadeSeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Layout",
		meta = (ToolTip = "短时播报基础活动视口高度，单位像素。旧正式 Feed 为 140；运行时还会按最少可见行数向上扩展，影响布局和位置衰减计算。"))
	float ActivityViewportHeightPixels = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Layout",
		meta = (ToolTip = "短时播报至少同时容纳的历史/结果行数，不含底部当前根行动。默认 5；运行时会在既有 WBP 上扩展视口与宿主高度，不会截断积压数据。"))
	int32 MinimumVisibleResultRows = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Layout",
		meta = (ToolTip = "单条活动行高度，单位像素。必须与 Row WBP 高度一致，正式布局推荐 40。"))
	float RowHeightPixels = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity|Retirement",
		meta = (ToolTip = "从活动视口顶部向下计算的加速衰减区域高度，单位像素。推荐 56–88。"))
	float TopFadeBandPixels = 72.0f;

	const FSlateBrush* ResolveTagIcon(FGameplayTag Tag) const;
	FSlateBrush ResolveActivityIconBrush(const FWacomBattleCombatActivityRowView& Row) const;
};
