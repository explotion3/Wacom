// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RunStateTypes.h"
#include "WacomZoneDropTarget.generated.h"

class UWacomBackpackScreen;

/**
 * 背包 zone 拖拽接收器（Stage 4.5.3a）。
 *
 * Widget 生命周期声明：
 *   数据源：父 UWacomBackpackScreen 注入的 URunSession，仅作为 drop 写命令出口。
 *   更新触发：自身不刷新列表；MoveInstance 成功后走 RunSession -> ViewModelProvider -> BackpackScreen RebuildAll。
 *   订阅时机：不订阅任何事件。
 *   反订阅时机：不持有委托，NativeDestruct 无需反订阅。
 *   焦点/输入：只处理 UMG DragOver / Drop，不主动获取键盘焦点。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomZoneDropTarget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(EZoneKind InZoneKind, FGuid InOwnerInstanceId);
	void SetOwnerScreen(UWacomBackpackScreen* InScreen);

protected:
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack")
	EZoneKind ZoneKind = EZoneKind::Backpack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack")
	FGuid OwnerInstanceId;

	UPROPERTY(Transient)
	TWeakObjectPtr<UWacomBackpackScreen> OwnerScreen;
};
