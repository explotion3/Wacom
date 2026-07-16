// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WacomRunFloorSceneDescriptorActor.generated.h"

class UWacomFloorMapDefinition;

/**
 * Loaded Run Floor 的唯一场景声明；只选择 Floor 数据，不保存图状态或运行命令。
 * 每个独立 Run Floor 关卡必须且只能放置一个实例。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunFloorSceneDescriptorActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomRunFloorSceneDescriptorActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Floor",
		meta = (ToolTip = "当前关卡实现的唯一 Floor 数据。一个独立 Run Floor 关卡必须且只能放置一个声明 Actor；不得为空。"))
	TObjectPtr<UWacomFloorMapDefinition> FloorDefinition = nullptr;

	const UWacomFloorMapDefinition* GetFloorDefinition() const
	{
		return FloorDefinition;
	}
};
