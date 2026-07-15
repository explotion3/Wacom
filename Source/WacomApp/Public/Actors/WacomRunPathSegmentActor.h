// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WacomRunPathSegmentActor.generated.h"

class USplineComponent;

/** 一个 Logical Edge 的场景 Spline 表现；不拥有目标 Node 或规则激活逻辑。 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunPathSegmentActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomRunPathSegmentActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Run Path",
		meta = (ToolTip = "当前 Floor 内的稳定 EdgeId。必须与 FloorMapDefinition 中一条边匹配；不允许用 Actor 名称或数组下标替代。"))
	FName EdgeId = NAME_None;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Path",
		meta = (ToolTip = "返回承载当前 Edge 场景路径的 Spline 组件；只读表现数据，不提交 Run 规则。"))
	USplineComponent* GetPathSpline() const { return PathSpline; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Path",
		meta = (ToolTip = "返回当前 PathSpline 的总长度，单位厘米。"))
	float GetSplineLength() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Path",
		meta = (ToolTip = "把输入距离限制在当前 Spline 的合法范围，输入和返回值单位均为厘米。"))
	float GetClampedDistance(float Distance) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Path",
		meta = (ToolTip = "返回 Spline 指定距离的世界变换，距离单位厘米；不会移动玩家或提交规则。"))
	FTransform GetSplineTransformAtDistance(float Distance) const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run Path",
		meta = (AllowPrivateAccess = "true", ToolTip = "当前 Edge 的场景 Spline 制作组件；控制路径几何和朝向，不保存规则连接。"))
	TObjectPtr<USplineComponent> PathSpline = nullptr;
};
