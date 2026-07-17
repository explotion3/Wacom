// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Map/WacomMapTypes.h"
#include "RunOutcomeTypes.generated.h"

/** 单次 Run 的权威结局。失败总结表现尚未由本合同定义。 */
UENUM(BlueprintType)
enum class ERunOutcome : uint8
{
	InProgress,
	Succeeded,
	Failed,
};

/** Journey 成功事务冻结的只读摘要；不包含 Widget、Actor 或本地化布局。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunCompletionSummary
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Completion")
	FName JourneyId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Completion")
	FWacomMapNodeHandle TerminalNode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Completion")
	int32 CompletionDay = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Completion")
	int32 EnteredFloorCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Completion")
	int32 TotalFloorCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Completion")
	int32 ResolvedNodeCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Completion")
	int32 TotalNodeCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Completion")
	int32 FinalPressure = 0;

	bool IsValid() const;
};
