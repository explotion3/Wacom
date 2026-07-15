// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WacomRunMapNodeAnchorActor.generated.h"

/** NodeId 到场景落点/View pose 的映射；不保存 Floor 图或 lifecycle。 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunMapNodeAnchorActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomRunMapNodeAnchorActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Run Map",
		meta = (ToolTip = "当前 Floor 内的稳定 NodeId。用于场景 Registry 定位；必须与 FloorMapDefinition 匹配。"))
	FName NodeId = NAME_None;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Map",
		meta = (ToolTip = "返回该节点锚点的世界 View Transform；只供表现定位，不提交节点移动。"))
	FTransform GetViewTransform() const { return GetActorTransform(); }
};
