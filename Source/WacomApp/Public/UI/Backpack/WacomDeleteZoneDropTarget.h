// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Backpack/WacomZoneDropTarget.h"
#include "WacomDeleteZoneDropTarget.generated.h"

class UCardDefinition;
class UWacomCardDragOperation;

/**
 * 背包删牌区拖拽接收器。
 *
 * Widget 生命周期声明：
 *   数据源：父 UWacomBackpackScreen 提供的只读删牌 preview，Widget 不直接读取 URunSession。
 *   更新触发：自身不刷新列表；DeleteCardForGold 成功后走 RunSession -> ViewModelProvider -> BackpackScreen RebuildAll。
 *   订阅时机：不订阅任何事件。
 *   反订阅时机：不持有委托，NativeDestruct 无需反订阅。
 *   焦点/输入：只处理 UMG DragOver / Drop，不主动获取键盘焦点。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomDeleteZoneDropTarget : public UWacomZoneDropTarget
{
	GENERATED_BODY()

public:
	/** 测试/诊断用：按当前删牌置换规则预估成功 Toast 中显示的金币数。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Backpack|Delete")
	static int32 GetDeleteGoldRewardPreviewForToast(UCardDefinition* Card);

	/** 测试/诊断用：删牌请求只认具体卡牌实例。 */
	static FGuid GetDeleteInstanceIdForRequest(const UWacomCardDragOperation& CardOp);

	static FText FormatDeleteFailureReasonForToast(FName DisabledReason);

protected:
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
};
