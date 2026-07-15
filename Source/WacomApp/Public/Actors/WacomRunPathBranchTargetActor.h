// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WacomRunPathBranchTargetActor.generated.h"

class UBoxComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FWacomRunPathBranchRequestedNative, FName);

/** 分支点击目标只上报 EdgeId；不保存目标 Segment、Node 或成本。 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunPathBranchTargetActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomRunPathBranchTargetActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Run Path",
		meta = (ToolTip = "点击时上报的当前 Floor EdgeId。Coordinator 会从规则 Snapshot 和场景 Registry 解析路径与目标，Actor 不保存规则连接。"))
	FName EdgeId = NAME_None;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Path",
		meta = (ToolTip = "返回用于鼠标命中和分支提示定位的碰撞盒；不表示分支规则是否合法。"))
	UBoxComponent* GetClickBounds() const { return ClickBounds; }

	/** 返回 false 表示 EdgeId 无效；成功只广播意图。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Path",
		meta = (ToolTip = "广播当前 EdgeId 的分支选择意图。返回 false 表示 EdgeId 无效；规则合法性由 Coordinator 和 RunSession 判断。"))
	bool RequestBranch() const;

	FWacomRunPathBranchRequestedNative& OnBranchRequestedNative() { return BranchRequestedNative; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run Path",
		meta = (AllowPrivateAccess = "true", ToolTip = "分支目标的鼠标命中范围，单位厘米；只影响场景交互区域，不改变地图连接。"))
	TObjectPtr<UBoxComponent> ClickBounds = nullptr;

	mutable FWacomRunPathBranchRequestedNative BranchRequestedNative;
};
