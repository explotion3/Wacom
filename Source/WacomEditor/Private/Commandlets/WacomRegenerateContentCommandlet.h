// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "WacomRegenerateContentCommandlet.generated.h"

/**
 * 重建 Wacom 原型内容 DataAsset。
 *
 * 用法（工程根目录下）：
 *   "<UE>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<project>/Wacom.uproject" -run=WacomRegenerateContent
 *
 * 幂等：每次运行都"create-or-replace"，旧 asset 的字段会被重新覆盖。
 */
UCLASS()
class UWacomRegenerateContentCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomRegenerateContentCommandlet();

	virtual int32 Main(const FString& Params) override;
};
