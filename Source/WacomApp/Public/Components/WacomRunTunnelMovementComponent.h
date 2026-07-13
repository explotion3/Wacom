// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Components/ActorComponent.h"
#include "WacomRunTunnelMovementComponent.generated.h"

class AWacomRunTunnelSegmentActor;
class AWacomPlayerCharacter;
class UCameraShakeBase;
class UWacomCursorLookDriverComponent;
class UWacomFirstPersonWalkBobComponent;
struct FWacomLocalSettingsSnapshot;
enum class EWacomRuntimeSettingsChangeReason : uint8;

/**
 * Movement driver for paper-tunnel Run exploration.
 *
 * When active, W/S advances along the active segment spline and mouse position
 * drives a clamped look offset around the spline direction. It owns no Run rules.
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent))
class WACOMAPP_API UWacomRunTunnelMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomRunTunnelMovementComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Movement", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "800.0", Units = "cm/s", ToolTip = "Movement speed along the active tunnel segment spline while Run Tunnel movement is active."))
	float MoveSpeed = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Movement", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "45.0", Units = "deg", ToolTip = "Maximum yaw offset around the active spline direction while Run Tunnel movement is active."))
	float YawClampDegrees = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Movement", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "45.0", Units = "deg", ToolTip = "Maximum pitch offset around the active spline direction while Run Tunnel movement is active."))
	float PitchClampDegrees = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Movement", meta = (UIMin = "0.0", UIMax = "5.0", ToolTip = "Multiplier applied to cursor-driven X look before clamping yaw."))
	float LookYawScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Movement", meta = (UIMin = "0.0", UIMax = "5.0", ToolTip = "Multiplier applied to cursor-driven Y look before clamping pitch."))
	float LookPitchScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Movement", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0", ToolTip = "Interpolation speed in inverse seconds used when camera look moves toward the mouse cursor driven target. Set to 0 to snap immediately."))
	float LookInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Camera Shake", meta = (ToolTip = "是否使用 UE CameraShakeBase 替代自定义 WalkBob 偏移来表现 Run Tunnel 走路晃动；需要同时配置 WalkCameraShakeClass 才会生效。"))
	bool bUseWalkCameraShake = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Camera Shake", meta = (EditCondition = "bUseWalkCameraShake", ToolTip = "Run Tunnel 实际沿样条移动时启动的 CameraShake 类；可配置 LegacyCameraShake 或 CameraShakeBase/Pattern 派生蓝图。建议使用无限循环并设置 blend out。"))
	TSubclassOf<UCameraShakeBase> WalkCameraShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Camera Shake", meta = (EditCondition = "bUseWalkCameraShake", ClampMin = "0.0", UIMin = "0.0", UIMax = "3.0", ToolTip = "启动走路 CameraShake 时传给 PlayerCameraManager 的强度缩放。"))
	float WalkCameraShakeScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Camera Shake", meta = (EditCondition = "bUseWalkCameraShake", ClampMin = "0.0", UIMin = "0.0", UIMax = "0.5", Units = "s", ToolTip = "真实样条移动暂时低于死区后，延迟停止走路 CameraShake 的宽限时间，单位秒；用于避免低帧率或样条采样抖动造成频繁启动/停止。"))
	float WalkCameraShakeStopGraceSeconds = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Camera Shake", meta = (EditCondition = "bUseWalkCameraShake", ToolTip = "调试走路 CameraShake。勾选后 PIE 左上角会显示当前运行时配置、真实样条移动距离、是否启动成功；只用于排查，不影响规则。"))
	bool bDebugWalkCameraShake = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Staging", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0", Units = "s", ToolTip = "从战斗、商店、剧情等临时镜头站位返回当前 Run Tunnel 样条位置时的默认过渡时长，单位秒；设为 0 则立即回到样条。"))
	float ReturnStageBlendTimeSeconds = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Staging", meta = (ToolTip = "从临时镜头站位返回当前 Run Tunnel 样条位置时的过渡速度曲线；只影响移动节奏，不改变目标样条 View Pose。"))
	EWacomFirstPersonViewStageBlendCurve ReturnStageBlendCurve = EWacomFirstPersonViewStageBlendCurve::SmoothStep;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Staging", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "6.0", ToolTip = "EaseIn / EaseOut / EaseInOut 回程曲线的缓动强度；1 接近线性，数值越大起止越柔，中段越快。SmoothStep 不使用该值。"))
	float ReturnStageBlendEasePower = 2.0f;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Movement")
	bool ActivateRunTunnel(AWacomRunTunnelSegmentActor* InitialSegment, float StartDistance = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Movement")
	void DeactivateRunTunnel();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Movement")
	bool SuspendRunTunnel();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Movement")
	bool ResumeRunTunnel();

	bool ResumeRunTunnel(bool bPreserveCursorLookOffset);

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Movement")
	bool IsRunTunnelActive() const { return bRunTunnelActive; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Movement")
	bool IsRunTunnelSuspended() const { return bRunTunnelSuspended; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Movement")
	bool SwitchToSegment(AWacomRunTunnelSegmentActor* TargetSegment, float StartDistance = 0.0f);

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Movement")
	AWacomRunTunnelSegmentActor* GetActiveSegment() const { return ActiveSegment.Get(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Movement")
	float GetDistanceAlongSpline() const { return DistanceAlongSpline; }

	bool TryGetCurrentTunnelViewTransform(FTransform& OutViewTransform) const;
	bool TryBuildReturnToRunTunnelStageRequest(FWacomFirstPersonViewStageRequest& OutRequest) const;

	bool HandleMoveInput(const FVector2D& Input);
	bool HandleLookInput(const FVector2D& Input);
	void SetCursorLookOverrideNormalized(FVector2D NormalizedCursor, float Scale = 1.0f, float InterpSpeedOverride = -1.0f);
	void ClearCursorLookOverride();

#if WITH_AUTOMATION_TESTS
	bool HasCursorLookOverrideForTest() const { return bHasCursorLookOverride; }
	FVector2D GetCursorLookOverrideNormalizedForTest() const { return CursorLookOverrideNormalized; }
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
#if WITH_AUTOMATION_TESTS
	friend struct FWacomRunTunnelMovementTestAccess;
#endif

	UPROPERTY(Transient)
	TWeakObjectPtr<AWacomRunTunnelSegmentActor> ActiveSegment;

	float DistanceAlongSpline = 0.0f;
	float MoveAxis = 0.0f;
	bool bRunTunnelActive = false;
	bool bRunTunnelSuspended = false;
	bool bHasCursorLookOverride = false;
	FVector2D CursorLookOverrideNormalized = FVector2D::ZeroVector;
	float CursorLookOverrideScale = 1.0f;
	float CursorLookOverrideInterpSpeed = -1.0f;

	AWacomPlayerCharacter* GetOwnerCharacter() const;
	APlayerController* GetOwnerPlayerController() const;
	UWacomCursorLookDriverComponent* GetCursorLookDriver() const;
	UWacomFirstPersonWalkBobComponent* GetWalkBob() const;
	void ResetWalkBob(bool bSnapToZero = true);
	bool ShouldUseWalkCameraShake() const;
	bool IsWalkCameraShakeMovementActive(float ActualDistanceDeltaCm) const;
	void UpdateWalkCameraShake(float DeltaTime, float ActualDistanceDeltaCm);
	void StopWalkCameraShake(bool bImmediately = false);
	void ShowWalkCameraShakeDebug(const TCHAR* Status, float ActualDistanceDeltaCm, const UCameraShakeBase* StartedInstance = nullptr) const;
	void ApplyInputProfile();
	void UpdateCursorLook(float DeltaTime);
	void ApplyTunnelTransform(float DeltaTime = 0.0f, float ActualDistanceDeltaCm = 0.0f);
	void BindRuntimeSettings();
	void UnbindRuntimeSettings();
	void HandleRuntimeSettingsChanged(
		const FWacomLocalSettingsSnapshot& Snapshot,
		EWacomRuntimeSettingsChangeReason Reason);

	float WalkCameraShakeStopGraceRemainingSeconds = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UCameraShakeBase> ActiveWalkCameraShakeInstance = nullptr;

	FDelegateHandle RuntimeSettingsChangedHandle;
	float RuntimeLookResponseStrength = 1.0f;
	float RuntimeCameraMotionStrength = 1.0f;
	bool bRuntimeInvertLookY = false;
};
