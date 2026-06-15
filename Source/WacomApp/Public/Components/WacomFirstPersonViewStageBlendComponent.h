// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Components/ActorComponent.h"
#include "WacomFirstPersonViewStageBlendComponent.generated.h"

class APlayerController;
class AWacomPlayerCharacter;
class UWacomCursorLookDriverComponent;

/**
 * Smoothly stages a first-person camera view pose before another camera mode
 * captures the staged control rotation.
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent))
class WACOMAPP_API UWacomFirstPersonViewStageBlendComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomFirstPersonViewStageBlendComponent();

	bool StartBlendToStageRequest(
		APlayerController& PlayerController,
		const FWacomFirstPersonViewStageRequest& Request,
		TFunction<void()>&& OnFinished);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Camera",
		meta = (ToolTip = "取消当前第一人称镜头站位过渡；不会触发完成回调，也不会额外吸附到目标站位。"))
	void CancelActiveBlend();

	UFUNCTION(BlueprintPure, Category = "Wacom|Camera",
		meta = (ToolTip = "当前是否正在执行第一人称镜头站位过渡。"))
	bool IsStageBlendActive() const { return bStageBlendActive; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Camera",
		meta = (ToolTip = "当前第一人称镜头站位过渡进度，范围 0 到 1。"))
	float GetStageBlendAlpha() const;

	bool TryGetCurrentBaseViewTransform(FTransform& OutViewTransform) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
#if WITH_AUTOMATION_TESTS
	friend struct FWacomFirstPersonViewStageBlendTestAccess;
	friend struct FWacomFirstPersonViewStageReturnFlowTestAccess;
	friend struct FWacomGameMenuViewpointStageReturnFlowTestAccess;
#endif

	TWeakObjectPtr<APlayerController> ActivePlayerController;
	FWacomFirstPersonViewStageRequest ActiveRequest;
	FTransform StartViewTransform = FTransform::Identity;
	FTransform CurrentBaseViewTransform = FTransform::Identity;
	TFunction<void()> OnBlendFinished;
	float BlendDurationSeconds = 0.0f;
	float BlendElapsedSeconds = 0.0f;
	bool bStageBlendActive = false;
	bool bHasCurrentBaseViewTransform = false;
#if WITH_AUTOMATION_TESTS
	bool bHasStageLookNormalizedCursorOverrideForTest = false;
	FVector2D StageLookNormalizedCursorOverrideForTest = FVector2D::ZeroVector;
#endif

	AWacomPlayerCharacter* GetOwnerCharacter() const;
	UWacomCursorLookDriverComponent* GetCursorLookDriver() const;
	FRotator UpdateStageLookOffset(APlayerController& PlayerController, float DeltaTime);
	void FinishActiveBlend();
	void ResetActiveBlendState(bool bResetCursorLookOffset = true);
};
