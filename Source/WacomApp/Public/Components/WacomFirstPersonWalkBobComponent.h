// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WacomFirstPersonWalkBobComponent.generated.h"

/**
 * Lightweight first-person walk bob offset generator.
 *
 * It only computes a local-space location / rotation offset. Camera modes decide
 * how to apply that offset to their own base view pose.
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent))
class WACOMAPP_API UWacomFirstPersonWalkBobComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomFirstPersonWalkBobComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Walk Bob", meta = (ToolTip = "是否启用第一人称走路晃动。关闭后组件始终输出零偏移，不影响 RunTunnel 移动轨迹。"))
	bool bEnableWalkBob = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Walk Bob|Legacy", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "5.0", Units = "Hz", ToolTip = "兼容旧配置的参数，当前脚步曲线不再使用该值；脚步快慢由 StepDistanceCm 和实际样条移动距离决定。"))
	float Frequency = 1.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Walk Bob", meta = (ClampMin = "1.0", UIMin = "30.0", UIMax = "160.0", Units = "cm", ToolTip = "完成一次脚步曲线需要沿样条实际移动的距离，单位厘米；数值越小脚步越密。"))
	float StepDistanceCm = 85.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Walk Bob", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "8.0", Units = "cm", ToolTip = "走路时镜头上下起伏幅度，单位厘米；建议保持很小，避免第一人称卡牌 UI 晃动过强。"))
	float VerticalAmplitudeCm = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Walk Bob", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "4.0", Units = "cm", ToolTip = "落脚瞬间镜头向下压的幅度，单位厘米；用于让上下起伏更有脚步落地感。"))
	float FootPlantDropCm = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Walk Bob", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "5.0", Units = "cm", ToolTip = "走路时镜头左右轻微摆动幅度，单位厘米；默认关闭，避免第一人称卡牌 UI 左右晃动。"))
	float LateralAmplitudeCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Walk Bob", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "3.0", Units = "deg", ToolTip = "走路时镜头俯仰轻微摆动幅度，单位角度。"))
	float PitchAmplitudeDegrees = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Walk Bob", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "3.0", Units = "deg", ToolTip = "走路时镜头左右倾斜轻微摆动幅度，单位角度；默认关闭，避免 roll 造成晕眩。"))
	float RollAmplitudeDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Walk Bob", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0", ToolTip = "从静止进入移动时晃动强度的插值速度，单位为反秒；0 表示立即达到目标强度。"))
	float BlendInSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Walk Bob", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0", ToolTip = "从移动回到静止时晃动强度的衰减速度，单位为反秒；0 表示立即回到零偏移。"))
	float BlendOutSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Camera|Walk Bob", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0", Units = "cm", ToolTip = "单帧实际移动距离低于该值时视为静止，避免样条末端或浮点误差导致镜头继续晃动。"))
	float MovementDeadZoneCm = 0.1f;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Camera|Walk Bob", meta = (ToolTip = "根据本帧真实移动距离更新走路晃动偏移。DistanceDeltaCm 是实际移动距离，ReferenceSpeedCmPerSecond 是强度归一化参考速度。"))
	void UpdateWalkBobFromMovementDelta(
		float DeltaTime,
		float DistanceDeltaCm,
		float ReferenceSpeedCmPerSecond = 220.0f);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Camera|Walk Bob", meta = (ToolTip = "重置走路晃动状态。bSnapToZero 为 true 时立即清零；为 false 时保留相位但目标强度回到 0。"))
	void ResetWalkBob(bool bSnapToZero = true);

	UFUNCTION(BlueprintPure, Category = "Wacom|Camera|Walk Bob", meta = (ToolTip = "当前本地空间镜头位置偏移，X 不使用，Y 为左右，Z 为上下，单位厘米。"))
	FVector GetCurrentLocationOffset() const { return CurrentLocationOffset; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Camera|Walk Bob", meta = (ToolTip = "当前镜头旋转偏移，只用于最终 ControlRotation，不改变角色根朝向。"))
	FRotator GetCurrentRotationOffset() const { return CurrentRotationOffset; }

private:
	float StepPhaseNormalized = 0.0f;
	float CurrentStrength = 0.0f;
	FVector CurrentLocationOffset = FVector::ZeroVector;
	FRotator CurrentRotationOffset = FRotator::ZeroRotator;

	void RecalculateOffsets();
	void ClearOffsets();
};
