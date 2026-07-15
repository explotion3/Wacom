// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "WacomRunPathTraversalComponent.generated.h"

class AWacomPlayerCharacter;
class AWacomRunPathSegmentActor;
class UCameraShakeBase;
class UWacomCursorLookDriverComponent;
class UWacomFirstPersonWalkBobComponent;
struct FWacomLocalSettingsSnapshot;
enum class EWacomRuntimeSettingsChangeReason : uint8;

UENUM(BlueprintType)
enum class EWacomRunPathTraversalState : uint8
{
	Inactive,
	Anchored,
	Traversing,
	Suspended,
};

DECLARE_MULTICAST_DELEGATE(FWacomRunPathBoundaryReachedNative);

/**
 * 只负责局部 Spline 移动、View Source 和镜头反馈。
 * 它不持有 RunSession、traversal ticket、Node 合法性或规则成本。
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent))
class WACOMAPP_API UWacomRunPathTraversalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomRunPathTraversalComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Path|Movement",
		meta = (ClampMin = "0.0", Units = "cm/s", ToolTip = "沿当前 PathSpline 的移动速度，单位厘米/秒。推荐 120-400；只影响表现，不改变规则提交时机。"))
	float MoveSpeed = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Path|Movement",
		meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Spline 起点/终点 one-shot 判定的迟滞距离，单位厘米。推荐 2-12；避免边界浮点抖动重复广播，不影响布局。"))
	float BoundaryHysteresisDistance = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Path|Look",
		meta = (ToolTip = "鼠标驱动视角的最大水平偏移，单位度。推荐 6-18；不改变 Path 方向。"))
	float YawClampDegrees = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Path|Look",
		meta = (ToolTip = "鼠标驱动视角的最大垂直偏移，单位度。推荐 4-12；不改变 Path 方向。"))
	float PitchClampDegrees = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Path|Look",
		meta = (ToolTip = "鼠标水平视角响应倍率。推荐 0.5-2；会与本地设置的视角响应强度相乘。"))
	float LookYawScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Path|Look",
		meta = (ToolTip = "鼠标垂直视角响应倍率。推荐 0.5-2；会与反转 Y 和本地设置相乘。"))
	float LookPitchScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Path|Look",
		meta = (ToolTip = "视角追随插值速度，单位 1/秒。0 表示立即到达，推荐 8-18。"))
	float LookInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Path|Camera Shake",
		meta = (ToolTip = "Traversing 且发生真实位移时是否播放 CameraShake。仍受镜头运动强度设置控制。"))
	bool bUseWalkCameraShake = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Path|Camera Shake",
		meta = (EditCondition = "bUseWalkCameraShake", ToolTip = "沿 Path 移动时使用的 CameraShake 类。建议无限循环并配置 blend out。"))
	TSubclassOf<UCameraShakeBase> WalkCameraShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Path|Camera Shake",
		meta = (EditCondition = "bUseWalkCameraShake", ToolTip = "CameraShake 基础强度倍率。推荐 0.5-1.5；运行时再乘镜头运动强度。"))
	float WalkCameraShakeScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Path|Camera Shake",
		meta = (EditCondition = "bUseWalkCameraShake", Units = "s", ToolTip = "停止真实移动后延迟关闭 Shake 的宽限时间，单位秒。推荐 0.03-0.15。"))
	float WalkCameraShakeStopGraceSeconds = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Path|Staging",
		meta = (Units = "s", ToolTip = "从临时第一人称站位回到当前 Path/Anchor View 的过渡时间，单位秒。推荐 0.2-0.6。"))
	float ReturnStageBlendTimeSeconds = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Path|Staging",
		meta = (ToolTip = "回到 Run Path View 时使用的过渡曲线。"))
	EWacomFirstPersonViewStageBlendCurve ReturnStageBlendCurve =
		EWacomFirstPersonViewStageBlendCurve::SmoothStep;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Path|Staging",
		meta = (ToolTip = "Ease 曲线强度。推荐 1-3；SmoothStep 不使用该值。"))
	float ReturnStageBlendEasePower = 2.0f;

	bool AnchorAtTransform(const FTransform& AnchorTransform);
	bool BeginTraversal(AWacomRunPathSegmentActor* Segment);
	void DeactivateTraversal();
	bool SuspendTraversal();
	bool ResumeTraversal(bool bPreserveCursorLookOffset = false);

	bool HandleMoveInput(const FVector2D& Input);
	bool HandleLookInput(const FVector2D& Input);
	void SetCursorLookOverrideNormalized(
		FVector2D NormalizedCursor,
		float Scale = 1.0f,
		float InterpSpeedOverride = -1.0f);
	void ClearCursorLookOverride();

	EWacomRunPathTraversalState GetTraversalState() const { return State; }
	AWacomRunPathSegmentActor* GetActivePath() const { return ActivePath.Get(); }
	float GetDistanceAlongSpline() const { return DistanceAlongSpline; }
	bool TryGetCurrentViewTransform(FTransform& OutViewTransform) const;
	bool TryBuildReturnToRunPathStageRequest(FWacomFirstPersonViewStageRequest& OutRequest) const;

	FWacomRunPathBoundaryReachedNative& OnReachedStartNative() { return ReachedStartNative; }
	FWacomRunPathBoundaryReachedNative& OnReachedEndNative() { return ReachedEndNative; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
#if WITH_AUTOMATION_TESTS
	friend struct FWacomRunPathTraversalTestAccess;
#endif

	TWeakObjectPtr<AWacomRunPathSegmentActor> ActivePath;
	FTransform AnchoredViewTransform = FTransform::Identity;
	float DistanceAlongSpline = 0.0f;
	float MoveAxis = 0.0f;
	EWacomRunPathTraversalState State = EWacomRunPathTraversalState::Inactive;
	EWacomRunPathTraversalState StateBeforeSuspend = EWacomRunPathTraversalState::Inactive;
	bool bStartBoundaryBroadcast = false;
	bool bEndBoundaryBroadcast = false;
	bool bLeftStartBoundary = false;

	bool bHasCursorLookOverride = false;
	FVector2D CursorLookOverrideNormalized = FVector2D::ZeroVector;
	float CursorLookOverrideScale = 1.0f;
	float CursorLookOverrideInterpSpeed = -1.0f;

	FWacomRunPathBoundaryReachedNative ReachedStartNative;
	FWacomRunPathBoundaryReachedNative ReachedEndNative;

	AWacomPlayerCharacter* GetOwnerCharacter() const;
	APlayerController* GetOwnerPlayerController() const;
	UWacomCursorLookDriverComponent* GetCursorLookDriver() const;
	UWacomFirstPersonWalkBobComponent* GetWalkBob() const;
	void TakeCharacterMovementOwnership();
	void ApplyInputProfile();
	void UpdateCursorLook(float DeltaTime);
	void ApplyViewTransform(float DeltaTime, float ActualDistanceDeltaCm);
	void UpdateBoundaryLatches();
	void ResetMotionFeedback();
	void UpdateWalkCameraShake(float DeltaTime, float ActualDistanceDeltaCm);
	void StopWalkCameraShake(bool bImmediately);
	void BindRuntimeSettings();
	void UnbindRuntimeSettings();
	void HandleRuntimeSettingsChanged(
		const FWacomLocalSettingsSnapshot& Snapshot,
		EWacomRuntimeSettingsChangeReason Reason);

	UPROPERTY(Transient)
	TObjectPtr<UCameraShakeBase> ActiveWalkCameraShakeInstance = nullptr;

	float WalkCameraShakeStopGraceRemainingSeconds = 0.0f;
	FDelegateHandle RuntimeSettingsChangedHandle;
	float RuntimeLookResponseStrength = 1.0f;
	float RuntimeCameraMotionStrength = 1.0f;
	bool bRuntimeInvertLookY = false;
};
