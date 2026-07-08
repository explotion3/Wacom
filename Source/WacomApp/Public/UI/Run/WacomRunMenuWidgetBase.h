// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "WacomRunMenuWidgetBase.generated.h"

/**
 * Run 专用 GameMenu Screen 基类。
 *
 * 当前阶段先作为 Backpack / Shop / RunEvent 的正式血统标记，方便后续把
 * Run first-person menu lease / drop 合同从通用 UWacomMenuWidgetBase 迁出。
 */
UCLASS(Abstract, Blueprintable)
class WACOMAPP_API UWacomRunMenuWidgetBase : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	UWacomRunMenuWidgetBase(const FObjectInitializer& ObjectInitializer);
};
