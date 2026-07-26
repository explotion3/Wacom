// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomWorldCardInteractionTypes.generated.h"

class UWidgetComponent;

/**
 * Activity-agnostic pointer sample for mouse-driven world card activities.
 *
 * Produced by whichever activity owns the Mouse WidgetInteraction for the
 * current visit, and consumed by presentation-only world card code. World Shop
 * is the first activity to produce it; three-choose-one and card-upgrade drop
 * activities are expected to reuse the same shape rather than define their own.
 */
struct FWacomWorldCardPointerSample
{
	TWeakObjectPtr<UWidgetComponent> HoveredComponent;
	FVector2D LocalHitLocation = FVector2D::ZeroVector;
	bool bOverHitTestVisibleWidget = false;

	bool HasHoveredComponent() const
	{
		return HoveredComponent.IsValid();
	}
};

/**
 * Reusable presentation-only behavior for mouse-driven world card activities.
 *
 * Authored per activity host (currently `BP_WacomWorldShop` Class Defaults) and
 * consumed by `FWacomWorldCardInteractionPresenter`. It only affects Hover,
 * keyword Tooltip and pinned Inspect presentation; it never touches purchase,
 * gold or any Run rule.
 *
 * The type name is load-bearing: existing Blueprint Class Defaults serialize
 * these values by struct name, so do not rename it when moving it between
 * headers.
 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomWorldCardInteractionStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Card Interaction",
		meta = (Units = "cm", ToolTip = "悬浮卡牌沿相机方向前移的距离，单位厘米。默认 8；推荐 4-14。只影响表现，不改变 Anchor。"))
	float HoverForwardDistanceCm = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Card Interaction",
		meta = (ToolTip = "悬浮卡牌的统一缩放倍率。默认 1.06；推荐 1.02-1.12。"))
	float HoverScale = 1.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Card Interaction",
		meta = (Units = "s", ToolTip = "卡牌进入或恢复悬浮 Transform 的时长，单位秒。默认 0.12；推荐 0.08-0.20。"))
	float HoverTransitionSeconds = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Card Interaction",
		meta = (Units = "s", ToolTip = "鼠标停留在同一关键词后显示 Tooltip 的延迟，单位秒。默认 0.15；推荐 0.10-0.30。"))
	float TooltipDelaySeconds = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Card Interaction",
		meta = (ToolTip = "Tooltip 相对真实鼠标的逻辑像素偏移。X 向右、Y 向下；默认 (16,-16)，表示优先显示在鼠标右上。"))
	FVector2D TooltipMouseOffsetPixels = FVector2D(16.0f, -16.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Card Interaction",
		meta = (ToolTip = "Tooltip 与游戏视口边缘保留的安全边距，单位逻辑像素。默认 16；推荐 8-32。"))
	float ViewportSafeMarginPixels = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Card Interaction",
		meta = (ToolTip = "关键词 Tooltip 的逻辑宽度，单位像素。默认 300；推荐 240-420。"))
	float TooltipWidthPixels = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Card Interaction",
		meta = (ToolTip = "固定卡牌详情面板的逻辑尺寸，单位像素。默认 360×420；推荐宽 320-480、高 360-640。"))
	FVector2D InspectPanelSizePixels = FVector2D(360.0f, 420.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Card Interaction",
		meta = (ToolTip = "固定详情面板到视口边缘的安全边距，单位逻辑像素。默认 24；推荐 16-48。"))
	float InspectPanelMarginPixels = 24.0f;

	FWacomWorldCardInteractionStyle Sanitized() const;
};
