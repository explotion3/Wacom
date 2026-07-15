// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Map/WacomMapTypes.h"
#include "WacomRunMapNodeBindingComponent.generated.h"

/**
 * 把 Battle / Shop / RunEvent / Treasure 场景 Host 映射到当前 Floor 的稳定 NodeId。
 * 组件不保存地图图结构、节点生命周期、行动成本或完成状态。
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent))
class WACOMAPP_API UWacomRunMapNodeBindingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomRunMapNodeBindingComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Run Map",
		meta = (ToolTip = "当前 Floor 内与此场景 Host 对应的稳定 NodeId。必须与 FloorMapDefinition 一致；不影响场景布局。"))
	FName NodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Run Map",
		meta = (ToolTip = "此 Host 承载的节点内容类型。必须与 FloorMapDefinition 中同 NodeId 的 NodeType 一致；Navigation 不需要 Host。"))
	EWacomMapNodeType NodeType = EWacomMapNodeType::Navigation;
};
