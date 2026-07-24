// Copyright Wacom. All Rights Reserved.

#pragma once

#include "ComponentVisualizer.h"

/** 在编辑器视口为无 Primitive 状态的正式商店 Anchor 绘制真实卡牌尺寸。 */
class FWacomWorldShopLayoutAnchorComponentVisualizer
	: public FComponentVisualizer
{
public:
	virtual void DrawVisualization(
		const UActorComponent* Component,
		const FSceneView* View,
		FPrimitiveDrawInterface* PDI) override;

	virtual void DrawVisualizationHUD(
		const UActorComponent* Component,
		const FViewport* Viewport,
		const FSceneView* View,
		FCanvas* Canvas) override;
};
