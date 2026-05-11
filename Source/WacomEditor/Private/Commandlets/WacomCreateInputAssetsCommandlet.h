// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "WacomCreateInputAssetsCommandlet.generated.h"

/**
 * 生成 Enhanced Input 资产：
 *   - 战斗：11 个 Bool IA + IMC_Battle（1..7 / W / E / R / P）
 *   - 探索：IA_Move / IA_Look（Axis2D）+ IMC_Exploration（WASD + Mouse）
 *
 * 用法：
 *   "<UE>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<project>/Wacom.uproject" -run=WacomCreateInputAssets -NoSplash -Unattended
 *
 * 幂等：每次运行 create-or-replace。
 * 生成位置：Content/Wacom/Input/
 */
UCLASS()
class UWacomCreateInputAssetsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomCreateInputAssetsCommandlet();
	virtual int32 Main(const FString& Params) override;
};
