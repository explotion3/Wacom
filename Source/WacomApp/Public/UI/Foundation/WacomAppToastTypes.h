// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomAppToastTypes.generated.h"

UENUM(BlueprintType)
enum class EWacomAppToastTone : uint8
{
	Neutral,
	Positive,
	Warning,
	Danger,
	System
};

/** UI-only data for app-level toast feedback outside battle event playback. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomAppToastView
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Toast")
	FText MessageText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Toast")
	EWacomAppToastTone Tone = EWacomAppToastTone::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Toast")
	FName IconKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Toast", meta = (ToolTip = "可选的 Toast 显示时长覆盖，单位为秒。0 或更小表示使用 Widget 默认显示时长；建议范围 0-10 秒。", ClampMin = "0.0", UIMin = "0.0", UIMax = "10.0"))
	float LifetimeOverride = 0.0f;
};
