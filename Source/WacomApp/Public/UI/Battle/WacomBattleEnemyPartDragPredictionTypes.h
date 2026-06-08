// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomBattleEnemyPartDragPredictionTypes.generated.h"

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyPartDragPredictionDebugInput
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "当前拖卡预测是否带有源卡，只用于场景部位目标诊断。"))
	bool bHasSourceCard = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "当前拖卡预测使用的源卡实例 ID。"))
	FGuid SourceCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "源卡当前运行时费用，用于排查先机预测。"))
	int32 SourceCardRuntimeCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "源卡是否为迅捷卡，用于排查先机预测。"))
	bool bSourceCardSwift = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "当前拖卡 preview 是否允许提交。"))
	bool bPreviewCanSubmit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "当前拖卡 preview 的拒绝原因。"))
	FName PreviewRejectReason = NAME_None;
};
