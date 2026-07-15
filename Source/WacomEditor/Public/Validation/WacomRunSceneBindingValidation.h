// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWorld;
class UWacomFloorMapDefinition;

/** Loaded exploration World 与当前 Floor 静态图之间的制作期绑定校验结果。 */
struct WACOMEDITOR_API FWacomRunSceneBindingValidationReport
{
	TArray<FText> Errors;
	TArray<FText> Warnings;

	bool HasErrors() const { return !Errors.IsEmpty(); }
	bool IsValid() const { return Errors.IsEmpty(); }
};

/** 只读取 loaded World；不会从 Actor 反向生成或修改 Floor 图。 */
struct WACOMEDITOR_API FWacomRunSceneBindingValidation
{
	static FWacomRunSceneBindingValidationReport ValidateLoadedWorld(
		const UWorld* World,
		const UWacomFloorMapDefinition* FloorDefinition);
};
