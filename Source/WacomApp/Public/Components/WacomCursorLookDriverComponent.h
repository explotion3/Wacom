// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WacomCursorLookDriverComponent.generated.h"

class APlayerController;

/** 一组可在不同第一人称活动间复制、但不会改写组件制作值的鼠标观察参数。 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCursorLookProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Cursor Look",
		meta = (Units = "deg", ToolTip = "最大水平观察偏移，单位度。推荐 6-18；只影响当前活动视角。"))
	float YawClampDegrees = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Cursor Look",
		meta = (Units = "deg", ToolTip = "最大垂直观察偏移，单位度。推荐 4-12；只影响当前活动视角。"))
	float PitchClampDegrees = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Cursor Look",
		meta = (ToolTip = "鼠标中心稳定区的半径，X 为水平、Y 为垂直，按视口半宽/半高归一化。0 表示无死区；推荐商店浏览使用 0.08-0.20。"))
	FVector2D CursorDeadZoneNormalized = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Cursor Look",
		meta = (ToolTip = "死区外的鼠标响应指数。1 为线性；大于 1 会让中心附近更稳定，同时仍在屏幕边缘精确达到 Clamp。推荐 1.0-1.6。"))
	float CursorResponseExponent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Cursor Look",
		meta = (ToolTip = "水平鼠标响应倍率。推荐 0.5-2；不会修改来源组件的制作值。"))
	float LookYawScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Cursor Look",
		meta = (ToolTip = "垂直鼠标响应倍率。推荐 0.5-2；负值可用于运行时反转 Y。"))
	float LookPitchScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Cursor Look",
		meta = (ToolTip = "视角追随插值速度，单位 1/秒。0 表示立即到达，推荐 8-18。"))
	float LookInterpSpeed = 12.0f;

	bool IsFinite() const;
	FWacomCursorLookProfile Sanitized() const;
};

/**
 * Shared cursor-to-look-offset driver.
 *
 * Computes a smoothed yaw / pitch offset from the mouse cursor location.
 * It never moves an actor or writes ControlRotation; callers decide how to
 * apply the offset to their current camera mode.
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent))
class WACOMAPP_API UWacomCursorLookDriverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomCursorLookDriverComponent();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Camera|Cursor Look")
	bool UpdateFromPlayerCursor(
		APlayerController* PlayerController,
		float DeltaTime,
		float YawClampDegrees,
		float PitchClampDegrees,
		float LookYawScale = 1.0f,
		float LookPitchScale = 1.0f,
		float LookInterpSpeed = 0.0f);

	bool UpdateFromPlayerCursor(
		APlayerController* PlayerController,
		float DeltaTime,
		const FWacomCursorLookProfile& Profile);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Camera|Cursor Look")
	void UpdateFromNormalizedCursor(
		FVector2D NormalizedCursor,
		float DeltaTime,
		float YawClampDegrees,
		float PitchClampDegrees,
		float LookYawScale = 1.0f,
		float LookPitchScale = 1.0f,
		float LookInterpSpeed = 0.0f);

	void UpdateFromNormalizedCursor(
		FVector2D NormalizedCursor,
		float DeltaTime,
		const FWacomCursorLookProfile& Profile);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Camera|Cursor Look")
	void ResetLookOffset();

	UFUNCTION(BlueprintPure, Category = "Wacom|Camera|Cursor Look")
	FRotator GetCurrentLookOffset() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Camera|Cursor Look")
	FVector2D GetCurrentLookOffset2D() const { return FVector2D(CurrentYawOffset, CurrentPitchOffset); }

private:
	float CurrentYawOffset = 0.0f;
	float CurrentPitchOffset = 0.0f;
	float TargetYawOffset = 0.0f;
	float TargetPitchOffset = 0.0f;
};
