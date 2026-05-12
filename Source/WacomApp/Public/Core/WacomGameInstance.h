// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "WacomGameInstance.generated.h"

/**
 * Wacom 项目的 GameInstance。
 *
 * 第一阶段：空壳。配置在 `Config/DefaultEngine.ini` 的 GameInstanceClass。
 *
 * 未来扩展位：
 *   - 全局音效音量 / 图形设置
 *   - 账号 / 云存档句柄
 *   - 跨关卡持久日志句柄
 *
 * 现在立它的目的是让 UGameInstanceSubsystem 有明确的 Host，
 * 并给未来的全局状态腾位置。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomGameInstance : public UGameInstance
{
	GENERATED_BODY()
};
