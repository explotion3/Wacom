// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "WacomBattleEnemyPartImpactAnchorComponent.generated.h"

/** 部位命中反馈的视口可编辑锚点；必须直接挂在对应 Enemy Part 下。 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent,
	ToolTip = "敌人部位命中反馈锚点。必须直接挂在 Enemy Part 组件下；缺失时运行时回退到 Part 原点。"))
class WACOMAPP_API UWacomBattleEnemyPartImpactAnchorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UWacomBattleEnemyPartImpactAnchorComponent();

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
};
