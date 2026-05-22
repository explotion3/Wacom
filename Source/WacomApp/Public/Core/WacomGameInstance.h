// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "WacomGameInstance.generated.h"

/**
 * Wacom 项目的 GameInstance。
 *
 * 当前主要作为项目 GameInstanceClass 和各类 GameInstanceSubsystem 的宿主。
 *
 * 后续扩展位：
 *   - 全局音效音量 / 图形设置
 *   - 账号 / 云存档句柄
 *   - 跨关卡持久日志句柄
 *
 * 它本身暂不持有玩法状态；Run / UI 等跨关卡服务通过 Subsystem 承接。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomGameInstance : public UGameInstance
{
	GENERATED_BODY()
};
