// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Engine/DataAsset.h"
#include "Map/WacomMapTypes.h"
#include "WacomJourneyDefinition.generated.h"

class UCharacterDefinition;
class UWacomFloorMapDefinition;

/** Journey 为五个时段提供的默认 Action Point 预算。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomRunPhaseActionPointBudgets
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Journey|Time",
		meta = (ToolTip = "Morning 初始 Action Point，单位为点。推荐首版 2；负数会被 Validator 拒绝。"))
	int32 Morning = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Journey|Time",
		meta = (ToolTip = "Day 初始 Action Point，单位为点。推荐首版 6；负数会被 Validator 拒绝。"))
	int32 Day = 6;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Journey|Time",
		meta = (ToolTip = "Dusk 初始 Action Point，单位为点。推荐首版 2；负数会被 Validator 拒绝。"))
	int32 Dusk = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Journey|Time",
		meta = (ToolTip = "Night 初始 Action Point，单位为点。推荐首版 2；负数会被 Validator 拒绝。"))
	int32 Night = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Journey|Time",
		meta = (ToolTip = "Sunrise 初始 Action Point，单位为点。推荐首版 1；负数会被 Validator 拒绝。"))
	int32 Sunrise = 1;
};

/** 一次旅程的 Floor 顺序、默认时间预算和 Decay 曲线。 */
UCLASS(BlueprintType)
class WACOMDATA_API UWacomJourneyDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UWacomJourneyDefinition();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Journey",
		meta = (ToolTip = "Journey 的稳定 ID。必须非空且在项目中唯一。"))
	FName JourneyId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Journey",
		meta = (ToolTip = "Journey 的玩家可读标题。成功总结页优先显示此值；留空时回退 JourneyId。"))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Journey|Completion",
		meta = (ToolTip = "唯一成功终局的 Floor-qualified Node handle。正式 Journey 必须指向最后一层可达、无出边的 Boss Encounter；旧 Debug/Authoring Journey 可暂时留空。"))
	FWacomMapNodeHandle SuccessTerminalNode;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Journey",
		meta = (ToolTip = "允许进入该 Journey 的角色定义。Validator 使用它检查强制 Floor Entrance 条件是否有保证满足方式。"))
	TArray<TObjectPtr<UCharacterDefinition>> SupportedCharacters;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Journey",
		meta = (ToolTip = "Journey 中不可逆的 Floor 顺序。至少包含一个有效 Floor，FloorId 必须唯一。"))
	TArray<TObjectPtr<UWacomFloorMapDefinition>> Floors;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Journey|Time",
		meta = (ToolTip = "五个时段的默认 Action Point 预算。复制到 Run working state 后可由未来正式规则调整。"))
	FWacomRunPhaseActionPointBudgets PhaseBudgets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Journey|Pressure",
		meta = (ToolTip = "按 JourneyDay 读取的 Base Decay 曲线，纵轴单位为压力点/天。首版推荐所有天数为 5；只在进入新 Morning 时结算。"))
	FRuntimeFloatCurve BaseDecayCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Journey|Pressure",
		meta = (ToolTip = "按 FloorDay 读取的 Overstay Decay 曲线，纵轴单位为压力点/天。首版推荐 1-3 天为 0、4 为 2、5 为 5、6 为 9、7+ 为 12。"))
	FRuntimeFloatCurve OverstayDecayCurve;

	const UWacomFloorMapDefinition* FindFloor(FName FloorId) const;
	int32 FindFloorIndex(FName FloorId) const;
	int32 EvaluateBaseDecay(int32 JourneyDay) const;
	int32 EvaluateOverstayDecay(int32 FloorDay) const;
};
