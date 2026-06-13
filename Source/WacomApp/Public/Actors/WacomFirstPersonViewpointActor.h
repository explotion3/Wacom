// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WacomFirstPersonViewpointActor.generated.h"

class UArrowComponent;
class USceneComponent;

/**
 * Editor-authored first-person camera view pose.
 *
 * The actor transform represents where the first-person camera should be,
 * not where the player capsule/root should be.
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomFirstPersonViewpointActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomFirstPersonViewpointActor();

	UFUNCTION(BlueprintPure, Category = "Wacom|Camera")
	USceneComponent* GetViewRootComponent() const { return ViewRootComponent; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Camera",
		meta = (AllowPrivateAccess = "true", ToolTip = "第一人称镜头站位根组件。Actor Transform 表示摄像机 View Pose，不是玩家 Capsule/root 位置。"))
	TObjectPtr<USceneComponent> ViewRootComponent = nullptr;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Camera",
		meta = (AllowPrivateAccess = "true", ToolTip = "编辑器中的朝向箭头。箭头方向表示进入该站位时第一人称镜头的朝向。"))
	TObjectPtr<UArrowComponent> ViewDirectionArrow = nullptr;
#endif
};
