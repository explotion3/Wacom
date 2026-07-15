// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunExplorationTypes.h"
#include "Types/WacomResult.h"

/** Camp handler 获得的只读上下文；不会暴露可写 FRunState。 */
struct WACOMRUN_API FRunCampActivityContext
{
	FWacomMapNodeHandle CampNode;
	int32 JourneyDay = 1;
	int32 CurrentPressure = 0;
};

/** Handler 只返回类型化完成事实，不能直接提交卡牌、压力或时间状态。 */
struct WACOMRUN_API FRunCampActivityOutcome
{
	ERunCampActivityKind Kind = ERunCampActivityKind::Rest;
	bool bCompleted = false;
	FName Detail = NAME_None;
};

/**
 * 未来 Camp 规则的类型安全扩展点。
 * 本轮没有 production handler；自动化使用 fake handler 验证生命周期。
 */
class WACOMRUN_API IRunCampActivityHandler
{
public:
	virtual ~IRunCampActivityHandler() = default;
	virtual ERunCampActivityKind GetKind() const = 0;
	virtual FWacomStatus Execute(
		const FRunCampActivityContext& Context,
		FRunCampActivityOutcome& OutOutcome) const = 0;
};
