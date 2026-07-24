// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomBackpackWorkspaceFrameScheduler.h"
#include "WacomBackpackWorkspaceGestureController.h"
#include "WacomBackpackWorkspaceMotionCoordinator.h"
#include "WacomBackpackWorkspaceNavigationController.h"
#include "WacomBackpackWorkspacePresentationController.h"
#include "WacomBackpackWorkspaceSaleDepartureController.h"
#include "WacomBackpackWorkspaceVisualState.h"
#include "WacomBackpackWorkspaceVisualRegistry.h"

/**
 * Workspace 的非反射运行时所有权根。
 *
 * UWidget 只保留输入适配和 WBP 绑定；视觉身份、跨层实例复用与卡牌局部运动
 * 由本 App-private 对象持有，避免把实现类型扩散到公共反射接口。
 */
class FWacomBackpackWorkspaceRuntime
{
public:
	FWacomBackpackWorkspaceVisualRegistry Visuals;
	FWacomBackpackWorkspaceVisualState VisualState;
	FWacomBackpackWorkspaceMotionCoordinator Motion;
	FWacomBackpackWorkspaceGestureController Gesture;
	FWacomBackpackWorkspaceNavigationController Navigation;
	FWacomBackpackWorkspacePresentationController Presentation;
	FWacomBackpackWorkspaceSaleDepartureController SaleDeparture;
	FWacomBackpackWorkspaceFrameScheduler FrameScheduler;

	void Reset(bool bRemovePileWidgets)
	{
		FrameScheduler.Reset();
		SaleDeparture.Reset(true);
		Motion.Reset();
		Gesture.Reset();
		Navigation.Clear();
		Presentation.Reset();
		VisualState.Reset();
		Visuals.ResetPiles(bRemovePileWidgets);
		Visuals.ResetIndexes();
	}
};
