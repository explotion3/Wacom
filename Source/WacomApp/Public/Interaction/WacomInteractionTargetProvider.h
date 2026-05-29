// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "WacomInteractionTargetProvider.generated.h"

/**
 * 交互目标提供者接口。
 *
 * 任何挂载在 Actor 上的 Component 可实现此接口，提供一个统一的交互目标描述。
 * PlayerController 的 cursor trace 通过此接口收集命中对象的目标信息，
 * 不再需要硬编码特定 Component 类型的查找。
 *
 * 当前仅用于 World 目标（场景敌人部位等）；
 * Card / Zone 目标后续通过 UMG slot 层接入。
 */
UINTERFACE(BlueprintType, MinimalAPI)
class UWacomInteractionTargetProvider : public UInterface
{
	GENERATED_BODY()
};

class WACOMAPP_API IWacomInteractionTargetProvider
{
	GENERATED_BODY()

public:
	/** 构建当前组件的统一交互目标描述。 */
	virtual FWacomInteractionTargetHandle BuildWorldTargetHandle() const = 0;
};
