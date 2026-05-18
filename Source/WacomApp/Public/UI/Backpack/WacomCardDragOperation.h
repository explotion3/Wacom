// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "RunStateTypes.h"
#include "WacomCardDragOperation.generated.h"

class UCardDefinition;

/**
 * 背包界面拖拽 payload（Stage 4.5.3a 引入，requirements R6.1）。
 *
 * 拖拽源（UWacomDeckCardWidget::NativeOnDragDetected）构造本对象并填四字段，
 * 拖拽目标（UWacomZoneDropTarget::NativeOnDrop）通过 `Cast<UWacomCardDragOperation>(Op)`
 * 取出 payload 并调用 URunSession::MoveInstance / DeleteCardForGold。
 *
 * 字段约束（R6.1）：
 *   - 当 `FromZone != EZoneKind::SpecialZone` 时，`FromZoneOwnerInstanceId` 必须为 `FGuid()`（invalid GUID）。
 *   - 当 `FromZone == EZoneKind::SpecialZone` 时，`FromZoneOwnerInstanceId` 必须等于源 SpecialZone 的 OwnerInstanceId。
 *
 * Widget 生命周期声明（注：UDragDropOperation 不是 widget，五项均 N/A）：
 *   数据源：N/A（仅作为 UMG 拖拽过程的 payload，不持有任何业务数据源）
 *   更新触发：N/A（一次性构造，drop 时被 Cast 取出，之后由 UMG 框架 GC）
 *   订阅时机：N/A
 *   反订阅时机：N/A
 *   焦点/输入：N/A
 */
UCLASS()
class WACOMAPP_API UWacomCardDragOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	/** 被拖拽 instance 的全局唯一 InstanceId。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Backpack")
	FGuid InstanceId;

	/** 被拖拽 instance 的源 zone 类别。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Backpack")
	EZoneKind FromZone = EZoneKind::Backpack;

	/**
	 * 仅当 `FromZone == EZoneKind::SpecialZone` 时有效，存源 SpecialZone 的 OwnerInstanceId；
	 * 其它 FromZone 取值时必须为 `FGuid()`（R6.1）。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Backpack")
	FGuid FromZoneOwnerInstanceId;

	/** 被拖拽卡的 Definition 指针（用于 DeleteZone 等需要按 Definition 取卡的入口）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Backpack")
	TObjectPtr<UCardDefinition> Definition = nullptr;
};
