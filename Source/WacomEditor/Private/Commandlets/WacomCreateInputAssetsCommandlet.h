// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "WacomCreateInputAssetsCommandlet.generated.h"

/**
 * 生成 Enhanced Input 资产：
 *   - 战斗：IA_PlayCard1~7 / IA_Wait / IA_EndTurn / IA_OpenMenu + IMC_Battle
 *   - 探索：IA_Move / IA_Look / IA_Interact / IA_OpenBackpack / IA_OpenMenu + IMC_Exploration
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
