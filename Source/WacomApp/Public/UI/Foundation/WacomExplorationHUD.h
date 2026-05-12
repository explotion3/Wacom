// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomActivatableWidget.h"
#include "WacomExplorationHUD.generated.h"

/**
 * 探索关卡默认 HUD 占位。
 *
 * 目前为空 Widget，只起两个作用：
 *   1. 作为 PrimaryLayout::Game 层的长期根，让 CommonUI UIActionRouter 有一个
 *      有效的 leaf-most node，避免切关卡后 Router 卡在旧 widget 的 UIInputConfig。
 *   2. 声明 FUIInputConfig(Game, CapturePermanently)，强制 Router 切回游戏输入模式。
 *
 * 战斗开始时 BattleHUD 会被 Push 到同一层叠在它上面，Router 自动切到 BattleHUD 的
 * Menu 模式；战斗结束 Pop BattleHUD 后 Router 回到 ExplorationHUD 的 Game 模式。
 *
 * 未来策划给出探索 HUD 方案时（小地图、任务提示、等）在此扩展。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomExplorationHUD : public UWacomActivatableWidget
{
	GENERATED_BODY()

public:
	UWacomExplorationHUD(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
};
